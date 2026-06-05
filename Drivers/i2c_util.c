/********************************** (C) COPYRIGHT  *******************************
* File Name          : i2c_util.c
* Author             : GSD Phase 01
* Version            : V1.0.0
* Date               : 2026/06/05
* Description        : Timeout-protected I2C master wrapper with automatic bus
*                      recovery. Implements non-blocking tick-count timeout on
*                      every I2C flag poll, 9-clock-pulse GPIO bus recovery,
*                      and automatic retry on timeout.
*******************************************************************************/
#include "i2c_util.h"
#include "debug.h"
#include "ch32v30x.h"

/* --------------------------------------------------------------------------
 * SysTick-based non-blocking timeout helpers
 * -------------------------------------------------------------------------- */

/* Enable SysTick free-running counter for I2C timeout measurement.
 * Uses the same clock configuration as Delay_Us/Delay_Ms (from debug.c).
 * SysTick is configured as a one-shot timer with maximum compare value so
 * it effectively free-runs during the entire I2C operation.
 * Call i2c_timeout_stop() after the operation completes. */
static void i2c_timeout_start(void)
{
    /* Configure SysTick in the same mode as Delay_Us:
     * - Clear match flag
     * - Set CMP to max value (counter runs without triggering match)
     * - Set CTLR bits: [4] and [5]|[0] for counter mode and clock source */
    SysTick->SR &= ~(1 << 0);
    SysTick->CMP = 0xFFFFFFFFFFFFFFFFULL;
    SysTick->CTLR |= (1 << 4);
    SysTick->CTLR |= (1 << 5) | (1 << 0);

    /* Allow a few cycles for the counter to stabilize */
    {
        volatile int _stb;
        for (_stb = 0; _stb < 20; _stb++)
        {
            __asm volatile ("nop");
        }
    }
}

/* Stop SysTick counter after I2C operation, reset CNT to zero.
 * This restores SysTick to a clean state so subsequent Delay_Us/Delay_Ms
 * calls (which reinitialize their own CMP and CTLR) work correctly. */
static void i2c_timeout_stop(void)
{
    SysTick->CTLR &= ~(1 << 0);
    SysTick->CNT = 0;
}

/* Get current 24-bit tick count from SysTick->CNT.
 * Uses 24-bit mask for wraparound-safe arithmetic per the plan spec. */
static uint32_t i2c_get_ticks(void)
{
    return (uint32_t)(SysTick->CNT & 0x00FFFFFF);
}

/* Compute elapsed milliseconds from a start tick value.
 * Uses modulo-safe 24-bit subtraction.
 * ticks_per_ms = (SystemCoreClock / 8000000) * 1000 per Delay_Init pattern. */
static uint32_t i2c_elapsed_ms(uint32_t start_ticks)
{
    uint32_t current;
    uint32_t elapsed;
    uint32_t ticks_per_ms;

    current = i2c_get_ticks();
    elapsed = (current - start_ticks) & 0x00FFFFFF;
    ticks_per_ms = (SystemCoreClock / 8000000) * 1000;

    return elapsed / ticks_per_ms;
}

/* Check if a timeout has been exceeded (non-blocking).
 * Returns 1 if elapsed_ms >= timeout_ms, 0 otherwise. */
static uint8_t i2c_has_timed_out(uint32_t start_ticks, uint32_t timeout_ms)
{
    return (i2c_elapsed_ms(start_ticks) >= timeout_ms) ? 1 : 0;
}

/* --------------------------------------------------------------------------
 * I2C flag-polling wrappers with timeout
 * -------------------------------------------------------------------------- */

/* Poll for an I2C event with timeout.
 * Returns I2C_OK if event detected, I2C_TIMEOUT if timeout elapsed.
 * Also returns I2C_NACK if I2C_FLAG_AF (Acknowledge Failure) is detected. */
