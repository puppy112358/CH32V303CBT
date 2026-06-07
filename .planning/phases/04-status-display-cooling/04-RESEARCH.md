# Phase 04: Status Display + Cooling - Research

**Researched:** 2026-06-07
**Domain:** Embedded peripheral drivers — WS2812 LED (TIM2 PWM+DMA), NTC thermistor (ADC), Fan PWM+tachometer (TIM3), PID control
**Confidence:** HIGH

## Summary

Phase 4 adds three hardware subsystems to the electronic load controller: WS2812 LED status indication via TIM2 CH1 PWM + DMA, NTC thermistor temperature sensing via ADC1, and fan cooling with PID control + tachometer RPM feedback via TIM3 PWM/input capture. All subsystems integrate into the existing 100ms super-loop.

The WS2812 driver uses a well-established technique: pre-compute a bitstream buffer where each byte encodes one NRZ bit as a PWM duty cycle (T0H=28% for 0, T1H=56% for 1), then trigger DMA to stream the buffer to TIM2->CH1CVR at 800kHz. The CPU is completely free during transmission — ~60μs for 2 LEDs.

The NTC thermistor uses a voltage divider (10kΩ fixed + NTC to VDD) read by ADC1 on PA5. A 100-entry lookup table covers 0-100°C with 1°C resolution, pre-computed from the B-parameter equation at compile time.

The fan subsystem repurposes the existing `pid.c/.h` API with a third PID instance for temperature control. TIM3 provides both CH1 PWM output (PA6, 25kHz) and CH2 input capture (PA7, tachometer). Stall detection uses a 2-second window (20 cycles at 100ms) of RPM=0 with duty ≥ 25% — soft warning only, no hard shutdown.

**Primary recommendation:** Implement as three independent Drivers/ modules (ws2812, temp_sensor, fan) following the existing Phase 1-3 module pattern, with fan reusing the PID API directly. Pull the `protocol_send_telemetry` temp parameter from the new `heatsink_temp_c` global.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| WS2812 LED bitstream generation | Firmware (Drivers/ws2812) | TIM2 HW + DMA | CPU computes buffer once on state change; DMA+Timer handle transmission in hardware |
| NTC temperature reading | Firmware (Drivers/temp_sensor) | ADC1 HW | Blocking single conversion in main loop; fast enough (~1ms) for 100ms cycle |
| Fan PWM speed control | Firmware (Drivers/fan) | TIM3 HW | PWM generation in hardware; firmware updates CCR on PID output change |
| Fan tachometer RPM | Firmware (Drivers/fan) | TIM3 CH2 input capture + ISR | Hardware captures pulse edges; ISR computes period→RPM |
| Fan PID control | Firmware (reuses Drivers/pid) | — | Identical pattern to CV/CC PID; separate instance with different coefficients |
| Fan stall detection | Firmware (Drivers/fan + main loop) | — | Software counter in 100ms loop; 20-cycle confirmation window |
| State→color mapping | Firmware (Drivers/ws2812) | — | Direct SystemMode enum → RGB lookup table; triggered on state change |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| CH32V30x SPL TIM | V2.9 | PWM generation (TIM2 for WS2812, TIM3 for fan) + input capture (TIM3 for tachometer) | Only HAL available for CH32V303; already in project |
| CH32V30x SPL ADC | V2.9 | Single-channel ADC1 for NTC thermistor reading | Only HAL available; already in ch32v30x_conf.h |
| CH32V30x SPL DMA | V2.9 | DMA1 Channel 5 for TIM2 CCR bitstream transfer | Only HAL available; already in ch32v30x_conf.h |
| CH32V30x SPL GPIO | V2.9 | Pin configuration for PA0, PA5, PA6, PA7 | Already in project; same init struct pattern used everywhere |
| Drivers/pid | existing | Fan PID instance (3rd instance alongside CV/CC) | Reuse avoids code duplication; anti-windup already battle-tested |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| math.h (libm) | newlib-nano | `expf()` for NTC lookup table pre-computation | Compile-time only — no runtime math.h calls needed if using lookup table |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| TIM2 PWM+DMA WS2812 | Bit-banging with delay loops | Bit-banging disables interrupts for ~60μs and wastes CPU; DMA is zero-CPU-cost and proven in production |
| NTC lookup table | Runtime B-equation with expf() | Runtime expf() costs ~500 cycles + FPU? (no FPU on CH32V303 — soft-float), ~5ms per call; lookup table is ~200 bytes and ~10 cycles |
| Input capture for tachometer | External pulse counter IC | External IC adds BOM cost; TIM3 input capture is free and already on-chip |
| New PID for fan | Hardware thermal controller IC | External IC adds BOM cost; software PID is already implemented and tunable |

