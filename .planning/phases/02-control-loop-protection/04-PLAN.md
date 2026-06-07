---
phase: "02"
plan: "04"
type: "execute"
wave: 3
depends_on: ["02", "03"]
files_modified:
  - "Drivers/fault.h"
  - "Drivers/fault.c"
  - "User/main.c"
autonomous: true
requirements:
  - "PROT-02"
  - "PROT-03"
  - "PROT-04"
---

# Plan 04: Fault State Machine + OPP + Calibration Re-validation

## Objective

Implement the complete fault state machine (Drivers/fault.c/.h): unified fault handler for both EXTI overcurrent and software OPP, auto-retry with 3-second cooldown and 5-retry permanent latch, fault register with per-channel overcurrent mask and fault type encoding, 30-second fault-free retry counter reset. Add total power over-limit (OPP) software check every control cycle (summary power > rated wattage). Add periodic INA226 calibration register re-validation (check every 10 control cycles, re-init if lost while bus voltage present). Wire the fault handler and state machine into the main loop so the system transitions IDLE↔CV/CC↔FAULT with full auto-recovery. This completes all Phase 2 requirements and success criteria.

## must_haves

- Fault handler module with unified entry point for both EXTI (hardware) and OPP (software) fault sources (D-04)
- Fault register: 16-bit packed struct/union with per-channel overcurrent bitmask, fault type (overcurrent/OPP/both), retry count, latch flag
- Auto-retry: FAULT→IDLE after 3-second cooldown, max 5 retries; 5th consecutive fault → permanent latch until explicit clear (D-01)
- Retry counter reset: 30 seconds of fault-free operation resets retry count to 0 (D-03)
- Diagnostic snapshot on fault: printf all 4 MOS channels, summary INA226, active mode, DAC value, fault type, retry count (D-02)
- Total power OPP: every control cycle, check summary INA226 power against RATED_WATTAGE #define; if exceeded, call the same unified fault handler (D-13)
- Calibration re-validation: every 10 control cycles, read all 5 INA226 calibration registers; if zero + bus voltage >0.1V, re-init that device (D-14, PROT-04)
- State machine transitions: CV/CC→FAULT on fault; FAULT→IDLE on auto-retry timeout; FAULT→FAULT on permanent latch
- Externally accessible `fault_triggered` volatile flag for the super-loop to check
- Fault state machine runs in main loop context (not ISR) — ISR only sets flags + zeroes DAC (Plan 02)

## truths

- The unified fault handler means both EXTI overcurrent and software OPP share the same DAC-zeroing, snapshot-logging, and mode-transition logic — the only difference is how the fault is detected
- 3-second cooldown timer uses the same 100ms control tick: count 30 cycles before attempting retry
- Permanent latch requires an explicit clear command (Phase 3 — for now, a `clear_fault()` stub function with TODO comment)
- RATED_WATTAGE is a #define constant: `#define RATED_WATTAGE 50.0f` — placeholder tuned to hardware spec
- Calibration check overhead: 5 × 2-byte reads = ~1ms every 10 cycles = 0.1% of I2C bus time — negligible
- The fault register is 16-bit to fit in a single register; union with bitfield struct for clean access

## Tasks

### Task 1: Create Fault Handler Module (fault.h + fault.c)

<type>implement</type>
<file>Drivers/fault.h</file>
<file>Drivers/fault.c</file>

<read_first>
- User/main.c — SystemMode enum already defined (MODE_IDLE/CV/CC/FAULT), devs[5] array, extern volatile declarations
- Drivers/ina226.h — INA226_Dev struct, all getter function signatures for diagnostic snapshot
- Drivers/dac8571.h — dac8571_set_output for DAC zeroing
</read_first>

