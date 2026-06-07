# Phase 02: Control Loop + Protection — Research

**Researched:** 2026-06-07
**Status:** Complete

## Table of Contents

1. [PID Control on CH32V303 (No FPU)](#1-pid-control-on-ch32v303)
2. [INA226 Alert Configuration](#2-ina226-alert-configuration)
3. [EXTI4 Interrupt Architecture](#3-exti4-interrupt-architecture)
4. [Soft-start DAC Ramp Strategy](#4-soft-start-dac-ramp-strategy)
5. [Fault State Machine Design](#5-fault-state-machine-design)
6. [I2C Timing Budget Analysis](#6-i2c-timing-budget-analysis)
7. [Calibration Re-validation (PROT-04)](#7-calibration-re-validation)
8. [Memory Budget Analysis](#8-memory-budget-analysis)
9. [Missing Driver APIs](#9-missing-driver-apis)
10. [Integration Plan](#10-integration-plan)

---

## 1. PID Control on CH32V303 (No FPU)

### Finding: Software float is acceptable at 100ms control period

The CH32V303CBT6 has the `RV32IMAC` ISA — **no `F` (single-precision float) or `D` (double-precision float) extensions**. All floating-point operations are emulated in software by the RISC-V GCC toolchain (`-march=rv32imac`).

**Performance analysis:**
- Software float multiply/add: ~50-100 cycles each on RV32IMAC
- One PID iteration: 6 multiplies + 5 adds + 1 divide ≈ ~800 cycles
- At 96 MHz: ~8.3 µs per PID iteration — negligible against 100ms period (0.0083% duty cycle)
- Two PID instances (CV + CC) per cycle: ~17 µs total

**Conclusion:** Software float is fast enough. No need for fixed-point math — the float API already used by INA226 getters (returns `float` values) integrates naturally.

### PID Algorithm Selection: Positional (parallel) form

```
error = setpoint - feedback
P_term = Kp * error
I_term += Ki * error * dt   // with anti-windup
D_term = Kd * (error - last_error) / dt
output = P_term + I_term + D_term
output = clamp(output, 0, 65535)   // DAC range
```

**Anti-windup (D-09):** Conditional integration — freeze `I_term` accumulation when `output` is saturated at 0 or 65535. Simple and effective for this slow control loop.

### PID Struct Design (per CONTEXT.md D-08)

```c
typedef struct {
    float Kp, Ki, Kd;       /* Tunable coefficients */
    float setpoint;          /* Target value (voltage or current) */
    float integral;          /* Accumulated error integral */
    float last_error;        /* Previous error (for derivative) */
    float output;            /* Last computed output (diagnostic) */
    uint8_t saturated;       /* Flags: 0=none, 1=hi_sat, 2=lo_sat */
} PID_Instance;
```

Two instances: `pid_cv` (feedback = summary bus voltage) and `pid_cc` (feedback = summary current). Coefficients stored as `#define` constants until Phase 3 adds runtime tuning via cJSON.

### Coefficient Storage

Per CONTEXT.md "Claude's Discretion" — PID coefficients are `#define` constants in a header:

```c
#define PID_CV_KP  1.0f
#define PID_CV_KI  0.1f
#define PID_CV_KD  0.01f
#define PID_CC_KP  0.5f
#define PID_CC_KI  0.05f
#define PID_CC_KD  0.005f
```

Tuning methodology: manual Ziegler-Nichols on hardware. Coefficients will be empirically determined; these are placeholders for compilation. Planning should note that coefficients are `#define`d and tunable without structural code changes.

### Derivative Kick Mitigation

Use "derivative on measurement" (not on error) to prevent setpoint-step derivative spikes:

```
D_term = -Kd * (feedback - last_feedback) / dt
```

This prevents the D term from producing a massive spike when the setpoint changes (e.g., IDLE→CV engage). Combined with soft-start, this ensures smooth output.

---

## 2. INA226 Alert Configuration

### Alert Limit Register (0x07) — Missing from current driver

The current `ina226.h` defines `INA226_REG_ALERT` (0x06) for the mask/enable register but does NOT define the alert limit register at address 0x07. Per the INA226 datasheet:

| Address | Register | Description |
|---------|----------|-------------|
| 0x06 | Mask/Enable | Which conditions trigger ALERT pin |
| 0x07 | Alert Limit | Threshold value for the selected alert function |

### Alert Configuration for Overcurrent (PROT-01)

1. **Write alert limit register (0x07):** Set threshold current value. Per REQUIREMENTS.md PROT-01: single MOS current limit = rated_current / 4 × 1.3.
   - If rated current = 5A: per-MOS limit = 5/4 × 1.3 = **1.625A**
   - Convert to INA226 current register value: 1.625 / INA226_CURRENT_LSB ≈ 10650 (0x299A)

2. **Write mask/enable register (0x06):** Configure for shunt-voltage-over-voltage (bit 15) with latch mode. Bits 1-0 set to `01` = Latch mode (alert stays asserted until mask register read).

### New Driver API Needed

```c
/* Write alert limit register (0x07). value = 16-bit threshold in INA226 current LSBs. */
i2c_status_t ina226_set_alert_limit(INA226_Dev *dev, uint16_t value);

/* Write mask/enable register (0x06) with latch and alert function selection. */
i2c_status_t ina226_set_alert_config(INA226_Dev *dev, uint16_t mask);
```

---

## 3. EXTI4 Interrupt Architecture

### PA4 EXTI4 Configuration

PA4 is on EXTI line 4. Configuration follows SPL pattern:

```c
// GPIO: PA4 input with pull-up (open-drain ALARM, active-low)
GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
GPIO_Init(GPIOA, &GPIO_InitStructure);

// EXTI4: falling edge interrupt
GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource4);
EXTI_InitStructure.EXTI_Line = EXTI_Line4;
EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
EXTI_InitStructure.EXTI_LineCmd = ENABLE;
EXTI_Init(&EXTI_InitStructure);

// NVIC
NVIC_SetPriority(EXTI4_IRQn, 0x01); // High priority (0x00=highest, 0x01=second)
NVIC_EnableIRQ(EXTI4_IRQn);
```

### ISR Design (PROT-02)

The EXTI4 ISR must:
1. Read 4 INA226 alert registers (0x06) to identify which channel triggered → ~4 I2C reads, ~3-4ms total
2. Read alarm register of triggered channel(s) to clear latch
3. Set volatile `fault_triggered` flag
4. Store fault type in fault register
5. Zero DAC output via `dac8571_set_output(0)` — one I2C write
6. Log diagnostic snapshot via printf (D-02)
7. Clear EXTI4 pending bit

**ISR context I2C operations:** The i2c_util functions block with timeout polling. They work in ISR context because interrupt nesting is enabled (CSR 0x804). However, during the ~4ms alert read window, lower-priority interrupts are blocked. This is acceptable because:
- The alert ISR is the highest-priority function (protection > everything)
- 4ms latency for lower-priority tasks (SysTick, UART TX) is tolerable
- The alternative (deferred I2C reads in main loop) creates a race condition — the ALARM pin de-asserts after the alert register is read, so reading in the ISR IS the right approach

### EXTI4_IRQHandler Implementation Pattern

```c
void EXTI4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")))
{
    /* Step 1: Identify triggered channels — read all 4 MOS alert registers */
    for (i = 0; i < 4; i++) {
        ina226_check_alert(&devs[i], &alert_mask);
        if (alert_mask & INA226_ALERT_SHUNT_OV) {
            fault_reg |= (1 << i);  /* Set per-channel fault bit */
        }
    }
    
    /* Step 2: Zero DAC immediately */
    dac8571_set_output(0);
    
    /* Step 3: Transition state to FAULT */
    system_mode = MODE_FAULT;
    fault_triggered = 1;
    
    /* Step 4: Log diagnostic snapshot (D-02) */
    printf("[FAULT] ch=%02x dac=%u retry=%u\r\n", fault_reg, last_dac_value, retry_count);
    
    /* Step 5: Clear EXTI pending */
    EXTI_ClearITPendingBit(EXTI_Line4);
}
```

---

## 4. Soft-start DAC Ramp Strategy

### Implementation (D-05, D-06)

500ms linear ramp: divide target DAC value into N steps, output one step per control cycle.

```c
#define SOFTSTART_DURATION_MS  500
#define CONTROL_PERIOD_MS      100
#define SOFTSTART_STEPS        (SOFTSTART_DURATION_MS / CONTROL_PERIOD_MS)  /* = 5 */

void softstart_engage(uint16_t target_dac) {
    uint16_t step = target_dac / SOFTSTART_STEPS;
    for (int i = 0; i < SOFTSTART_STEPS; i++) {
        uint16_t dac_out = (i == SOFTSTART_STEPS - 1) ? target_dac : (i + 1) * step;
        dac8571_set_output(dac_out);
        Delay_Ms(CONTROL_PERIOD_MS);
    }
}
```

### When Soft-start Applies (D-06)
- **IDLE → CV**: Apply soft-start to PID_CV's initial output
- **IDLE → CC**: Apply soft-start to PID_CC's initial output
- **FAULT → CV/CC** (auto-retry): Apply soft-start (system was at DAC=0)
- **CV ↔ CC**: No soft-start — instant PID handoff (PID instance switch only)
- **Setpoint change within same mode**: No soft-start — PID tracks the new setpoint

### Integration with PID

Soft-start is a feed-forward ramp that runs BEFORE PID engagement. Once ramp reaches target, PID takes over:
1. Compute PID target output
2. Run soft-start ramp to target
3. Engage PID with initialized integral = target (bumpless transfer)
4. Normal PID control thereafter

---

## 5. Fault State Machine Design

### State Enum (D-15)

```c
typedef enum {
    MODE_IDLE  = 0,
    MODE_CV    = 1,
    MODE_CC    = 2,
    MODE_FAULT = 3
} SystemMode;
```

### State Transitions

```
IDLE ──[engage CV w/ soft-start]──▶ CV
IDLE ──[engage CC w/ soft-start]──▶ CC
CV   ──[switch to CC]─────────────▶ CC  (instant handoff)
CC   ──[switch to CV]─────────────▶ CV  (instant handoff)
CV   ──[EXTI overcurrent OR OPP]──▶ FAULT (DAC→0, snapshot, retry++)
CC   ──[EXTI overcurrent OR OPP]──▶ FAULT (DAC→0, snapshot, retry++)
FAULT──[3s cooldown, retry<5]─────▶ IDLE (D-01: auto-retry)
FAULT──[3s cooldown, retry≥5]─────▶ FAULT (permanent latch, D-01)
FAULT──[explicit clear cmd]───────▶ IDLE (Phase 3 feature)
```

### Fault Register Bit Layout

```c
/* Fault register — 16-bit packed fault state */
typedef union {
    uint16_t raw;
    struct {
        uint8_t  overcurrent_mask : 4;  /* Which MOS channel(s) faulted (bits 0-3) */
        uint8_t  fault_type       : 2;  /* 0=overcurrent, 1=OPP, 2=both, 3=reserved */
        uint8_t  reserved         : 2;
        uint8_t  retry_count      : 4;  /* 0-15 (max 5 used) */
        uint8_t  latched          : 1;  /* Permanent latch (5th fault) */
        uint8_t  auto_recovery    : 1;  /* 0=disabled (latched), 1=enabled */
        uint8_t  reserved2        : 2;
    } bits;
} FaultRegister;
```

### Retry Counter Reset (D-03)

```c
/* In main loop: after 30s fault-free operation, reset retry counter */
static uint32_t fault_free_ticks = 0;  /* ms counter */
if (system_mode != MODE_FAULT) {
    fault_free_ticks++;
    if (fault_free_ticks >= 30000) {
        fault_reg.bits.retry_count = 0;
        fault_free_ticks = 0;
    }
} else {
    fault_free_ticks = 0;
}
```

### Total Power OPP (D-13)

Software check in main loop every control cycle:
```c
if (summary_power_w > RATED_WATTAGE) {
    fault_reg.bits.fault_type = 1;  /* OPP */
    fault_handler();  /* Same unified handler as EXTI path (D-04) */
}
```

Rated wattage: `#define RATED_WATTAGE 50.0f` (placeholder — adjust for hardware spec).

---

## 6. I2C Timing Budget Analysis

### Per-Control-Cycle I2C Operations

| Operation | Count | Time per op | Total |
|-----------|-------|------------|-------|
| Read summary bus voltage (2 bytes) | 1 | ~200µs | 0.2ms |
| Read summary current (2 bytes) | 1 | ~200µs | 0.2ms |
| Read summary power (2 bytes) | 1 | ~200µs | 0.2ms |
| Read MOS-1 current (2 bytes) | 1 | ~200µs | 0.2ms |
| Read MOS-2 current (2 bytes) | 1 | ~200µs | 0.2ms |
| Read MOS-3 current (2 bytes) | 1 | ~200µs | 0.2ms |
| Read MOS-4 current (2 bytes) | 1 | ~200µs | 0.2ms |
| Write DAC (3 bytes) | 1 | ~300µs | 0.3ms |
| **Subtotal per cycle** | | | **~1.7ms** |

At 100kHz I2C, each byte takes ~100µs (9 bits + ACK). A 2-byte read (register pointer + 2 data bytes + RESTART overhead) takes ~200µs. 8 reads + 1 write = ~1.7ms per cycle.

Budget = 100ms per cycle → I2C uses ~1.7% of cycle time. Very comfortable.

### ISR Context I2C (Fault Path)

Reading 4 alert registers during ISR = ~0.8ms. DAC write = ~0.3ms. Total ISR I2C = ~1.1ms. This is <2ms and well within acceptable ISR latency when nesting is enabled.

### Bus Contention

All 6 devices (5 INA226 + 1 DAC8571) share I2C1 at 100kHz. No arbitration conflicts because this is a single-master bus. The described timing is worst-case; actual INA226 reads may be faster if the device has data ready (conversion time = 1.1ms per reading in the default config, but conversions run continuously).

**CONTEXT.md D-10 states ~30ms per cycle** — this may include conversion waiting time. Even at 30ms worst-case, the budget is well within 100ms.

---

## 7. Calibration Re-validation (PROT-04)

### Strategy

Periodically re-read the calibration register (0x05) of all 5 INA226 devices. If the register reads back zero while bus voltage is present (>0V on that channel), the calibration has been lost and the device must be re-initialized.

### Detection Logic

```c
void calibration_check(void) {
    for (int i = 0; i < DEV_COUNT; i++) {
        uint16_t cal_val;
        if (ina226_read_calibration(&devs[i], &cal_val) != I2C_OK) continue;
        
        if (cal_val == 0) {
            /* Read bus voltage to confirm device is powered */
            float bus_v = 0.0f;
            ina226_get_bus_voltage(&devs[i], &bus_v);
            
            if (bus_v > 0.1f) {
                /* Bus voltage present, calibration lost — re-init */
                printf("[PROT-04] CH%d cal=0, bus=%.2fV — re-initializing\r\n",
                       devs[i].channel, bus_v);
                ina226_init(&devs[i]);
            }
        }
    }
}
```

### Frequency

Check once per second (every 10 control cycles). This balances the protection requirement with I2C bus overhead: reading 5 calibration registers = 5 × ~200µs = 1ms every 10 cycles = negligible overhead.

### New API Needed

```c
/* Read calibration register (0x05) and return the raw uint16_t value. */
i2c_status_t ina226_read_calibration(INA226_Dev *dev, uint16_t *cal_value);
```

---

## 8. Memory Budget Analysis

### New Static Allocations for Phase 2

| Item | Type | Size |
|------|------|------|
| `pid_cv` | PID_Instance | 24 bytes (6 × float32) |
| `pid_cc` | PID_Instance | 24 bytes |
| `fault_reg` | FaultRegister | 2 bytes |
| `system_mode` | SystemMode (enum→uint8) | 1 byte |
| `retry_count` | uint8_t | 1 byte |
| `last_dac_value` | uint16_t | 2 bytes |
| `fault_triggered` | volatile uint8_t | 1 byte |
| `cooldown_timer` | uint32_t | 4 bytes |
| `fault_free_ticks` | uint32_t | 4 bytes |
| `softstart_active` | uint8_t | 1 byte |
| `summary_voltage, current, power` | 3 × float | 12 bytes |
| `mos_currents[4]` | 4 × float | 16 bytes |
| **Total new allocations** | | **~92 bytes** |

### Existing Phase 1 Allocations

| Item | Size |
|------|------|
| `devs[5]` (INA226_Dev array) | 10 bytes |
| `TxBuf[1024]` (printf buffer, in debug.c) | 1024 bytes |
| `p_us`, `p_ms` (delay globals) | 8 bytes |
| Stack (linker: 2KB) | 2048 bytes |
| Various SPL globals | ~200 bytes |
| **Existing** | **~3.3 KB** |

### Total SRAM Usage

~3.3 KB (existing) + ~0.1 KB (Phase 2) = **~3.4 KB** out of 32 KB (10.6%).

**Conclusion:** Memory is not a constraint for Phase 2. No heap usage (all statically allocated), no risk of fragmentation.

---

## 9. Missing Driver APIs

### INA226 Driver Extensions Required

| Function | Register | Purpose | Phase |
|----------|----------|---------|-------|
| `ina226_set_alert_limit()` | 0x07 (Alert Limit) | Set per-device overcurrent threshold | PROT-01 |
| `ina226_set_alert_config()` | 0x06 (Mask/Enable) | Configure latch mode + alert trigger | PROT-01 |
| `ina226_read_calibration()` | 0x05 (Calibration) | Read-back for re-validation check | PROT-04 |

All follow the existing INA226 driver pattern: 3-byte write via `i2c_util_write`, 2-byte read via `i2c_util_read`. Same error handling, same return type (`i2c_status_t`).

### I2C Util — No Changes Required

The existing `i2c_util_write()` and `i2c_util_read()` with 10ms timeout work correctly for all new operations. The alert register read in ISR context uses the same `i2c_util_read()` — the timeout-based polling works because interrupt nesting is enabled.

### No New Peripheral Drivers

Phase 2 uses only peripherals already initialized in Phase 1 (I2C1, GPIOs). EXTI uses SPL directly (no wrapper needed — the SPL EXTI API is simple enough). SysTick is already initialized for `Delay_Ms`/`Delay_Init`.

---

## 10. Integration Plan

### Files to Create
| File | Purpose |
|------|---------|
| `Drivers/pid.c` | PID controller implementation (init, compute, reset) |
| `Drivers/pid.h` | PID struct, coefficient defines, API |
| `Drivers/fault.c` | Fault handler, state machine, retry logic |
| `Drivers/fault.h` | Fault register types, state enum, handler API |

### Files to Modify
| File | Changes |
|------|---------|
| `Drivers/ina226.h` | Add `ina226_set_alert_limit`, `ina226_set_alert_config`, `ina226_read_calibration` declarations |
| `Drivers/ina226.c` | Implement the 3 new functions |
| `User/main.c` | Replace read-only super-loop with control loop; add GPIO+EXTI init; add state machine; add fault handler |
| `User/ch32v30x_it.c` | Add `EXTI4_IRQHandler` |
| `User/ch32v30x_it.h` | Add EXTI4 IRQ handler declaration (optional — WCH convention is to declare in .c only) |

### Super-Loop Structure (Replaces current main.c while(1))

```
while (1) {
    if (!control_tick_ready()) continue;  // Wait for 100ms period (D-07)
    
    // 1. Read summary INA226 (bus voltage + current for PID feedback)
    // 2. Read 4 MOS INA226 currents (for protection monitoring)
    
    // 3. Process fault flag from ISR
    if (fault_triggered) fault_handler();
    
    // 4. State machine dispatch
    switch (system_mode) {
        case MODE_IDLE:  /* ... */ break;
        case MODE_CV:    pid_compute(CV); dac_update(); break;
        case MODE_CC:    pid_compute(CC); dac_update(); break;
        case MODE_FAULT: fault_state_machine(); break;
    }
    
    // 5. OPP check (total power > rated wattage) — D-13
    if (summary_power > RATED_WATTAGE) fault_handler();
    
    // 6. Periodic calibration re-validation (every 10 cycles) — PROT-04
    if (cycle_count % 10 == 0) calibration_check();
}
```

### Wave Grouping (for planning)

**Wave 1 — Driver Foundation:** Add INA226 alert API (alert limit write, mask/enable config, calibration read). These are pure extensions to the existing driver and have no dependencies on control logic.

**Wave 2 — Control Core:** Implement PID controller, soft-start ramp, state machine, fault handler, EXTI ISR. These are the "business logic" of Phase 2 and depend on Wave 1 for the alert API.

**Wave 3 — Integration:** Replace the main.c super-loop with the complete control loop. Depends on Waves 1 + 2.

**MVP Note (Mode: mvp):** Per vertical-slice rules, each wave should deliver a testable increment. Wave 1 = alert registers can be written and read back via printf. Wave 2 = PID computes correct output and soft-start ramps (printf-verify). Wave 3 = full integration, DAC responds to load changes, fault ISR fires on overcurrent.

---

## Research Summary

| Area | Finding | Risk |
|------|---------|------|
| PID compute | Software float at 96MHz is ~17µs/iteration — negligible | Low |
| I2C timing | ~1.7ms per cycle against 100ms budget (1.7%) | Low |
| ISR I2C | ~1.1ms for alert reads in ISR context — acceptable | Low |
| Memory | Phase 2 adds ~92 bytes to 3.4KB used (10.6% of 32KB) | Low |
| Missing APIs | 3 new INA226 functions needed (alert limit, alert config, cal read) | Low — straightforward register writes |
| PID tuning | Coefficients are #define constants — tuned empirically | Medium — requires hardware testing |
| Fault state machine | 4 states, well-defined transitions, simple logic | Low |
| EXTI ISR nesting | I2C blocking in ISR blocks lower-priority interrupts for ~4ms | Low — protection is highest priority |
| Soft-start | 5-step linear ramp at 100ms/step = 500ms total | Low |

**Overall risk assessment: Low.** Phase 2 is primarily integration work — combining existing Phase 1 drivers (INA226, DAC8571, I2C) with new control algorithms. The PID math is simple, the state machine has 4 states, and the timing budget is generous. The main complexity is getting the EXTI ISR and fault handler correct, which is a correctness concern (not a feasibility one).

## RESEARCH COMPLETE
