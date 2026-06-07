/********************************** (C) COPYRIGHT  *******************************
* File Name          : pid.h
* Author             : GSD Phase 02
* Version            : V1.0.0
* Date               : 2026/06/07
* Description        : PID controller header.
*                      Positional (parallel) form PID with anti-windup via
*                      conditional integration freeze and derivative-on-measurement.
*                      Provides two independent instances for CV (constant voltage)
*                      and CC (constant current) modes.
*******************************************************************************/
#ifndef __PID_H
#define __PID_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* --------------------------------------------------------------------------
 * PID Coefficient Constants (tunable #defines)
 *
 * These are initial values — Phase 3 adds runtime tuning via cJSON.
 * CV (constant voltage): primary feedback is bus voltage, larger Kp
 *   for fast voltage correction, moderate Ki for steady-state accuracy.
 * CC (constant current): primary feedback is summary current, smaller
 *   Kp for current loop stability, conservative Ki to avoid overshoot.
 * -------------------------------------------------------------------------- */
#define PID_CV_KP         1.0f
#define PID_CV_KI         0.1f
#define PID_CV_KD         0.01f
#define PID_CC_KP         0.5f
#define PID_CC_KI         0.05f
#define PID_CC_KD         0.005f

/* Output clamping range for DAC8571 (16-bit) */
#define PID_OUTPUT_MAX    65535.0f
#define PID_OUTPUT_MIN    0.0f

/* Integral term clamping limits */
#define PID_INTEGRAL_MAX   65535.0f
#define PID_INTEGRAL_MIN  -65535.0f

/* --------------------------------------------------------------------------
 * PID Instance Structure
 *
 * Each instance maintains its own state for independent CV/CC control.
 * The `saturated` field tracks output clamping state:
 *   0 = not saturated
 *   1 = saturated at PID_OUTPUT_MAX (65535)
 *   2 = saturated at PID_OUTPUT_MIN (0)
 * -------------------------------------------------------------------------- */
typedef struct
{
    float Kp;             /* Proportional gain */
    float Ki;             /* Integral gain */
    float Kd;             /* Derivative gain */
    float setpoint;       /* Target value (voltage for CV, current for CC) */
    float integral;       /* Accumulated integral term */
    float last_feedback;  /* Previous feedback value for derivative-on-measurement */
    float output;         /* Last computed output (clamped to [0, 65535]) */
    uint8_t saturated;    /* Saturation flag: 0=normal, 1=high, 2=low */
} PID_Instance;

/* --------------------------------------------------------------------------
 * PID Mode Enumeration
 * -------------------------------------------------------------------------- */
typedef enum
{
    PID_MODE_CV = 0,      /* Constant voltage mode (feedback = bus voltage) */
    PID_MODE_CC = 1       /* Constant current mode (feedback = summary current) */
} PID_Mode;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/* Initialize a PID instance with given coefficients.
 * Zeros integral, last_feedback, output, and saturated flag.
 * Sets setpoint to 0 — caller should set via pid_compute argument. */
void pid_init(PID_Instance *pid, float kp, float ki, float kd);

/* Run one PID iteration using the positional (parallel) form:
 *   output = Kp*error + integral + Kd*d(error)/dt
 *
 * Features:
 * - Anti-windup: conditional integration — integral freezes when output
 *   is saturated at MAX with positive error, or at MIN with negative error.
 * - Derivative-on-measurement: D term = -Kd * (feedback - last_feedback) / dt,
 *   avoiding setpoint-step derivative kicks.
 * - Output clamped to [PID_OUTPUT_MIN, PID_OUTPUT_MAX].
 *
 * @param pid       Pointer to PID_Instance
 * @param setpoint  Target value (voltage or current)
 * @param feedback  Current measured value
 * @param dt        Time delta in seconds (e.g., 0.1f for 100ms)
 * @return          Clamped PID output in range [0, 65535]
 */
float pid_compute(PID_Instance *pid, float setpoint, float feedback, float dt);

/* Reset a PID instance: zero integral, last_feedback, output, and saturated flag.
 * Used when switching modes or after fault recovery. */
void pid_reset(PID_Instance *pid);

/* Pre-load the integral term for bumpless transfer.
 * Clamps value to [PID_INTEGRAL_MIN, PID_INTEGRAL_MAX].
 * Used after soft-start to set integral = current output so PID picks up
 * smoothly without a step. */
void pid_set_integral(PID_Instance *pid, float value);

#ifdef __cplusplus
}
#endif

#endif /* __PID_H */
