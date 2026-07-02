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
    printf("calValue:%d \r\n",calValue);
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
// i2c_status_t ina226_check_alert(INA226_Dev *dev, uint16_t *mask)
// {
//     i2c_status_t status;
//     uint8_t buf[2];

//     /* Read 2 bytes from alert register (0x06) */
//     status = i2c_util_read(dev->address, INA226_REG_ALERT,
//                            buf);
//     if (status != I2C_OK)
//     {
//         return status;
//     }

//     /* Parse MSB-first 16-bit value — raw mask, no interpretation */
//     *mask = ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];

//     return I2C_OK;
// }

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
// i2c_status_t ina226_set_alert_limit(INA226_Dev *dev, uint16_t value)
// {
//     i2c_status_t status;
//     uint8_t data[3];

//     data[0] = INA226_REG_ALERT_LIMIT;         /* Register pointer */
//     data[1] = (uint8_t)(value >> 8);          /* MSB */
//     data[2] = (uint8_t)(value);               /* LSB */

//     status = i2c_util_write(dev->address, data, 3, I2C_TIMEOUT_MS);

//     return status;
// }

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
// i2c_status_t ina226_set_alert_config(INA226_Dev *dev, uint16_t mask)
// {
//     i2c_status_t status;
//     uint8_t data[3];

//     data[0] = INA226_REG_ALERT;               /* Register pointer */
//     data[1] = (uint8_t)(mask >> 8);           /* MSB */
//     data[2] = (uint8_t)(mask);                /* LSB */

//     status = i2c_util_write(dev->address, data, 3, I2C_TIMEOUT_MS);

//     return status;
// }

