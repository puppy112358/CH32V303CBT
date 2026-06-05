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

/* Global typedef */

/* Global define */

/* Global Variable */

static INA226_Dev dev_ch0 = {0x40, 0};  /* I2C address 0x40, channel 0 */


/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	SystemCoreClockUpdate();
	Delay_Init();
	USART_Printf_Init(115200);	
	printf("SystemClk:%d\r\n", SystemCoreClock);
	printf("ChipID:%08x\r\n", DBGMCU_GetCHIPID());
	printf("Phase 01: Hardware Foundation\r\n");

	/* Initialize I2C1 bus and INA226 device */
	printf("Initializing I2C1 at 100kHz...\r\n");
	i2c_util_init();
	printf("I2C1 initialized\r\n");

	{
		i2c_status_t init_status;
		init_status = ina226_init(&dev_ch0);
		if (init_status == I2C_OK)
		{
			printf("INA226 CH0 init OK\r\n");
		}
		else
		{
			printf("INA226 CH0 init FAIL: %d\r\n", init_status);
		}
	}

	/* Main loop: read INA226 bus voltage and print via USART1 */
	while (1)
	{
		float bus_voltage;
		i2c_status_t status;

		bus_voltage = 0.0f;
		status = ina226_get_bus_voltage(&dev_ch0, &bus_voltage);

		if (status == I2C_OK)
		{
			printf("CH0 Bus: %.3f V\r\n", bus_voltage);
		}
		else
		{
			printf("CH0 Read Error: %d\r\n", status);
		}

		Delay_Ms(500);
	}
}

