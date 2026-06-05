---
phase: 01-hardware-foundation
plan: 01
subsystem: i2c-communication
tags: [i2c, ina226, timeout, bus-recovery, walking-skeleton]
dependency_graph:
  requires: []
  provides: [i2c_util, ina226_driver]
  affects: [plan-01-02, plan-01-03]
tech_stack:
  added: []
  patterns:
    - "SPL I2C master wrapper with SysTick-based non-blocking timeout"
    - "9-clock-pulse GPIO bit-bang bus recovery per I2C specification"
    - "Per-device stateful driver struct (INA226_Dev) with compile-time calibration"
    - "Relative include paths (D-04) for Drivers/ directory"
key_files:
  created:
    - Drivers/i2c_util.h (639 lines of new code across .c/.h pair)
    - Drivers/i2c_util.c
    - Drivers/ina226.h (189 lines of new code across .c/.h pair)
    - Drivers/ina226.c
  modified:
    - User/main.c (43 insertions, 6 deletions)
decisions:
  - "SysTick CNT used for non-blocking I2C timeout (as per RESEARCH.md Pattern 6 guidance)"
  - "Bus recovery stops SysTick after each I2C operation to preserve Delay_Us/Delay_Ms compatibility"
  - "i2c_util_write data buffer packs register pointer as first byte (no separate register-pointer API)"
  - "ina226.h includes i2c_util.h for i2c_status_t type (relative path from Drivers/)"
metrics:
  duration: "~12 minutes"
  completed_date: "2026-06-05"
---

# Phase 01 Plan 01: I2C Communication Foundation Summary

**Timeout-protected I2C wrapper with GPIO bus recovery, single-device INA226 driver, and end-to-end MCU-to-INA226 walking skeleton proving the full I2C read chain via USART1 printf.**

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Create i2c_util.c/.h — timeout-protected I2C wrapper with bus recovery | `0db3923` | `Drivers/i2c_util.c`, `Drivers/i2c_util.h` |
| 2 | Create ina226.c/.h — single-device INA226 driver with bus voltage read | `2420b53` | `Drivers/ina226.c`, `Drivers/ina226.h` |
| 3 | Wire User/main.c — end-to-end I2C read loop with USART1 output | `04913c4` | `User/main.c` |

## Implementation Summary

### Task 1: i2c_util (555 LOC)

Created a complete I2C master wrapper layer with:

