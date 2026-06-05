/********************************** (C) COPYRIGHT  *******************************
* File Name          : ina226.h
* Author             : GSD Phase 01
* Version            : V1.0.0
* Date               : 2026/06/05
* Description        : INA226 current/power monitor driver.
*                      Provides per-device initialization (calibration + config),
*                      and per-register getter functions. All I2C operations go
*                      through the shared i2c_util wrapper layer (D-05).
*******************************************************************************/
#ifndef __INA226_H
#define __INA226_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../Drivers/i2c_util.h"

/* --------------------------------------------------------------------------
 * INA226 Register Addresses (from datasheet)
 * -------------------------------------------------------------------------- */
#define INA226_REG_CONFIG        0x00   /* Configuration register */
#define INA226_REG_SHUNT_V       0x01   /* Shunt voltage register */
#define INA226_REG_BUS_V         0x02   /* Bus voltage register */
#define INA226_REG_POWER         0x03   /* Power register */
#define INA226_REG_CURRENT       0x04   /* Current register */
#define INA226_REG_CALIB         0x05   /* Calibration register */
#define INA226_REG_ALERT         0x06   /* Mask/Enable register (alert) */

/* --------------------------------------------------------------------------
 * Alert Register Flag Bits (Mask/Enable register 0x06)
 * -------------------------------------------------------------------------- */
#define INA226_ALERT_SHUNT_OV      ((uint16_t)1 << 15)  /* Shunt Voltage Over-Voltage */
#define INA226_ALERT_SHUNT_UV      ((uint16_t)1 << 14)  /* Shunt Voltage Under-Voltage */
#define INA226_ALERT_BUS_OV        ((uint16_t)1 << 13)  /* Bus Voltage Over-Voltage */
#define INA226_ALERT_BUS_UV        ((uint16_t)1 << 12)  /* Bus Voltage Under-Voltage */
#define INA226_ALERT_POWER_OV      ((uint16_t)1 << 11)  /* Power Over-Limit */
#define INA226_ALERT_CONV_READY    ((uint16_t)1 << 3)   /* Conversion Ready */

/* --------------------------------------------------------------------------
 * Calibration Constants (compile-time, all devices share these per D-10)
 *
 * Users MUST adjust INA226_R_SHUNT and INA226_MAX_CURRENT to match their
 * PCB design. The calibration formula follows the INA226 datasheet:
 *   Current_LSB = Max_Current / 32768
 *   Calibration = 0.00512 / (Current_LSB * R_Shunt)
 *
 * Defaults: 10 mOhm shunt, 5A max current
 *   -> Current_LSB = 5.0 / 32768 = 0.0001526 A/bit
 *   -> Calibration  = 0.00512 / (0.0001526 * 0.010) = 3355 = 0x0D1B
 * -------------------------------------------------------------------------- */

/* Shunt resistor value in ohms — MUST match PCB BOM */
#define INA226_R_SHUNT           0.010f

/* Maximum expected current in amperes */
#define INA226_MAX_CURRENT       5.0f

/* Current LSB = Max_Current / 32768 (A/bit) */
#define INA226_CURRENT_LSB       (INA226_MAX_CURRENT / 32768.0f)

/* Calibration register value (uint16_t) per datasheet Equation 1 */
#define INA226_CAL_VALUE         ((uint16_t)(0.00512f / (INA226_CURRENT_LSB * INA226_R_SHUNT)))

/* Configuration register default value:
 * - 1-sample averaging
 * - 1.1ms bus voltage conversion time
 * - 1.1ms shunt voltage conversion time
 * - Continuous shunt and bus voltage mode */
#define INA226_CONFIG_VALUE      0x4127

/* Bus voltage LSB = 1.25 mV */
#define INA226_BUS_VOLTAGE_LSB   0.00125f

/* Shunt voltage LSB = 2.5 uV */
#define INA226_SHUNT_VOLTAGE_LSB 0.0000025f

/* --------------------------------------------------------------------------
 * Device Structure (per D-09: per-device stateful struct)
 * -------------------------------------------------------------------------- */

typedef struct
{
    uint8_t address;    /* I2C 7-bit slave address */
    uint8_t channel;    /* Logical channel ID (0-4 for up to 5 devices) */
} INA226_Dev;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/* Initialize an INA226 device: write calibration register, then config register.
 * Returns the combined status (fail if either write fails). */
i2c_status_t ina226_init(INA226_Dev *dev);

/* Read the bus voltage register and convert to volts.
 * Bus voltage LSB = 1.25 mV. Stores result in *voltage_v.
 * Returns I2C status code from the underlying i2c_util_read call. */
i2c_status_t ina226_get_bus_voltage(INA226_Dev *dev, float *voltage_v);

/* Read the shunt voltage register (0x01) and convert to millivolts.
 * Shunt voltage LSB = 2.5 uV. Stores result in *voltage_mv.
 * Returns I2C status code from the underlying i2c_util_read call. */
i2c_status_t ina226_get_shunt_voltage(INA226_Dev *dev, float *voltage_mv);

/* Read the current register (0x04) and convert to amperes.
 * Current LSB = INA226_CURRENT_LSB. Stores result in *current_a.
 * Returns I2C status code from the underlying i2c_util_read call. */
i2c_status_t ina226_get_current(INA226_Dev *dev, float *current_a);

/* Read the power register (0x03) and convert to watts.
 * Power LSB = 25 * INA226_CURRENT_LSB. Stores result in *power_w.
 * Returns I2C status code from the underlying i2c_util_read call. */
i2c_status_t ina226_get_power(INA226_Dev *dev, float *power_w);

/* Read the alert Mask/Enable register (0x06) and return the raw mask value.
 * Per D-12: reads and returns the raw register value in *mask.
 * Application code decides what protective action to take.
 * Returns I2C status code from the underlying i2c_util_read call. */
i2c_status_t ina226_check_alert(INA226_Dev *dev, uint16_t *mask);

#ifdef __cplusplus
}
#endif

#endif /* __INA226_H */
