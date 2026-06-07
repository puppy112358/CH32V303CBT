---
phase: "02-control-loop-protection"
plan: "04"
subsystem: "protection"
tags: ["fault", "state-machine", "opp", "calibration", "auto-retry", "overcurrent"]

requires:
  - phase: "02"
    plan: "02"
    provides: "INA226 alert API, EXTI4 ISR, fault flag globals"
  - phase: "02"
    plan: "03"
    provides: "PID controller, 100ms control loop, SystemMode enum, engage functions"
provides:
  - "Unified fault handler for hardware (EXTI overcurrent) and software (OPP) faults"
  - "FaultRegister: per-channel OC mask, fault type, retry count, latch flag"
  - "Auto-retry state machine: 3s cooldown, 5-retry permanent latch (D-01)"
  - "30-second fault-free retry counter reset (D-03)"
  - "OPP software check: summary power > RATED_WATTAGE (D-13)"
  - "Periodic INA226 calibration re-validation every 1s (PROT-04)"
affects: ["uart-command", "phase-03"]

tech-stack:
  added: []
  patterns:
    - "FaultRegister union with bitfield struct for compact state encoding"
    - "Unified fault handler pattern: shared retry/latch logic for HW and SW sources"
    - "ISR/main-loop split: ISR does hardware protection (DAC zero), main loop does state machine"

key-files:
  created:
    - "Drivers/fault.h"
    - "Drivers/fault.c"
  modified:
    - "User/main.c"

key-decisions:
  - "SystemMode enum relocated to fault.h as single source of truth"
  - "Fault state machine runs in main loop context (not ISR) — ISR only flags + zeroes DAC"
  - "fault_reg declared extern in fault.h for main.c retry_count reset access"
  - "fault_clear() is a stub with TODO for Phase 3 cJSON command"

patterns-established:
  - "Fault handling pattern: ISR sets flags + protects hardware → main loop runs state machine"
  - "Periodic validation pattern: every N cycles, read calibration registers, re-init if lost"

requirements-completed:
  - "PROT-02"
  - "PROT-03"
  - "PROT-04"

duration: 15min
completed: 2026-06-07
---

# Phase 02 Plan 04: Fault State Machine + OPP + Calibration Re-validation Summary

**Unified fault handler with auto-retry state machine, OPP software protection, and periodic INA226 calibration integrity checks**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-06-07T08:55:00Z
- **Completed:** 2026-06-07T09:10:00Z
- **Tasks:** 3
- **Files modified:** 3 (2 created, 1 modified)

## Accomplishments
- Created fault handler module (fault.h/c) with FaultRegister union and unified fault_handler_hw/fault_handler_opp
- Implemented auto-retry state machine: 3s cooldown (30 cycles), 5-retry permanent latch, 30s fault-free counter reset
- Added OPP software check every control cycle (summary power > RATED_WATTAGE)
- Wired fault_print_snapshot with full per-channel + summary INA226 diagnostics (D-02)
- Added periodic calibration re-validation every 10 control cycles (1 second) with re-init on loss (PROT-04)
- Replaced simple fault_triggered check with full fault state machine integration in main loop

## Task Commits

Each task was committed atomically:

1. **Task 1: Create Fault Handler Module** - `6530684` (feat)
2. **Task 2: OPP Check + Calibration Re-validation + Wiring** - `72df3cc` (feat)
3. **Task 3: End-to-End Integration Verification** — All checks passed (no separate commit)

**Plan metadata:** (`docs(02-04)` commit to follow)

## Files Created/Modified
- `Drivers/fault.h` - SystemMode enum, FaultRegister union, FaultType enum, protection constants, public API
- `Drivers/fault.c` - fault_init, fault_handler_hw/opp, fault_state_machine, fault_clear, fault_print_snapshot
- `User/main.c` - fault.h include, OPP check, fault state machine dispatch, retry counter reset, calibration re-validation

## Decisions Made
- SystemMode enum relocated to fault.h as single source of truth (removed duplicate from main.c)
- Fault state machine in main loop context, not ISR — ISR only handles hardware protection (DAC zero)
- fault_reg declared extern for main.c retry_count reset access
- fault_clear() is a stub with TODO for Phase 3 cJSON clear command

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Phase 02 complete — all control loop and protection features implemented
- Ready for Phase 03: UART cJSON command integration (setpoint control, mode switching)
- fault_clear() stub awaits cJSON-based clear command in Phase 3
- engage_cv(5.0f) test call in main.c ready to be replaced by runtime commands

---
*Phase: 02-control-loop-protection*
*Completed: 2026-06-07*
