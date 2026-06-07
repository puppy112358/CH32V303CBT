# Phase 4: Status Display + Cooling - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-07
**Phase:** 4-Status Display + Cooling
**Areas discussed:** Temperature Sensor Hardware, LED Strip Visual Behavior, Fan Control Strategy, Fan Stall Handling

---

## Temperature Sensor Hardware

| Option | Description | Selected |
|--------|-------------|----------|
| NTC thermistor + ADC (Recommended) | Standard for embedded power electronics. Voltage divider into ADC channel. | ✓ |
| Digital temp sensor on I2C1 | LM75/TMP102 etc. on existing I2C1 bus — no ADC needed but adds 7th device to loaded bus. | |
| MCU internal temp sensor | CH32V303 ADC1_CH16 — measures die temperature, not heatsink. | |

**User's choice:** NTC thermistor + ADC
**Notes:** Standard B=3950 10kΩ at 25°C, voltage divider with 10kΩ fixed resistor to VDD. Single sensor on main heatsink only — no per-channel or ambient. Any free ADC channel — researcher picks best available pin. Sampled every 100ms control cycle.

---

## LED Strip Visual Behavior

### Number of LEDs

| Option | Description | Selected |
|--------|-------------|----------|
| 8 LEDs (Recommended) | Standard indicator strip, 192-byte buffer. | |
| 4 LEDs | Minimal — one per MOS channel, 96-byte buffer. | |
| 16 LEDs | Larger strip, 384-byte buffer, higher current. | |
| Other: 2 LEDs | User specified 2 LEDs, both same color. | ✓ |

**User's choice:** 2 WS2812 LEDs, both always the same color
**Notes:** Only 48 bytes of bitstream buffer needed. Both LEDs driven from the same bitstream data.

### Color Effects

| Option | Description | Selected |
|--------|-------------|----------|
| Static solid colors (Recommended) | Each state = one solid color, instant change on transition. | ✓ |
| Solid + slow blink on fault | Fault = red blinking (500ms on/off). Needs blink timer. | |
| Solid + fault blink + breathing idle | More visual polish but needs animation timers. | |

**User's choice:** Static solid colors only — no blinking, breathing, or animation

### Brightness

| Option | Description | Selected |
|--------|-------------|----------|
| Full brightness (255/255) | Maximum visibility, ~120mA for 2 LEDs. | |
| 50% brightness (Recommended) | Good visibility without glare, ~60mA total. Halve RGB values. | ✓ |
| Configurable via #define | Compile-time knob for tuning. | |

**User's choice:** 50% brightness
**Notes:** State-to-color mapping: standby=green, CV=cyan, CC=blue, fault=red. IDLE = standby = green.

---

## Fan Control Strategy

### Control Target Type

| Option | Description | Selected |
|--------|-------------|----------|
| Fixed temperature setpoint PID (Recommended) | PID targets single temperature threshold. Simple, predictable. | ✓ |
| Temperature + power feed-forward | PID(temperature) + feed_forward(power × coeff). Preemptive at high power. | |
| Power-proportional only (no PID) | Duty cycle = linear mapping from power. Simplest but may over/under-cool. | |

**User's choice:** Fixed temperature setpoint — 50°C

### Minimum Fan Speed / Off Behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Off below target, minimum 30% when active (Recommended) | Fan OFF when temp < 50°C. Active = 30-100% duty. | ✓ |
| Always minimum 20% duty | Fan always spins at >=20% in any active mode. | |
| Off below target, 0-100% full PID range | No minimum clamp — fan may stall at low PID output. | |

**User's choice:** Fan OFF below 50°C, 30-100% duty when active
**Notes:** 30% minimum ensures fan starts spinning above stall threshold. PID recomputed every 100ms.

---

## Fan Stall Handling

### Severity

| Option | Description | Selected |
|--------|-------------|----------|
| Soft warning — telemetry flag only (Recommended) | Set stall flag in telemetry + printf. Load continues operating. | ✓ |
| Hard fault — shut down like overcurrent | Stall → FAULT state, zero DAC, auto-retry. | |
| Two-tier: warning below 60°C, fault above 60°C | Context-aware — only fault if temperature is high. | |

**User's choice:** Soft warning only — telemetry flag + printf, no load shutdown
**Notes:** Hard thermal shutdown (>80°C) deferred to v2. Fan stall alone doesn't mean instant danger.

### Detection Window

| Option | Description | Selected |
|--------|-------------|----------|
| 2 seconds (Recommended) | 20 control cycles. Rejects glitches, catches real stalls fast. | ✓ |
| 500ms | 5 cycles. Fast but may false-trigger on tachometer glitches. | |
| 5 seconds | 50 cycles. Conservative but 5s without cooling before flag. | |

**User's choice:** 2-second continuous RPM=0 detection window

### Minimum Duty Threshold

| Option | Description | Selected |
|--------|-------------|----------|
| 25% duty (Recommended) | If PWM ≥ 25% AND RPM = 0 for 2s → stall. Slightly below 30% active min. | ✓ |
| 30% duty | Matches active minimum exactly. May miss edge cases. | |
| 10% duty | Very low — fans won't spin at 10%, false stall flags. | |

**User's choice:** 25% duty threshold for stall detection

---

## Claude's Discretion

- Exact ADC channel for NTC (researcher identifies free pin)
- TIM channel assignments for fan PWM output and tachometer input capture
- NTC linearization lookup table size and resolution
- Fan PID coefficient defaults (Kp, Ki, Kd)
- WS2812 bitstream generation algorithm (TIM2 CCR DMA timing pattern)
- DMA channel assignment for TIM2 CCR update
- Tachometer input capture prescaler and period-to-RPM formula
- `fan.h`, `temp_sensor.h`, `ws2812.h` API design
- Whether to combine fan PID and fan PWM into one module or split them

## Deferred Ideas

- Hard thermal shutdown (OTP at >80°C) — v2 ADV-PROT-01
- Runtime fan target temperature via cJSON command — v2
- Per-MOSFET temperature sensing — v2 ADV-PROT-02
- LED animation effects (blinking fault, breathing standby) — post-v1