static i2c_status_t i2c_wait_event(uint32_t event, uint32_t start_ticks,
                                   uint32_t timeout_ms)
{
    while (!I2C_CheckEvent(I2C1, event))
    {
        /* Check for NACK (slave did not acknowledge) */
        if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF) == SET)
        {
            I2C_ClearFlag(I2C1, I2C_FLAG_AF);
            return I2C_NACK;
        }

        /* Check for bus error */
        if (I2C_GetFlagStatus(I2C1, I2C_FLAG_BERR) == SET)
        {
            I2C_ClearFlag(I2C1, I2C_FLAG_BERR);
            return I2C_TIMEOUT;
        }

        /* Check for arbitration lost */
        if (I2C_GetFlagStatus(I2C1, I2C_FLAG_ARLO) == SET)
        {
            I2C_ClearFlag(I2C1, I2C_FLAG_ARLO);
            return I2C_TIMEOUT;
        }

        if (i2c_has_timed_out(start_ticks, timeout_ms))
        {
            return I2C_TIMEOUT;
        }
    }

    return I2C_OK;
}

/* Poll for I2C_FLAG_RXNE (receive buffer not empty) with timeout.
 * Used during multi-byte reads where we need to wait for each byte. */
static i2c_status_t i2c_wait_rxne(uint32_t start_ticks, uint32_t timeout_ms)
{
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET)
    {
        if (i2c_has_timed_out(start_ticks, timeout_ms))
        {
            return I2C_TIMEOUT;
        }
    }

    return I2C_OK;
}

/* --------------------------------------------------------------------------
 * Single-attempt I2C master write (no recovery/retry)
 * Internal helper: called by i2c_util_write after timeout management is set up.
 * Returns status code. On I2C_NACK or I2C_TIMEOUT, STOP is generated. */
static i2c_status_t i2c_write_once(uint8_t dev_addr, const uint8_t *data,
                                   uint8_t len, uint32_t start_ticks,
                                   uint32_t timeout_ms)
{
    i2c_status_t status;
    uint8_t i;
    uint8_t addr;

    /* Shift 7-bit address to 8-bit format with write direction bit=0 */
    addr = dev_addr << 1;

    /* Step 1: Generate START */
    I2C_GenerateSTART(I2C1, ENABLE);

    /* Step 2: Wait for EVT5 (Master Mode Selected) */
    status = i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT, start_ticks, timeout_ms);
    if (status != I2C_OK)
    {
        I2C_GenerateSTOP(I2C1, ENABLE);
        return status;
    }

    /* Step 3: Send 7-bit slave address with transmitter direction */
    I2C_Send7bitAddress(I2C1, addr, I2C_Direction_Transmitter);

    /* Step 4: Wait for EVT6 (Master Transmitter Mode Selected) */
    status = i2c_wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED,
                            start_ticks, timeout_ms);
    if (status != I2C_OK)
    {
        I2C_GenerateSTOP(I2C1, ENABLE);
        return status;
    }

    /* Step 5-6: Send each data byte, wait for byte-transmitted */
    for (i = 0; i < len; i++)
    {
        I2C_SendData(I2C1, data[i]);

        status = i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED,
                                start_ticks, timeout_ms);
        if (status != I2C_OK)
        {
            I2C_GenerateSTOP(I2C1, ENABLE);
            return status;
        }
    }

    /* Step 7: Generate STOP */
    I2C_GenerateSTOP(I2C1, ENABLE);

    return I2C_OK;
}

/* --------------------------------------------------------------------------
 * Single-attempt I2C master read (no recovery/retry)
 * Internal helper: writes register pointer, repeated START, reads len bytes.
 * ACK on all bytes except last (NACK before STOP). */
