---
phase: "02-control-loop-protection"
plan: "02"
subsystem: "protection"
tags: ["ina226", "exti", "overcurrent", "alert", "isr", "fault-detection"]

requires:
  - phase: "01"
    provides: "INA226 driver, I2C utility layer, DAC8571 driver"
provides:
  - "INA226 alert limit and mask/enable register write API"
  - "INA226 calibration register read-back API"
  - "PA4 EXTI4 falling-edge interrupt for wired-OR ALARM signal"
  - "EXTI4 ISR with per-channel fault detection and DAC zeroing"
  - "ISR-to-main-loop fault communication flags (fault_triggered, fault_source_mask)"
affects: ["control-loop", "fault-handler", "protection"]

tech-stack:
  added: []
  patterns:
    - "Per-device I2C register write pattern for INA226 alert configuration"
    - "WCH-Interrupt-fast ISR pattern with __disable_irq/__enable_irq for atomic I2C"

key-files:
  created: []
  modified:
    - "Drivers/ina226.h"
    - "Drivers/ina226.c"
    - "User/main.c"
    - "User/ch32v30x_it.c"
    - "User/ch32v30x_it.h"

key-decisions:
  - "Alert limit and config writes use existing 3-byte i2c_util_write pattern"
  - "EXTI4 ISR reads all 4 MOS channel alert registers on any trigger (wired-OR topology)"
  - "DAC zeroing in ISR uses __disable_irq/__enable_irq for atomic I2C transaction"
  - "Fault communication via volatile flags (fault_triggered, fault_source_mask) for ISR-to-main-loop"

patterns-established:
  - "INA226 driver extension pattern: new register read/write functions follow existing i2c_util conventions"
  - "ISR pattern: hardware protection (DAC zero) in ISR, state machine logic in main loop"

requirements-completed:
  - "PROT-01"
  - "PROT-02"

duration: 12min
completed: 2026-06-07
---

# Phase 02 Plan 02: INA226 Alert API Extension + EXTI4 ISR Foundation Summary

**INA226 overcurrent alert configuration with wired-OR EXTI4 fault detection and ISR-driven DAC zeroing for hardware protection**

## Performance

- **Duration:** ~12 min
- **Started:** 2026-06-07T08:28:21Z
- **Completed:** 2026-06-07T08:40:00Z
- **Tasks:** 3
- **Files modified:** 5

## Accomplishments
- Added ina226_set_alert_limit, ina226_set_alert_config, ina226_read_calibration to INA226 driver following existing i2c_util patterns
- Configured PA4 as EXTI4 falling-edge input with pull-up for wired-OR INA226 ALARM signal
- Implemented EXTI4_IRQHandler that reads all 4 MOS channel alerts, identifies fault source, zeroes DAC atomically, and prints diagnostic snapshot

## Task Commits

Each task was committed atomically:

1. **Task 1: INA226 Alert and Calibration Register Support** - `fd52949` (feat)
2. **Task 2: Configure PA4 EXTI4 for Falling-Edge Interrupt** - `cce74d9` (feat)
3. **Task 3: Implement EXTI4 ISR with Fault Detection and Snapshot** - `9254ee4` (feat)

**Plan metadata:** (`docs(02-02)` commit to follow)

## Files Created/Modified
- `Drivers/ina226.h` - Added INA226_REG_ALERT_LIMIT define and 3 new function declarations
- `Drivers/ina226.c` - Implemented ina226_set_alert_limit, ina226_set_alert_config, ina226_read_calibration
- `User/main.c` - Made devs[] non-static, added PA4 EXTI4 GPIO+EXTI+NVIC configuration
- `User/ch32v30x_it.c` - Implemented EXTI4_IRQHandler with fault detection, DAC zeroing, diagnostic snapshot
- `User/ch32v30x_it.h` - Added ina226.h and dac8571.h includes for ISR context

## Decisions Made
- Alert configuration uses 3-byte write format matching existing INA226 register write pattern
- ISR reads all 4 MOS alert registers because wired-OR topology means any channel can trigger
- __disable_irq/__enable_irq around DAC zeroing ensures atomic I2C transaction in ISR context
- Volatile globals (fault_triggered, fault_source_mask, last_dac_value) provide ISR-to-main-loop communication without heap allocation

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- EXTI4 ISR foundation ready for Wave 2 (PID control loop) and Wave 3 (fault state machine)
- fault_triggered and fault_source_mask volatiles available for main loop fault handling
- last_dac_value volatile defined for ISR snapshot (maintained by main loop in Plan 03)

---
*Phase: 02-control-loop-protection*
*Completed: 2026-06-07*