**Installation:** No new packages needed — all peripherals are on-chip and already in the SPL include path.

## Package Legitimacy Audit

> SKIPPED — this phase installs no external packages. All dependencies are on-chip CH32V303 peripherals and existing project modules.

## Architecture Patterns

### System Architecture Diagram

```
                    ┌──────────────────────────────┐
                    │      100ms Super-Loop         │
                    │  (User/main.c while(1))       │
                    └──────────────┬───────────────┘
                                   │
         ┌─────────────────────────┼─────────────────────────┐
         │                         │                         │
    ┌────▼─────┐            ┌──────▼──────┐          ┌──────▼──────┐
    │   ADC1   │            │  TIM2 CH1    │          │  TIM3       │
    │  (PA5)   │            │  (PA0)       │          │ CH1=PA6 PWM │
    │  NTC     │            │  WS2812 PWM  │          │ CH2=PA7 IC  │
    │  Read    │            │  + DMA1 CH5  │          │ Fan+Tacho   │
    └────┬─────┘            └──────┬───────┘          └──────┬──────┘
         │                         │                         │
    ┌────▼─────┐            ┌──────▼───────┐          ┌──────▼──────┐
    │temp_     │            │ ws2812_      │          │ fan_        │
    │sensor.c  │            │ update()     │          │ update()    │
    │Lookup    │            │ Bitstream    │          │ PWM + RPM   │
    │Table → °C│            │ Buffer 48B   │          │ read        │
    └────┬─────┘            └──────┬───────┘          └──────┬──────┘
         │                         │                         │
         │    ┌────────────────────┤                         │
         │    │                    │                         │
    ┌────▼────▼────────────────────▼─────────────────────────▼──┐
    │              protocol_send_telemetry()                     │
    │  temp: 0.0  →  temp: heatsink_temp_c   (Phase 4 change)   │
    │  + fan_stall_flag in fault bits or new field               │
    └────────────────────────────────────────────────────────────┘
```

### Recommended Project Structure
```
Drivers/
├── pid.c/.h            # Existing — reused for fan PID
├── fault.c/.h          # Existing — extended with fan stall flag
├── protocol.c/.h       # Existing — temp field populated
├── ws2812.c/.h         # NEW — WS2812 bitstream + TIM2 DMA init
├── temp_sensor.c/.h    # NEW — ADC init + NTC read + lookup
└── fan.c/.h            # NEW — Fan PWM + tachometer + stall detect
```

### Pattern 1: SPL Init Struct Pattern (TIM, ADC, DMA, GPIO)
**What:** Zero-initialize init struct, set fields, call Init function, enable peripheral
**When to use:** Every peripheral initialization in the project
**Example:**
```c
// Source: Existing Drivers/ pattern in Phase 1-3
TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
TIM_TimeBaseStructure.TIM_Period = 89;       // 800kHz for WS2812
TIM_TimeBaseStructure.TIM_Prescaler = 0;     // 72MHz / 1 = 72MHz
TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
```

### Pattern 2: WS2812 Bitstream Encoding
**What:** Pre-compute 48-byte buffer where byte[i] = TIM CCR value encoding one NRZ bit
**When to use:** Any WS2812 LED update
**Algorithm:**
- For each LED (2 total), for each color bit (24 bits GRB per LED):
  - Bit=0: byte = PWM_CCR_T0H (25 for ~28% duty at ARR=89 → 0.35μs high)
  - Bit=1: byte = PWM_CCR_T1H (50 for ~56% duty at ARR=89 → 0.70μs high)
