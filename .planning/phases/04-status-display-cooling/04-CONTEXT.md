# Phase 4: Status Display + Cooling - Context

**Gathered:** 2026-06-07
**Status:** Ready for planning

## Phase Boundary

WS2812 LED strip (2 LEDs) displays operating mode via solid color — standby=green, CV=cyan, CC=blue, fault=red — at 50% brightness, driven by TIM2 CH1 PWM + DMA from a pre-computed bitstream buffer (zero CPU overhead during transmission). A single NTC thermistor (B=3950 10kΩ) on the main heatsink provides temperature feedback via an ADC channel. Fan speed is controlled by a position-based PID maintaining a fixed 50°C target — fan OFF below target, 30-100% duty when active — with tachometer RPM measurement via input capture. Fan stall (RPM=0 with PWM ≥ 25% for 2 seconds) raises a soft warning flag in telemetry; the load continues operating. The telemetry `temp` field (Phase 3 placeholder) is populated with real temperature data.

No new cJSON commands, no display beyond two solid-color LEDs, no thermal hard-fault protection (deferred to v2). This phase is the final v1 milestone phase.

## Implementation Decisions

### Temperature Sensing
- **D-01:** NTC thermistor + ADC — external sensor on heatsink. B=3950, 10kΩ at 25°C, voltage divider with 10kΩ fixed resistor to VDD.
- **D-02:** Single sensor on the main heatsink (shared across all 4 MOSFETs). No per-channel or ambient sensing.
- **D-03:** Any free ADC channel — researcher determines best available pin from CH32V303 pinout, avoiding conflicts with PA0 (TIM2 CH1), PA4 (EXTI4), PA2/PA3 (USART2), PA9 (USART1 TX), PA11/PA12 (USB), PB6/PB7 (I2C1).
- **D-04:** Sampled every 100ms control cycle. Temperature value replaces the Phase 3 telemetry `temp` field placeholder (`0.0`). Linearization via lookup table from the NTC B-parameter equation.

### WS2812 LED Driver
- **D-05:** 2 WS2812 LEDs, both always showing the same color. 48-byte bitstream buffer (2 LEDs × 24 bits per LED).
- **D-06:** Static solid colors only — no blinking, breathing, or animation effects. Color changes immediately on state transition. State-to-color mapping: IDLE/Standby = green, CV active = cyan, CC active = blue, Fault = red.
- **D-07:** 50% brightness — halve each RGB byte in firmware when building the bitstream.
- **D-08:** TIM2 CH1 PWM output on PA0, 800kHz bit rate. DMA channel transfers the pre-computed bitstream buffer to the TIM2 CCR register. CPU is free during the entire WS2812 transmission — no interrupt disabling needed.

### Fan Control
- **D-09:** Fixed 50°C temperature setpoint PID. `#define FAN_TARGET_TEMP_C 50.0f`. Reuses Phase 2's `pid_compute()` pattern with anti-windup (freeze integrator when output saturated at 30% or 100%).
- **D-10:** Fan OFF when temperature < 50°C. When active (temp ≥ target): 30-100% PWM duty range. 30% minimum ensures the fan actually starts spinning above its stall threshold.
- **D-11:** Fan PID computed every 100ms control cycle — same timing as the CV/CC PID loop. Output clamped to [0, MAX_DUTY] where 0 = fan off, MAX_DUTY = TIM ARR value for 100%.
- **D-12:** TIM PWM output for fan speed control — researcher determines which TIM channel and pin (must avoid conflicting with TIM2 CH1 on PA0 for WS2812). Standard 25kHz PWM frequency for 4-wire PC fans.
- **D-13:** Fan tachometer feedback via TIM input capture on a separate TIM channel. Measures period between tachometer pulses to compute RPM. Standard 2 pulses per revolution for PC fans.

### Fan Stall Detection
- **D-14:** Soft warning only — stall sets a flag in the telemetry packet and logs via printf. The electronic load continues operating normally. Hard thermal shutdown (>80°C) is deferred to v2.
- **D-15:** 2-second continuous RPM=0 detection window (20 control cycles at 100ms). Rejects brief tachometer measurement glitches.
- **D-16:** Stall check only when PWM duty ≥ 25%. Below 25% the fan is expected to be off or spinning too slowly for reliable tachometer reading. This is slightly below the 30% active minimum to avoid edge cases.

### Integration
- **D-17:** New drivers in `Drivers/`: `ws2812.c/.h` (LED bitstream generation + TIM2 DMA init), `fan.c/.h` (fan PID + PWM + tachometer), `temp_sensor.c/.h` (ADC init + NTC read + linearization). Follows Phase 1 D-01 to D-04 conventions.
- **D-18:** LED bitstream recomputed only on state change or color change — not every cycle. Fan PID runs each cycle. NTC read each cycle.
- **D-19:** All new code integrates into the existing 100ms super-loop in `User/main.c`, following the same blocking-call pattern as the existing INA226 reads and PID compute.