<action>
Create Drivers/fault.h:
1. Include guard `__FAULT_H`, C++ extern block, `#include <stdint.h>`
2. Fault type enum: `typedef enum { FAULT_TYPE_OC = 0, FAULT_TYPE_OPP = 1, FAULT_TYPE_BOTH = 2 } FaultType;`
3. Fault register union:
```
typedef union {
    uint16_t raw;
    struct {
        uint16_t overcurrent_mask : 4;  /* Bits 0-3: per-channel fault */
        uint16_t fault_type       : 2;  /* 0=OC, 1=OPP, 2=both */
        uint16_t reserved         : 2;
        uint16_t retry_count      : 4;  /* 0-15 (max 5 used) */
        uint16_t latched          : 1;  /* Permanent latch on 5th fault */
        uint16_t auto_recovery    : 1;  /* 0=disabled (latched), 1=enabled */
        uint16_t reserved2        : 2;
    } bits;
} FaultRegister;
```
4. `#define RATED_WATTAGE 50.0f`
5. `#define MAX_RETRY_COUNT 5`
6. `#define COOLDOWN_CYCLES 30` (30 cycles × 100ms = 3000ms = 3 seconds)
7. `#define FAULT_FREE_RESET_MS 30000` (30 seconds to reset retry counter)
8. Function declarations:
   - `void fault_init(void);` — Initialize fault register to zero, auto_recovery = 1
   - `void fault_handler_hw(uint16_t alert_mask);` — Called from ISR path: records OC channels from alert_mask, sets fault_type
   - `void fault_handler_opp(void);` — Called from main loop OPP check: sets fault_type to OPP
   - `void fault_state_machine(void);` — Called each control cycle: manages cooldown, retry, latch, transition to IDLE
   - `void fault_clear(void);` — Explicit clear: reset register, transition FAULT→IDLE (stub with TODO for Phase 3 cJSON)
   - `void fault_print_snapshot(FaultRegister *fr, uint16_t dac_value, SystemMode mode);` — Print full diagnostic (D-02)

Create Drivers/fault.c:
1. Include `../Drivers/fault.h`, `../Drivers/ina226.h`, `"debug.h"`
2. Declare static globals: `static FaultRegister fault_reg;`, `static uint8_t cooldown_counter;`, `static uint32_t fault_free_ms;`
3. extern references: `extern INA226_Dev devs[5];`, `extern SystemMode system_mode;`, `extern volatile uint8_t fault_triggered;`, `extern volatile uint16_t fault_source_mask;`
4. Implement fault_init: zero fault_reg.raw, set fault_reg.bits.auto_recovery = 1, zero cooldown_counter and fault_free_ms
5. Implement fault_handler_hw: set fault_reg.bits.overcurrent_mask = alert_mask & 0x0F, set fault_reg.bits.fault_type |= FAULT_TYPE_OC (bitwise OR to support BOTH), increment fault_reg.bits.retry_count (clamp at 5), set fault_reg.bits.auto_recovery = (retry_count < 5), set fault_reg.bits.latched = (retry_count >= 5), set fault_triggered = 0 (clear for next detection), log snapshot
6. Implement fault_handler_opp: set fault_reg.bits.fault_type |= FAULT_TYPE_OPP, same retry/latch logic as hw handler
7. Implement fault_state_machine:
   - Only acts when system_mode == MODE_FAULT
   - If fault_reg.bits.latched: return (permanent latch — wait for explicit clear)
   - If fault_reg.bits.auto_recovery:
     - Increment cooldown_counter
     - If cooldown_counter >= COOLDOWN_CYCLES: set system_mode = MODE_IDLE, cooldown_counter = 0, printf("[FAULT] Auto-retry %d — transitioning FAULT→IDLE\r\n", fault_reg.bits.retry_count)
8. Implement fault_clear: reset fault_reg, set system_mode = MODE_IDLE, printf("[FAULT] Cleared\r\n")
9. Implement fault_print_snapshot: printf bus voltage, shunt voltage, current for all 4 MOS channels + summary, plus mode, DAC value, fault type string, retry count. Format: `[FAULT SNAPSHOT] ...` one line per field.
</action>

<acceptance_criteria>
- Drivers/fault.h exists with FaultRegister union (16-bit with bitfields), FaultType enum, RATED_WATTAGE (50.0f), MAX_RETRY_COUNT (5), COOLDOWN_CYCLES (30), FAULT_FREE_RESET_MS (30000)
- Drivers/fault.h declares fault_init, fault_handler_hw, fault_handler_opp, fault_state_machine, fault_clear, fault_print_snapshot
- Drivers/fault.c implements all six functions
- fault_handler_hw increments retry_count and sets latched=1 when retry_count >= 5
- fault_state_machine increments cooldown_counter each call while in FAULT, transitions to MODE_IDLE after 30 cycles (3 seconds)
- fault_state_machine returns immediately if latched=1 (no auto-recovery)
- fault_clear resets fault register and sets mode to IDLE
- fault_print_snapshot outputs all 4 MOS channels + summary INA226 data + mode + DAC + fault type + retry count via printf
- fault.c compiles without errors (compile check)
</acceptance_criteria>

### Task 2: Add OPP Check and Calibration Re-validation to main.c

<type>implement</type>
<file>User/main.c</file>