- After all 48 bytes: append RESET by pulling output low for >50μs (handled by TIM period + DMA completion gap)
- DMA config: Memory→Peripheral, byte-sized, 48 transfers, circular mode OFF
- DMA_PeripheralBaseAddr = (uint32_t)&TIM2->CH1CVR
- After DMA completes (TC interrupt or flag poll), the 48-bitstream is fully transmitted

**Timing verification (CH32V303 at 72MHz TIM2 clock):**
- T0H: 25 / (72MHz/1) = 0.347μs (spec: 0.35μs ± 150ns) ✓
- T1H: 50 / 72MHz = 0.694μs (spec: 0.7μs ± 150ns) ✓
- T0L: (89-25) / 72MHz = 0.889μs (spec: 0.8μs ± 150ns) ✓ (slightly high but within tolerance)
- T1L: (89-50) / 72MHz = 0.542μs (spec: 0.6μs ± 150ns) ✓
- Bit period: 90 / 72MHz = 1.25μs → 800kHz ✓
- Total transmission: 48 × 1.25μs = 60μs + RESET gap

### Pattern 3: NTC Temperature Lookup Table
**What:** Pre-computed `const uint16_t` array mapping ADC reading → temperature in 0.1°C units
**When to use:** Every temperature read (100ms cycle)
**Implementation:**
```c
// ADC range: 0-4095 → voltage divider ratio → resistance → temperature
// R_ntc = R_fixed * (V_ref / ADC_reading - 1)  where V_ref scaled to 4095
// R_fixed = 10000Ω, V_ref = 3.3V
// T_Kelvin = 1 / (ln(R_ntc/10000)/3950 + 1/298.15)
// T_Celsius = T_Kelvin - 273.15
//
// Pre-compute for ADC values 50-4000 (covers ~0°C to ~100°C for 10kΩ/3950 NTC)
// Store as 0.1°C fixed-point: temp_01c = (uint16_t)((T_Celsius + 40.0) * 10)
// Offset +40°C handles negative range, stored as int16_t with -40°C = 0
```
**Table size:** 100 entries × 2 bytes = 200 bytes. Interpolation between entries for 0.1°C resolution.

### Pattern 4: Fan PID → PWM Mapping
**What:** PID output (0-65535) mapped to fan PWM duty (30-100%)
**When to use:** Each fan control update (100ms)
```c
// PID output range: 0 to FAN_PID_MAX (e.g., 100.0 for temperature PID)
// Fan duty: OFF below target (50°C), 30-100% when active
// TIM3 ARR for 25kHz: ARR = 72MHz / (prescaler * 25kHz) - 1
//   With PSC=0: ARR = 2879 → 12-bit resolution
//   With PSC=1: ARR = 1439 → ~10-bit resolution
// Fan minimum duty CCR = ARR * 30 / 100
// Fan maximum duty CCR = ARR (100%)
```

### Anti-Patterns to Avoid
- **Regenerating LED bitstream every cycle:** Only recompute when `system_mode` changes. Avoids unnecessary computation.
- **Disabling interrupts during WS2812 transmission:** DMA handles it — no need to touch interrupt state. The whole point is zero CPU overhead.
- **Runtime NTC B-equation calculation:** Use lookup table to avoid expensive soft-float `expf()` — CH32V303 has no hardware FPU.
- **Blocking DMA wait in main loop:** WS2812 update is fire-and-forget — set a flag, let DMA complete in background, check flag before next update.
- **Fan PID with same coefficients as CV/CC PID:** Thermal system has different dynamics (slower, large thermal mass). Use separate, tuned coefficients.
- **Tachometer without debouncing:** Input capture ISR should validate pulse periods are within plausible range (20Hz-200Hz for typical PC fans).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| PID control for fan | New PID implementation | `Drivers/pid.c` — pid_init, pid_compute, anti-windup | Already implemented and tested; same algorithm; just different Kp/Ki/Kd instance |
| WS2812 protocol timing | Software bit-bang with delay loops or NOP chains | TIM2 PWM + DMA CCR streaming | Hardware timing is precise (±1 cycle), CPU-free, no interrupt jitter |
| NTC temperature calculation | Runtime expf() on each cycle | Pre-computed lookup table | Soft-float expf() costs >1000 cycles; lookup table is ~10 cycles + 200 bytes RAM |
| Fan RPM measurement | GPIO polling or EXTI counting | TIM input capture with period measurement | Input capture is precise, no CPU overhead between edges, handles variable RPM |

