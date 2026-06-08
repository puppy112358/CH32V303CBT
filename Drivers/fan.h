/********************************** (C) COPYRIGHT  *******************************
* File Name          : fan.h
* Author             : GSD Phase 04
* Version            : V1.0.0
* Date               : 2026/06/08
* Description        : Fan control header.
*                      Drives 4-wire PWM cooling fan via TIM3 CH1 (PA6, 25kHz PWM),
*                      measures RPM via TIM3 CH2 (PA7, input capture tachometer),
*                      runs PID temperature control (setpoint 50°C), and detects
*                      fan stall (RPM=0 with duty≥25% for 2 seconds).
*******************************************************************************/
#ifndef __DRIVERS_FAN_H
#define __DRIVERS_FAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../Drivers/pid.h"

/* --------------------------------------------------------------------------
 * Fan Control Constants
 * -------------------------------------------------------------------------- */
#define FAN_TARGET_TEMP_C            50.0f    /* PID setpoint in °C */
#define FAN_DUTY_MIN_PCT             30       /* Minimum active duty (30%) */
#define FAN_DUTY_MAX_PCT             100      /* Maximum duty (100%) */
#define FAN_PWM_FREQ_HZ              25000    /* 25kHz PWM (Intel 4-wire spec) */
#define FAN_STALL_DUTY_THRESHOLD_PCT 25       /* Below this duty, stall check disabled */
#define FAN_STALL_CYCLES             20       /* 20 cycles × 100ms = 2 seconds */
#define FAN_KP                       0.2f     /* PID proportional gain */
#define FAN_KI                       0.02f    /* PID integral gain */
#define FAN_KD                       0.0f     /* PID derivative gain (PI-only) */
#define FAN_PID_MAX                  100.0f   /* PID output max (maps to 100% duty) */

/* --------------------------------------------------------------------------
 * External Globals
 * -------------------------------------------------------------------------- */
extern volatile uint16_t fan_rpm;       /* Current fan RPM (0 if stopped/stalled) */
extern volatile uint8_t  fan_stall;     /* 1 = stall detected, 0 = normal */
extern PID_Instance       fan_pid;      /* Fan PID instance (for external tuning) */

/* --------------------------------------------------------------------------
 * Tachometer Variables (accessed by TIM3_IRQHandler in ch32v30x_it.c)
 * -------------------------------------------------------------------------- */
extern volatile uint32_t tacho_last_capture;   /* Previous TIM3 counter capture value */
extern volatile uint32_t tacho_period_ticks;   /* Period between last two captures */
extern volatile uint8_t  tacho_valid;          /* 1 = valid period measured */
extern volatile uint16_t tacho_timeout;        /* Cycles since last valid capture */

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/**
 * @brief  Initialize fan controller hardware and PID.
 *
 *         Configures TIM3 CH1 (PA6) for 25kHz PWM output, TIM3 CH2 (PA7)
 *         for tachometer input capture on rising edge, initializes the
 *         fan PID instance (PI-only: Kp=0.20, Ki=0.02, Kd=0), and zeros
 *         all tachometer and stall state.
 *
 *         TIM3 clock: APB1 PCLK1 × 2 = 144MHz (APB1 prescaler /2).
 *         PSC=143 → 1MHz counter. ARR=39 → 25kHz PWM.
 *         Tachometer resolution: 1μs/tick → RPM = 30,000,000 / period_ticks.
 *
 * @return none
 */
void fan_init(void);

/**
 * @brief  Run one iteration of fan control (called each 100ms cycle).
 *
 *         Computes PID output from current temperature, maps to 30-100%
 *         PWM duty when temp ≥ 50°C (fan OFF when temp < 50°C), updates
 *         TIM3 CCR1, reads RPM from tachometer capture, and runs stall
 *         detection state machine (RPM=0 + duty ≥ 25% for 2s → stall).
 *
 * @param  current_temp_c  Current heatsink temperature in °C (from temp_sensor)
 * @return none
 */
void fan_update(float current_temp_c);

/**
 * @brief  Get current fan RPM from latest tachometer measurement.
 *
 * @return RPM as uint16_t (0 if stopped, stalled, or no valid measurement)
 */
uint16_t fan_get_rpm(void);

/**
 * @brief  Get current PWM duty cycle percentage.
 *
 * @return Duty cycle 0-100 percent
 */
uint8_t fan_get_duty_pct(void);

/**
 * @brief  Check if fan stall is currently detected.
 *
 * @return 1 if stalled, 0 if normal
 */
uint8_t fan_is_stalled(void);

#ifdef __cplusplus
}
#endif

#endif /* __DRIVERS_FAN_H */
