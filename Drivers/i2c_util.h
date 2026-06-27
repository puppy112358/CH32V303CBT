/********************************** (C) COPYRIGHT  *******************************
* File Name          : i2c_util.h
* Author             : GSD Phase 01
* Version            : V1.0.0
* Date               : 2026/06/05
* Description        : Timeout-protected I2C master wrapper with automatic bus
*                      recovery. Wraps SPL I2C1 functions with non-blocking
*                      tick-count timeout, 9-clock-pulse GPIO bus recovery,
*                      and automatic retry on timeout.
*******************************************************************************/
#ifndef __I2C_UTIL_H
#define __I2C_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* I2C operation timeout in milliseconds (per D-07: 5-10ms range) */
#define I2C_TIMEOUT_MS    1000

/* I2C operation status codes */
typedef enum
{
    I2C_OK = 0,
    I2C_TIMEOUT = -1,          /* Operation timed out during flag polling */
    I2C_BUS_FAULT = -2,        /* Bus recovery attempted but SDA still stuck */
    I2C_NACK = -3              /* Slave device did not acknowledge address */
} i2c_status_t;

/* Initialize I2C1 peripheral:
 * - Enables RCC clocks for I2C1 (APB1) and GPIOB (APB2)
 * - Configures PB6 (SCL) and PB7 (SDA) as alternate-function open-drain
 * - Initializes I2C1 at 100kHz standard mode with 7-bit addressing
 * - Enables I2C1 peripheral
 */
void i2c_util_init(void);

/* Write len bytes to an I2C slave device.
 * Handles START, address+W, data bytes, STOP sequence with timeout at each step.
 * On timeout, attempts bus recovery and one retry before returning error.
 *
 * @param dev_addr   7-bit I2C slave address
 * @param data       Pointer to data buffer to send
 * @param len        Number of bytes to send
 * @param timeout_ms Timeout in milliseconds for the entire operation
 * @return           I2C_OK on success, or error code on failure
 */
i2c_status_t i2c_util_write(uint8_t dev_addr, const uint8_t *data,
                            uint8_t len, uint32_t timeout_ms);

/* Read len bytes from an I2C slave device register.
 * First writes the register pointer (no STOP), then issues repeated START
 * and reads len bytes (NACK on last byte before STOP).
 * On timeout, attempts bus recovery and one retry before returning error.
 *
 * @param dev_addr   7-bit I2C slave address
 * @param reg_ptr    Register pointer byte to write before reading
 * @param data       Pointer to buffer to receive read data
 * @param len        Number of bytes to read
 * @param timeout_ms Timeout in milliseconds for the entire operation
 * @return           I2C_OK on success, or error code on failure
 */
i2c_status_t i2c_util_read(uint8_t dev_addr, uint8_t reg_ptr,
                           uint8_t *data, uint8_t len, uint32_t timeout_ms);

/* Attempt I2C bus recovery via 9-clock-pulse GPIO bit-banging.
 * Performs the following sequence:
 *   1. Disable I2C1 peripheral
 *   2. Reconfigure PB6 as GPIO Out_OD, PB7 as GPIO IN_FLOATING
 *   3. Generate up to 9 SCL pulses, checking SDA after each; early exit if released
 *   4. Generate STOP condition (SDA low -> SCL high -> SDA high)
 *   5. Re-initialize I2C1 and GPIO pins via i2c_util_init()
 *
 * @return I2C_OK if SDA was released, I2C_BUS_FAULT if still stuck low
 */
i2c_status_t i2c_util_bus_recovery(void);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_UTIL_H */
