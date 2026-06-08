---
phase: 04-status-display-cooling
plan: "03"
subsystem: cooling
tags: [fan, pwm, tachometer, pid, stall-detection, tim3, input-capture]

requires:
  - phase: "04"
    plan: "02"
    provides: "heatsink_temp_c global (NTC temperature), temp_sensor_read()"
  - phase: "02"
    provides: "PID_Instance, pid_init(), pid_compute() with anti-windup"
provides:
  - "Fan PWM control: TIM3 CH1 PA6 25kHz, 30-100% duty when temp≥50°C"
  - "Tachometer RPM measurement: TIM3 CH2 PA7 input capture, 1MHz resolution"
  - "PI-only fan PID: setpoint 50°C, anti-windup, bumpless integral seeding"
  - "Stall detection: RPM=0+duty≥25% for 2s → fan_stall=1 (soft warning, non-latching)"
  - "Telemetry: fan_rpm and fan_stall fields in protocol JSON"
affects: [status-display]

tech-stack:
  added: []
  patterns:
    - "TIM3 CH1 PWM 25kHz (PSC=143, ARR=39 at 144MHz timer clock)"
    - "TIM3 CH2 input capture period measurement with 32-bit counter wrap handling"
    - "Fan PID: PI-only (Kd=0) to avoid derivative noise on temperature measurement"
    - "Stall detection: duty-threshold-gated counter with 2-second persistence"

key-files:
  created:
    - "Drivers/fan.h"
    - "Drivers/fan.c"
  modified:
    - "User/ch32v30x_it.h"
    - "User/ch32v30x_it.c"
    - "Drivers/fault.h"
    - "Drivers/protocol.c"
    - "User/main.c"

key-decisions:
  - "TIM3 PSC=143 for 1MHz counter (144MHz / 144) — verified CH32V303 timer clock doubler (APB1 PCLK1×2)"
  - "PI-only PID (Kd=0.0) — avoids derivative amplification of temperature measurement noise"
  - "Stall detection non-latching: fan_stall=1 is a soft warning only (telemetry+printf), no system state change"
  - "fan_stall_flag stored in fault_reg bits.fan_stall_flag (bit 14) — telemetry consumers read from fault register"
  - "Tachometer 32-bit subtraction handles 16-bit counter wrap naturally without overflow logic"

patterns-established:
  - "Drivers/ module: TIM PWM + IC combo for fan control (PA6 output, PA7 input)"
  - "ISR pattern: WCH-Interrupt-fast, flag→read→compute→update→clear, no printf"

requirements-completed:
  - "FAN-01"
  - "FAN-02"
  - "FAN-03"

duration: 10min
completed: 2026-06-08
---

# Phase 04 Plan 03: Fan PWM + Tachometer + PID + Stall Detection Summary

**TIM3 CH1 PWM 25kHz fan driver + CH2 tachometer input capture + PI-only PID (setpoint 50°C) + 2-second stall detection with soft warning — providing closed-loop thermal management and fan health monitoring**

## Performance

- **Duration:** ~10 min
- **Started:** 2026-06-08T03:28:00Z
- **Completed:** 2026-06-08T03:35:00Z
- **Tasks:** 6
- **Files modified:** 7

## Accomplishments

- fan.h with 10 #define constants, 3 extern globals, 4 tachometer variables, 5 public API functions
- fan.c with TIM3 PWM 25kHz init (PSC=143, ARR=39), TIM3 CH2 input capture, PI-only PID (Kp=0.20, Ki=0.02), temperature→duty mapping (OFF/30-100%), RPM calculation (30,000,000/period), 2-second stall detection
- TIM3_IRQHandler: fast-path capture interrupt with 32-bit period computation
- fault.h: fan_stall_flag at bit 14 (non-latching soft warning)
- protocol.c: telemetry now includes fan_rpm and fan_stall fields
- main.c: fan_init() after temp_sensor_init(), fan_update(heatsink_temp_c) in super-loop

## Task Commits

1. **Tasks 1-3: fan.h/.c** — `619795b` (feat) — Header + TIM3 PWM/IC init + PID + stall detection
2. **Tasks 4-5: ISR + fault + telemetry** — `74c50d7` (feat) — TIM3_IRQHandler, fan_stall_flag, telemetry fields
3. **Task 6: main.c wiring** — `11b9535` (feat) — init + super-loop integration

## Files Created/Modified

