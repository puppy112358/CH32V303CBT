---
phase: "02"
plan: "03"
type: "execute"
wave: 2
depends_on: ["02"]
files_modified:
  - "Drivers/pid.h"
  - "Drivers/pid.c"
  - "User/main.c"
autonomous: true
requirements:
  - "CTRL-01"
  - "CTRL-02"
  - "CTRL-03"
  - "PROT-03"
---

# Plan 03: PID Controller + Soft-Start Ramp

## Objective

Implement the PID control module (Drivers/pid.c/.h) with two independent PID instances for CV and CC modes. Implement the soft-start linear DAC ramp (500ms, 5 steps) for IDLE→engage transitions. Wire PID compute and soft-start into main.c, replacing the read-only super-loop with the first stage of the control loop: SysTick-gated 100ms timing, summary INA226 read for feedback, PID compute, DAC update, and MOS channel current read for monitoring. This delivers CV and CC convergence (Success Criteria 1, 2, 4) but without the fault state machine (Wave 3).

## must_haves

- PID controller module with positional (parallel) form: P + I + D, anti-windup via conditional integration freeze when output saturated at 0 or 65535
- Two independent PID instances: pid_cv (feedback = summary bus voltage) and pid_cc (feedback = summary current), each with own Kp/Ki/Kd/integral/last_error
- Derivative-on-measurement to prevent setpoint-step derivative kicks
- PID coefficients as #define constants in pid.h: PID_CV_KP, PID_CV_KI, PID_CV_KD, PID_CC_KP, PID_CC_KI, PID_CC_KD
- PID output clamped to [0, 65535] DAC range with saturation flag
- Soft-start: 500ms linear DAC ramp from 0 to PID target, 5 steps of 100ms each
- Soft-start applies only on IDLE→CV and IDLE→CC transitions (not on CV↔CC or setpoint changes)
- Super-loop restructured with SysTick millisecond gate for 100ms control period (D-07)
- Summary INA226 reads (bus voltage + current) provide PID feedback every control cycle
- 4 MOS channel INA226 current reads for monitoring every control cycle
- System mode variable (IDLE/CV/CC) with state transitions IDLE→CV and IDLE→CC via function calls

## truths

- Software float PID at 96 MHz RV32IMAC is ~17µs per iteration — negligible against 100ms control period
- I2C traffic per cycle: 1 summary voltage read + 1 summary current read + 4 MOS current reads + 1 DAC write ≈ 1.7ms, well within 100ms budget
- PID coefficients are #define constants initially; Phase 3 adds runtime tuning via cJSON
- Bumpless PID transfer: on engage, pre-load integral term with initial DAC target so PID starts from soft-start endpoint without a step
- The EXTI4 ISR (Plan 02) sets `fault_triggered` and `fault_source_mask` volatiles; this plan reads those flags in the super-loop and calls dac8571_set_output(0) if set (fault handler details in Plan 04)

## Tasks

### Task 1: Create PID Controller Header (pid.h)

<type>implement</type>
<file>Drivers/pid.h</file>

<read_first>
- Drivers/ina226.h — Existing driver header pattern: include guard format, C++ extern block, include style (quoted path), typedef struct, function declaration style with i2c_status_t returns
- Drivers/dac8571.h — Another reference for the driver header pattern in this project
</read_first>

<action>
Create Drivers/pid.h with:

1. Include guard: `#ifndef __PID_H` / `#define __PID_H`
2. C++ extern "C" block
3. Include `<stdint.h>`
4. PID coefficient #defines:
   - `#define PID_CV_KP  1.0f`
   - `#define PID_CV_KI  0.1f`
   - `#define PID_CV_KD  0.01f`
   - `#define PID_CC_KP  0.5f`
   - `#define PID_CC_KI  0.05f`
   - `#define PID_CC_KD  0.005f`
   - `#define PID_OUTPUT_MAX  65535.0f`
   - `#define PID_OUTPUT_MIN  0.0f`
   - `#define PID_INTEGRAL_MAX  65535.0f`
   - `#define PID_INTEGRAL_MIN  -65535.0f`