static i2c_status_t i2c_read_once(uint8_t dev_addr, uint8_t reg_ptr,
                                  uint8_t *data, uint8_t len,
                                  uint32_t start_ticks, uint32_t timeout_ms)
{
    i2c_status_t status;
    uint8_t i;
    uint8_t addr;

    /* Shift 7-bit address to 8-bit format */
    addr = dev_addr << 1;

    /* ====== Phase 1: Write register pointer (no STOP) ====== */

    /* Step 1: Generate START */
    I2C_GenerateSTART(I2C1, ENABLE);

    /* Step 2: Wait for EVT5 */
    status = i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT, start_ticks, timeout_ms);
    if (status != I2C_OK)
    {
        I2C_GenerateSTOP(I2C1, ENABLE);
        return status;
    }

    /* Step 3: Send address with transmitter direction */
    I2C_Send7bitAddress(I2C1, addr, I2C_Direction_Transmitter);

    /* Step 4: Wait for EVT6 */
    status = i2c_wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED,
                            start_ticks, timeout_ms);
    if (status != I2C_OK)
    {
        I2C_GenerateSTOP(I2C1, ENABLE);
        return status;
    }

    /* Step 5: Send register pointer byte */
    I2C_SendData(I2C1, reg_ptr);

    /* Step 6: Wait for EVT8_2 (byte transmitted, BTF set) */
    status = i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED,
                            start_ticks, timeout_ms);
    if (status != I2C_OK)
    {
        I2C_GenerateSTOP(I2C1, ENABLE);
        return status;
    }

    /* ====== Phase 2: Repeated START, read len bytes ====== */

    /* Step 7: Generate repeated START */
    I2C_GenerateSTART(I2C1, ENABLE);

    /* Step 8: Wait for EVT5 */
    status = i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT, start_ticks, timeout_ms);
    if (status != I2C_OK)
    {
        I2C_GenerateSTOP(I2C1, ENABLE);
        return status;
    }

    /* Step 9: Send address with receiver direction */
    I2C_Send7bitAddress(I2C1, addr, I2C_Direction_Receiver);

    /* Step 10: Wait for EVT6_RX (BUSY+MSL+ADDR, master receiver selected) */
    status = i2c_wait_event(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED,
                            start_ticks, timeout_ms);
    if (status != I2C_OK)
    {
        I2C_GenerateSTOP(I2C1, ENABLE);
        return status;
    }

    /* Handle single-byte read (len == 1): NACK + STOP before reading */
    if (len == 1)
    {
        /* Disable ACK (NACK) then generate STOP before receiving */
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        I2C_GenerateSTOP(I2C1, ENABLE);

        /* Wait for RXNE (EVT7) */
        status = i2c_wait_rxne(start_ticks, timeout_ms);
        if (status != I2C_OK)
        {
            return status;
        }

        data[0] = I2C_ReceiveData(I2C1);

        return I2C_OK;
    }

    /* Multi-byte read (len >= 2): ACK all except last byte */
    for (i = 0; i < len; i++)
    {
        if (i == (len - 1))
        {
            /* Last byte: NACK then generate STOP */
            I2C_AcknowledgeConfig(I2C1, DISABLE);
            I2C_GenerateSTOP(I2C1, ENABLE);
        }
        else
        {
            /* Not last byte: ACK to request more data */
            I2C_AcknowledgeConfig(I2C1, ENABLE);
        }

        /* Wait for RXNE (EVT7) */
        status = i2c_wait_rxne(start_ticks, timeout_ms);
        if (status != I2C_OK)
        {
            return status;
        }

        data[i] = I2C_ReceiveData(I2C1);
    }

    return I2C_OK;
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/*********************************************************************
 * @fn      i2c_util_init
 *
 * @brief   Initialize I2C1 peripheral for 100kHz standard mode.
 *          Configures PB6 (SCL) and PB7 (SDA) as alternate-function
 *          open-drain, sets up I2C1 clock tree, and enables the peripheral.
 *
 * @return  none
 */
void i2c_util_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitStructure = {0};

    /* Enable peripheral clocks */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* Configure PB6 (SCL) as alternate-function open-drain */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Configure PB7 (SDA) as alternate-function open-drain */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Configure I2C1: 100kHz standard mode */
    I2C_InitStructure.I2C_ClockSpeed = 100000;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitStructure);

    /* Enable I2C1 peripheral */
    I2C_Cmd(I2C1, ENABLE);
}

/*********************************************************************
 * @fn      i2c_util_write
 *
 * @brief   Write len bytes to an I2C slave device with timeout protection.
 *          Attempts one write with timeout; on failure, performs bus
 *          recovery and retries once.
 *
 * @param   dev_addr   7-bit I2C slave address
 * @param   data       Data buffer to send
 * @param   len        Number of bytes to send
 * @param   timeout_ms Timeout in milliseconds
 *
 * @return  i2c_status_t - I2C_OK on success, error code on failure
 */
i2c_status_t i2c_util_write(uint8_t dev_addr, const uint8_t *data,
                            uint8_t len, uint32_t timeout_ms)
{
    i2c_status_t status;
    uint32_t start_ticks;

    /* First attempt */
    i2c_timeout_start();
    start_ticks = i2c_get_ticks();

    status = i2c_write_once(dev_addr, data, len, start_ticks, timeout_ms);

    i2c_timeout_stop();

    if (status == I2C_OK)
    {
        return I2C_OK;
    }

    /* On timeout or bus fault: attempt recovery and retry once */
    i2c_util_bus_recovery();

    /* Retry */
    i2c_timeout_start();
    start_ticks = i2c_get_ticks();

    status = i2c_write_once(dev_addr, data, len, start_ticks, timeout_ms);

    i2c_timeout_stop();

    return status;
}

