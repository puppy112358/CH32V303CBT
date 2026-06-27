/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : Main program body.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

/*
 *@Note
 * Electronic Load Controller - main firmware.
 *   printf  -> USB-CDC virtual COM port (debug output)
 *   USART1   -> cJSON commands & 10Hz telemetry (PA9=TX, PA10=RX)
 *   100ms control loop: CV/CC PID -> DAC8571 -> INA226 feedback
 */

#include "debug.h"
void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
#include "../Drivers/i2c_util.h"
#include "../Drivers/ina226.h"
#include "../Drivers/dac8571.h"
#include "../Drivers/pid.h"
#include "../Drivers/fault.h"
#include "../Drivers/ws2812.h"
#include "../Drivers/temp_sensor.h"
#include "../Drivers/fan.h"
#include "../Drivers/protocol.h"
#include "../Drivers/usb_cdc.h"

/* USB device enumeration status (set by USBFS_IRQHandler after SET_CONFIGURATION) */
extern volatile uint8_t USBFS_DevEnumStatus;

/* External references for ISR modules */
extern INA226_Dev devs[5];
extern ringbuffer ring_buffer;
extern USART_DMA_CTRL_  USART_DMA_CTRL;
/* Number of INA226 devices on I2C1 bus */
#define DEV_COUNT    5

/* Control loop timing */
#define CONTROL_PERIOD_MS   100
#define SOFTSTART_STEPS     5

/* ISR-to-main-loop fault flags (defined in ch32v30x_it.c) */
extern volatile uint8_t  fault_triggered;
extern volatile uint16_t fault_source_mask;
extern volatile uint16_t last_dac_value;

/* NTC temperature reading (defined in Drivers/temp_sensor.c) */
extern volatile float heatsink_temp_c;

/* Fan status (defined in Drivers/fan.c) */
extern volatile uint16_t fan_rpm;
extern volatile uint8_t  fan_stall;

/* PID instances */
PID_Instance pid_cv;
PID_Instance pid_cc;

/* System state */
SystemMode system_mode = MODE_IDLE;
float cv_target_voltage = 0.0f;
float cc_target_current = 0.0f;

/* Running cycle counter (extern-visible for protocol.c uptime in get_status) */
uint32_t cycle_count = 0;

/* Device presence flags (global - accessed by engage helpers) */
uint8_t dev_ok[DEV_COUNT];
uint8_t dac_ok;

/*
 * INA226 device array - 5 devices on I2C1 at addresses confirmed by hardware:
 *   0x40: MOS Channel 1 (A1=GND, A0=GND)
 *   0x41: MOS Channel 2 (A1=GND, A0=VS)
 *   0x42: MOS Channel 3 (A1=GND, A0=SDA)
 *   0x43: MOS Channel 4 (A1=GND, A0=SCL)
 *   0x44: Summary      (A1=VS,  A0=GND)
 */
INA226_Dev devs[DEV_COUNT] = {
    {0x40, 0},  /* MOS Channel 1 */
    {0x41, 1},  /* MOS Channel 2 */
    {0x42, 2},  /* MOS Channel 3 */
    {0x43, 3},  /* MOS Channel 4 */
    {0x44, 4},  /* Summary */
};

/*********************************************************************
 * @fn      softstart_engage
 *
 * @brief   Soft-start DAC ramp: linearly increase from 0 to target
 *          over 500ms (5 steps x 100ms).
 *
 * @param   target_dac - target DAC value (0-65535)
 *
 * @return  none
 */
static void softstart_engage(uint16_t target_dac)
{
    uint8_t step;
    uint16_t dac_out;
    uint16_t step_size;

    step_size = target_dac / SOFTSTART_STEPS;

    for (step = 0; step < SOFTSTART_STEPS; step++)
    {
        if (step == SOFTSTART_STEPS - 1)
        {
            dac_out = target_dac;
        }
        else
        {
            dac_out = (uint16_t)((step + 1) * step_size);
        }

        if (dac_ok) dac8571_set_output(dac_out);
        last_dac_value = dac_out;
        Delay_Ms(CONTROL_PERIOD_MS);
    }
}