5. PID_Instance typedef struct with fields: float Kp, float Ki, float Kd, float setpoint, float integral, float last_feedback, float output, uint8_t saturated
6. PID_Mode enum: PID_MODE_CV = 0, PID_MODE_CC = 1
7. Function declarations:
   - `void pid_init(PID_Instance *pid, float kp, float ki, float kd);` — Initialize PID with coefficients, zero integral and last_feedback
   - `float pid_compute(PID_Instance *pid, float setpoint, float feedback, float dt);` — Run one PID iteration, return clamped output [0, 65535]
   - `void pid_reset(PID_Instance *pid);` — Zero integral and last_feedback, clear saturated flag
   - `void pid_set_integral(PID_Instance *pid, float value);` — Pre-load integral term for bumpless transfer (clamp to INTEGRAL_MIN..INTEGRAL_MAX)

Match the project's function docblock style: `/** */` with @fn, @brief, @param, @return tags.
</action>

<acceptance_criteria>
- Drivers/pid.h exists with `__PID_H` include guard
- Contains PID_CV_KP, PID_CV_KI, PID_CV_KD, PID_CC_KP, PID_CC_KI, PID_CC_KD, PID_OUTPUT_MAX, PID_OUTPUT_MIN, PID_INTEGRAL_MAX, PID_INTEGRAL_MIN as #define constants
- Contains `PID_Instance` typedef struct with all 8 fields (Kp, Ki, Kd, setpoint, integral, last_feedback, output, saturated)
- Contains `PID_Mode` enum with PID_MODE_CV and PID_MODE_CC
- Contains declarations for pid_init, pid_compute, pid_reset, pid_set_integral with proper parameter types
- Header follows project conventions: `#ifndef` guard, `#ifdef __cplusplus` wrapper, `#include <stdint.h>`, 4-space indent
</acceptance_criteria>

### Task 2: Implement PID Controller (pid.c)

<type>implement</type>
<file>Drivers/pid.c</file>

<read_first>
- Drivers/pid.h — The header created in Task 1: coefficient defines, PID_Instance struct fields, function signatures
- Drivers/ina226.c — Reference for driver implementation style: function docblocks, local variable declarations at top, return pattern
</read_first>

<action>
Create Drivers/pid.c implementing the four PID functions:

1. `pid_init`: Set pid->Kp/kp, pid->Ki/ki, pid->Kd/kd, zero pid->integral, pid->last_feedback, pid->output, pid->saturated. Set pid->setpoint to 0.

2. `pid_compute`: 
   - Compute error = setpoint - feedback
   - P_term = pid->Kp * error
   - Check saturation: if output saturated at 65535 with positive error, skip I accumulation; if saturated at 0 with negative error, skip I accumulation (D-09: conditional integration anti-windup)
   - If not saturated: I_term increment = pid->Ki * error * dt, add to pid->integral, clamp pid->integral to [PID_INTEGRAL_MIN, PID_INTEGRAL_MAX]
   - D_term = -pid->Kd * (feedback - pid->last_feedback) / dt (derivative on measurement — avoids setpoint-step kick)
   - output = P_term + pid->integral + D_term
   - Clamp output to [0, 65535], set pid->saturated = 1 if clamped at 65535, 2 if clamped at 0, 0 otherwise
   - Update pid->last_feedback = feedback, pid->output = output, pid->setpoint = setpoint
   - Return pid->output

3. `pid_reset`: Set pid->integral = 0, pid->last_feedback = 0, pid->saturated = 0, pid->output = 0

4. `pid_set_integral`: Set pid->integral = value, clamp to [PID_INTEGRAL_MIN, PID_INTEGRAL_MAX]

All local variables (error, p_term, d_term) declared at top of each function. Add `#include "../Drivers/pid.h"` and `#include <math.h>` (for fabsf if needed — though simple comparisons suffice).
</action>