<read_first>
- User/main.c — Current super-loop structure (modified by Plan 03): summary INA226 reads, MOS current reads, PID dispatch, DAC update, fault_triggered check
- Drivers/ina226.h — ina226_read_calibration signature (added in Plan 02), ina226_init signature, ina226_get_bus_voltage signature, INA226_CAL_VALUE
- Drivers/fault.h — RATED_WATTAGE, fault_handler_opp, fault_state_machine, fault_init, fault_print_snapshot
- User/main.c — Existing extern declarations for fault_triggered, system_mode
</read_first>

<action>
In User/main.c:

**Add includes:**
- Add `#include "../Drivers/fault.h"` after pid.h include

**Add globals + init (before while(1)):**
1. Add `static uint32_t cycle_count = 0;` for calibration check gating
2. Add `static uint32_t fault_free_ms = 0;` for retry counter reset timer (D-03)
3. Call `fault_init();` after PID init, before while(1). Print: `printf("Fault handler initialized (rated: %.1fW, max retries: %d)\r\n", RATED_WATTAGE, MAX_RETRY_COUNT);`

**Replace the fault check block in the super-loop:**

Replace the existing `if (fault_triggered)` block (from Plan 03) with:
```
if (fault_triggered) {
    system_mode = MODE_FAULT;
    fault_handler_hw(fault_source_mask);
    dac8571_set_output(0);
    last_dac_value = 0;
    fault_print_snapshot(&fault_reg, 0, system_mode);
    fault_triggered = 0;
}
```

**Add after MOS current reads (after the for loop reading mos_i[4]):**
```
// OPP check (D-13): total power over-limit
if (bus_p > RATED_WATTAGE && system_mode != MODE_FAULT) {
    fault_handler_opp();
    dac8571_set_output(0);
    last_dac_value = 0;
    fault_print_snapshot(&fault_reg, last_dac_value, system_mode);
    system_mode = MODE_FAULT;
}
```

**Add fault state machine call:**
After the state dispatch block (CV/CC PID blocks), add:
```
// Fault state machine: manage auto-retry, cooldown, latch
if (system_mode == MODE_FAULT) {
    fault_state_machine();
}
```

**Add retry counter reset (D-03):**
After the state machine call, add:
```
// Retry counter reset: 30s fault-free → reset to 0
if (system_mode != MODE_FAULT) {
    fault_free_ms += CONTROL_PERIOD_MS;
    if (fault_free_ms >= FAULT_FREE_RESET_MS) {
        fault_reg.bits.retry_count = 0;
        fault_free_ms = 0;
    }
} else {
    fault_free_ms = 0;
}
```

**Add calibration re-validation (PROT-04) at end of loop:**
```
// Periodic calibration check: every 10 cycles (1 second)
if (++cycle_count >= 10) {
    cycle_count = 0;
    for (i = 0; i < DEV_COUNT; i++) {
        uint16_t cal_val;
        if (ina226_read_calibration(&devs[i], &cal_val) != I2C_OK) continue;
        if (cal_val == 0) {
            float bus_v = 0.0f;
            if (ina226_get_bus_voltage(&devs[i], &bus_v) == I2C_OK && bus_v > 0.1f) {
                printf("[PROT-04] CH%d cal=0 bus=%.2fV — re-initializing\r\n",
                       devs[i].channel, bus_v);
                ina226_init(&devs[i]);
            }
        }
    }
}
```

Note: `fault_reg` must be non-static in fault.c or exposed via extern. In fault.h, declare `extern FaultRegister fault_reg;` so main.c can access retry_count for reset. Add this extern declaration.
</action>

<acceptance_criteria>
- User/main.c includes Drivers/fault.h
- User/main.c calls fault_init() before while(1)
- Super-loop OPP block checks `bus_p > RATED_WATTAGE` (50.0f) every cycle and calls fault_handler_opp() + dac8571_set_output(0) when tripped
- Super-loop calls fault_state_machine() when system_mode == MODE_FAULT
- Super-loop tracks fault_free_ms and resets fault_reg.bits.retry_count to 0 after 30000ms fault-free
- Calibration re-validation block runs every 10 cycles, reads all 5 INA226 calibration registers, and re-inits any device with zero calibration while bus voltage > 0.1V
- Super-loop prints "[PROT-04]" message when re-initializing a lost-calibration device
- Extern declaration of `fault_reg` is accessible in main.c for retry_count reset
- Entire project compiles without errors (Plans 02+03+04 together)
- All printf output follows existing pattern (descriptive text + values + \r\n)
</acceptance_criteria>

### Task 3: End-to-End Integration Verification

<type>verify</type>
<file>User/main.c</file>
<file>User/ch32v30x_it.c</file>
<file>Drivers/pid.c</file>
<file>Drivers/fault.c</file>
<file>Drivers/ina226.c</file>

