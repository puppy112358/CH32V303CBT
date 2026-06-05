/********************************** (C) COPYRIGHT  *******************************
* File Name          : ina226.c
* Author             : GSD Phase 01
* Version            : V1.0.0
* Date               : 2026/06/05
* Description        : INA226 current/power monitor driver implementation.
*                      All I2C operations go through the shared i2c_util wrapper
*                      layer (per D-05). No direct SPL I2C calls.
*******************************************************************************/
#include "../Drivers/ina226.h"
#include "../Drivers/i2c_util.h"

/*********************************************************************
 * @fn      ina226_init
 *
 * @brief   Initialize an INA226 device by writing the calibration
 *          register (0x05) and configuration register (0x00).
 *
 *          Writes use the format: {register_pointer, MSB, LSB}
 *          where the register pointer is packed as the first byte
 *          of the data buffer passed to i2c_util_write.
 *
 * @param   dev - Pointer to INA226_Dev struct with address and channel
 *
 * @return  I2C_OK on success, or i2c_status_t error code
 */
i2c_status_t ina226_init(INA226_Dev *dev)
{
    i2c_status_t status;
    uint8_t data[3];
    uint16_t calValue;
    uint16_t cfgValue;

    /* Write calibration register (0x05) */
    calValue = INA226_CAL_VALUE;
    data[0] = INA226_REG_CALIB;         /* Register pointer */
    data[1] = (uint8_t)(calValue >> 8); /* MSB */
    data[2] = (uint8_t)(calValue);      /* LSB */

    status = i2c_util_write(dev->address, data, 3, I2C_TIMEOUT_MS);
    if (status != I2C_OK)
    {
        return status;
    }

    /* Write configuration register (0x00) */
    cfgValue = INA226_CONFIG_VALUE;
    data[0] = INA226_REG_CONFIG;        /* Register pointer */
    data[1] = (uint8_t)(cfgValue >> 8); /* MSB */
    data[2] = (uint8_t)(cfgValue);      /* LSB */

    status = i2c_util_write(dev->address, data, 3, I2C_TIMEOUT_MS);

    return status;
}

/*********************************************************************
 * @fn      ina226_get_bus_voltage
 *
 * @brief   Read the bus voltage register (0x02) from an INA226 device
 *          and convert to volts.
 *
 *          INA226 bus voltage register is 16-bit MSB-first.
 *          Conversion: voltage = raw_value * 1.25 mV (0.00125 V)
 *
 * @param   dev       - Pointer to INA226_Dev struct
 * @param   voltage_v - Output: bus voltage in volts
 *
 * @return  I2C_OK on success, or i2c_status_t error code
 */
i2c_status_t ina226_get_bus_voltage(INA226_Dev *dev, float *voltage_v)
{
    i2c_status_t status;
    uint8_t buf[2];
    uint16_t rawValue;

    /* Read 2 bytes from bus voltage register (0x02) */
    status = i2c_util_read(dev->address, INA226_REG_BUS_V,
                           buf, 2, I2C_TIMEOUT_MS);
    if (status != I2C_OK)
    {
        return status;
    }

    /* Parse MSB-first 16-bit value */
    rawValue = ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];

    /* Convert to volts: LSB = 1.25 mV */
    *voltage_v = (float)rawValue * INA226_BUS_VOLTAGE_LSB;

    return I2C_OK;
}
