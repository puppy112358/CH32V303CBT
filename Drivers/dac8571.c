/********************************** (C) COPYRIGHT  *******************************
* File Name          : dac8571.c
* Author             : GSD Phase 01
* Version            : V1.0.0
* Date               : 2026/06/05
* Description        : DAC8571 16-bit I2C DAC driver implementation.
*                      All I2C operations go through the shared i2c_util wrapper
*                      layer (per D-05). No direct SPL I2C calls.
*******************************************************************************/
#include "../Drivers/dac8571.h"
#include "../Drivers/i2c_util.h"
#include <stdio.h>

/*********************************************************************
 * @fn      dac8571_init
 *
 * @brief   Initialize the DAC8571 by sending a probe write of zero
 *          to verify the device is present and ACKing on the bus.
 *
 *          The DAC8571 has no configuration registers — the write
 *          protocol itself is the only interface. A successful ACK
 *          on the 3-byte write confirms the device is wired correctly.
 *
 * @return  none
 */
void dac8571_init(void)
{
    i2c_status_t status;
    uint8_t data[3];

    /* Probe write: control=0x00 (normal mode), value=0x0000 */
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;

    status = i2c_util_write(DAC8571_ADDR, data, 3, I2C_TIMEOUT_MS);

    if (status == I2C_OK)
    {
        printf("DAC8571 init: OK\r\n");
    }
    else
    {
        printf("DAC8571 init: FAIL (%d)\r\n", status);
    }
}

/*********************************************************************
 * @fn      dac8571_set_output
 *
 * @brief   Set the DAC8571 output voltage by writing a 16-bit DAC
 *          code via I2C.
 *
 *          Write sequence (3 bytes, single I2C transaction):
 *            byte 0: control byte (0x00 = normal mode, update DAC)
 *            byte 1: MSB of DAC value
 *            byte 2: LSB of DAC value
 *
 *          The DAC latches the new output on the falling edge of
 *          ACK after the LS byte per datasheet.
 *
 * @param   value - 16-bit DAC code (0 = 0V, 65535 ≈ VREF)
 *
 * @return  I2C_OK on success, or i2c_status_t error code
 */
i2c_status_t dac8571_set_output(uint16_t value)
{
    uint8_t data[3];

    data[0] = 0x00;                        /* Control byte: normal mode */
    data[1] = (uint8_t)(value >> 8);       /* MSB */
    data[2] = (uint8_t)(value & 0xFF);     /* LSB */

    return i2c_util_write(DAC8571_ADDR, data, 3, I2C_TIMEOUT_MS);
}
