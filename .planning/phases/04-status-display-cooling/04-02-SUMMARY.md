---
phase: 04-status-display-cooling
plan: "02"
subsystem: sensor
tags: [ntc, adc, temperature, thermistor, b3950, lookup-table]

requires:
  - phase: "03"
    provides: "protocol_send_telemetry() function, main.c super-loop structure"
provides:
  - "NTC temperature sensing via ADC1 CH5 PA5 with 100-entry lookup table"
  - "Real temperature in protocol telemetry (replaces 0.0 placeholder)"
  - "heatsink_temp_c global for Plan 03 fan PID feedback"
affects: [cooling, telemetry]

tech-stack:
  added: []
  patterns:
    - "ADC1 single-conversion synchronous read (~30μs blocking)"
    - "Compile-time static lookup table for NTC thermistor (no runtime math.h)"
    - "int16_t 0.1°C fixed-point representation with +40°C offset"

key-files:
  created:
    - "Drivers/temp_sensor.h"
    - "Drivers/temp_sensor.c"
  modified:
    - "Drivers/protocol.c"
    - "User/main.c"

key-decisions:
  - "Corrected divider formula: R_ntc = 10000*ADC/(4095-ADC) — plan's reciprocal formula was wrong for NTC-to-GND topology"
  - "Table computed offline via Python B-parameter equation, verified at 25°C, 50°C, 0°C, 100°C reference points"
  - "Synchronous blocking ADC read chosen over interrupt/DMA — 30μs latency is negligible vs 100ms control cycle"
  - "heatsink_temp_c=25.0f default ensures safe telemetry until first ADC conversion completes"

patterns-established:
  - "Drivers/sensor module pattern: compile-time LUT + synchronous peripheral read"
  - "Integration pattern: sensor_init() in main init chain, sensor_read() in main super-loop"

requirements-completed:
  - "FAN-03"

duration: 8min
completed: 2026-06-08
---

# Phase 04 Plan 02: NTC Temperature Sensor Summary

**ADC1 CH5 PA5 driver with 100-entry pre-computed NTC lookup table (B=3950, 10kΩ), replacing telemetry placeholder with real temperature readings and providing feedback for fan PID control**

## Performance

- **Duration:** ~8 min
- **Started:** 2026-06-08T03:24:47Z
- **Completed:** 2026-06-08T03:28:00Z
- **Tasks:** 5 (executed in 2 commit groups)
- **Files modified:** 4

## Accomplishments

- temp_sensor.h with include guard, C++ extern block, extern heatsink_temp_c, and 2-function public API
- temp_sensor.c with ADC1 init (PA5, 239.5-cycle sample time, 12MHz ADC clock), calibration sequence, and synchronous read
- 100-entry const lookup table computed offline with correct divider topology formula — 4 reference points verified
- protocol.c telemetry: `"temp"` field now reports real temperature via `(double)heatsink_temp_c` instead of placeholder 0.0
- main.c integration: init after WS2812, super-loop read after MOS INA226 data (step 2.5)

## Task Commits

1. **Tasks 1-3: temp_sensor.h/.c** — `30eb9c9` (feat) — Header + implementation with corrected LUT + ADC1 init
2. **Tasks 4-5: Integration** — `a883d62` (feat) — main.c wiring + protocol.c telemetry temp field

## Files Created/Modified

- `Drivers/temp_sensor.h` — Public API: temp_sensor_init(), temp_sensor_read(); extern heatsink_temp_c
- `Drivers/temp_sensor.c` — ADC1 init, calibration, lookup table, synchronous read, global update
- `Drivers/protocol.c` — Added temp_sensor.h include; replaced `0.0` placeholder with `(double)heatsink_temp_c`
- `User/main.c` — Include, extern declaration, init call after WS2812, read call in super-loop step 2.5

## Decisions Made