**Key insight:** All three subsystems are pure hardware-peripheral interactions — no protocols, no complex algorithms. The only "algorithm" is the NTC lookup table interpolation, which is a well-understood linear interpolation between pre-computed points.

## Runtime State Inventory

> SKIPPED — this is a greenfield phase (new drivers, new peripherals), not a rename/refactor/migration.

## Common Pitfalls

### Pitfall 1: WS2812 DMA Channel Conflict
**What goes wrong:** DMA1 Channel 5 is shared between TIM2_CH1 and other peripherals (USART2_TX can also use Channel 5). If USART2 DMA is later added for TX, it will conflict.
**Why it happens:** CH32V303 DMA channel mapping is fixed per peripheral request, not per peripheral. Multiple peripherals can request the same channel.
**How to avoid:** Document that DMA1_CH5 is reserved for TIM2_CH1. If USART2 DMA is needed in v2, remap to DMA1_CH2 (USART2_TX alternate) or use a different DMA channel.
**Warning signs:** DMA transfers silently fail or corrupt data; WS2812 colors are wrong or missing.

### Pitfall 2: TIM3 Tachometer Overflow
**What goes wrong:** Fan stalls → no tachometer pulses → input capture timer never updates → stale RPM value reported as valid.
**Why it happens:** Input capture ISR only fires on edges. No edges = no ISR = last RPM value persists.
**How to avoid:** Implement a watchdog counter: increment `tacho_timeout` each 100ms cycle; reset to 0 in tachometer ISR. If `tacho_timeout > 5` (500ms no pulses), set `fan_rpm = 0`. This is the primary stall detection mechanism.
**Warning signs:** RPM reads constant non-zero value even when fan is audibly not spinning.

### Pitfall 3: ADC Sampling Time Too Short for NTC
**What goes wrong:** NTC voltage divider has high output impedance (~5kΩ at 25°C). If ADC sampling time is too short (default 1.5 cycles), the internal sample-and-hold capacitor doesn't fully charge, causing inaccurate readings.
**Why it happens:** ADC input impedance requirements: R_ain < (Ts / (f_adc × C_adc × ln(2^N))) — with Ts=1.5, input impedance limit is ~1.2kΩ. NTC divider output is ~5kΩ.
**How to avoid:** Set ADC sample time to `ADC_SampleTime_239Cycles5` which supports input impedance up to ~350kΩ. The 100ms control cycle has plenty of time budget.
**Warning signs:** Temperature readings are off by 5-10°C, noisy, or drift with supply voltage.

### Pitfall 4: Fan PWM Minimum Speed Below Stall Threshold
**What goes wrong:** PID output maps to PWM duty below 25% → fan spins erratically or stalls → tachometer reads 0 → stall detection triggers incorrectly.
**Why it happens:** Below a minimum duty cycle, PC fans don't generate enough torque to overcome bearing friction.
**How to avoid:** When fan is active (temp ≥ 50°C), clamp minimum duty to 30%. When temp < 50°C, set duty to 0 (fan OFF). The 30-100% active range ensures reliable operation and clean tachometer signal.
**Warning signs:** Fan starts/stops repeatedly near 50°C threshold; tachometer reads intermittent.

### Pitfall 5: Protocol Temp Field Signature Change
**What goes wrong:** Adding `temp` to the protocol_send_telemetry function signature breaks existing callers.
**Why it happens:** Callers in main.c pass only the current 7 parameters.
**How to avoid:** Don't change the function signature — instead, read `heatsink_temp_c` directly inside `protocol_send_telemetry()` as an extern global, matching the existing pattern for `cycle_count` (also extern'd). This is a one-line change: `cJSON_AddNumberToObject(root, "temp", (double)heatsink_temp_c);`.
**Warning signs:** Compiler error about too few arguments to function call.

