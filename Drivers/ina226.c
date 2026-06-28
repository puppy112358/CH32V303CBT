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
                           buf);
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

/*********************************************************************
 * @fn      ina226_get_shunt_voltage
 *
 * @brief   Read the shunt voltage register (0x01) from an INA226
 *          device and convert to millivolts.
 *
 *          INA226 shunt voltage register is 16-bit MSB-first, signed.
 *          Conversion: shunt_mv = raw * 2.5 uV (0.0025 mV)
 *
 * @param   dev        - Pointer to INA226_Dev struct
 * @param   voltage_mv - Output: shunt voltage in millivolts
 *
 * @return  I2C_OK on success, or i2c_status_t error code
 */
i2c_status_t ina226_get_shunt_voltage(INA226_Dev *dev, float *voltage_mv)
{
    i2c_status_t status;
    uint8_t buf[2];
    int16_t rawValue;

    /* Read 2 bytes from shunt voltage register (0x01) */
    status = i2c_util_read(dev->address, INA226_REG_SHUNT_V,
                           buf);
    if (status != I2C_OK)
    {
        return status;
    }

    /* Parse MSB-first 16-bit signed value */
    rawValue = (int16_t)(((uint16_t)buf[0] << 8) | (uint16_t)buf[1]);

    /* Convert to millivolts: LSB = 2.5 uV */
    *voltage_mv = (float)rawValue * 0.0025f;

    return I2C_OK;
}

/*********************************************************************
 * @fn      ina226_get_current
 *
 * @brief   Read the current register (0x04) from an INA226 device
 *          and convert to amperes.
 *
 *          INA226 current register is 16-bit MSB-first, signed.
 *          Conversion: current_a = raw * INA226_CURRENT_LSB
 *
 * @param   dev       - Pointer to INA226_Dev struct
 * @param   current_a - Output: current in amperes
 *
 * @return  I2C_OK on success, or i2c_status_t error code
 */
i2c_status_t ina226_get_current(INA226_Dev *dev, float *current_a)
{
    i2c_status_t status;
    uint8_t buf[2];
    int16_t rawValue;

    /* Read 2 bytes from current register (0x04) */
    status = i2c_util_read(dev->address, INA226_REG_CURRENT,
                           buf);
    if (status != I2C_OK)
    {
        return status;
    }

    /* Parse MSB-first 16-bit signed value */
    rawValue = (int16_t)(((uint16_t)buf[0] << 8) | (uint16_t)buf[1]);

    /* Convert to amperes */
    *current_a = (float)rawValue * INA226_CURRENT_LSB;

    return I2C_OK;
}

/*********************************************************************
 * @fn      ina226_get_power
 *
 * @brief   Read the power register (0x03) from an INA226 device
 *          and convert to watts.
 *
 *          INA226 power register is 16-bit MSB-first, unsigned.
 *          Conversion: power_w = raw * 25 * INA226_CURRENT_LSB
 *
 * @param   dev     - Pointer to INA226_Dev struct
 * @param   power_w - Output: power in watts
 *
 * @return  I2C_OK on success, or i2c_status_t error code
 */
i2c_status_t ina226_get_power(INA226_Dev *dev, float *power_w)
{
    i2c_status_t status;
    uint8_t buf[2];
    uint16_t rawValue;

    /* Read 2 bytes from power register (0x03) */
    status = i2c_util_read(dev->address, INA226_REG_POWER,
                           buf);
    if (status != I2C_OK)
    {
        return status;
    }

    /* Parse MSB-first 16-bit unsigned value */
    rawValue = ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];

    /* Convert to watts: P_LSB = 25 * Current_LSB */
    *power_w = (float)rawValue * 25.0f * INA226_CURRENT_LSB;

    return I2C_OK;
}

