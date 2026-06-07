# Phase 2: Control Loop + Protection - Context

**Gathered:** 2026-06-07
**Status:** Ready for planning

## Phase Boundary

The electronic load maintains a set voltage or current via PID control, with all hardware over-current protection active and a soft-start ramp preventing inrush. Two independent PID instances (CV and CC) run in a SysTick-timed super-loop at ~100ms. Four INA226 ALARM outputs are OR'd to a single EXTI line for hardware fault detection, with software total-power monitoring sharing the same fault path. Auto-retry recovery with permanent latch after max retries. INA226 calibration registers are periodically re-validated.

No cJSON protocol (Phase 3), no WS2812 LEDs (Phase 4), no fan control (Phase 4).

## Implementation Decisions

### Fault Protection Behavior
- **D-01:** Auto-retry recovery after fault: system transitions IDLE→CV/CC after 3-second cooldown. Up to 5 retries; on the 5th consecutive fault, latch permanently until explicit clear command (Phase 3).
- **D-02:** Full diagnostic snapshot on fault: ISR reads all 4 MOS channels (current, voltage) + summary INA226 + active mode + DAC value + fault type + retry count. Logged via printf before zeroing DAC.
- **D-03:** Separate fault types tracked in a fault register: per-channel overcurrent (INA226 hardware alarm) vs total power over-limit (software check). Retry counter resets to 0 after 30 seconds of fault-free operation.
- **D-04:** Unified fault handler: both hardware EXTI (overcurrent) and software main-loop check (total power OPP) call the same fault entry function. Same DAC-zero, mode→FAULT, snapshot, and auto-retry path.

### Soft-start Ramp Strategy
- **D-05:** Fixed 500ms linear DAC ramp from 0 to PID-computed target on initial engagement.
- **D-06:** Soft-start applies only on IDLE→CV or IDLE→CC transitions (including post-fault recovery). Mode switches (CV↔CC) and setpoint changes within the same mode use instant PID tracking — no re-ramp.

### Control Loop Timing & Structure
- **D-07:** ~100ms control period enforced via SysTick millisecond timestamp check in the super-loop. No additional timer peripheral required.
- **D-08:** Two independent PID instances — one for CV mode (feedback = summary bus voltage), one for CC mode (feedback = summary current). Each has its own Kp, Ki, Kd, error integral, and last error.
- **D-09:** Anti-windup via conditional integration: freeze the integrator when the computed DAC output is saturated at 0 or 65535. Integrator only accumulates when output has headroom.
- **D-10:** All 5 INA226 devices read every control cycle: summary (bus voltage + current for PID), 4 MOS channels (current for protection monitoring). ~30ms I2C traffic per cycle at 100kHz.

### EXTI Pin Assignment & Fault Isolation
- **D-11:** Four INA226 ALARM outputs (open-drain, active-low) wired-OR to a single GPIO: PA4, EXTI4, falling-edge interrupt. One MCU pin, one EXTI handler.
- **D-12:** ISR reads all 4 INA226 alert registers (0x06) over I2C to identify which channel(s) triggered. This is acceptable because the full snapshot (D-02) already requires I2C reads in ISR context.
- **D-13:** Total power over-limit (OPP) is a software check in the main loop: every control cycle, check summary INA226 power against rated wattage. If exceeded, call the same fault handler as the EXTI path.

### INA226 Calibration Re-validation (PROT-04)
- **D-14:** Periodic calibration check in the main loop — re-read the calibration register (0x05) of each INA226. If zero-valued while bus voltage is present (>0V), re-initialize that device via `ina226_init()`. Frequency and implementation details left to Claude's discretion.

### State Machine
- **D-15:** Four states: IDLE, CV, CC, FAULT. Transitions: IDLE→CV (engage with soft-start), IDLE→CC (engage with soft-start), CV↔CC (instant PID handoff, no soft-start), CV/CC→FAULT (hardware EXTI or software OPP), FAULT→IDLE (auto-retry timeout or explicit clear).

### Claude's Discretion
- PID coefficient selection (Kp, Ki, Kd for CV and CC) — researcher investigates tuning methodology, planner determines how coefficients are stored/applied
- INA226 alert register configuration (setting alert limit register and mask/enable register for overcurrent threshold)
- Rated wattage for total power OPP — #define constant, researcher verifies against hardware spec
- Calibration re-validation check frequency and threshold details
- Fault register bit layout and struct design
- ISR/minimal I2C read strategy for alert identification — whether to batch reads or sequential

## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Planning & Requirements
- `.planning/ROADMAP.md` — Phase 2 goal, success criteria (5 items), phase dependencies
- `.planning/PROJECT.md` — Project constraints (32K SRAM, 128K Flash, bare-metal), key decisions, evolution rules
- `.planning/REQUIREMENTS.md` — Full requirements: CTRL-01 (CV PID), CTRL-02 (CC PID), CTRL-03 (anti-windup + soft-start), PROT-01 (alarm config), PROT-02 (EXTI ISR), PROT-03 (summary read), PROT-04 (cal re-validation)
- `.planning/phases/01-hardware-foundation/01-CONTEXT.md` — Phase 1 decisions: Drivers/ directory structure, I2C abstraction via i2c_util, INA226 per-device structs (D-09), shared compile-time calibration (D-10), per-register getters (D-11), check_alert() returns mask (D-12)