/*********************************************************************
 * @fn      engage_cv
 *
 * @brief   Switch to Constant-Voltage (CV) mode.
 *          Compute initial PID output, ramp DAC via soft-start,
 *          pre-load integral term for bumpless transfer.
 *
 * @param   target_voltage - target voltage (V)
 *
 * @return  none
 */
void engage_cv(float target_voltage)
{
    system_mode = MODE_CV;
    cv_target_voltage = target_voltage;
    last_dac_value = 0; /* hardware not active yet */
    printf("[CV] target=%.2fV\r\n", target_voltage);
}

/*********************************************************************
 * @fn      engage_cc
 *
 * @brief   Switch to Constant-Current (CC) mode.
 *          Compute initial PID output, ramp DAC via soft-start,
 *          pre-load integral term for bumpless transfer.
 *
 * @param   target_current - target current (A)
 *
 * @return  none
 */
void engage_cc(float target_current)
{
    system_mode = MODE_CC;
    cc_target_current = target_current;
    last_dac_value = 0; /* hardware not active yet */
    printf("[CC] target=%.2fA\r\n", target_current);
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *          printf -> USB-CDC virtual COM port (debug log)
 *          USART1 -> cJSON command input & 10Hz telemetry output
 *          100ms control loop: CV/CC PID -> DAC8571 -> INA226 feedback
 *
 * @return  none
 */
int main(void)
{
    // uint8_t i;
    i2c_status_t init_status;
    i2c_status_t dac_status;
    // float mos_v[4];
    // float mos_i[4];
    // float bus_v, bus_i, bus_p;
    uint32_t enum_timeout;
    // static uint32_t fault_free_ms = 0;

    /* ---- 1. System Core Setup ---- */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SystemCoreClockUpdate();
    Delay_Init();

    USART_Printf_Init(115200);  /* step 1: TX-only (example pattern) */
    protocol_init();            /* step 2: TX+RX+interrupt */
    DMA_INIT();                 /* step 3: DMA for USART1 RX (REQUIRED for IDLE+DMA reception) */
    

    printf("I2C1 init...\r\n");
    i2c_util_init();
    printf("I2C1 ready\r\n");

    for (int i = 0; i < DEV_COUNT; i++)
    {
        init_status = ina226_init(&devs[i]);
        dev_ok[i] = (init_status == I2C_OK);
        if (dev_ok[i])
        {
            printf("INA226[%d] 0x%02X OK\r\n", i, devs[i].address);
        }
        else
        {
            printf("INA226[%d] 0x%02X FAIL(%d)\r\n", i, devs[i].address, init_status);
        }
    }

    dac8571_init();
    dac_status = dac8571_set_output(0);
    dac_ok = (dac_status == I2C_OK);
    printf("DAC8571 %s\r\n", dac_ok ? "OK" : "FAIL");



    /* ---- 2. USB-CDC early init (printf -> USB CDC from here on) ---- */
    usb_cdc_init();

    printf("\r\n=== Electronic Load Controller ===\r\n");
    printf("SystemClk:%d\r\n", SystemCoreClock);
    printf("ChipID:%08x\r\n", DBGMCU_GetCHIPID());

    /* Wait for USB host enumeration */
    enum_timeout = 5000;
    while (USBFS_DevEnumStatus == 0 && enum_timeout > 0)
    {
        Delay_Ms(10);
        enum_timeout -= 10;
    }
    if (USBFS_DevEnumStatus)
    {
        printf("USB-CDC enumerated\r\n");
    }
    else
    {
        printf("USB-CDC timeout, continuing\r\n");
    }

    /* ---- 3. I2C1 Bus ---- */



    /* ---- 4. Protocol (USART1 cJSON) ----
     * Two-step init matches WCH USART example: TX-only first,
     * then full TX+RX+interrupt configuration. */
   

    /* ---- 5. INA226 Devices ---- */


    /* ---- 6. EXTI4 - INA226 wired-OR ALARM ---- */
    // {
    //     GPIO_InitTypeDef GPIO_InitStructure = {0};
    //     EXTI_InitTypeDef EXTI_InitStructure = {0};

    //     GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;
    //     GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    //     GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    //     GPIO_Init(GPIOA, &GPIO_InitStructure);

    //     GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource4);

    //     EXTI_InitStructure.EXTI_Line    = EXTI_Line4;
    //     EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    //     EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    //     EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    //     EXTI_Init(&EXTI_InitStructure);

    //     NVIC_SetPriority(EXTI4_IRQn, 0x01);
    //     NVIC_EnableIRQ(EXTI4_IRQn);
    //     printf("EXTI4 PA4 ready\r\n");
    // }

    /* ---- 7. DAC8571 ---- */



    /* ---- 8. PID Controllers ---- */
    // pid_init(&pid_cv, PID_CV_KP, PID_CV_KI, PID_CV_KD);
    // pid_init(&pid_cc, PID_CC_KP, PID_CC_KI, PID_CC_KD);
    // printf("PID ready\r\n");

    /* ---- 9. Fault Handler ---- */
    // fault_init();
    // printf("Fault ready, max retries %d\r\n", MAX_RETRY_COUNT);

    /* ---- 10. Optional peripherals (uncomment when hw ready) ---- */
    /* ws2812_init(); */
    /* temp_sensor_init(); */
    /* fan_init(); */

    // printf("Init complete, entering loop\r\n");

    /* ==================================================================
     * Main Control Loop - 100ms cycle (10Hz)
     *
     * UART1 (odd parity): receive cJSON set_mode CV/CC commands
     * USB-CDC:            send cJSON telemetry at 10 Hz
     * ================================================================== */
    while (1)
    {
        /* ---- Poll USART1 for cJSON commands ---- */
        {
            const char *cmd_line = protocol_poll();
            if (cmd_line != NULL)
            {
                protocol_process_command(cmd_line);
            }
        }

        /* ---- Send telemetry over USB-CDC at 10 Hz ---- */
        cdc_send_telemetry();

        cycle_count++;
        Delay_Ms(CONTROL_PERIOD_MS);
    }
}
/*********************************************************************
 * @fn      USART1_IRQHandler
 *
 * @brief   This function handles USART1 global interrupt request.
 *
 * @return  none
 */
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        // IDLE
        uint16_t rxlen     = (RX_BUFFER_LEN - USART_RX_CH->CNTR);
        uint8_t  oldbuffer = USART_DMA_CTRL.DMA_USE_BUFFER;

        USART_DMA_CTRL.DMA_USE_BUFFER = !oldbuffer;

        DMA_Cmd(USART_RX_CH, DISABLE);
        DMA_SetCurrDataCounter(USART_RX_CH, RX_BUFFER_LEN);
        // Switch buffer
        USART_RX_CH->MADDR = (uint32_t)(USART_DMA_CTRL.Rx_Buffer[USART_DMA_CTRL.DMA_USE_BUFFER]);
        DMA_Cmd(USART_RX_CH, ENABLE);

        USART_ReceiveData(USART1); // clear IDLE flag
        ring_buffer_push_huge(USART_DMA_CTRL.Rx_Buffer[oldbuffer], rxlen);
    }
}

/*********************************************************************
 * @fn      DMA1_Channel5_IRQHandler
 *
 * @brief   This function handles DMA1 Channel 5 global interrupt request.
 *
 * @return  none
 */
void DMA1_Channel5_IRQHandler(void)
{
    uint16_t rxlen     = RX_BUFFER_LEN;
    uint8_t  oldbuffer = USART_DMA_CTRL.DMA_USE_BUFFER;
    // FULL

    USART_DMA_CTRL.DMA_USE_BUFFER = !oldbuffer;

    DMA_Cmd(USART_RX_CH, DISABLE);
    DMA_SetCurrDataCounter(USART_RX_CH, RX_BUFFER_LEN);
    // Switch buffer
    USART_RX_CH->MADDR = (uint32_t)(USART_DMA_CTRL.Rx_Buffer[USART_DMA_CTRL.DMA_USE_BUFFER]);
    DMA_Cmd(USART_RX_CH, ENABLE);

    ring_buffer_push_huge(USART_DMA_CTRL.Rx_Buffer[oldbuffer], rxlen);

    DMA_ClearITPendingBit(DMA1_IT_TC5);
}

