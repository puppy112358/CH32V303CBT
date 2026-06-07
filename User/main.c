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
#include "../Drivers/protocol.h"
#include "../Drivers/usb_cdc.h"

/* External references for ISR modules */
extern INA226_Dev devs[5];

/* Global typedef */

/* Global define */

/* Number of INA226 devices on I2C1 bus */
#define DEV_COUNT    5

/* Control loop timing */
#define CONTROL_PERIOD_MS   100
#define SOFTSTART_STEPS     5

/* System mode enumeration — defined in Drivers/fault.h */
/* (SystemMode: MODE_IDLE=0, MODE_CV=1, MODE_CC=2, MODE_FAULT=3) */

/* Global Variable */

/* ISR-to-main-loop fault flags (defined in ch32v30x_it.c) */
extern volatile uint8_t  fault_triggered;
extern volatile uint16_t fault_source_mask;
extern volatile uint16_t last_dac_value;

/* PID instances */
PID_Instance pid_cv;
PID_Instance pid_cc;

/* System state */
SystemMode system_mode = MODE_IDLE;
float cv_target_voltage = 0.0f;
float cc_target_current = 0.0f;
uint32_t last_control_tick = 0;

/*
 * INA226 device array — 5 devices on I2C1 at addresses confirmed by hardware:
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
 * @brief   软启动线性 DAC 斜坡: 500ms 内从 0 线性增加到目标值。
 *          5 步 × 100ms，仅在 IDLE→CV 和 IDLE→CC 转换时调用。
 *
 * @param   target_dac - 目标 DAC 值 (0-65535)
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
 * @brief   切换到恒压模式 (CV)。
 *          计算初始 PID 输出，执行软启动斜坡，预加载积分项
 *          以实现无扰切换。
 *
 * @param   target_voltage - 目标电压 (V)
 *
 * @return  none
 */
static void engage_cv(float target_voltage)
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
 * @brief   切换到恒流模式 (CC)。
 *          计算初始 PID 输出，执行软启动斜坡，预加载积分项
 *          以实现无扰切换。
 *
 * @param   target_current - 目标电流 (A)
 *
 * @return  none
 */