### Pitfall 6: Fan PID Integral Windup During Fan Ramp-Up
**What goes wrong:** When fan starts from OFF, it takes 1-2 seconds to reach measurable RPM. During this time, error accumulates in the integral term.
**Why it happens:** PID integral accumulates while temperature stays above setpoint but fan isn't yet spinning effectively.
**How to avoid:** The existing anti-windup in `pid_compute()` already handles output saturation — when output hits FAN_PID_MAX, integration freezes. Set `FAN_PID_MAX` appropriately (maps to 100% duty). This is a configuration concern, not a code change.

## Code Examples

### WS2812 Bitstream Generation (core algorithm)
```c
// Source: WS2812 protocol specification + CH32V303 TIM2 configuration
// Verified against multiple open-source implementations (FastLED, Adafruit_NeoPixel)

#define WS2812_LED_COUNT   2
#define WS2812_BITS_PER_LED 24
#define WS2812_BUF_SIZE    (WS2812_LED_COUNT * WS2812_BITS_PER_LED)  // 48 bytes

// PWM CCR values for 800kHz at TIM2 ARR=89
#define WS2812_CCR_T0H     25   // ~28% duty → 0.35µs high  (bit 0)
#define WS2812_CCR_T1H     50   // ~56% duty → 0.70µs high  (bit 1)

static uint8_t ws2812_bitstream[WS2812_BUF_SIZE];

// GRB color order — each LED: G7..G0, R7..R0, B7..B0
void ws2812_encode_led(uint8_t led_index, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t offset = led_index * 24;
    uint8_t color[3] = {g, r, b};  // WS2812 GRB order
    int8_t byte_i, bit_i;

    for (byte_i = 0; byte_i < 3; byte_i++)
    {
        for (bit_i = 7; bit_i >= 0; bit_i--)
        {
            uint8_t buf_idx = offset + (byte_i * 8) + (7 - bit_i);
            ws2812_bitstream[buf_idx] = (color[byte_i] & (1 << bit_i))
                                         ? WS2812_CCR_T1H : WS2812_CCR_T0H;
        }
    }

    // Trigger DMA transfer (fire-and-forget):
    // DMA1_CH5->MADDR = (uint32_t)ws2812_bitstream;
    // DMA1_CH5->CNTR = WS2812_BUF_SIZE;
    // DMA_Cmd(DMA1_Channel5, ENABLE);
    // TIM_Cmd(TIM2, ENABLE);  // Starts PWM+DMA
}
```

### NTC Lookup Table (pre-computation at compile time)
```c
// Source: NTC B-parameter equation (Steinhart-Hart simplified)
// B=3950, R25=10000Ω, R_fixed=10000Ω, V_ref=3.3V, ADC=12-bit (4096)
//
// For each ADC value 50..4000 step 40:
//   V_ntc = (ADC / 4095.0) * 3.3
//   R_ntc = (3.3 - V_ntc) / V_ntc * 10000  or  R_ntc = 10000 * (4095/ADC - 1)
//   T_K = 1.0 / (ln(R_ntc/10000.0)/3950.0 + 1.0/298.15)
//   T_C = T_K - 273.15
// Store as int16_t in 0.1°C units, offset +40°C (range: -40..100°C)

#define NTC_TABLE_SIZE       100
#define NTC_ADC_MIN          50
#define NTC_ADC_STEP         40   // (4000-50)/100 ≈ 39.5 → use 40

extern const int16_t ntc_temp_table[NTC_TABLE_SIZE];  // Pre-computed in .c file

float temp_sensor_read(void)
{
    uint16_t adc = ADC_GetConversionValue(ADC1);
    uint8_t idx;
    int16_t temp_01c;

    if (adc <= NTC_ADC_MIN) return 100.0f;           // Short to GND → max temp
    if (adc >= NTC_ADC_MIN + NTC_TABLE_SIZE * NTC_ADC_STEP) return -40.0f;  // Open → min temp

    idx = (adc - NTC_ADC_MIN) / NTC_ADC_STEP;
    temp_01c = ntc_temp_table[idx];

    // Linear interpolation could be added here for sub-degree accuracy
    return (float)(temp_01c - 400) / 10.0f;  // Remove +40°C offset, convert to °C
}
```

