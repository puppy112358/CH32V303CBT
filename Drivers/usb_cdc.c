/********************************** (C) COPYRIGHT *******************************
* File Name          : usb_cdc.c
* Author             : (adapted from WCH SimulateCDC)
* Version            : V1.0.0
* Date               : 2026/06/05
* Description        : USB-CDC helper functions: init, usb_printf for debug output.
*******************************************************************************/

#include "../Drivers/usb_cdc.h"
#include "../Drivers/ch32v30x_usbfs_device.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

/*********************************************************************
 * @fn      usb_cdc_init
 *
 * @brief   Initialize USB-CDC device.
 *          Configures USBFS RCC clock, initializes device and endpoints,
 *          enables USBFS interrupt.
 *
 * @return  void
 */
void usb_cdc_init(void)
{
    USBFS_RCC_Init();
    USBFS_Device_Init(ENABLE);
    printf("USB-CDC initialized\r\n");
}

/*********************************************************************
 * @fn      usb_printf
 *
 * @brief   Print formatted string to USB-CDC virtual COM port.
 *          Formats into a local buffer, then uploads to EP3 (bulk IN) via
 *          USBFS_Endp_DataUp with copy mode.
 *          Fire-and-forget: if EP3 is still busy from a previous call
 *          (host hasn't polled), force-resets the endpoint and overwrites.
 *          Non-blocking, super-loop safe.
 *
 * @param   fmt    printf-style format string
 * @param   ...    variable arguments
 * @return  number of bytes sent on success, -1 on format error
 */
int usb_printf(const char *fmt, ...)
{
    va_list args;
    char buf[USB_CDC_BUF_SIZE];
    int len;

    va_start(args, fmt);
    len = vsnprintf(buf, USB_CDC_BUF_SIZE, fmt, args);
    va_end(args);

    if (len <= 0)
    {
        return -1;
    }

    /* If EP3 is still busy from a previous transfer that the host never
     * read (e.g. terminal not open), force-reset the endpoint so we
     * don't permanently block.  Otherwise the very first usb_printf()
     * after boot would make all subsequent calls fail. */
    if (USBFS_Endp_Busy[DEF_UEP3] != 0)
    {
        USBFSD->UEP3_TX_CTRL = (USBFSD->UEP3_TX_CTRL & ~USBFS_UEP_T_RES_MASK) | USBFS_UEP_T_RES_NAK;
        USBFS_Endp_Busy[DEF_UEP3] = 0;
    }

    /* Upload to EP3 (bulk IN) via copy mode */
    if (USBFS_Endp_DataUp(DEF_UEP3, (uint8_t *)buf, (uint16_t)len, DEF_UEP_CPY_LOAD) != 0)
    {
        return -1;
    }

    return len;
}