### Claude's Discretion
- Exact ADC channel for NTC (researcher identifies free pin from datasheet)
- TIM channel assignments for fan PWM output and tachometer input capture (researcher avoids PA0 conflict)
- NTC linearization lookup table size and resolution
- Fan PID coefficient defaults (Kp, Ki, Kd) — researcher investigates tuning methodology
- WS2812 bitstream generation algorithm (TIM2 CCR DMA timing pattern for WS2812 800kHz protocol)
- DMA channel assignment for TIM2 CCR update
- Tachometer input capture prescaler and period-to-RPM conversion formula
- `fan.h` and `temp_sensor.h` API design (init, read, compute function signatures)
- Whether to combine fan PID and fan PWM/tachometer into one `fan.c/.h` module or split into `fan.c/.h` + `fan_pid.c/.h`

## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Planning & Requirements
- `.planning/ROADMAP.md` — Phase 4 goal, 4 success criteria, phase dependencies (Phase 1)
- `.planning/PROJECT.md` — Project constraints (32K SRAM, 128K Flash, bare-metal), key decisions, evolution rules
- `.planning/REQUIREMENTS.md` — Full requirements: LED-01, LED-02, FAN-01, FAN-02, FAN-03

### Prior Phase Context (decisions this phase depends on)
- `.planning/phases/01-hardware-foundation/01-CONTEXT.md` — Drivers/ directory structure (D-01 to D-04), SPL init struct pattern, I2C at 100kHz, relative includes
- `.planning/phases/02-control-loop-protection/02-CONTEXT.md` — State machine IDLE/CV/CC/FAULT (D-15), PID instances + anti-windup (D-08, D-09), soft-start ramp (D-05, D-06), SysTick-gated ~100ms super-loop (D-07)
- `.planning/phases/03-communication/03-CONTEXT.md` — Telemetry schema with `temp` field placeholder (D-07, D-11), Drivers/ module pattern, UART0 at 115200 with odd parity

### Existing Driver APIs (Phase 1-3)
- `Drivers/pid.h` — PID API: pid_init, pid_compute, pid_set_integral. Fan PID reuses this directly.
- `Drivers/fault.h` — Fault register, SystemMode enum (MODE_IDLE=0, MODE_CV=1, MODE_CC=2, MODE_FAULT=3). Used for LED state-to-color mapping.
- `Drivers/protocol.h` — protocol_send_telemetry() signature. Temp field is the last float in the telemetry struct/JSON — must be populated by this phase.

### SPL Peripherals (new for Phase 4)
- `Peripheral/inc/ch32v30x_tim.h` — TIM API: TimeBaseInit, OC1Init (PWM), PWM mode config, ICInit (input capture), DMA/IT config for CCR update, ARR/PSC for baud rate + frequency
- `Peripheral/inc/ch32v30x_adc.h` — ADC API: Init (single channel, continuous or software trigger), calibration, GetConversionValue, sampling time config. NTC read uses one regular channel.
- `Peripheral/inc/ch32v30x_dma.h` — DMA API: Init (channel, peripheral base, memory base, buffer size), Cmd, ITConfig for TC (transfer complete) interrupt. TIM2 CCR DMA update for WS2812.
- `Peripheral/inc/ch32v30x_gpio.h` — GPIO pin config for fan PWM output, tachometer input, and ADC input. Alternate function mapping for TIM channels.

### Application Integration Points
- `User/main.c` — Super-loop structure (lines 291-428): SysTick gate → command poll → INA226 reads → PID compute → DAC update → fault checks → telemetry. Phase 4 insertions: NTC read after INA226 reads, fan PID after CV/CC PID, LED update on state change, telemetry temp field populated.
- `User/ch32v30x_it.c` — Where TIM DMA TC interrupt handler and fan tachometer input capture interrupt handler are added
- `User/ch32v30x_conf.h` — TIM, ADC, DMA already included. No changes needed.
- `Core/core_riscv.h` — NVIC_EnableIRQ for TIM2, ADC, and fan TIM interrupts

### Device Datasheets
- WS2812/WS2812B datasheet — 800kHz protocol timing: T0H/T0L, T1H/T1L, RESET code (>50μs low). 24-bit GRB color order.
- NTC thermistor datasheet (B=3950, 10kΩ) — Resistance-temperature table, B-parameter equation: R(T) = R25 × exp(B × (1/T - 1/298.15))

## Existing Code Insights

