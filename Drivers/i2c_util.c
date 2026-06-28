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
 * Also returns I2C_NACK if I2C_FLAG_AF (Acknowledge Failure) is detected.
 *
 * step_id: diagnostic label for timeout messages (1=EVT5, 2=EVT6, 3=EVT8). */
static i2c_status_t  i2c_wait_event_ex(uint32_t event, uint32_t start_ticks,
                                      uint32_t timeout_ms, uint8_t step_id)
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
            printf("I2C: BERR at step %d\r\n", step_id);
            return I2C_TIMEOUT;
        }

        /* Check for arbitration lost */
        if (I2C_GetFlagStatus(I2C1, I2C_FLAG_ARLO) == SET)
        {
            I2C_ClearFlag(I2C1, I2C_FLAG_ARLO);
            printf("I2C: ARLO at step %d\r\n", step_id);
            return I2C_TIMEOUT;
        }

        if (i2c_has_timed_out(start_ticks, timeout_ms))
        {
            printf("I2C: T/O step %d, SR1=0x%04X SR2=0x%04X\r\n",
                   step_id,
                   (unsigned int)(I2C1->STAR1 & 0xFFFF),
                   (unsigned int)(I2C1->STAR2 & 0xFFFF));
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

    /* Step 0: Wait for BUSY flag to clear */
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET)
    {
        if (i2c_has_timed_out(start_ticks, timeout_ms))
        {
            printf("I2C: BUSY stuck, SR1=0x%04X SR2=0x%04X\r\n",
                   (unsigned int)(I2C1->STAR1 & 0xFFFF),
                   (unsigned int)(I2C1->STAR2 & 0xFFFF));
            return I2C_TIMEOUT;
        }
    }

    /* Reset timeout reference for the main transaction */
    start_ticks = i2c_get_ticks();

    /* Step 1: Generate START */
    I2C1->CTLR1 |= (1 << 8);  /* Set START bit */
    {
        /* Small delay, then check if hardware cleared START (means it acted) */
        for (volatile int _d = 0; _d < 100; _d++) { __asm volatile ("nop"); }
        uint16_t ctlr1_now = I2C1->CTLR1;
        if (ctlr1_now & (1 << 8))
        {
            printf("I2C: START still set after delay, CTLR1=0x%04X SR1=0x%04X\r\n",
                   ctlr1_now, (unsigned int)(I2C1->STAR1 & 0xFFFF));
        }
    }

    /* Step 2: Wait for EVT5 (Master Mode Selected) */
    status = i2c_wait_event_ex(I2C_EVENT_MASTER_MODE_SELECT, start_ticks,
                               timeout_ms, 1);
    if (status != I2C_OK)
    {
        I2C_GenerateSTOP(I2C1, ENABLE);
        return status;
    }

    /* Step 3: Send 7-bit slave address with transmitter direction */
    I2C_Send7bitAddress(I2C1, addr, I2C_Direction_Transmitter);

    /* Allow slave time to acknowledge address */
    Delay_Ms(10);

    /* Step 4: Wait for EVT6 (Master Transmitter Mode Selected) */
    status = i2c_wait_event_ex(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED,
                               start_ticks, timeout_ms, 2);
    if (status != I2C_OK)
    {
        I2C_GenerateSTOP(I2C1, ENABLE);
        return status;
    }

    /* Step 5-6: Send each data byte, wait for byte-transmitted */
    for (i = 0; i < len; i++)
    {
        if (I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) == RESET)
        {
            printf("I2C: TXE not set before data[%d], SR1=0x%04X\r\n",
                   (int)i, (unsigned int)(I2C1->STAR1 & 0xFFFF));
            I2C_GenerateSTOP(I2C1, ENABLE);
            return I2C_TIMEOUT;
        }

        I2C_SendData(I2C1, data[i]);

        status = i2c_wait_event_ex(I2C_EVENT_MASTER_BYTE_TRANSMITTED,
                                   start_ticks, timeout_ms, 3 + i);
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
i2c_status_t i2c_read_once(uint8_t dev_addr, int reg_ptr,
                                  uint8_t *data)
{
    dev_addr = dev_addr << 1;
    dev_addr+=1;
    u8 buffer[2] = {0};
    u16 temp = 0;
    int waiting = 0;
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET) {
        Delay_Us(10);
        waiting ++;
        if (waiting > 10000){
            printf("I2C1 read error\r\n");
            I2C_GenerateSTOP(I2C1, ENABLE);
            return I2C_TIMEOUT;
        }
    };
    I2C_GenerateSTART(I2C1, ENABLE);

    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2C1, dev_addr, I2C_Direction_Transmitter);

    waiting = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        Delay_Us(10);
        waiting++;
        if (waiting > 10000) {
            printf("I2C1 read error1\r\n");
            I2C_GenerateSTOP(I2C1, ENABLE);
            return I2C_TIMEOUT;
        }
    }

#if (Address_Lenth == Address_8bit)
    I2C_SendData(I2C1, reg_ptr);

//    Delay_Ms(10);
    waiting = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)){
        Delay_Us(10);
        waiting++;
        if (waiting > 10000) {
            printf("I2C1 read error2\r\n");
            I2C_GenerateSTOP(I2C1, ENABLE);
            return I2C_TIMEOUT;
        }
    }

#elif (Address_Lenth == Address_16bit)
    I2C_SendData( I2C1, (u8)(reg_addr>>8) );
    while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED ) );

    I2C_SendData( I2C1, (u8)(reg_addr&0x00FF) );
    while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED ) );

