# Phase 2: Control Loop + Protection - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-07
**Phase:** 2-Control Loop + Protection
**Areas discussed:** Fault Protection Behavior, Soft-start Ramp Strategy, Control Loop Timing & Structure, EXTI Pin Assignment & Fault Isolation

---

## Fault Protection Behavior

### Recovery Strategy

| Option | Description | Selected |
|--------|-------------|----------|
| Latched until command clear | System stays in FAULT until explicit cJSON clear (Phase 3) | |
| Auto-retry after timeout | Configurable timeout, auto-recover, latch after N retries | ✓ |
| Latched with GPIO button clear | Physical button or debug command to clear | |

**User's choice:** Auto-retry after timeout — autonomous recovery with safety backstop.

### Retry Parameters

| Option | Description | Selected |
|--------|-------------|----------|
| Conservative: 5s timeout, 3 retries | Long cooldown, few strikes | |
| Moderate: 3s timeout, 5 retries | Balanced recovery vs persistence | ✓ |
| Aggressive: 1s timeout, unlimited retries | Fast recovery, no permanent latch | |

**User's choice:** Moderate — 3 second cooldown, 5 retries max.

### Diagnostic Data on Fault

| Option | Description | Selected |
|--------|-------------|----------|
| Full snapshot: all channels + state | Read all 4 MOS channels, summary, mode, DAC, fault type, retry count | ✓ |
| Minimal: trigger source + retry count | Fast ISR, just channel ID and count | |
| Deferred read: flag in ISR, main loop reads | Keep ISR fast, get full data later | |

**User's choice:** Full snapshot — maximum diagnostic detail at fault time.

### Fault Type Tracking

| Option | Description | Selected |
|--------|-------------|----------|
| Separate fault types, auto-reset counter | Distinct OC vs OPP types, counter resets after 30s fault-free | ✓ |
| Unified fault, counter never resets | Single fault state, counter persists until manual clear | |
| Unified fault, counter resets on mode change | Counter resets when user changes mode/setpoint | |

**User's choice:** Separate fault types with auto-reset counter after 30 seconds fault-free.

---

## Soft-start Ramp Strategy

### Ramp Duration

| Option | Description | Selected |
|--------|-------------|----------|
| Fixed duration: 500ms linear ramp | Always 500ms from 0 to target | ✓ |
| Proportional to setpoint magnitude | Duration scales with step size | |
| Fixed 1-second ramp, always | Conservative 1s ramp | |

**User's choice:** Fixed 500ms linear ramp — simple and predictable.

### Ramp Application Scope

| Option | Description | Selected |
|--------|-------------|----------|
| Engage from idle only | Only IDLE→CV or IDLE→CC | ✓ |
| Engage + mode switches | Also CV↔CC transitions | |
| Every state change including setpoint | All changes trigger ramp | |

**User's choice:** Soft-start only on IDLE→active engagement. Mode switches and setpoint changes use PID tracking.

---

## Control Loop Timing & Structure

### Timing Mechanism

| Option | Description | Selected |
|--------|-------------|----------|
| Super-loop with SysTick timestamp | Check millisecond counter in while(1) | ✓ |
| TIM interrupt-driven | Hardware timer sets periodic flag | |
| Free-running with I2C reads inline | No explicit timing, run as fast as I2C allows | |

**User's choice:** SysTick timestamp — simple, uses existing infrastructure, no extra timer needed.

### PID Architecture

| Option | Description | Selected |
|--------|-------------|----------|
| Two independent PID instances | Separate CV and CC PIDs with own Kp/Ki/Kd | ✓ |
| Single PID with mode-switched coefficients | One PID instance, reload gains on mode change | |
| Single unified PID with shared coefficients | Same gains for both modes | |

**User's choice:** Two independent PID instances — clean separation, independently tuned.

### Anti-windup Strategy

| Option | Description | Selected |
|--------|-------------|----------|
| Conditional integration (clamping) | Freeze integrator at output saturation | ✓ |
| Back-calculation (tracking) | Feed back saturation difference to integrator | |
| Output clamping only, no anti-windup | Just clamp final value, let integrator wind up | |

**User's choice:** Conditional integration — embedded standard, zero overhead.

### I2C Read Scheduling

| Option | Description | Selected |
|--------|-------------|----------|
| Read all 5 every cycle | Summary + 4 MOS channels every 100ms | ✓ |
| Summary every cycle, MOS staggered | 2 MOS per cycle, each read every 200ms | |
| Summary every cycle, MOS on-demand | Only read MOS after ALARM | |

**User's choice:** All 5 every cycle — always-fresh data, 30% bus utilization is acceptable.

---

## EXTI Pin Assignment & Fault Isolation

### EXTI Architecture

| Option | Description | Selected |
|--------|-------------|----------|
| 4 independent EXTI lines | Each ALARM to its own GPIO+EXTI, instant channel ID | |
| OR'd to single EXTI line | All 4 ALARMs wired together to one pin | ✓ |
| 2+2 split: paired EXTI lines | MOS1+2 on one line, MOS3+4 on another | |

**User's choice:** Single EXTI line — saves 3 MCU pins. ISR reads INA226 alert registers over I2C to identify channel.

### GPIO Pin Selection

| Option | Description | Selected |
|--------|-------------|----------|
| PA4 on EXTI4 | Uncommitted, near I2C pins | ✓ |
| PB0 on EXTI0 | Uncommitted, far from I2C | |
| Delegate to researcher | Analyze pin mux for best choice | |

**User's choice:** PA4 on EXTI4 — convenient physical placement near I2C bank.

### Total Power OPP Integration

| Option | Description | Selected |
|--------|-------------|----------|
| Software check in main loop, same fault path | OPP checked every 100ms, calls unified fault handler | ✓ |
| Software OPP uses separate lower-severity path | Warning/reduction instead of full shutdown | |
| Connect summary INA226 ALERT to same OR'd line | 5-way OR, fully hardware-driven | |

**User's choice:** Software OPP in main loop, same unified fault path as hardware overcurrent.

---

## Claude's Discretion

- PID coefficient selection (Kp, Ki, Kd) — researcher investigates methodology
- INA226 alert register configuration (limit + mask/enable for OC threshold)
- Rated wattage #define for OPP check
- Calibration re-validation frequency and threshold
- Fault register bit layout and struct design
- ISR I2C read strategy for alert identification

## Deferred Ideas

None — discussion stayed within phase scope.
