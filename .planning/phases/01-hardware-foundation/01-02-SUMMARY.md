---
phase: 01-hardware-foundation
plan: 02
subsystem: i2c-drivers
tags: [ina226, dac8571, i2c, ch32v303, embedded, bare-metal]

# Dependency graph
requires:
  - phase: 01-01
    provides: "i2c_util.h (timeout-protected I2C wrapper), ina226_init(), ina226_get_bus_voltage(), INA226_Dev struct"
provides:
  - "Full INA226 driver: shunt voltage, current, power getters; alert register check; alert flag bit defines"
  - "DAC8571 16-bit DAC driver: init (probe write), set_output (3-byte I2C)"
  - "Expanded main.c: 5-device INA226 array at confirmed addresses 0x40-0x44, DAC8571 mid-scale test"
affects: ["phase-02-control-loop", "phase-03-protocol", "phase-04-fan-ws2812"]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Per-register I2C getter pattern: each getter function reads exactly one 16-bit register on demand via i2c_util_read (D-11)"
    - "Device struct pattern: INA226_Dev {address, channel} passed as pointer to all INA226 functions (D-09)"
    - "Loop init pattern: for-loop over INA226_Dev array for multi-device initialization with per-device OK/FAIL logging"
    - "DAC8571 3-byte write pattern: control_byte(0x00) + MSB + LSB via single i2c_util_write call"

key-files:
  created:
    - "Drivers/dac8571.h: DAC8571_ADDR (0x4C), dac8571_init(), dac8571_set_output()"
    - "Drivers/dac8571.c: probe-write init, 16-bit DAC output via 3-byte I2C write"
  modified:
    - "Drivers/ina226.h: added 4 new function declarations, 6 alert flag bit defines"
    - "Drivers/ina226.c: added 4 new function implementations (shunt voltage, current, power, alert check)"
    - "User/main.c: replaced single-device code with 5-device array + loop init + DAC init + expanded while(1)"

key-decisions:
  - "INA226 address plan confirmed by user: 0x40-0x44 (5 devices using A0/A1 pin combinations to GND/VS/SDA/SCL)"
  - "DAC8571 address confirmed: 0x4C (7-bit I2C)"
  - "All getter functions use i2c_util_read per D-05 (no raw SPL I2C calls)"
  - "check_alert() returns raw mask without interpretation per D-12 (application decides action)"

patterns-established:
  - "INA226 getter pattern: uint8_t buf[2], i2c_util_read(addr, reg, buf, 2, timeout), MSB-first parse, float conversion, return status"
  - "DAC8571 write pattern: uint8_t data[3] = {0x00, MSB, LSB}, i2c_util_write(DAC8571_ADDR, data, 3, timeout)"
  - "Multi-device loop: for(i=0; i<DEV_COUNT; i++) { read all registers for devs[i]; print with channel ID }"

requirements-completed: [I2C-01, I2C-02]

# Metrics
duration: ~15min
completed: 2026-06-05
---

# Phase 01 Plan 02: INA226 Full Driver + DAC8571 Driver Summary

**Full INA226 driver expanded with shunt voltage, current, power, and alert register getters for 5 devices at user-confirmed addresses 0x40-0x44, plus DAC8571 16-bit DAC driver at 0x4C with mid-scale test output**

## Performance

- **Duration:** ~15 min (2 implementation tasks; checkpoint resolved before resume)
- **Started:** 2026-06-05 (resumed from Task 1 checkpoint)
- **Completed:** 2026-06-05
- **Tasks:** 3 (1 checkpoint resolved, 2 implemented)
- **Files modified/created:** 5 (2 modified, 2 created, 1 rewritten)

## Accomplishments
- Expanded INA226 driver from 1 getter (bus voltage) to 5 (bus voltage, shunt voltage, current, power, alert check)
- Added 6 INA226 alert register flag bit defines for application-level fault detection
- Created complete DAC8571 driver: probe-write initialization and 16-bit value output
- Rewired main.c to initialize and read all 5 INA226 devices via loop, plus DAC8571 mid-scale test
- All I2C operations go through i2c_util wrapper per D-05 (zero raw SPL I2C calls in device drivers)

## Task Commits

Each task was committed atomically:

| # | Task | Commit | Type |
|---|------|--------|------|
| 1 | INA226 address plan confirmation checkpoint | *(resolved by user before resume)* | checkpoint:human-verify |
| 2 | Expand Drivers/ina226.c/.h — all devices, all getters, alert check | `2d33cd0` | feat |
| 3 | Create Drivers/dac8571.c/.h + wire all devices into User/main.c | `f3a38fa` | feat |

## Files Created/Modified
- `Drivers/ina226.h` (modified) — 4 new function declarations, 6 alert flag bit defines
- `Drivers/ina226.c` (modified) — 4 new function implementations (~125 additional lines)
- `Drivers/dac8571.h` (created) — DAC8571_ADDR, dac8571_init, dac8571_set_output
- `Drivers/dac8571.c` (created) — probe-write init + 16-bit DAC output via 3-byte I2C
- `User/main.c` (modified) — 5-device array, loop init, DAC init, expanded while(1) reading all registers

## Decisions Made
- Used user-confirmed address plan: 0x40-0x44 for 5 INA226 devices (A0/A1 pin combinations to GND/VS/SDA/SCL), 0x4C for DAC8571
- All getter functions return `i2c_status_t` — consistent error propagation from i2c_util layer
- `ina226_check_alert()` returns raw mask without interpretation per D-12 — application code in Phase 2 decides action
- Shunt voltage output in millivolts (not volts) for practical readability on serial console
- DAC8571 `dac8571_init()` returns void and prints status directly (no config registers to initialize)
- Mid-scale test value 0x8000 (DAC code 32768) chosen for DAC8571 verification

## Deviations from Plan

### Checkpoint Resolution

**Task 1: INA226 address plan confirmation checkpoint** — The plan originally noted that INA226 datasheet supports only 4 addresses (0x40, 0x41, 0x44, 0x45) and flagged addresses 0x42/0x43 as physically impossible. The user confirmed that the hardware uses 16 INA226 addresses via A0/A1 pin combinations (each pin connects to GND, VS, SDA, or SCL — 4 values per pin = 16 combinations). The full address plan was confirmed as 0x40-0x44 for the 5 devices. No plan changes needed — the confirmed addresses were used as-is in the device array.

### Auto-fixed Issues

None — plan executed as written for Tasks 2 and 3.

## Issues Encountered
- Worktree path isolation: had to redirect all file operations from main-repo paths to worktree-relative paths (`C:/GitProject/CH32V303CBT/.claude/worktrees/agent-a74f3f13dc7c4e24b/...`). Standard worktree behavior, handled transparently.

## User Setup Required
None — no external service configuration required. All device addresses are compile-time defines. To change shunt resistor values or max current, edit the `#define` constants in `Drivers/ina226.h`.

## Next Phase Readiness
- All 5 INA226 devices are readable with all register getters
- DAC8571 is writable with verified mid-scale output
- Main loop exercises all devices without hanging on any single fault (each read is independently timeout-protected)
- Ready for Plan 01-03 (USB-CDC debug output)

---
*Phase: 01-hardware-foundation*
*Completed: 2026-06-05*