static void engage_cc(float target_current)
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
    uint8_t i;
    i2c_status_t init_status;
    i2c_status_t dac_status;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    printf("SystemClk:%d\r\n", SystemCoreClock);
    printf("ChipID:%08x\r\n", DBGMCU_GetCHIPID());
    printf("Phase 01: Hardware Foundation\r\n");

    /* Initialize I2C1 bus */
    printf("Initializing I2C1 at 100kHz...\r\n");
    i2c_util_init();
    printf("I2C1 initialized\r\n");

    /* Initialize all INA226 devices in a loop */
    printf("Initializing %d INA226 devices...\r\n", DEV_COUNT);
    for (i = 0; i < DEV_COUNT; i++)
    {
        init_status = ina226_init(&devs[i]);
        if (init_status == I2C_OK)
        {
            printf("INA226[%d] at 0x%02X init: OK\r\n",
                   i, devs[i].address);
        }
        else
        {
            printf("INA226[%d] at 0x%02X init: FAIL (%d)\r\n",
                   i, devs[i].address, init_status);
        }
    }

    /* Configure PA4 as EXTI4 falling-edge input for INA226 wired-OR ALARM signal */
    {
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
    }

    /* Initialize DAC8571 and write mid-scale test value */
    dac8571_init();
    dac_status = dac8571_set_output(0x8000);
    if (dac_status == I2C_OK)
    {
        printf("DAC8571 mid-scale test: OK\r\n");
    }
    else
    {
        printf("DAC8571 mid-scale test: FAIL (%d)\r\n", dac_status);
    }

    /* Initialize USB-CDC for debug output via virtual COM port */
    printf("Initializing USB-CDC...\r\n");
    usb_cdc_init();
    printf("USB-CDC ready — connect USB for virtual COM port\r\n");

    /* Initialize protocol (USART2 command/telemetry channel) */
    protocol_init();

    /* Initialize PID controllers */
    pid_init(&pid_cv, PID_CV_KP, PID_CV_KI, PID_CV_KD);
    pid_init(&pid_cc, PID_CC_KP, PID_CC_KI, PID_CC_KD);
    printf("PID controller initialized (CV: Kp=%.2f Ki=%.2f Kd=%.2f, CC: Kp=%.2f Ki=%.2f Kd=%.2f)\r\n",
           PID_CV_KP, PID_CV_KI, PID_CV_KD,
           PID_CC_KP, PID_CC_KI, PID_CC_KD);

    /* Initialize fault handler */
    fault_init();
    printf("Fault handler initialized (rated: %.1fW, max retries: %d)\r\n",
           RATED_WATTAGE, MAX_RETRY_COUNT);

    /* Phase 2 test engage replaced by cJSON commands in Phase 3 */
    /* System now starts in MODE_IDLE and waits for commands via USART2 */

    /* Main control loop: 100ms SysTick-gated */
    while (1)
    {
        float bus_v;
        float bus_i;
        float bus_p;
        float mos_i[4];
        uint32_t now;
        static uint32_t cycle_count = 0;
        static uint32_t fault_free_ms = 0;

        /* 100ms control period gating via SysTick counter */
        now = SysTick->CNT;
        if ((now - last_control_tick) < (SystemCoreClock / 1000 * CONTROL_PERIOD_MS / 1000))
        {
            continue;
        }
        last_control_tick = now;

        /* 1. Read summary INA226 for PID feedback (devs[4] = summary) */
        bus_v = 0.0f;
        bus_i = 0.0f;
        bus_p = 0.0f;
        ina226_get_bus_voltage(&devs[4], &bus_v);
        ina226_get_current(&devs[4], &bus_i);
        ina226_get_power(&devs[4], &bus_p);

        /* 2. Read 4 MOS channel currents for monitoring */
        for (i = 0; i < 4; i++)
        {
            mos_i[i] = 0.0f;
            ina226_get_current(&devs[i], &mos_i[i]);
        }

        /* 3. Check fault flag from EXTI4 ISR */
        if (fault_triggered)
        {
            system_mode = MODE_FAULT;
            fault_handler_hw(fault_source_mask);
            dac8571_set_output(0);
            last_dac_value = 0;
            fault_print_snapshot(&fault_reg, 0, MODE_FAULT);
            fault_triggered = 0;
        }

        /* 4. OPP check (D-13): total power over-limit */
        if (bus_p > RATED_WATTAGE && system_mode != MODE_FAULT)
        {
            fault_handler_opp();
            dac8571_set_output(0);
            last_dac_value = 0;
            fault_print_snapshot(&fault_reg, last_dac_value, system_mode);
            system_mode = MODE_FAULT;
        }

        /* 5. State machine dispatch */
        if (system_mode == MODE_CV && !fault_triggered)
        {
            float output = pid_compute(&pid_cv, cv_target_voltage, bus_v, 0.1f);
            last_dac_value = (uint16_t)output;
            dac8571_set_output(last_dac_value);
        }
        else if (system_mode == MODE_CC && !fault_triggered)
        {
            float output = pid_compute(&pid_cc, cc_target_current, bus_i, 0.1f);
            last_dac_value = (uint16_t)output;
            dac8571_set_output(last_dac_value);
        }

        /* 6. Fault state machine: manage auto-retry, cooldown, latch */
        if (system_mode == MODE_FAULT)
        {
            fault_state_machine();
        }

        /* 7. Retry counter reset (D-03): 30s fault-free → reset to 0 */
        if (system_mode != MODE_FAULT)
        {
            fault_free_ms += CONTROL_PERIOD_MS;
            if (fault_free_ms >= FAULT_FREE_RESET_MS)
            {
                fault_reg.bits.retry_count = 0;
                fault_free_ms = 0;
            }
        }
        else
        {
            fault_free_ms = 0;
        }

        /* 8. Periodic calibration check (PROT-04): every 10 cycles (1s) */
        cycle_count++;
        if (cycle_count >= 10)
        {
            uint16_t cal_val;
            float check_v;

            cycle_count = 0;
            for (i = 0; i < DEV_COUNT; i++)
            {
                cal_val = 0;
                if (ina226_read_calibration(&devs[i], &cal_val) != I2C_OK)
                {
                    continue;
                }
                if (cal_val == 0)
                {
                    check_v = 0.0f;
                    if (ina226_get_bus_voltage(&devs[i], &check_v) == I2C_OK
                        && check_v > 0.1f)
                    {
                        printf("[PROT-04] CH%d cal=0 bus=%.2fV — re-initializing\r\n",
                               devs[i].channel, check_v);
                        ina226_init(&devs[i]);
                    }
                }
            }
        }

        /* 9. Print summary */
        printf("Mode:%d V:%.2f I:%.2f P:%.2f DAC:%u MOS:%.2f %.2f %.2f %.2f\r\n",
               system_mode, bus_v, bus_i, bus_p, last_dac_value,
               mos_i[0], mos_i[1], mos_i[2], mos_i[3]);
    }
}