<acceptance_criteria>
- Drivers/pid.c contains implementations for pid_init, pid_compute, pid_reset, pid_set_integral
- pid_compute applies anti-windup: integral freezes when (output == 65535 && error > 0) OR (output == 0 && error < 0)
- pid_compute clamps output to [0, 65535] and sets saturated flag (0/1/2)
- pid_compute uses derivative-on-measurement: D_term = -Kd * (feedback - last_feedback) / dt
- pid_reset zeros integral, last_feedback, saturated, and output
- pid_set_integral clamps value to [PID_INTEGRAL_MIN, PID_INTEGRAL_MAX]
- pid.c follows project code style: Allman braces, 4-space indent, comments in `/* */` style
- pid.c compiles without errors (no linker — compile check only)
</acceptance_criteria>

### Task 3: Implement Soft-Start Ramp + Wire PID into main.c

<type>implement</type>
<file>User/main.c</file>

<read_first>
- User/main.c — Current super-loop structure (lines 114-187): INA226 read loop, Delay_Ms(500). Understand what to replace.
- Drivers/pid.h — PID_Instance struct, pid_init/pid_compute/pid_reset signatures, PID_Mode enum, coefficient defines
- Drivers/dac8571.h — dac8571_set_output(uint16_t) signature
- Drivers/ina226.h — INA226_Dev struct, ina226_get_bus_voltage, ina226_get_current, ina226_get_power signatures
- Debug/debug.h — Delay_Ms, SysTick-based delay — verify Control Period implementation (D-07)
</read_first>

<action>
In User/main.c, replace the existing while(1) super-loop (lines 114-187) and add supporting code before it:

**Before the while(1) — add after DAC8571 init section (around line 105):**

1. Add `#include "../Drivers/pid.h"` to includes (after dac8571.h)
2. Declare system globals:
   - `SystemMode system_mode = MODE_IDLE;` (add MODE_IDLE/CV/CC/FAULT enum in pid.h or a new section)
   - `PID_Instance pid_cv, pid_cc;` 
   - `float cv_target_voltage = 0.0f;` 
   - `float cc_target_current = 0.0f;`
   - `uint16_t last_dac_value = 0;` (volatile for ISR access)
   - `uint32_t last_control_tick = 0;` (for 100ms gating)
3. Initialize PID instances: `pid_init(&pid_cv, PID_CV_KP, PID_CV_KI, PID_CV_KD);` `pid_init(&pid_cc, PID_CC_KP, PID_CC_KI, PID_CC_KD);`
4. Declare extern volatile variables from ch32v30x_it.c: `extern volatile uint8_t fault_triggered;` `extern volatile uint16_t fault_source_mask;`
5. Add `extern volatile uint16_t last_dac_value;` for the ISR to snapshot
6. Print init status: `printf("PID controller initialized (CV: Kp=%.2f Ki=%.2f Kd=%.2f, CC: Kp=%.2f Ki=%.2f Kd=%.2f)\r\n", ...);`
7. Define soft-start constants: `#define SOFTSTART_STEPS 5`, `#define CONTROL_PERIOD_MS 100`
8. Implement `void softstart_engage(uint16_t target_dac)` function:
   - Compute step = target_dac / SOFTSTART_STEPS
   - Loop i=0 to i<SOFTSTART_STEPS: dac = (i == SOFTSTART_STEPS-1) ? target_dac : (i+1)*step; dac8571_set_output(dac); Delay_Ms(CONTROL_PERIOD_MS)
9. Implement `void engage_cv(float target_voltage)` function:
   - Set system_mode = MODE_CV, cv_target_voltage = target_voltage
   - Compute initial PID output using pid_compute with current bus voltage feedback
   - Call softstart_engage((uint16_t)pid_cv.output)
   - Pre-load integral: pid_set_integral(&pid_cv, pid_cv.output) for bumpless transfer
10. Implement `void engage_cc(float target_current)` function similarly for CC mode

**Replace the while(1) loop:**