/*********************************************************************
 * @fn      ina226_check_alert
 *
 * @brief   Read the alert Mask/Enable register (0x06) from an INA226
 *          device and return the raw mask value.
 *
 *          Per D-12: reads and returns the raw register value.
 *          Application code decides what protective action to take
 *          based on the alert flags.
 *
 * @param   dev  - Pointer to INA226_Dev struct
 * @param   mask - Output: raw 16-bit alert mask register value
 *
 * @return  I2C_OK on success, or i2c_status_t error code
 */
i2c_status_t ina226_check_alert(INA226_Dev *dev, uint16_t *mask)
{
    i2c_status_t status;
    uint8_t buf[2];

    /* Read 2 bytes from alert register (0x06) */
    status = i2c_util_read(dev->address, INA226_REG_ALERT,
                           buf);
    if (status != I2C_OK)
    {
        return status;
    }

    /* Parse MSB-first 16-bit value — raw mask, no interpretation */
    *mask = ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];

    return I2C_OK;
}

/*********************************************************************
 * @fn      ina226_set_alert_limit
 *
 * @brief   写入 INA226 器件的报警阈值寄存器 (0x07)。
 *          使用3字节格式: {寄存器指针, MSB, LSB} 通过 I2C 写入。
 *
 * @param   dev   - 指向 INA226_Dev 结构体的指针
 * @param   value - 16位报警阈值 (电流值, 如 0x299A)
 *
 * @return  I2C_OK 成功, 或 i2c_status_t 错误码
 */
i2c_status_t ina226_set_alert_limit(INA226_Dev *dev, uint16_t value)
{
    i2c_status_t status;
    uint8_t data[3];

    data[0] = INA226_REG_ALERT_LIMIT;         /* Register pointer */
    data[1] = (uint8_t)(value >> 8);          /* MSB */
    data[2] = (uint8_t)(value);               /* LSB */

    status = i2c_util_write(dev->address, data, 3, I2C_TIMEOUT_MS);

    return status;
}

/*********************************************************************
 * @fn      ina226_set_alert_config
 *
 * @brief   写入 INA226 器件的屏蔽/使能寄存器 (0x06)。
 *          配置报警条件和锁存模式（如设置位15为分流电压过压，
 *          位1:0=01 为锁存模式）。
 *          使用3字节格式: {寄存器指针, MSB, LSB}。
 *
 * @param   dev  - 指向 INA226_Dev 结构体的指针
 * @param   mask - 16位报警屏蔽/使能配置值
 *
 * @return  I2C_OK 成功, 或 i2c_status_t 错误码
 */
i2c_status_t ina226_set_alert_config(INA226_Dev *dev, uint16_t mask)
{
    i2c_status_t status;
    uint8_t data[3];

    data[0] = INA226_REG_ALERT;               /* Register pointer */
    data[1] = (uint8_t)(mask >> 8);           /* MSB */
    data[2] = (uint8_t)(mask);                /* LSB */

    status = i2c_util_write(dev->address, data, 3, I2C_TIMEOUT_MS);

    return status;
}

/*********************************************************************
 * @fn      ina226_read_calibration
 *
 * @brief   读取 INA226 器件的校准寄存器 (0x05) 原始值。
 *          用于校准重验证 (PROT-04): 周期性读取校准值，
 *          若为0且总线电压>0.1V则重新初始化该器件。
 *
 * @param   dev       - 指向 INA226_Dev 结构体的指针
 * @param   cal_value - 输出: 16位原始校准值 (MSB-first)
 *
 * @return  I2C_OK 成功, 或 i2c_status_t 错误码
 */
i2c_status_t ina226_read_calibration(INA226_Dev *dev, uint16_t *cal_value)
{
    i2c_status_t status;
    uint8_t buf[2];

    /* Read 2 bytes from calibration register (0x05) */
    status = i2c_util_read(dev->address, INA226_REG_CALIB,
                           buf);
    if (status != I2C_OK)
    {
        return status;
    }

    /* Parse MSB-first 16-bit value */
    *cal_value = ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];

    return I2C_OK;
}