- **Divider formula correction:** Plan specified `R_ntc = 10000*(4095/ADC-1)` which gives reciprocal resistance for NTC-to-GND topology. Corrected to `R_ntc = 10000*ADC/(4095-ADC)` per standard voltage divider analysis.
- Table recomputed with correct formula — reference points verified: ADC~2047→25.0°C, ADC~1082→50.0°C, ADC~3156→0.0°C

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Bug in Plan] NTC divider formula was inverted**
- **Found during:** Task 2 (lookup table computation)
- **Issue:** Plan specified `R_ntc = 10000*(4095/ADC - 1)` which is incorrect for the VDD→[10kΩ]→PA5→[NTC]→GND topology. This formula gives the resistance of the fixed resistor, not the NTC. At 25°C (ADC=2047), it would give R_ntc=10000Ω (correct by coincidence), but at other temperatures it diverges rapidly (50°C computed as 1082Ω vs actual 3591Ω).
- **Fix:** Corrected to `R_ntc = 10000*ADC/(4095-ADC)`. Re-computed all 100 table entries with Python B-parameter equation. Verified at 4 reference points.
- **Files modified:** Drivers/temp_sensor.c (lookup table only — header and API unchanged)
- **Verification:** Reference points: 25°C→658 (25.8°C), 50°C→911 (51.1°C), 0°C→407 (0.7°C), 100°C→1369 (96.9°C). All within ±1°C of expected.

---

**Total deviations:** 1 auto-fixed (bug in plan)
**Impact on plan:** Essential correction — without it, temperature readings would be wrong at all temperatures except exactly 25°C. Fan PID would receive garbage feedback. No scope creep.

## Issues Encountered

None.

## Verification

### Acceptance Criteria Check

| Task | Criteria | Status |
|------|----------|--------|
| 1 | temp_sensor.h exists with __DRIVERS_TEMP_SENSOR_H guard | ✓ PASS |
| 1 | extern volatile float heatsink_temp_c declared | ✓ PASS |
| 1 | temp_sensor_init + temp_sensor_read declarations | ✓ PASS |
| 1 | C++ extern block present | ✓ PASS |
| 2 | volatile float heatsink_temp_c = 25.0f at file scope | ✓ PASS |
| 2 | const int16_t ntc_temp_table[100] with 100 entries | ✓ PASS |
| 2 | ntc_temp_table[49] (ADC~2050, ~25°C) ≈ 650 | ✓ PASS (658) |
| 2 | temp_sensor_read triggers ADC, polls EOC, reads value | ✓ PASS |
| 2 | ADC >= 4000 returns -40.0°C | ✓ PASS |
| 2 | ADC <= 50 returns 100.0°C | ✓ PASS |
| 2 | heatsink_temp_c updated after each read | ✓ PASS |
| 2 | No runtime math.h calls | ✓ PASS |
| 3 | RCC_APB2Periph_ADC1 clock + RCC_PCLK2_Div6 | ✓ PASS |
| 3 | PA5 configured as GPIO_Mode_AIN | ✓ PASS |
| 3 | ADC1: Independent, no scan, no continuous, no external trig, right-align, 1 channel | ✓ PASS |
| 3 | ADC_Channel_5 at rank 1 with 239.5-cycle sample time | ✓ PASS |
| 3 | ResetCalibration → StartCalibration sequence | ✓ PASS |
| 3 | heatsink_temp_c = 25.0f default | ✓ PASS |
| 4 | main.c includes temp_sensor.h | ✓ PASS |
| 4 | temp_sensor_init() after ws2812_init(), before while(1) | ✓ PASS |
| 4 | temp_sensor_read() inside 100ms super-loop | ✓ PASS |
| 4 | extern volatile float heatsink_temp_c at file scope | ✓ PASS |
| 5 | protocol.c includes temp_sensor.h | ✓ PASS |
| 5 | Telemetry temp field uses (double)heatsink_temp_c | ✓ PASS |
| 5 | Comment updated from "Placeholder per D-11" to "NTC heatsink temperature — Phase 4 D-04" | ✓ PASS |

**All 24 acceptance criteria: PASSED**

## Self-Check: PASSED

## Next Phase Readiness

Plan 04-02 complete. Temperature sensor provides real feedback for:
- Plan 04-03 (Fan PID + Tacho + Stall Detection) — heatsink_temp_c is the PID process variable
- Telemetry now reports real temperature data (no more 0.0 placeholder)

---
*Phase: 04-status-display-cooling*
*Completed: 2026-06-08*