### Reusable Assets
- **PID implementation** (`Drivers/pid.c/.h`): `pid_init()`, `pid_compute()`, `pid_set_integral()`. Fan PID uses an identical instance — just different Kp/Ki/Kd coefficients. Anti-windup already built in.
- **SysTick timing** (`Delay_Init`, SysTick->CNT): Already provides the 100ms control period gating. Phase 4 piggybacks on the same timestamp — no new timer needed for scheduling.
- **State machine** (`Drivers/fault.h`, `system_mode` global): IDLE/CV/CC/FAULT enum already used by main loop. WS2812 driver reads `system_mode` to determine LED color — direct mapping, no new state.
- **Fault handler** (`Drivers/fault.c/.h`): `fault_reg` bitfield for flags. Fan stall flag can be added as a new bit in the fault register (or a separate stall flag variable).
- **Debug printf** (`Debug/debug.c`): Fan stall warnings, temperature readings, and LED state changes logged via printf/USB-CDC during development.
- **Telemetry assembly** (`Drivers/protocol.c`): `protocol_send_telemetry()` already has a `temp` parameter. Phase 4 passes the real NTC temperature value instead of `0.0f`. No protocol changes needed.
- **SPL TIM/ADC/DMA headers**: Already included in `ch32v30x_conf.h` — no build system changes needed for peripheral access.

### Established Patterns
- **SPL init struct pattern**: `TIM_TimeBaseInitTypeDef`, `TIM_OCInitTypeDef`, `ADC_InitTypeDef`, `DMA_InitTypeDef` — all zero-initialized with `= {0}`, config fields set, then `XXX_Init(periph, &struct)`. Same pattern as GPIO, I2C, USART already used.
- **Weak interrupt handlers**: Override in `ch32v30x_it.c` — `TIM2_IRQHandler` (DMA TC), `TIMx_IRQHandler` (fan tachometer input capture), `ADC1_2_IRQHandler` (if interrupt-driven ADC).
- **Super-loop with blocking calls**: Current main.c uses blocking I2C reads and blocking UART TX. NTC ADC read is also blocking but fast (~1-2ms). Fan PID compute is non-blocking (~50 cycles). LED bitstream DMA transfer is fire-and-forget — DMA handles it in hardware.
- **Global state for peripheral data**: `system_mode`, `last_dac_value`, `bus_v`/`bus_i` — module-level globals. New: `heatsink_temp_c` (float), `fan_rpm` (uint16_t), `fan_duty` (uint16_t), `fan_stall_flag` (volatile uint8_t).
- **Static allocation**: All bitstream buffers, PID instances, and NTC lookup tables as static arrays — no dynamic allocation. WS2812 bitstream buffer: 48 bytes. NTC lookup table: ~200 bytes (100 entries × 2 bytes). Fan PID: ~40 bytes. Total new RAM: < 500 bytes.
- **Drivers/ module pattern**: New `.c/.h` files in `Drivers/` with lowercase naming (e.g., `ws2812.c/.h`), relative includes from main.c (`#include "../Drivers/ws2812.h"`), SPL include guard with `__DRIVERS_WS2812_H` format.

### Integration Points
- **main.c super-loop insertions** (after existing step 5 — state machine dispatch, before step 6.5 — telemetry):
  ```
  Step 5.5: Read NTC temperature (ADC single conversion)
  Step 5.6: Compute fan PID → update fan PWM duty
  Step 5.7: Read fan RPM from tachometer input capture
  Step 5.8: Check fan stall condition (RPM=0 && duty≥25% for 2s)
  Step 5.9: Update WS2812 LED color if system_mode changed
  ```
- **main.c init section insertions** (after existing protocol_init, before PID init):
  ```
  Initialize ADC1 for NTC channel
  Initialize TIM2 CH1 + DMA for WS2812
  Initialize fan TIM (PWM channel + input capture channel)
  Initialize fan PID instance
  Initialize WS2812 (show standby=green)
  ```
- **ch32v30x_it.c**: Add `TIM2_IRQHandler` (DMA TC — triggers next bitstream DMA transfer or signals completion), fan TIM IRQ handler (input capture — measures tachometer pulse period).
- **ch32v30x_conf.h**: No changes — TIM, ADC, DMA already included.
- **Telemetry integration**: `protocol_send_telemetry()` signature already includes `temp` parameter. In the main loop, pass `heatsink_temp_c` instead of `0.0f`. Fan stall flag can be added to the `fault` bitfield or as a new telemetry field — Claude's discretion.

## Specific Ideas

No specific UI/UX references — this is an embedded hardware indication and cooling phase. LED colors and fan behavior defined by the implementation decisions above.

## Deferred Ideas

- **Hard thermal shutdown (OTP at >80°C)**: v2 requirement ADV-PROT-01. Fan stall + high temperature should eventually trigger a hard fault like overcurrent. Noted for future phase.
- **Runtime fan target temperature via cJSON**: Could be added as a command (`set_fan`) but out of scope for v1. The fixed 50°C target is sufficient.
- **Per-MOSFET temperature sensing**: Deferred to v2 with ADV-PROT-02 (MOSFET thermal derating). Single heatsink NTC is adequate for v1.
- **LED animation effects**: Blinking fault, breathing standby — nice visual polish but adds complexity without changing functionality. Static solid colors meet the success criteria.

---
*Phase: 4-Status Display + Cooling*
*Context gathered: 2026-06-07*
