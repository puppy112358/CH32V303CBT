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
 *          Formats into a local buffer, checks if EP3 (bulk IN) is
 *          available, then uploads via USBFS_Endp_DataUp in copy mode.
 *          Non-blocking: drops data if endpoint is busy.
 *
 * @param   fmt    printf-style format string
 * @param   ...    variable arguments
 * @return  number of bytes sent on success, -1 on busy or error
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

    /* Check if EP3 is available — drop if busy (host not connected or pending transfer) */
    if (USBFS_Endp_Busy[DEF_UEP3] != 0)
    {
        return -1;
    }

    /* Upload to EP3 (bulk IN) via copy mode */
    if (USBFS_Endp_DataUp(DEF_UEP3, (uint8_t *)buf, (uint16_t)len, DEF_UEP_CPY_LOAD) != 0)
    {
        return -1;
    }

    return len;
}
