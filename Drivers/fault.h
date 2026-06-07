/********************************** (C) COPYRIGHT  *******************************
* File Name          : fault.h
* Author             : GSD Phase 02
* Version            : V1.0.0
* Date               : 2026/06/07
* Description        : Fault state machine header.
*                      Unified fault handler for EXTI hardware overcurrent and
*                      software OPP (over-power protection). Auto-retry with
*                      3-second cooldown, 5-retry permanent latch, and 30-second
*                      fault-free retry counter reset.
*******************************************************************************/
#ifndef __FAULT_H
#define __FAULT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../Drivers/ina226.h"

/* --------------------------------------------------------------------------
 * System Mode Enumeration (shared with main.c)
 * -------------------------------------------------------------------------- */
typedef enum
{
    MODE_IDLE  = 0,
    MODE_CV    = 1,
    MODE_CC    = 2,
    MODE_FAULT = 3
} SystemMode;

/* --------------------------------------------------------------------------
 * Protection Constants
 * -------------------------------------------------------------------------- */

/* Rated total system wattage — trip OPP when summary power exceeds this */
#define RATED_WATTAGE         50.0f

/* Maximum consecutive auto-retries before permanent latch */
#define MAX_RETRY_COUNT       5

/* Cooldown cycles: 30 cycles × 100ms = 3000ms = 3 seconds */
#define COOLDOWN_CYCLES       30

/* Fault-free duration to reset retry counter: 30000 ms = 30 seconds */
#define FAULT_FREE_RESET_MS   30000

/* --------------------------------------------------------------------------
 * Fault Type Enumeration
 * -------------------------------------------------------------------------- */
typedef enum
{
    FAULT_TYPE_OC   = 0,    /* Overcurrent (EXTI4 hardware trigger) */
    FAULT_TYPE_OPP  = 1,    /* Over-power protection (software check) */
    FAULT_TYPE_BOTH = 2     /* Both OC and OPP occurred */
} FaultType;

/* --------------------------------------------------------------------------
 * Fault Register (16-bit packed union with bitfield access)
 *
 * Bit layout:
 *   bits  0-3: overcurrent_mask  — per-channel fault flags (4 MOS channels)
 *   bits  4-5: fault_type        — 0=OC, 1=OPP, 2=both
 *   bits  6-7: reserved
 *   bits 8-11: retry_count       — current retry attempt (0-15, max 5 used)
 *   bit    12: latched            — permanent latch (1 = no auto-recovery)
 *   bit    13: auto_recovery      — 0=disabled (latched), 1=auto-retry enabled
 *   bits 14-15: reserved
 * -------------------------------------------------------------------------- */
typedef union
{
    uint16_t raw;
    struct
    {
        uint16_t overcurrent_mask : 4;   /* Bits 0-3: per-channel OC */
        uint16_t fault_type       : 2;   /* Bits 4-5: OC/OPP/both */
        uint16_t reserved         : 2;   /* Bits 6-7 */
        uint16_t retry_count      : 4;   /* Bits 8-11: 0-15 */
        uint16_t latched          : 1;   /* Bit 12: permanent latch */
        uint16_t auto_recovery    : 1;   /* Bit 13: auto-retry enabled */
        uint16_t reserved2        : 2;   /* Bits 14-15 */
    } bits;
} FaultRegister;

/* --------------------------------------------------------------------------
 * External Fault Register Reference
 *
 * Declared here so main.c can access retry_count for the 30-second
 * fault-free reset timer (D-03).
 * -------------------------------------------------------------------------- */
extern FaultRegister fault_reg;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/* Initialize fault handler: zero fault register, set auto_recovery=1,
 * zero cooldown counter and fault-free timer. */
void fault_init(void);

/* Hardware fault handler — called from main loop when EXTI4 ISR
 * has set fault_triggered. Records OC channels from alert_mask
 * and applies retry/latch logic. */
void fault_handler_hw(uint16_t alert_mask);

/* Software OPP fault handler — called from main loop when
 * summary power exceeds RATED_WATTAGE. */
void fault_handler_opp(void);

/* Fault state machine — called each control cycle when in MODE_FAULT.
 * Manages cooldown timer, auto-retry transition to IDLE, and permanent
 * latch blocking. */
void fault_state_machine(void);

/* Explicit fault clear — reset register and transition FAULT→IDLE.
 * Stub with TODO for Phase 3 cJSON clear command. */
void fault_clear(void);

/* Print full diagnostic snapshot: all 4 MOS channels + summary INA226,
 * active mode, DAC value, fault type string, retry count.
 * Implements D-02 diagnostic requirement. */
void fault_print_snapshot(FaultRegister *fr, uint16_t dac_value, SystemMode mode);

#ifdef __cplusplus
}
#endif

#endif /* __FAULT_H */