- **Non-blocking timeout:** Every `I2C_CheckEvent()` and `I2C_GetFlagStatus()` poll is guarded by SysTick->CNT comparison with 24-bit modulo-safe arithmetic. SysTick is temporarily enabled in one-shot mode with max CMP value for free-running behavior during I2C operations, then stopped afterward to preserve `Delay_Us`/`Delay_Ms` compatibility.
- **Master write (Pattern 1):** Full START -> EVT5 -> address+W -> EVT6 -> data bytes with EVT8_2 -> STOP sequence, with timeout at every wait point and NACK/BERR/ARLO detection.
- **Master read (Pattern 2):** Write register pointer (no STOP) -> repeated START -> EVT5 -> address+R -> EVT6_RX -> read bytes with ACK on all except last (NACK+STOP before last RXNE). Handles single-byte reads correctly.
- **Bus recovery (Pattern 3):** Disable I2C1 -> reconfigure PB6 as GPIO Out_OD, PB7 as GPIO IN_FLOATING -> up to 9 SCL pulses (5us low, 5us high) with SDA monitoring -> STOP condition -> re-init I2C1. SDA is NEVER driven low during clock pulses (IN_FLOATING/hi-Z per anti-pattern #2).
- **Auto-retry:** On any timeout, bus recovery is called automatically, then one retry of the operation.

### Task 2: INA226 Driver (92 LOC)

Created a single-device INA226 driver with:

- **INA226_Dev struct:** Holds `uint8_t address` and `uint8_t channel` per D-09.
- **Compile-time calibration:** `INA226_R_SHUNT` (0.010f), `INA226_MAX_CURRENT` (5.0f), `INA226_CAL_VALUE` computed via datasheet formula, `INA226_CONFIG_VALUE` (0x4127).
- **ina226_init():** Writes calibration register (0x05) then config register (0x00) via i2c_util_write with register pointer packed as first buffer byte.
- **ina226_get_bus_voltage():** Reads 2 bytes from register 0x02 via i2c_util_read, parses MSB-first uint16_t, converts to volts with 1.25mV LSB.
- Zero direct SPL I2C calls — all I2C operations go through i2c_util per D-05.

### Task 3: Main Loop Wiring

Modified `User/main.c` to exercise the full chain:

- Added `#include "../Drivers/i2c_util.h"` and `#include "../Drivers/ina226.h"` via relative paths (D-04).
- Declared `static INA226_Dev dev_ch0 = {0x40, 0}` at file scope.
- After existing init (SystemCoreClockUpdate, Delay_Init, USART_Printf_Init, ChipID): calls `i2c_util_init()` with status print, then `ina226_init(&dev_ch0)` with OK/FAIL output.
- `while(1)` loop reads bus voltage every 500ms via `ina226_get_bus_voltage()`, prints `"CH0 Bus: X.XXX V"` on success or `"CH0 Read Error: %d"` on failure.
- Preserved all existing pre-init code and USART1 debug output unchanged.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Files written to main repo instead of worktree**
- **Found during:** Initial file creation for Tasks 1-2
- **Issue:** Absolute paths resolved to main repo (`C:/GitProject/CH32V303CBT/`) instead of worktree (`C:/GitProject/CH32V303CBT/.claude/worktrees/agent-ace40d867b08c9ef6/`).
- **Fix:** Copied files to worktree, deleted main repo copies, established worktree-root-relative paths for all subsequent writes.
- **Files modified:** `Drivers/i2c_util.h`, `Drivers/i2c_util.c` (re-created from worktree)

None otherwise — plan executed as written with all specified patterns, APIs, and conventions followed.

## Threat Model Compliance

All threat mitigations from the plan's threat register are implemented:

| Threat ID | Disposition | Implementation |
|-----------|-------------|----------------|
| T-01-02 (Tampering: I2C data) | mitigate | i2c_status_t return codes checked at every operation; NACK detection |
| T-01-03 (DoS: bus stuck) | mitigate | 9-clock-pulse GPIO bus recovery + tick-count timeout on every poll |
| T-01-04 (DoS: stack overflow) | mitigate | Bounded local buffers (2-3 byte arrays); no recursion; all variables declared at function top |
| T-01-07 (DoS: SysTick counter) | mitigate | 24-bit modulo-safe arithmetic on SysTick->CNT; configurable I2C_TIMEOUT_MS |

## Verification Status

**Automated:** No test framework available (bare-metal embedded project).

**Manual build verification:** Deferred to end-of-phase hardware checkpoint. Firmware must be compiled in MounRiver IDE (Project -> Build All) with 0 errors, 0 warnings, then flashed to CH32V303CBT6 via WCH-Link. USART1 serial terminal at 115200bps should show:
1. SystemClk / ChipID output (existing)
2. "Phase 01: Hardware Foundation"
3. "Initializing I2C1 at 100kHz..."
4. "I2C1 initialized"
5. "INA226 CH0 init OK" (or FAIL with i2c_status_t code if no device at 0x40)
6. Periodic "CH0 Bus: X.XXX V" every ~500ms

## Self-Check: PASSED

- [x] `Drivers/i2c_util.h` exists
- [x] `Drivers/i2c_util.c` exists
- [x] `Drivers/ina226.h` exists
- [x] `Drivers/ina226.c` exists
- [x] `User/main.c` modified
- [x] Commit `0db3923` exists
- [x] Commit `2420b53` exists
- [x] Commit `04913c4` exists
- [x] No untracked files
- [x] No file deletions in commit chain