### Existing Driver APIs (Phase 1)
- `Drivers/ina226.h` — INA226 API: init, get_bus_voltage, get_shunt_voltage, get_current, get_power, check_alert. Calibration constants, register addresses, alert bit masks.
- `Drivers/dac8571.h` — DAC8571 API: init, set_output(uint16_t). I2C address 0x4C.
- `Drivers/i2c_util.h` — I2C wrapper: i2c_util_write, i2c_util_read, i2c_util_bus_recovery. Timeout 10ms, 100kHz, auto-retry on bus fault.

### SPL Peripherals (for EXTI, GPIO, SysTick)
- `Peripheral/inc/ch32v30x_exti.h` — EXTI init struct, EXTI_Mode_Interrupt, EXTI_Trigger_Falling, GPIO port config for EXTI lines
- `Peripheral/inc/ch32v30x_gpio.h` — GPIO pin definitions, GPIO_Pin_4 (PA4), input mode configuration
- `Peripheral/inc/ch32v30x_i2c.h` — I2C SPL driver (already wrapped by i2c_util, but needed for understanding flag/event handling in ISR context)

### Application Integration Points
- `User/main.c` — Current super-loop structure, INA226 device array (devs[5]), init sequence. Phase 2 replaces the read-only loop with the control loop.
- `User/ch32v30x_it.c` — Where the EXTI4 ISR and I2C1 event/error handlers are added
- `Core/core_riscv.h` — NVIC_EnableIRQ, NVIC_SetPriority, __disable_irq/__enable_irq for ISR critical sections

### Device Datasheets
- INA226 datasheet — Alert register (0x06) bit layout, alert limit register, latch mode configuration (PROT-01). Calibration register validation (PROT-04).
- DAC8571 datasheet — Output transfer function, power-on reset value, I2C write timing

## Existing Code Insights

### Reusable Assets
- **INA226 driver** (`Drivers/ina226.c/.h`): Per-device init, getters for all registers, `check_alert()` returns raw mask. Already handles I2C timeout via i2c_util. Extended with alert limit configuration for PROT-01.
- **DAC8571 driver** (`Drivers/dac8571.c/.h`): `dac8571_set_output(uint16_t)` — the PID effector. Already handles I2C timeout.
- **i2c_util** (`Drivers/i2c_util.c/.h`): Timeout-protected reads/writes with bus recovery. ISR must use these carefully — I2C operations in ISR context are blocking but tolerable for the ~12ms needed to read 4 alert registers.
- **SysTick** (`Delay_Init`, `Delay_Ms`): Already initialized in main.c. A global millisecond counter variable (incremented in SysTick_Handler) provides the timestamp for D-07.
- **Debug printf** (`Debug/debug.c`): USART1 + USB-CDC dual printf for fault snapshot logging.

### Established Patterns
- **SPL init struct pattern**: `EXTI_InitTypeDef exti_init = {0}` with config struct, `EXTI_Init(&exti_init)`. Follows the same pattern as GPIO, I2C, etc.
- **Weak interrupt handlers**: Override in `ch32v30x_it.c` for EXTI4_IRQHandler. Add I2C1_EV_IRQHandler and I2C1_ER_IRQHandler if not already present.
- **Super-loop with blocking I2C**: Current main.c reads all 5 INA226 in sequence with no timing control. Phase 2 adds SysTick timestamp gating and PID computation between reads.
- **Global state for ISR communication**: Volatile flags for fault detection — ISR sets `fault_triggered`, main loop reads and processes.
- **Static allocation**: All PID state, fault registers, and mode state as static globals — no malloc needed.

### Integration Points
- **main.c super-loop**: Replace the current read-only while(1) with: SysTick check → read summary INA226 → PID compute → DAC update → read MOS channels → OPP check → fault state machine → sleep until next tick.
- **ch32v30x_it.c**: Add `EXTI4_IRQHandler` — read 4 INA226 alert registers, log snapshot, zero DAC via `dac8571_set_output(0)`, set fault flag, clear EXTI pending.
- **PA4 GPIO config**: Configure as input with pull-up (ALARM open-drain), EXTI4 falling edge, in main.c GPIO init section.
- **I2C1 in ISR context**: i2c_util functions are blocking and use SysTick delays — they work in ISR context because interrupt nesting is enabled (CSR 0x804). But this means other interrupts may be delayed during the ~12ms alert read window. Researcher should verify this is acceptable.

## Specific Ideas

No specific UI/UX references — this is an embedded control and protection phase. Output is DAC voltage regulation and fault log messages via printf.

## Deferred Ideas

None — discussion stayed within phase scope.

---

*Phase: 2-Control Loop + Protection*
*Context gathered: 2026-06-07*