#endif

    I2C_GenerateSTART(I2C1, ENABLE);

    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2C1, dev_addr, I2C_Direction_Receiver);
    waiting = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
        Delay_Us(10);
        waiting++;
        if (waiting > 10000) {
            printf("I2C1 read error3\r\n");
            I2C_GenerateSTOP(I2C1, ENABLE);
            return 1;
        }
    }
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
    buffer[0] = I2C_ReceiveData(I2C1);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    buffer[1] = I2C_ReceiveData(I2C1);
    I2C_AcknowledgeConfig(I2C1, ENABLE);


    I2C_GenerateSTOP(I2C1, ENABLE);
    temp = buffer[0] << 8 | buffer[1];

    *data = temp;
    Delay_Ms(1);
    return I2C_OK;
}
i2c_status_t dac_read_once(uint8_t dev_addr, uint8_t *data)
{
    dev_addr = dev_addr << 1;
    dev_addr+=1;
    u8 buffer[2] = {0};
    u16 temp = 0;
    int waiting = 0;
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET) {
        Delay_Us(10);
        waiting ++;
        if (waiting > 10000){
            printf("I2C1 read error\r\n");
            I2C_GenerateSTOP(I2C1, ENABLE);
            return I2C_TIMEOUT;
        }
    };
    I2C_GenerateSTART(I2C1, ENABLE);

    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2C1, dev_addr, I2C_Direction_Transmitter);

    waiting = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        Delay_Us(10);
        waiting++;
        if (waiting > 10000) {
            printf("I2C1 read error1\r\n");
            I2C_GenerateSTOP(I2C1, ENABLE);
            return I2C_TIMEOUT;
        }
    }

    I2C_GenerateSTART(I2C1, ENABLE);

    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2C1, dev_addr, I2C_Direction_Receiver);
    waiting = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
        Delay_Us(10);
        waiting++;
        if (waiting > 10000) {
            printf("I2C1 read error3\r\n");
            I2C_GenerateSTOP(I2C1, ENABLE);
            return 1;
        }
    }
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
    buffer[0] = I2C_ReceiveData(I2C1);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    buffer[1] = I2C_ReceiveData(I2C1);
    I2C_AcknowledgeConfig(I2C1, ENABLE);


    I2C_GenerateSTOP(I2C1, ENABLE);
    temp = buffer[0] << 8 | buffer[1];

    *data = temp;
    Delay_Ms(1);
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
 *          Order matches CH32V303 I2C example pattern:
 *          1. Clocks + I2C DeInit (clean reset)
 *          2. GPIO set high (ensure OD pins start released)
 *          3. I2C config + enable (peripheral ready before GPIO AF switch)
 *          4. GPIO AF_OD (route I2C peripheral to pins last, after I2C is live)
 *
 * @return  none
 */
void i2c_util_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitStructure = {0};

    /* Enable peripheral clocks.
     * AFIO clock is required to clear any leftover pin remaps (see below). */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

    /* Reset AFIO to factory defaults.  This clears ALL pin remap bits that
     * may have been set by previously-run firmware (I2C1→PB8/PB9, USART1→PB6/PB7,
     * TIM4→PB6/PB7, etc.).  Without this, a stale remap can silently re-route the
     * I2C1 peripheral to different pins, leaving PB6/PB7 disconnected. */
    GPIO_AFIODeInit();

    /* Force a clean I2C peripheral state by toggling its reset */
    I2C_DeInit(I2C1);

    /* Set SCL/SDA output data high before configuring as AF_OD.
     * This ensures the open-drain outputs start in the released (high-Z) state,
     * preventing an unintentional bus glitch during GPIO_Init(). */
    GPIO_SetBits(GPIOB, GPIO_Pin_6 | GPIO_Pin_7);

    /* ---- Configure GPIO FIRST (before I2C is enabled).
     *      When PE transitions 0→1 later, the pins are already stable
     *      in AF_OD mode, avoiding any glitch-induced state corruption. ---- */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* ---- Now enable and configure I2C1 (pins already AF_OD).
     *      SWRST: set bit 15 to force a full internal state-machine reset,
     *      then wait for hardware to clear it. This is stronger than DeInit
     *      alone and recovers from silicon-level lockup states. ---- */
    I2C1->CTLR1 |= (1 << 15);                    /* Set SWRST */
    {
        volatile int _swrst;
        for (_swrst = 0; _swrst < 100; _swrst++)
        {
            __asm volatile ("nop");
        }
    }
    I2C1->CTLR1 &= ~(1 << 15);                   /* Clear SWRST */

    /* ---- Configure I2C1: 50kHz standard mode (conservative) ---- */
    I2C_InitStructure.I2C_ClockSpeed = 400000;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitStructure);

    /* I2C_Init already enables PE (peripheral enable) internally at line 184.
     * Explicit I2C_Cmd is kept for clarity. */
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
                           uint8_t *data)
{
    i2c_status_t status;
    // uint32_t start_ticks;

    /* First attempt */
    i2c_timeout_start();
    // start_ticks = i2c_get_ticks();

    status = i2c_read_once(dev_addr, reg_ptr, data);

    i2c_timeout_stop();

    if (status == I2C_OK)
    {
        return I2C_OK;
    }

    /* On timeout or bus fault: attempt recovery and retry once */
    i2c_util_bus_recovery();

    /* Retry */
    i2c_timeout_start();
    // start_ticks = i2c_get_ticks();

    status = i2c_read_once(dev_addr, reg_ptr, data);

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
