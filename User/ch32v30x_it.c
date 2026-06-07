/********************************** (C) COPYRIGHT *******************************
* File Name          : ch32v30x_it.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2024/03/06
* Description        : Main Interrupt Service Routines.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include "ch32v30x_it.h"
#include "../Drivers/protocol.h"

/* External references to application globals */
extern INA226_Dev devs[5];

/* ISR-to-main-loop fault communication flags */
volatile uint8_t  fault_triggered = 0;
volatile uint16_t fault_source_mask = 0;
volatile uint16_t last_dac_value = 0;

/* External ring buffer variables (defined in Drivers/protocol.c) */
extern volatile uint8_t  rx_buf[512];
extern volatile uint16_t rx_head;
extern volatile uint16_t rx_tail;
extern volatile uint8_t  rx_overflow;
extern volatile uint8_t  rx_parity_err;
extern volatile uint8_t  rx_framing_err;

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USBFS_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USART2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/*********************************************************************
 * @fn      NMI_Handler
 *
 * @brief   This function handles NMI exception.
 *
 * @return  none
 */
void NMI_Handler(void)
{
  while (1)
  {
  }
}

/*********************************************************************
 * @fn      HardFault_Handler
 *
 * @brief   This function handles Hard Fault exception.
 *
 * @return  none
 */
void HardFault_Handler(void)
{
  NVIC_SystemReset();
  while (1)
  {
  }
}

/*********************************************************************
 * @fn      EXTI4_IRQHandler
 *
 * @brief   EXTI4 interrupt handler for INA226 wired-OR ALARM signal.
 *          On falling-edge trigger (any MOS channel overcurrent):
 *          1. Read all 4 MOS channel alert registers to identify source
 *          2. Set per-channel bits in fault_source_mask
 *          3. Zero DAC immediately for hardware protection
 *          4. Print diagnostic snapshot with fault details
 *          5. Set fault_triggered flag for main loop state machine
 *
 * @return  none
 */
void EXTI4_IRQHandler(void)
{
    uint8_t i;
    uint16_t alert_mask;
    i2c_status_t st;

    /* Step 1: Read alert registers from all 4 MOS channels (devs[0]-devs[3]).
     *         Reading the mask register also clears the INA226 latch. */
    fault_source_mask = 0;
    for (i = 0; i < 4; i++)
    {
        alert_mask = 0;
        st = ina226_check_alert(&devs[i], &alert_mask);
        if (st == I2C_OK && (alert_mask & INA226_ALERT_SHUNT_OV))
        {
            /* Set bit corresponding to this channel's logical ID */
            fault_source_mask |= (uint16_t)(1 << devs[i].channel);
        }
    }

    /* Step 2: Zero DAC immediately — hardware protection takes priority.
     *         Disable IRQ during DAC I2C transaction for atomic zeroing. */
    __disable_irq();
    dac8571_set_output(0);
    __enable_irq();

    /* Step 3: Signal fault to main loop */
    fault_triggered = 1;

    /* Step 4: Diagnostic snapshot — fault mask, DAC (now zeroed), MOS addresses */
    printf("[FAULT] mask=0x%04X dac=%u addr=0x%02X-0x%02X-0x%02X-0x%02X\r\n",
           fault_source_mask,
           last_dac_value,
           devs[0].address, devs[1].address,
           devs[2].address, devs[3].address);

    /* Step 5: Clear EXTI4 pending bit to allow next falling-edge detection */
    EXTI_ClearITPendingBit(EXTI_Line4);
}

/*********************************************************************
 * @fn      USART2_IRQHandler
 *
 * @brief   USART2 interrupt handler — RXNE-driven ring buffer fill.
 *          On RXNE: read RDR byte, write to rx_buf at rx_tail with
 *          wrap-around modulo 512. Discards byte if buffer is full
 *          (next_tail == rx_head) and sets rx_overflow flag.
 *          Checks and clears parity, framing, and overrun error flags.
 *          No blocking calls — all logging deferred to main loop via
 *          volatile error counters.
 *
 * @return  none
 */
void USART2_IRQHandler(void)
{
    uint8_t byte;
    uint16_t next_tail;

    /* Receive Data Register Not Empty */
    if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET)
    {
        byte = (uint8_t)USART_ReceiveData(USART2);
        next_tail = (rx_tail + 1) % RX_BUF_SIZE;

        if (next_tail == rx_head)
        {
            /* Buffer full — discard byte */
            rx_overflow = 1;
        }
        else
        {
            rx_buf[rx_tail] = byte;
            rx_tail = next_tail;
        }
    }

    /* Parity error — byte already read, discard, count for main loop */
    if (USART_GetFlagStatus(USART2, USART_FLAG_PE) != RESET)
    {
        rx_parity_err++;
        USART_ClearFlag(USART2, USART_FLAG_PE);
    }

    /* Framing error — count for main loop diagnostics */
    if (USART_GetFlagStatus(USART2, USART_FLAG_FE) != RESET)
    {
        rx_framing_err++;
        USART_ClearFlag(USART2, USART_FLAG_FE);
    }

    /* Overrun error — data lost, flag for main loop */
    if (USART_GetFlagStatus(USART2, USART_FLAG_ORE) != RESET)
    {
        rx_overflow = 1;
        USART_ClearFlag(USART2, USART_FLAG_ORE);
    }
}


