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
 USART Print debugging routine:
 USART1_Tx(PA9).
 This example demonstrates using USART1(PA9) as a print debug port output.

*/

#include "debug.h"
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
/* USB IN Endpoint Busy Flag and endpoint numbers from USBFS device stack */
extern volatile uint8_t USBFS_Endp_Busy[];
#define DEF_UEP3  0x03

/* External references for ISR modules */
extern INA226_Dev devs[5];

/* Global typedef */

/* Global define */

/* Number of INA226 devices on I2C1 bus */
#define DEV_COUNT    5

/* Control loop timing */
#define CONTROL_PERIOD_MS   100
#define SOFTSTART_STEPS     5

/* System mode enumeration �?? defined in Drivers/fault.h */
/* (SystemMode: MODE_IDLE=0, MODE_CV=1, MODE_CC=2, MODE_FAULT=3) */

/* Global Variable */

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
uint32_t last_control_tick = 0;

/* Running cycle counter (extern-visible for protocol.c uptime in get_status) */
uint32_t cycle_count = 0;

/*
 * INA226 device array �?? 5 devices on I2C1 at addresses confirmed by hardware:
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
 * @brief   软启动线�?? DAC 斜坡: 500ms 内从 0 线性增加到目标值�?
 *          5 �?? × 100ms，仅�?? IDLE→CV �?? IDLE→CC 转换时调用�?
 *
 * @param   target_dac - 目标 DAC �?? (0-65535)
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

        dac8571_set_output(dac_out);
        last_dac_value = dac_out;
        Delay_Ms(CONTROL_PERIOD_MS);
    }
}

/*********************************************************************
 * @fn      engage_cv
 *
 * @brief   切换到恒压模�?? (CV)�??
 *          计算初始 PID 输出，执行软启动斜坡，预加载积分�??
 *          以实现无扰切捀�??
 *
 * @param   target_voltage - 目标电压 (V)
 *
 * @return  none
 */
void engage_cv(float target_voltage)
{
    float bus_v;
    float output;

    system_mode = MODE_CV;
    cv_target_voltage = target_voltage;

    /* Compute initial PID output based on current feedback */
    bus_v = 0.0f;
    ina226_get_bus_voltage(&devs[4], &bus_v);
    output = pid_compute(&pid_cv, cv_target_voltage, bus_v, 0.1f);

    /* Soft-start ramp from 0 to PID output */
    softstart_engage((uint16_t)output);

    /* Pre-load integral term for bumpless transfer */
    pid_set_integral(&pid_cv, pid_cv.output);

    printf("Engage CV: target=%.2fV DAC=%u\r\n", target_voltage, (uint16_t)pid_cv.output);
}

/*********************************************************************
 * @fn      engage_cc
 *
 * @brief   切换到恒流模�?? (CC)�??
 *          计算初始 PID 输出，执行软启动斜坡，预加载积分�??
 *          以实现无扰切捀�??
 *
 * @param   target_current - 目标电流 (A)
 *
 * @return  none
 */
void engage_cc(float target_current)
{
    float bus_i;
    float output;

    system_mode = MODE_CC;
    cc_target_current = target_current;

    /* Compute initial PID output based on current feedback */
    bus_i = 0.0f;
    ina226_get_current(&devs[4], &bus_i);
    output = pid_compute(&pid_cc, cc_target_current, bus_i, 0.1f);

    /* Soft-start ramp from 0 to PID output */
    softstart_engage((uint16_t)output);

    /* Pre-load integral term for bumpless transfer */
    pid_set_integral(&pid_cc, pid_cc.output);

    printf("Engage CC: target=%.2fA DAC=%u\r\n", target_current, (uint16_t)pid_cc.output);
}


