/********************************** (C) COPYRIGHT  *******************************
* File Name          : ws2812.h
* Author             : GSD Phase 04
* Version            : V1.0.0
* Date               : 2026/06/08
* Description        : WS2812 LED status display header.
*                      Drives 2 WS2812 LEDs via TIM2 CH1 PWM (PA0) + DMA1
*                      Channel 5 at 800kHz. Pre-computes 48-byte GRB bitstream
*                      with 50% brightness and maps SystemMode to display colors.
*******************************************************************************/
#ifndef __DRIVERS_WS2812_H
#define __DRIVERS_WS2812_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../Drivers/fault.h"

/* --------------------------------------------------------------------------
 * Color Constant #defines (50% brightness, GRB order)
 * -------------------------------------------------------------------------- */
#define WS2812_COLOR_GREEN  {0, 32, 0}     /* IDLE/Standby */
#define WS2812_COLOR_CYAN   {0, 32, 32}    /* CV active */
#define WS2812_COLOR_BLUE   {0, 0, 32}     /* CC active */
#define WS2812_COLOR_RED    {32, 0, 0}     /* Fault */

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/**
 * @brief  Initialize WS2812 LED driver.
 *
 *         Configures PA0 as alternate-function push-pull for TIM2 CH1,
 *         initializes TIM2 at 800kHz PWM mode 1, sets up DMA1 Channel 5
 *         for CCR streaming, and transmits initial green (IDLE) color.
 *
 *         Clocks enabled: GPIOA (APB2), TIM2 (APB1), DMA1 (AHB).
 *         DMA1 Channel 5 TC interrupt enabled at NVIC priority 0x03.
 *
 * @return none
 */
void ws2812_init(void);

/**
 * @brief  Update LED color based on current system operating mode.
 *
 *         Maps SystemMode to pre-defined RGB color, encodes the 48-byte
 *         GRB bitstream at 50% brightness, and triggers a single-shot
 *         DMA transfer. Skips recomputation if the mode has not changed
 *         since the last call (except FAULT which always re-triggers).
 *
 * @param  mode  Current system mode (MODE_IDLE, MODE_CV, MODE_CC, MODE_FAULT)
 * @return none
 */
void ws2812_update_from_mode(SystemMode mode);

/**
 * @brief  Set both LEDs to an explicit RGB color with 50% brightness.
 *
 *         Encodes the supplied 8-bit-per-channel RGB values into a 48-byte
 *         GRB bitstream (green-first per WS2812 data ordering) and triggers
 *         a single-shot DMA transfer. Can be used for test, calibration,
 *         or future dynamic color selection.
 *
 * @param  r  Red channel value (0-255, halved internally)
 * @param  g  Green channel value (0-255, halved internally)
 * @param  b  Blue channel value (0-255, halved internally)
 * @return none
 */
void ws2812_set_color(uint8_t r, uint8_t g, uint8_t b);

/* DMA busy flag — set when DMA transfer is active, cleared by ISR on TC.
 * Declared here so ch32v30x_it.c can reference it for the DMA1_Channel5 ISR. */
extern volatile uint8_t ws2812_dma_busy;

#ifdef __cplusplus
}
#endif

#endif /* __DRIVERS_WS2812_H */