<read_first>
- User/main.c — Complete control loop with all Phase 2 features integrated
- User/ch32v30x_it.c — EXTI4 ISR with fault detection
- Drivers/pid.c — PID controller implementation
- Drivers/fault.c — Fault state machine implementation
- Drivers/ina226.c — Extended INA226 driver with alert API
- .planning/REQUIREMENTS.md — All phase requirements and their acceptance criteria
- .planning/phases/02-control-loop-protection/02-CONTEXT.md — All implementation decisions (D-01 through D-15)
</read_first>

<action>
Perform end-to-end integration verification by checking the following against the source files:

1. **Build verification:** Run the build command to confirm all files compile and link:
   - Check that `make` or the Eclipse build produces an ELF binary without errors
   - Verify no warnings about implicit function declarations (all headers included)

2. **Decision traceability:** Verify each CONTEXT.md decision (D-01 through D-15) is reflected in the code:
   - D-01 (auto-retry 5x, 3s cooldown): Check fault.c fault_state_machine has COOLDOWN_CYCLES=30, MAX_RETRY_COUNT=5, latched check
   - D-02 (fault snapshot): Check fault_print_snapshot reads all 4 MOS + summary + mode + DAC + fault type + retry count
   - D-03 (fault types + 30s reset): Check fault_reg.bits.fault_type has OC/OPP/BOTH, fault_free_ms >= 30000 reset logic
   - D-04 (unified handler): Check fault_handler_hw and fault_handler_opp share same retry/latch/snapshot logic
   - D-05 (500ms soft-start): Check SOFTSTART_STEPS=5, CONTROL_PERIOD_MS=100 in main.c
   - D-06 (soft-start only on IDLE→CV/CC): Check engage_cv/engage_cc call softstart, CV↔CC path does not
   - D-07 (100ms SysTick gate): Check control tick gating at top of while(1)
   - D-08 (two PID instances): Check pid_cv and pid_cc are separate PID_Instance globals
   - D-09 (anti-windup via conditional integration): Check pid_compute skips integral accumulation when saturated
   - D-10 (read all 5 INA226 every cycle): Check summary read + 4 MOS reads in super-loop
   - D-11 (PA4 EXTI4): Check GPIO_Init with GPIO_Pin_4, EXTI_Init with EXTI_Line4
   - D-12 (ISR reads all 4 alerts): Check EXTI4_IRQHandler loops i=0 to i<4 calling ina226_check_alert
   - D-13 (OPP software check): Check bus_p > RATED_WATTAGE check in super-loop
   - D-14 (calibration re-validation): Check calibration_check block with ina226_read_calibration + ina226_init recovery
   - D-15 (4-state machine): Check MODE_IDLE/CV/CC/FAULT enum and all transitions

3. **Requirement traceability:** Verify each phase requirement:
   - CTRL-01 (CV PID): pid_compute(&pid_cv, cv_target_voltage, bus_v, 0.1f) in super-loop
   - CTRL-02 (CC PID): pid_compute(&pid_cc, cc_target_current, bus_i, 0.1f) in super-loop
   - CTRL-03 (anti-windup + soft-start): conditional integration in pid_compute, softstart_engage in engage_cv/cc
   - PROT-01 (alert config): ina226_set_alert_limit + ina226_set_alert_config functions exist and are called
   - PROT-02 (EXTI ISR): EXTI4_IRQHandler reads alerts, sets flags, zeroes DAC
   - PROT-03 (summary read + OPP): summary INA226 read + bus_p > RATED_WATTAGE check
   - PROT-04 (cal re-validation): calibration_check block with re-init

4. **Cross-reference check:** No function calls a symbol that doesn't exist. Every extern declaration has a corresponding definition.

5. **Memory check:** Verify no malloc or heap usage (bare-metal project policy). All state is static globals.

Output a verification checklist with each item marked [PASS] or [FAIL].
If all pass, return `## VERIFICATION PASSED`.
If any fail, return structured issue list with D-ID or REQ-ID, file:line, and description.
</action>

<acceptance_criteria>
- Project compiles to ELF binary without errors
- All 15 CONTEXT.md decisions (D-01 through D-15) are found in source with correct implementation
- All 7 phase requirements (CTRL-01/02/03, PROT-01/02/03/04) are traceable to specific code locations
- No undefined symbol references (every extern has a matching definition)
- No dynamic memory allocation (malloc/free not present in code)
- Printf output shows the expected information flow: init messages → control cycle readings → fault messages (on trigger)
- VERIFICATION PASSED marker returned on success
</acceptance_criteria>