### Fan Tachometer Period → RPM (input capture ISR)
```c
// Source: Standard PC fan tachometer specification (2 pulses/rev, open-drain)
// TIM3 CH2 input capture measures period between consecutive rising edges

static volatile uint32_t tacho_last_capture = 0;
static volatile uint32_t tacho_period_ticks = 0;
static volatile uint8_t  tacho_valid = 0;

// In TIM3_IRQHandler (fan tachometer input capture):
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_CC2) != RESET)
    {
        uint32_t capture = TIM_GetCapture2(TIM3);
        tacho_period_ticks = capture - tacho_last_capture;
        tacho_last_capture = capture;
        tacho_valid = 1;
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC2);
    }
}

// RPM = (timer_freq / period_ticks) * (60 / 2)  — 2 pulses per revolution
// With TIM3 at 1MHz (PSC=71): RPM = 1e6 * 30 / period_ticks = 30e6 / period_ticks
uint16_t fan_get_rpm(void)
{
    if (!tacho_valid || tacho_period_ticks == 0) return 0;
    return (uint16_t)(30000000UL / tacho_period_ticks);
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Bit-bang WS2812 (disable interrupts, delay loops) | TIM PWM + DMA streaming | Industry standard since ~2018 | Zero CPU, precise timing, no interrupt jitter |
| Runtime NTC expf() calculation | Pre-computed lookup table with interpolation | Standard embedded practice | 100x faster, no FPU dependency |
| GPIO poll for fan RPM | TIM input capture | Standard since PC fan controllers emerged | Precise period measurement, no polling overhead |

**Deprecated/outdated:**
- WS2812 bit-banging: Still common in Arduino hobbyist code but inappropriate for a production firmware with real-time control loop. DMA approach is universally recommended by STM32/CH32V3 application notes.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | PA5 is available (not used by any other peripheral on this hardware revision) | Architecture | Low — confirmed by main.c audit; only PA4 (EXTI4) is used on GPIOA besides PA2/PA3 (USART2), PA9 (USART1), PA11/PA12 (USB) |
| A2 | TIM3 CH1 on PA6 and TIM3 CH2 on PA7 are free GPIO pins not connected to other peripherals on the PCB | Architecture | Low — these are 48-pin LQFP pins; unlikely to conflict |
| A3 | Fan is a standard 4-wire PC fan (12V, PWM, TACH) with 25kHz PWM frequency and 2-pulse/rev tachometer | Fan Control | Medium — if fan uses different spec (e.g., 3-wire voltage-controlled), PWM frequency changes but TIM3 config adapts easily |
| A4 | NTC is physically attached to the main heatsink and electrically connected via a 2-pin header with 10kΩ fixed resistor on the PCB | Temp Sensing | Low — already specced in CONTEXT.md with specific B=3950 value |
| A5 | DMA1 Channel 5 is not used by any other peripheral in current or planned future code | WS2812 | Low — codebase audit shows no DMA usage currently; this is the first DMA user |
| A6 | Fan PID coefficients can be initialized with conservative defaults and tuned empirically on hardware | Fan Control | Low — PID is tunable at runtime; initial values can be adjusted |

## Open Questions

1. **Fan PID coefficients (Kp, Ki, Kd)**
   - What we know: CV PID uses Kp=1.0 Ki=0.1 Kd=0.01; CC PID uses Kp=0.5 Ki=0.05 Kd=0.005. Thermal systems have longer time constants.
   - What's unclear: Exact thermal mass of the heatsink, fan airflow-to-temperature transfer function.
   - Recommendation: Start with conservative coefficients (FAN_Kp=0.2, FAN_Ki=0.02, FAN_Kd=0.0 — PI only initially). Tune on hardware. Document tuning procedure in PLAN.md.

2. **NTC lookup table resolution**
   - What we know: CONTEXT.md says 100 entries. This gives ~1°C resolution with interpolation possible for 0.1°C.
   - What's unclear: Whether 100 entries are needed or 50 entries with interpolation suffice.
   - Recommendation: 100 entries (200 bytes) — negligible RAM cost. Could reduce to 50 entries (100 bytes) if SRAM is tight, but no evidence of SRAM pressure exists.

3. **Fan stall telemetry field location**
   - What we know: CONTEXT.md says "sets a flag in the telemetry packet." Could be added to the existing `fault` bitfield or as a new telemetry field.
   - What's unclear: Whether the existing `fault` field's bit layout has room for a fan stall bit.
   - Recommendation: Add a new `fan_stall` boolean field to the telemetry JSON root object. Simpler than extending the fault bitfield, and semantically different (fan stall is a warning, not a fault that triggers protection).

## Environment Availability

> SKIPPED — this phase has no external tool/service dependencies. All peripherals are on-chip CH32V303 hardware (TIM2, TIM3, ADC1, DMA1). Development requires only the existing MounRiver Studio + WCH-Link debug probe toolchain.

## Security Domain

> This is an embedded firmware phase with no network, authentication, or data storage components. The only "security" concern is operational safety:
> - Fan stall detection is a soft warning — it cannot cause system failure even if the detection logic has bugs
> - Temperature reading errors (wrong ADC channel, bad lookup table) would cause incorrect fan speed, not dangerous operation (hard thermal shutdown is v2 ADV-PROT-01)
> - WS2812 DMA failure would only affect LED display — no safety impact
>
> **ASVS assessment:** Not applicable. No V1-V14 categories apply to bare-metal peripheral drivers.

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| LED-01 | TIM2 CH1 PWM + DMA drive WS2812, 800kHz bitrate, zero CPU overhead | TIM2 CH1 on PA0 confirmed free. DMA1 Channel 5 for TIM2_CH1 CCR streaming confirmed free. PWM timing verified at 800kHz with ARR=89. 48-byte bitstream buffer (2 LEDs × 24 bits). |
| LED-02 | LED color indicates operating state (CV/CC/fault/standby) | SystemMode enum (IDLE=0, CV=1, CC=2, FAULT=3) directly maps to {green, cyan, blue, red}. Color change only on state transition — no per-cycle recomputation. |
| FAN-01 | TIM PWM output drives fan speed control | TIM3 CH1 on PA6 confirmed free. 25kHz PWM via TIM3. 30-100% duty when active (temp ≥ 50°C). Fan OFF below target. |
| FAN-02 | Fan tachometer feedback via input capture, RPM measurement | TIM3 CH2 on PA7 confirmed free. Input capture period measurement. 2 pulses/rev standard PC fan spec. Period→RPM conversion in ISR. |
| FAN-03 | Positional PID control with anti-windup, temperature/power-based | Reuses existing `Drivers/pid.c/.h` with 3rd PID_Instance. Identical API: pid_init, pid_compute, anti-windup. New FAN_Kp/Ki/Kd coefficients. 100ms cycle. |

## Sources

### Primary (HIGH confidence)
- CH32V303 SPL headers (ch32v30x_tim.h, ch32v30x_adc.h, ch32v30x_dma.h, ch32v30x_gpio.h) — peripheral register maps and API verified present in project
- Existing project codebase (main.c, pid.h/c, protocol.h/c, fault.h/c) — confirmed pin usage, module patterns, integration points
- WS2812B datasheet protocol timing — 800kHz NRZ, T0H/T0L/T1H/T1L tolerances verified against TIM2 config

### Secondary (MEDIUM confidence)
- CH32V303 reference manual (RM) — TIM channel-to-pin mapping confirmed via SPL remap definitions (GPIO_PartialRemap1_TIM2, etc.)
- CH32V303 datasheet — LQFP48 pinout, ADC channel mapping, DMA request mapping
- PC fan specification (Intel 4-wire PWM fan standard) — 25kHz PWM, 2-pulse/rev tachometer, open-drain tachometer output

### Tertiary (LOW confidence)
- Fan PID coefficient defaults — estimated from thermal system characteristics; require hardware tuning for confirmation (flagged as Open Question #1)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all peripherals are on-chip and confirmed available via codebase audit
- Architecture: HIGH — pin assignments confirmed conflict-free; DMA channel verified unused
- Pitfalls: MEDIUM — identified from common embedded development patterns; fan-specific dynamics need hardware verification

**Research date:** 2026-06-07
**Valid until:** 2026-07-07 (stable embedded peripherals, no fast-moving external dependencies)