/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    // uint8_t i;
    // i2c_status_t init_status;
    // i2c_status_t dac_status;
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    printf("SystemClk:%d\r\n", SystemCoreClock);
    printf("ChipID:%08x\r\n", DBGMCU_GetCHIPID());
    printf("Phase 01: Hardware Foundation\r\n");
    // /* Initialize I2C1 bus */
    printf("Initializing I2C1 at 100kHz...\r\n");
    i2c_util_init();
    printf("I2C1 initialized\r\n");

    /* Initialize all INA226 devices in a loop */
    printf("Initializing %d INA226 devices...\r\n", DEV_COUNT);
    // for (i = 0; i < DEV_COUNT; i++)
    // {
    //     init_status = ina226_init(&devs[i]);
    //     if (init_status == I2C_OK)
    //     {
    //         printf("INA226[%d] at 0x%02X init: OK\r\n",
    //                i, devs[i].address);
    //     }
    //     else
    //     {
    //         printf("INA226[%d] at 0x%02X init: FAIL (%d)\r\n",
    //                i, devs[i].address, init_status);
    //     }
    // }

    /* Configure PA4 as EXTI4 falling-edge input for INA226 wired-OR ALARM signal */
        GPIO_InitTypeDef GPIO_InitStructure = {0};
        EXTI_InitTypeDef EXTI_InitStructure = {0};

        /* PA4: input with pull-up for active-low wired-OR ALARM */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        /* Map EXTI line 4 to GPIOA port */
        GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource4);

        /* EXTI4: interrupt mode, falling-edge trigger */
        EXTI_InitStructure.EXTI_Line = EXTI_Line4;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        EXTI_Init(&EXTI_InitStructure);

        /* NVIC: EXTI4_IRQn at priority 0x01 (second-highest after system exceptions) */
        NVIC_SetPriority(EXTI4_IRQn, 0x01);
        NVIC_EnableIRQ(EXTI4_IRQn);

        printf("EXTI4 on PA4 configured (INA226 ALARM input)\r\n");


    /* Initialize DAC8571 and write mid-scale test value */
    // dac8571_init();
    // dac_status = dac8571_set_output(0x8000);
    // if (dac_status == I2C_OK)
    // {
    //     printf("DAC8571 mid-scale test: OK\r\n");
    // }
    // else
    // {
    //     printf("DAC8571 mid-scale test: FAIL (%d)\r\n", dac_status);
    // }

    /* Initialize USB-CDC for debug output via virtual COM port */
    printf("Initializing USB-CDC...\r\n");
    usb_cdc_init();
    printf("USB-CDC waiting for host enumeration...\r\n");

    /* Wait for USB enumeration to complete (host sends SET_CONFIGURATION).
     * Without this, usb_printf data would be queued but never polled by the host,
     * permanently blocking EP3 (Busy flag stays set). */
    {
        uint32_t enum_timeout = 5000;  /* 5 seconds max wait */
        while (USBFS_DevEnumStatus == 0 && enum_timeout > 0)
        {
            Delay_Ms(10);
            enum_timeout -= 10;
        }
        if (USBFS_DevEnumStatus)
        {
            printf("USB-CDC enumerated, ready\r\n");
        }
        else
        {
            printf("USB-CDC enumeration timeout �? host not connected?\r\n");
        }
    }

    /* Initialize protocol (USART2 command/telemetry channel) */
    protocol_init();

    /* Initialize PID controllers */
    // pid_init(&pid_cv, PID_CV_KP, PID_CV_KI, PID_CV_KD);
    // pid_init(&pid_cc, PID_CC_KP, PID_CC_KI, PID_CC_KD);
    // printf("PID controller initialized (CV: Kp=%.2f Ki=%.2f Kd=%.2f, CC: Kp=%.2f Ki=%.2f Kd=%.2f)\r\n",
    //        PID_CV_KP, PID_CV_KI, PID_CV_KD,
    //        PID_CC_KP, PID_CC_KI, PID_CC_KD);

    /* Initialize fault handler */
    // fault_init();
    // printf("Fault handler initialized (rated: %.1fW, max retries: %d)\r\n",
    //        RATED_WATTAGE, MAX_RETRY_COUNT);

    /* Initialize WS2812 LED driver (TIM2 CH1 PA0 PWM + DMA1 CH5) */
    // ws2812_init();

    /* Initialize NTC temperature sensor (ADC1 CH5 PA5) */
    // temp_sensor_init();

    /* Initialize fan controller (TIM3 PA6 PWM 25kHz + PA7 tacho + PID 50°C) */
    // fan_init();

    /* Phase 2 test engage replaced by cJSON commands in Phase 3 */
    /* System now starts in MODE_IDLE and waits for commands via USART2 */

    /* Main control loop: 100ms SysTick-gated */

    /* Diagnostic: test usb_printf and report via UART1 */
    {
        int ret;
        ret = usb_printf("SystemClk:%d\r\n", SystemCoreClock);
        printf("[USB-DIAG] usb_printf(SystemClk) returned %d\r\n", ret);
        ret = usb_printf("USB-CDC ready\r\n");
        printf("[USB-DIAG] usb_printf(ready) returned %d\r\n", ret);
        printf("[USB-DIAG] EP3_Busy=%d, DevEnumStatus=%d\r\n",
               USBFS_Endp_Busy[DEF_UEP3], USBFS_DevEnumStatus);
    }

    while (1)
    {
        static uint32_t last_usb_test_ms = 0;
        int ret;

        /* Periodic USB CDC test �? attempt every 1000ms */
        if ((cycle_count - last_usb_test_ms) >= 10)  /* 10 loops × 100ms = 1s */
        {
            last_usb_test_ms = cycle_count;

            ret = usb_printf("USB CDC alive, cycle=%lu\r\n", cycle_count);
            if (ret < 0)
            {
                /* Report failure via UART1 so user can see what's happening */
                printf("[USB-DIAG] usb_printf failed (ret=%d, EP3_Busy=%d, Enum=%d)\r\n",
                       ret, USBFS_Endp_Busy[DEF_UEP3], USBFS_DevEnumStatus);
            }
        }

        Delay_Ms(100);
        cycle_count++;
        // {
        //     continue;
        // }
        // last_control_tick = now;

        /* 0. Poll for incoming commands (Phase 3) */
        // {
        //     const char *cmd_line = protocol_poll();
        //     if (cmd_line != NULL)
        //     {
        //         protocol_process_command(cmd_line);
        //     }
        // }

        /* 1. Read summary INA226 for PID feedback (devs[4] = summary) */
        // bus_v = 0.0f;
        // bus_i = 0.0f;
        // bus_p = 0.0f;
        // ina226_get_bus_voltage(&devs[4], &bus_v);
        // ina226_get_current(&devs[4], &bus_i);
        // ina226_get_power(&devs[4], &bus_p);

        /* 2. Read 4 MOS channel currents and voltages for monitoring */
        // for (i = 0; i < 4; i++)
        // {
        //     mos_i[i] = 0.0f;
        //     mos_v[i] = 0.0f;
        //     ina226_get_current(&devs[i], &mos_i[i]);
        //     ina226_get_bus_voltage(&devs[i], &mos_v[i]);
        // }

        /* 2.5. Read NTC heatsink temperature (for telemetry + fan control) */
        // temp_sensor_read();

        /* 3. Check fault flag from EXTI4 ISR */
        // if (fault_triggered)
        // {
        //     system_mode = MODE_FAULT;
        //     fault_handler_hw(fault_source_mask);
        //     dac8571_set_output(0);
        //     last_dac_value = 0;
        //     fault_print_snapshot(&fault_reg, 0, MODE_FAULT);
        //     fault_triggered = 0;
        // }

        /* 4. OPP check (D-13): total power over-limit */
        // if (bus_p > RATED_WATTAGE && system_mode != MODE_FAULT)
        // {
        //     fault_handler_opp();
        //     dac8571_set_output(0);
        //     last_dac_value = 0;
        //     fault_print_snapshot(&fault_reg, last_dac_value, system_mode);
        //     system_mode = MODE_FAULT;
        // }

        /* 5. State machine dispatch */
        // if (system_mode == MODE_CV && !fault_triggered)
        // {
        //     float output = pid_compute(&pid_cv, cv_target_voltage, bus_v, 0.1f);
        //     last_dac_value = (uint16_t)output;
        //     dac8571_set_output(last_dac_value);
        // }
        // else if (system_mode == MODE_CC && !fault_triggered)
        // {
        //     float output = pid_compute(&pid_cc, cc_target_current, bus_i, 0.1f);
        //     last_dac_value = (uint16_t)output;
        //     dac8571_set_output(last_dac_value);
        // }

        /* 5.5. Update WS2812 LED color if system mode changed */
        // ws2812_update_from_mode(system_mode);

        /* 5.6. Compute fan PID + update PWM duty + read RPM + check stall */
        // fan_update(heatsink_temp_c);

        /* 6. Fault state machine: manage auto-retry, cooldown, latch */
        // if (system_mode == MODE_FAULT)
        // {
        //     fault_state_machine();
        // }

        /* 6.5. Send telemetry packet (Phase 3 �?? COMM-02) */
        // protocol_send_telemetry(bus_v, bus_i, bus_p, mos_i, mos_v);

        /* 7. Retry counter reset (D-03): 30s fault-free �?? reset to 0 */
        // if (system_mode != MODE_FAULT)
        // {
        //     fault_free_ms += CONTROL_PERIOD_MS;
        //     if (fault_free_ms >= FAULT_FREE_RESET_MS)
        //     {
        //         fault_reg.bits.retry_count = 0;
        //         fault_free_ms = 0;
        //     }
        // }
        // else
        // {
        //     fault_free_ms = 0;
        // }

        /* 8. Periodic calibration check (PROT-04): every 10 cycles (1s) */
        // cycle_count++;
        // if (cycle_count >= 10)
        // {
            // uint16_t cal_val;
            // float check_v;

            // cycle_count = 0;
            // for (i = 0; i < DEV_COUNT; i++)
            // {
            //     cal_val = 0;
            //     if (ina226_read_calibration(&devs[i], &cal_val) != I2C_OK)
            //     {
            //         continue;
            //     }
            //     if (cal_val == 0)
            //     {
            //         check_v = 0.0f;
            //         if (ina226_get_bus_voltage(&devs[i], &check_v) == I2C_OK
            //             && check_v > 0.1f)
            //         {
            //             printf("[PROT-04] CH%d cal=0 bus=%.2fV �?? re-initializing\r\n",
            //                    devs[i].channel, check_v);
            //             ina226_init(&devs[i]);
            //         }
            //     }
            // }
        // }

        /* 9. Print summary */
        // printf("Mode:%d V:%.2f I:%.2f P:%.2f DAC:%u MOS:%.2f %.2f %.2f %.2f\r\n",
        //        system_mode, bus_v, bus_i, bus_p, last_dac_value,
        //        mos_i[0], mos_i[1], mos_i[2], mos_i[3]);
    }
}