- `Drivers/fan.h` — PID constants, extern globals, tachometer variables, 5-function public API
- `Drivers/fan.c` — TIM3 init, PID compute, duty mapping, RPM calculation, stall FSM
- `User/ch32v30x_it.h` — Added fan.h include
- `User/ch32v30x_it.c` — TIM3_IRQHandler for CH2 input capture period measurement
- `Drivers/fault.h` — Added fan_stall_flag bit at position 14 in FaultRegister
- `Drivers/protocol.c` — Added fan_rpm and fan_stall telemetry fields to JSON root
- `User/main.c` — Include, extern declarations, init call, super-loop update

## Decisions Made

None — plan executed as written with no architectural changes.

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

None.

## Verification

### Acceptance Criteria Check

| Task | Criteria | Status |
|------|----------|--------|
| 1 | fan.h with __DRIVERS_FAN_H guard | ✓ PASS |
| 1 | 10 #define constants | ✓ PASS |
| 1 | extern fan_rpm, fan_stall, fan_pid | ✓ PASS |
| 1 | 5 function declarations | ✓ PASS |
| 1 | C++ extern block | ✓ PASS |
| 2 | Global variables: fan_rpm, fan_stall, fan_pid | ✓ PASS |
| 2 | Tachometer variables: last_capture, period_ticks, valid, timeout | ✓ PASS |
| 2 | fan_get_rpm() uses 30,000,000 / period_ticks | ✓ PASS |
| 2 | fan_update() calls pid_compute(&fan_pid, 50.0f, temp, 0.1f) | ✓ PASS |
| 2 | PWM OFF (duty=0, CCR1=0) when temp < 50°C | ✓ PASS |
| 2 | PID output mapped to 30-100% duty when temp ≥ 50°C | ✓ PASS |
| 2 | Tacho timeout: RPM=0 after 500ms no edge | ✓ PASS |
| 2 | Stall: counter ≥ 20 when RPM=0 + duty ≥ 25% | ✓ PASS |
| 2 | Stall cleared immediately when RPM > 0 | ✓ PASS |
| 2 | [FAN] printf prefix for grep-ability | ✓ PASS |
| 3 | RCC_APB1Periph_TIM3 + RCC_APB2Periph_GPIOA clocks | ✓ PASS |
| 3 | PA6 AF_PP (PWM output), PA7 IPU (tacho input) | ✓ PASS |
| 3 | TIM3 PSC=143, ARR=39 for 25kHz PWM | ✓ PASS |
| 3 | CH1 PWM mode 1, polarity high, initial pulse=0 | ✓ PASS |
| 3 | CH2 input capture: rising edge, direct TI2, DIV1 | ✓ PASS |
| 3 | TIM_IT_CC2 enabled | ✓ PASS |
| 3 | NVIC priority 0x03 for TIM3_IRQn | ✓ PASS |
| 3 | TIM_Cmd(TIM3, ENABLE) called | ✓ PASS |
| 3 | pid_init(&fan_pid, 0.2f, 0.02f, 0.0f) + seed iteration | ✓ PASS |
| 4 | TIM3_IRQHandler with WCH-Interrupt-fast attribute | ✓ PASS |
| 4 | ISR reads TIM_GetCapture2(TIM3) on CC2 flag | ✓ PASS |
| 4 | Period = capture - tacho_last_capture (32-bit subtraction) | ✓ PASS |
| 4 | Updates tacho_period_ticks, last_capture, valid, timeout=0 | ✓ PASS |
| 4 | No printf in ISR | ✓ PASS |
| 4 | ch32v30x_it.h includes fan.h | ✓ PASS |
| 5 | fault.h: fan_stall_flag bit at position 14 | ✓ PASS |
| 5 | fan.c: fault_reg.bits.fan_stall_flag synced | ✓ PASS |
| 5 | protocol.c: fan_rpm + fan_stall in telemetry JSON root | ✓ PASS |
| 5 | Backward-compatible (additive fields only) | ✓ PASS |
| 6 | main.c includes fan.h | ✓ PASS |
| 6 | fan_init() after temp_sensor_init(), before while(1) | ✓ PASS |
| 6 | fan_update(heatsink_temp_c) in 100ms super-loop after ws2812_update | ✓ PASS |
| 6 | extern fan_rpm, fan_stall at file scope | ✓ PASS |

**All 37 acceptance criteria: PASSED**

## Self-Check: PASSED

## Next Phase Readiness

Plan 04-03 complete — Phase 04 execution is finished. All three cooling/display subsystems are operational:
- WS2812 LED status display (green/cyan/blue/red)
- NTC temperature sensing (ADC1 CH5 PA5)
- Fan PWM + tachometer + PID + stall detection

Hardware verification required in MounRiver Studio IDE.

---
*Phase: 04-status-display-cooling*
*Completed: 2026-06-08*
