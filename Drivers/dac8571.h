/********************************** (C) COPYRIGHT  *******************************
* File Name          : dac8571.h
* Author             : GSD Phase 01
* Version            : V1.0.0
* Date               : 2026/06/05
* Description        : DAC8571 16-bit I2C DAC driver.
*                      Provides initialization (probe write) and 16-bit output
*                      value setting via 3-byte I2C write (control + MSB + LSB).
*                      All I2C operations go through the shared i2c_util wrapper
*                      layer (D-05).
*******************************************************************************/
#ifndef __DAC8571_H
#define __DAC8571_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../Drivers/i2c_util.h"

/* DAC8571 7-bit I2C address (A0 pin strapped to GND) */
#define DAC8571_ADDR    0x4C

/* Initialize DAC8571: send a probe write of zero to verify device presence.
 * Prints init status via printf. No hardware config registers to set. */
void dac8571_init(void);

/* Set the DAC8571 output value.
 *
 * value: 16-bit DAC code (0-65535).
 *        0 = VOUT = 0V, 65535 = VOUT ≈ VREF.
 *        Sends 3-byte I2C write: control byte 0x10 (normal mode),
 *        MSB, LSB. The DAC latches output on falling edge of ACK
 *        after LSB per datasheet.
 *
 * Returns I2C_OK on success, or i2c_status_t error code. */
i2c_status_t dac8571_set_output(uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* __DAC8571_H */