```
while (1)
{
    // 100ms control period gating (D-07)
    if ((uint32_t)(SysTick->CNT - last_control_tick) < (SystemCoreClock / 10000)) continue;
    last_control_tick = SysTick->CNT;
    
    // 1. Read summary INA226 for PID feedback
    float bus_v = 0.0f, bus_i = 0.0f, bus_p = 0.0f;
    ina226_get_bus_voltage(&devs[4], &bus_v);
    ina226_get_current(&devs[4], &bus_i);
    ina226_get_power(&devs[4], &bus_p);
    
    // 2. Read 4 MOS channels for monitoring
    float mos_i[4] = {0};
    for (i = 0; i < 4; i++) ina226_get_current(&devs[i], &mos_i[i]);
    
    // 3. Check fault flag from ISR (fault handler detail in Plan 04)
    if (fault_triggered) {
        dac8571_set_output(0);
        system_mode = MODE_FAULT;
        printf("[FAULT] ISR triggered, mask=0x%04X\r\n", fault_source_mask);
        // Full fault handler in Plan 04
    }
    
    // 4. State machine dispatch
    if (system_mode == MODE_CV && !fault_triggered) {
        float output = pid_compute(&pid_cv, cv_target_voltage, bus_v, 0.1f);
        last_dac_value = (uint16_t)output;
        dac8571_set_output(last_dac_value);
    }
    else if (system_mode == MODE_CC && !fault_triggered) {
        float output = pid_compute(&pid_cc, cc_target_current, bus_i, 0.1f);
        last_dac_value = (uint16_t)output;
        dac8571_set_output(last_dac_value);
    }
    
    // 5. Print summary
    printf("Mode:%d V:%.2f I:%.2f P:%.2f DAC:%u MOS:%.2f %.2f %.2f %.2f\r\n",
           system_mode, bus_v, bus_i, bus_p, last_dac_value,
           mos_i[0], mos_i[1], mos_i[2], mos_i[3]);
}
```

Also add mode enum: `typedef enum { MODE_IDLE = 0, MODE_CV = 1, MODE_CC = 2, MODE_FAULT = 3 } SystemMode;` in main.c before the variable declarations.

**For testing:** Add test engage calls after PID init (before while(1)):
- `engage_cv(5.0f);` — engage CV mode with 5.0V target
- Comment: `/* TODO: replace with cJSON command in Phase 3 */`

Use `SysTick->CNT` for timestamp gating. The SysTick is configured at SystemCoreClock ticks/second in the standard CH32V30x init. `SystemCoreClock / 10000` = ticks per 100µs, which is not quite right — each tick is 1/SystemCoreClock seconds. For 100ms: `((SystemCoreClock / 1000) * 100)` ticks. Or simpler: maintain a millisecond counter via SysTick_Handler increment in a volatile uint32_t global, and check `if (ms_counter - last_ms < 100) continue;`.
</action>

<acceptance_criteria>
- User/main.c contains `SystemMode` enum with MODE_IDLE, MODE_CV, MODE_CC, MODE_FAULT
- User/main.c contains `engage_cv(float)` and `engage_cc(float)` functions with soft-start ramp calls
- User/main.c super-loop uses SysTick-based 100ms gating (control_tick condition at top of loop)
- Super-loop reads summary INA226 (devs[4]) for bus voltage and current every control cycle
- Super-loop reads 4 MOS INA226 currents every control cycle
- Super-loop calls pid_compute with CV PID when mode is MODE_CV, CC PID when mode is MODE_CC
- Super-loop calls dac8571_set_output with clamped PID output every control cycle
- Super-loop checks fault_triggered volatile flag and zeroes DAC if set
- Soft-start ramp runs 5 steps × 100ms = 500ms, outputting linear DAC ramp from 0 to target
- softstart_engage is called by engage_cv/engage_cc, not by CV↔CC or setpoint changes
- `last_dac_value` is updated before each DAC write and declared volatile for ISR access
- Entire project compiles without errors (Plans 02 + 03 together)
</acceptance_criteria>