/*********************************************************************
 * @fn      i2c_util_read
 *
 * @brief   Read len bytes from an I2C slave device register with timeout
 *          protection. Writes register pointer (no STOP), issues repeated
 *          START, reads len bytes (NACK on last byte).
 *          On failure, performs bus recovery and retries once.
 *
 * @param   dev_addr   7-bit I2C slave address
 * @param   reg_ptr    Register pointer byte to write
 * @param   data       Buffer to store read data
 * @param   len        Number of bytes to read
 * @param   timeout_ms Timeout in milliseconds
 *
 * @return  i2c_status_t - I2C_OK on success, error code on failure
 */
i2c_status_t i2c_util_read(uint8_t dev_addr, uint8_t reg_ptr,
                           uint8_t *data, uint8_t len, uint32_t timeout_ms)
{
    i2c_status_t status;
    uint32_t start_ticks;

    /* First attempt */
    i2c_timeout_start();
    start_ticks = i2c_get_ticks();

    status = i2c_read_once(dev_addr, reg_ptr, data, len,
                           start_ticks, timeout_ms);

    i2c_timeout_stop();

    if (status == I2C_OK)
    {
        return I2C_OK;
    }

    /* On timeout or bus fault: attempt recovery and retry once */
    i2c_util_bus_recovery();

    /* Retry */
    i2c_timeout_start();
    start_ticks = i2c_get_ticks();

    status = i2c_read_once(dev_addr, reg_ptr, data, len,
                           start_ticks, timeout_ms);

    i2c_timeout_stop();

    return status;
}

/*********************************************************************
 * @fn      i2c_util_bus_recovery
 *
 * @brief   Attempt I2C bus recovery via 9-clock-pulse GPIO bit-banging.
 *
 *          Sequence:
 *          1. Disable I2C1 peripheral
 *          2. Reconfigure PB6 as GPIO Out_OD, PB7 as GPIO IN_FLOATING
 *          3. Generate up to 9 SCL pulses (5us low, 5us high each),
 *             checking SDA after each pulse; early exit if SDA released
 *             SDA is NEVER driven low during the clock pulses (kept as
 *             IN_FLOATING/hi-Z to avoid false ACK).
 *          4. Generate STOP condition: drive SDA low -> SCL high -> SDA high
 *          5. Re-initialize I2C1 peripheral and GPIO pins via i2c_util_init()
 *
 * @return  I2C_OK if SDA was released, I2C_BUS_FAULT if still stuck low
 */
i2c_status_t i2c_util_bus_recovery(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    uint8_t i;
    uint8_t sda_released;

    /* Step 1: Disable I2C1 peripheral */
    I2C_Cmd(I2C1, DISABLE);

    /* Step 2: Reconfigure PB6 as GPIO Out_OD, PB7 as GPIO IN_FLOATING
     *         (SDA must be INPUT/hi-Z during clock pulses per Pattern 3) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Step 3: Generate up to 9 SCL clock pulses
     *         SDA is monitored as input (hi-Z) — never driven low here */
    sda_released = 0;

    for (i = 0; i < 9; i++)
    {
        /* SCL low */
        GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_RESET);
        Delay_Us(5);

        /* SCL high */
        GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_SET);
        Delay_Us(5);

        /* Check if SDA has been released by the slave */
        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7) == Bit_SET)
        {
            sda_released = 1;
            break;
        }
    }

    /* Step 4: Generate STOP condition
     *         STOP = SDA goes high while SCL is high
     *         First drive SDA low (reconfigure as output), then release:
     *         SDA low -> SCL high -> SDA high */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_RESET);
    Delay_Us(5);

    GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_SET);
    Delay_Us(5);

    GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_SET);
    Delay_Us(5);

    /* Step 5: Re-initialize I2C1 and GPIO pins back to AF_OD mode */
    i2c_util_init();

    if (sda_released)
    {
        return I2C_OK;
    }

    return I2C_BUS_FAULT;
}
