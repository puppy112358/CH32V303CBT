/********************************** (C) COPYRIGHT  *******************************
* File Name          : temp_sensor.c
* Author             : GSD Phase 04
* Version            : V1.0.0
* Date               : 2026/06/08
* Description        : NTC temperature sensor implementation.
*                      Reads NTC thermistor (B=3950, 10kΩ at 25°C) via ADC1 on
*                      PA5. Voltage divider: 10kΩ fixed from VDD to PA5, NTC
*                      from PA5 to GND. 100-entry lookup table maps ADC readings
*                      to 0.1°C units with +40°C offset. Synchronous single
*                      conversion — no interrupt, no DMA.
*******************************************************************************/
#include "../Drivers/temp_sensor.h"
#include "debug.h"

/* --------------------------------------------------------------------------
 * Module Constants
 * -------------------------------------------------------------------------- */
#define NTC_TABLE_SIZE     100
#define NTC_ADC_MIN        50     /* Minimum valid ADC (~100°C, R_ntc ≈ 0Ω) */
#define NTC_ADC_MAX        4000   /* Maximum valid ADC (~0°C, R_ntc very high) */
#define NTC_ADC_STEP       40     /* (4000 - 50) / 100 ≈ 39.5 → use 40 */
#define NTC_TEMP_OFFSET    400    /* +40°C offset in 0.1°C units (0 = -40°C) */

/* --------------------------------------------------------------------------
 * Global Variable
 * -------------------------------------------------------------------------- */
volatile float heatsink_temp_c = 25.0f;   /* Default room temp until first ADC read */

/* --------------------------------------------------------------------------
 * NTC Temperature Lookup Table (100 entries)
 *
 * Topology: VDD ---[10kΩ]--- PA5 ---[NTC]--- GND
 *
 * Formula used for table generation:
 *   R_ntc = 10000.0 * ADC / (4095.0 - ADC)
 *   T_K   = 1.0 / (ln(R_ntc / 10000.0) / 3950.0 + 1.0 / 298.15)
 *   T_C   = T_K - 273.15
 *   temp_01c = round((T_C + 40.0) * 10.0)
 *
 * Deviation from plan (D-04-02-01): The plan specified
 *   R_ntc = 10000.0 * (4095.0 / ADC - 1.0)
 * which is incorrect for the NTC-to-GND divider topology (it gives the
 * reciprocal of the correct resistance). The correct formula for a
 * divider with fixed R from VDD and NTC to GND is:
 *   R_ntc = R_fixed * ADC / (VREF_max - ADC)
 * = 10000 * ADC / (4095 - ADC).
 *
 * Key reference points (verified):
 *   ADC=2047 → R=10000Ω → 25.0°C → temp_01c=650
 *   ADC=1082 → R=  3591Ω → 50.0°C → temp_01c=900
 *   ADC=3156 → R= 33668Ω →  0.0°C → temp_01c=400
 * -------------------------------------------------------------------------- */
static const int16_t ntc_temp_table[NTC_TABLE_SIZE] = {
    2129, 1847, 1687, 1576, 1492, 1425, 1369, 1321, 1279, 1242,
    1209, 1179, 1151, 1125, 1102, 1079, 1058, 1039, 1020, 1002,
     985,  969,  954,  939,  924,  911,  897,  884,  872,  859,
     847,  836,  824,  813,  802,  792,  781,  771,  761,  751,
     741,  732,  722,  713,  703,  694,  685,  676,  667,  658,
     649,  641,  632,  623,  615,  606,  597,  589,  580,  572,
     563,  554,  546,  537,  528,  519,  511,  502,  493,  484,
     475,  465,  456,  446,  437,  427,  417,  407,  396,  386,
     375,  364,  352,  340,  328,  315,  302,  288,  273,  258,
     241,  224,  205,  185,  162,  137,  108,   74,   33,  -22
};

