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
#include "../Drivers/usb_cdc.h"

/* External references for ISR modules */
extern INA226_Dev devs[5];

/* Global typedef */

/* Global define */

/* Number of INA226 devices on I2C1 bus */
#define DEV_COUNT    5

/* Global Variable */

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

    /* Main loop: read all INA226 channels and print via USART1 */
    while (1)
    {
        for (i = 0; i < DEV_COUNT; i++)
        {
            float busVoltage;
            float shuntVoltage;
            float current;
            float power;
            i2c_status_t st;

            /* Read bus voltage */
            busVoltage = 0.0f;
            st = ina226_get_bus_voltage(&devs[i], &busVoltage);
            if (st == I2C_OK)
            {
                printf("CH%d Bus: %.3f V  ", devs[i].channel, busVoltage);
                usb_printf("CH%d Bus: %.3f V  ", devs[i].channel, busVoltage);
            }
            else
            {
                printf("CH%d Bus: ERR(%d)  ", devs[i].channel, st);
                usb_printf("CH%d Bus: ERR(%d)  ", devs[i].channel, st);
            }

            /* Read shunt voltage */
            shuntVoltage = 0.0f;
            st = ina226_get_shunt_voltage(&devs[i], &shuntVoltage);
            if (st == I2C_OK)
            {
                printf("Shunt: %.3f mV  ", shuntVoltage);
                usb_printf("Shunt: %.3f mV  ", shuntVoltage);
            }
            else
            {
                printf("Shunt: ERR(%d)  ", st);
                usb_printf("Shunt: ERR(%d)  ", st);
            }

            /* Read current */
            current = 0.0f;
            st = ina226_get_current(&devs[i], &current);
            if (st == I2C_OK)
            {
                printf("Cur: %.3f A  ", current);
                usb_printf("Cur: %.3f A  ", current);
            }
            else
            {
                printf("Cur: ERR(%d)  ", st);
                usb_printf("Cur: ERR(%d)  ", st);
            }

            /* Read power */
            power = 0.0f;
            st = ina226_get_power(&devs[i], &power);
            if (st == I2C_OK)
            {
                printf("Pwr: %.3f W\r\n", power);
                usb_printf("Pwr: %.3f W\r\n", power);
            }
            else
            {
                printf("Pwr: ERR(%d)\r\n", st);
                usb_printf("Pwr: ERR(%d)\r\n", st);
            }
        }

        /* DAC status reminder: mid-scale value is active */
        printf("DAC=0x8000 (mid-scale)\r\n\r\n");
        usb_printf("DAC=0x8000 (mid-scale)\r\n\r\n");

        Delay_Ms(500);
    }
}
