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

/* External references to application globals */
extern INA226_Dev devs[5];

/* ISR-to-main-loop fault communication flags */
volatile uint8_t  fault_triggered = 0;
volatile uint16_t fault_source_mask = 0;
volatile uint16_t last_dac_value = 0;

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USBFS_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

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


