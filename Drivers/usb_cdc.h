/********************************** (C) COPYRIGHT *******************************
* File Name          : usb_cdc.h
* Author             : (adapted from WCH SimulateCDC)
* Version            : V1.0.0
* Date               : 2026/06/05
* Description        : High-level USB-CDC API for debug output.
*                      Provides init and usb_printf() over USB virtual COM port.
*******************************************************************************/

#ifndef __USB_CDC_H
#define __USB_CDC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/******************************************************************************/
/* Global Define */

/* Buffer size for usb_printf formatting — 2x USB max packet size for headroom */
#define USB_CDC_BUF_SIZE    128

/******************************************************************************/
/* external functions */

/*********************************************************************
 * @fn      usb_cdc_init
 *
 * @brief   Initialize USB-CDC device: configure USBFS clock, device init,
 *          endpoint init, NVIC enable. Prints "USB-CDC initialized" via USART1.
 *
 * @return  void
 */
void usb_cdc_init(void);

/*********************************************************************
 * @fn      usb_printf
 *
 * @brief   Print formatted string to USB-CDC virtual COM port.
 *          Formats into a buffer, then uploads to EP3 (bulk IN) via
 *          USBFS_Endp_DataUp with copy mode.
 *          Drops data silently if endpoint is busy (USB host not connected
 *          or pending transfer) — non-blocking, super-loop safe.
 *
 * @param   fmt    printf-style format string
 * @param   ...    variable arguments
 * @return  number of bytes formatted (>=0) on success,
 *          -1 if buffer busy or format error
 */
int usb_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#ifdef __cplusplus
}
#endif

#endif /* __USB_CDC_H */