/*********************************************************************
 * @fn      temp_sensor_read
 *
 * @brief   Trigger ADC1 conversion on Channel 5 (PA5), convert raw
 *          reading to degrees Celsius via lookup table, update the
 *          heatsink_temp_c global, and return the temperature.
 *
 *          Blocks for ~30μs (ADC sample + conversion at 12MHz).
 *          No runtime math.h calls — lookup table is pre-computed.
 *
 *          Clamping:
 *            ADC <= NTC_ADC_MIN (50):  returns +100.0°C (short)
 *            ADC >= NTC_ADC_MAX (4000): returns -40.0°C (open)
 *
 * @return  Temperature in degrees Celsius
 */
float temp_sensor_read(void)
{
    uint16_t adc;
    uint8_t  idx;
    int16_t  temp_01c;
    float    temp_c;

    /* Trigger single ADC conversion on channel 5 */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    /* Wait for conversion complete (~30μs at 12MHz ADC clock) */
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET)
    {
        /* Spin-wait — safe because ADC EOC always completes */
    }

    /* Read 12-bit result (right-aligned) */
    adc = ADC_GetConversionValue(ADC1);

    /* Clamp out-of-range values */
    if (adc <= NTC_ADC_MIN)
    {
        heatsink_temp_c = 100.0f;
        return 100.0f;
    }
    if (adc >= NTC_ADC_MAX)
    {
        heatsink_temp_c = -40.0f;
        return -40.0f;
    }

    /* Compute lookup table index */
    idx = (uint8_t)((adc - NTC_ADC_MIN) / NTC_ADC_STEP);

    /* Boundary safety — should never trigger due to clamping above */
    if (idx >= NTC_TABLE_SIZE)
    {
        idx = NTC_TABLE_SIZE - 1;
    }

    /* Lookup temperature in 0.1°C units with +40°C offset */
    temp_01c = ntc_temp_table[idx];

    /* Convert to float °C: temp_c = (temp_01c - 400) / 10.0 */
    temp_c = (float)(temp_01c - NTC_TEMP_OFFSET) / 10.0f;

    /* Update global */
    heatsink_temp_c = temp_c;

    return temp_c;
}

/*********************************************************************
 * @fn      temp_sensor_init
 *
 * @brief   Initialize ADC1 for NTC temperature sensing on PA5.
 *
 *          Sequence:
 *          1. Enable GPIOA (APB2) and ADC1 (APB2) clocks
 *          2. Set ADC prescaler to PCLK2/6 (12MHz from 72MHz)
 *          3. Configure PA5 as analog input
 *          4. Init ADC1: independent mode, single channel, single
 *             conversion, software trigger, right-aligned data
 *          5. Configure regular channel 5 at rank 1 with 239.5-cycle
 *             sample time (for high-impedance NTC source)
 *          6. Power up and calibrate ADC1
 *          7. Set default temperature to 25.0°C
 *
 * @return  none
 */
void temp_sensor_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    ADC_InitTypeDef  ADC_InitStructure = {0};

    /* ---- 1. Enable peripheral clocks ---- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    /* Set ADC clock: PCLK2 / 6 = 72MHz / 6 = 12MHz (within CH32V303 spec) */
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    /* ---- 2. Configure PA5 as analog input ---- */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* ---- 3. Configure ADC1 for single-channel single-conversion ---- */
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    /* ---- 4. Configure regular channel 5 at rank 1 with long sample time ---- */
    /* 239.5 cycles at 12MHz = ~20μs sample time for high-impedance NTC source */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 1, ADC_SampleTime_239Cycles5);

    /* ---- 5. Power up and calibrate ADC1 ---- */
    ADC_Cmd(ADC1, ENABLE);

    /* Reset calibration */
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1))
    {
        /* Wait for calibration reset to complete */
    }

    /* Start calibration */
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1))
    {
        /* Wait for calibration to complete */
    }

    /* ---- 6. Set initial temperature default (room temp assumption) ---- */
    heatsink_temp_c = 25.0f;

    printf("Temp sensor init: ADC1 CH5 PA5, NTC B=3950 10kΩ, 100-entry LUT\r\n");
}
