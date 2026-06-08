/********************************** (C) COPYRIGHT  *******************************
* File Name          : temp_sensor.h
* Author             : GSD Phase 04
* Version            : V1.0.0
* Date               : 2026/06/08
* Description        : NTC temperature sensor header.
*                      Reads NTC thermistor (B=3950, 10kΩ at 25°C) via ADC1
*                      on PA5 using a voltage divider (10kΩ fixed to VDD, NTC to
*                      GND). Pre-computed 100-entry lookup table converts ADC
*                      readings to 0.1°C units. Synchronous single-conversion
*                      mode — no interrupt or DMA.
*******************************************************************************/
#ifndef __DRIVERS_TEMP_SENSOR_H
#define __DRIVERS_TEMP_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* --------------------------------------------------------------------------
 * External Global
 * -------------------------------------------------------------------------- */

/** Current heatsink temperature in degrees Celsius.
 *  Updated each 100ms cycle by temp_sensor_read().
 *  Initialized to 25.0f (room temperature default). */
extern volatile float heatsink_temp_c;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/**
 * @brief  Initialize NTC temperature sensor hardware.
 *
 *         Enables ADC1 + GPIOA clocks, configures PA5 as analog input,
 *         sets up ADC1 for single-channel single-conversion on Channel 5
 *         at 239.5-cycle sample time (for high-impedance NTC source),
 *         calibrates ADC1, and initializes heatsink_temp_c to 25.0°C.
 *
 *         ADC1 clock: PCLK2 / 6 = 72MHz / 6 = 12MHz (within CH32V303 spec).
 *         Total conversion time: ~30μs (sample + 12.5 ADC cycles).
 *
 * @return none
 */
void temp_sensor_init(void);

/**
 * @brief  Trigger ADC1 conversion and return temperature in °C.
 *
 *         Blocks for ~30μs while conversion completes. Converts raw ADC
 *         reading to °C via static lookup table. Updates heatsink_temp_c
 *         global with the new reading.
 *
 *         Clamping:
 *         - ADC >= 4000: returns -40.0°C (open circuit / extremely cold)
 *         - ADC <= 50:   returns 100.0°C (short circuit / extremely hot)
 *
 *         No runtime math.h calls — lookup table is pre-computed at compile time.
 *
 * @return Temperature in degrees Celsius as float
 */
float temp_sensor_read(void);

#ifdef __cplusplus
}
#endif

#endif /* __DRIVERS_TEMP_SENSOR_H */
