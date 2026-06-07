---
phase: "02"
plan: "02"
type: "execute"
wave: 1
depends_on: []
files_modified:
  - "Drivers/ina226.h"
  - "Drivers/ina226.c"
  - "User/main.c"
autonomous: true
requirements:
  - "PROT-01"
  - "PROT-02"
---

# Plan 02: INA226 Alert API Extension + EXTI ISR Foundation

## Objective

Extend the INA226 driver with alert limit configuration, alert mask/enable register write, and calibration register read-back. Configure PA4 as EXTI4 falling-edge input for the wired-OR INA226 ALARM signal. Implement the EXTI4 ISR that reads all 4 MOS channel alert registers on fault, zeros the DAC, and logs a diagnostic snapshot. This provides the hardware fault detection foundation that Wave 2 (control loop) and Wave 3 (state machine) build on.

## must_haves

- INA226 alert limit register (0x07) writable per device with per-channel overcurrent threshold
- INA226 mask/enable register (0x06) configurable for shunt-voltage-over-voltage in latch mode
- INA226 calibration register (0x05) readable for PROT-04 validation
- PA4 GPIO configured as input with pull-up (IPU), EXTI4 falling-edge interrupt
- EXTI4 ISR reads 4 MOS alert registers, zeroes DAC, prints diagnostic snapshot
- All new functions follow existing INA226 driver patterns (i2c_util_write/read, i2c_status_t returns)

## truths

- EXTI4 ISR uses the existing i2c_util_read() — interrupt nesting is enabled (CSR 0x804), making blocking I2C in ISR context acceptable for the ~1.1ms alert read window
- Per-MOS overcurrent threshold = INA226_MAX_CURRENT / 4 × 1.3 = 1.625A, converted to INA226 current register value 10650 (0x299A)
- Alert configuration sets bit 15 (Shunt Voltage Over-Voltage) and bits 1:0 = 01 (Latch mode) in the mask/enable register (0x06)
- All alert writes and reads go through the existing i2c_util layer — no direct SPL I2C calls
- The wired-OR ALARM signal connects all 4 MOS INA226 ALERT pins to PA4 — any channel fault triggers the same ISR; the ISR must read all 4 alert registers to identify the source

## Tasks

### Task 1: Add INA226 Alert and Calibration Register Support

<type>implement</type>
<file>Drivers/ina226.h</file>
<file>Drivers/ina226.c</file>

<read_first>
- Drivers/ina226.h — Current INA226 API surface: register defines, struct, existing function signatures
- Drivers/ina226.c — Existing implementation patterns: i2c_util_write with 3-byte register+data format, i2c_util_read with register pointer, status return conventions
- Drivers/i2c_util.h — i2c_status_t definition, i2c_util_write and i2c_util_read signatures, I2C_TIMEOUT_MS constant
</read_first>

<action>
Add three new function declarations to Drivers/ina226.h after the existing `ina226_check_alert` declaration (around line 122):

1. `i2c_status_t ina226_set_alert_limit(INA226_Dev *dev, uint16_t value);` — Write alert limit register (0x07)
2. `i2c_status_t ina226_set_alert_config(INA226_Dev *dev, uint16_t mask);` — Write mask/enable register (0x06)  
3. `i2c_status_t ina226_read_calibration(INA226_Dev *dev, uint16_t *cal_value);` — Read calibration register (0x05)

Also add the register address define `#define INA226_REG_ALERT_LIMIT 0x07` in the register address section (after line 30).

In Drivers/ina226.c, implement all three functions following the existing pattern:
- `ina226_set_alert_limit`: 3-byte write {REG_ALERT_LIMIT, MSB, LSB} using i2c_util_write with I2C_TIMEOUT_MS
- `ina226_set_alert_config`: 3-byte write {REG_ALERT, MSB, LSB} using i2c_util_write with I2C_TIMEOUT_MS
- `ina226_read_calibration`: 2-byte read from REG_CALIB using i2c_util_read, parse MSB-first uint16_t, store in *cal_value

Add function docblocks matching the existing `/** */` style with Chinese-friendly descriptions.
</action>

<acceptance_criteria>
- Drivers/ina226.h contains `#define INA226_REG_ALERT_LIMIT 0x07` in the register address section
- Drivers/ina226.h contains declarations for `ina226_set_alert_limit`, `ina226_set_alert_config`, `ina226_read_calibration` with proper `i2c_status_t` return types and INA226_Dev* parameters
- Drivers/ina226.c contains implementations for all three functions, each following the existing i2c_util_write/i2c_util_read pattern
- `ina226_set_alert_limit` sends register pointer 0x07 followed by MSB and LSB of the 16-bit value
- `ina226_set_alert_config` sends register pointer 0x06 followed by MSB and LSB of the 16-bit mask
- `ina226_read_calibration` reads register 0x05, parses MSB-first 16-bit value into *cal_value
- All three functions compile without errors (no linker — compile check only at this stage since main.c won't call them yet)
- All functions use `I2C_TIMEOUT_MS` (10) for the timeout parameter
</acceptance_criteria>

### Task 2: Configure PA4 EXTI4 for Falling-Edge Interrupt

<type>implement</type>
<file>User/main.c</file>

<read_first>
- User/main.c — Current GPIO init section (lines 60-112): NVIC config pattern, I2C init, INA226 init, DAC8571 init sequence. Understand where to insert EXTI init.
- Peripheral/inc/ch32v30x_gpio.h — GPIO_Pin_4 definition, GPIO_Mode_IPU, GPIO_Init and GPIO_InitStructure pattern
- Peripheral/inc/ch32v30x_exti.h — EXTI_Line4, EXTI_Mode_Interrupt, EXTI_Trigger_Falling, EXTI_Init and EXTI_InitStructure pattern, GPIO_EXTILineConfig
- Core/core_riscv.h — NVIC_EnableIRQ, NVIC_SetPriority signature, EXTI4_IRQn constant
- User/main.c — Current `devs[5]` array (lines 44-50): device addresses and channel assignments, needed because ISR references devs[]
</read_first>

<action>
In User/main.c, after the INA226 initialization block (after line 94, before DAC8571 init):

1. Add PA4 GPIO configuration: GPIO_InitTypeDef with GPIO_Pin_4, GPIO_Mode_IPU, GPIO_Speed_50MHz
2. Call GPIO_Init(GPIOA, &gpio_init) for PA4
3. Configure EXTI line source: GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource4)
4. Configure EXTI4: EXTI_InitTypeDef with EXTI_Line4, EXTI_Mode_Interrupt, EXTI_Trigger_Falling, EXTI_LineCmd=ENABLE
5. Call EXTI_Init(&exti_init)
6. Set NVIC priority: NVIC_SetPriority(EXTI4_IRQn, 0x01) — second-highest priority after system exceptions
7. Enable NVIC: NVIC_EnableIRQ(EXTI4_IRQn)
8. Add printf("EXTI4 on PA4 configured (INA226 ALARM input)\r\n") after configuration

Move the `devs[5]` array declaration from `static` (file-scope, line 44) to non-static (remove `static` keyword) so the ISR in ch32v30x_it.c can reference it. Add `extern INA226_Dev devs[5];` to a new section comment `/* External references for ISR modules */` near the top of main.c after includes.

All new GPIO/EXTI variables follow the existing pattern: declare at top of main(), initialize with = {0}.
</action>

<acceptance_criteria>
- User/main.c contains `GPIO_Init(GPIOA, ...)` with GPIO_Pin_4 and GPIO_Mode_IPU before DAC8571 init
- User/main.c contains `GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource4)` 
- User/main.c contains `EXTI_Init(...)` with EXTI_Line4, EXTI_Mode_Interrupt, EXTI_Trigger_Falling
- User/main.c contains `NVIC_EnableIRQ(EXTI4_IRQn)` with priority set to 0x01
- `devs[5]` array is non-static (extern-visible) with `extern INA226_Dev devs[5];` declared
- printf output includes "EXTI4 on PA4 configured" message
- Code compiles successfully
</acceptance_criteria>

### Task 3: Implement EXTI4 ISR with Fault Detection and Snapshot

<type>implement</type>
<file>User/ch32v30x_it.c</file>
<file>User/ch32v30x_it.h</file>

<read_first>
- User/ch32v30x_it.c — Current ISR structure: NMI_Handler, HardFault_Handler with WCH-Interrupt-fast attribute. Understand ISR declaration pattern.
- User/ch32v30x_it.h — Current header: includes debug.h only. Understand header structure.
- Drivers/ina226.h — INA226_Dev struct fields (address, channel), ina226_check_alert signature, INA226_ALERT_SHUNT_OV bit flag (0x8000)
- Drivers/dac8571.h — dac8571_set_output signature, DAC8571_ADDR
- Peripheral/inc/ch32v30x_exti.h — EXTI_ClearITPendingBit, EXTI_Line4
- User/main.c — External reference to devs[5] array (from Task 2)
</read_first>

<action>
In User/ch32v30x_it.h:
1. Add `#include "../Drivers/ina226.h"` after the existing `#include "debug.h"`
2. Add `#include "../Drivers/dac8571.h"` after the ina226 include

In User/ch32v30x_it.c:
1. Add `#include "../Drivers/ina226.h"` and `#include "../Drivers/dac8571.h"` after existing includes
2. Add `extern INA226_Dev devs[5];` reference to access the device array from main.c
3. Add `volatile uint8_t fault_triggered = 0;` global for ISR-to-main-loop communication
4. Add `volatile uint16_t fault_source_mask = 0;` global to identify which channel(s) triggered
5. Declare `void EXTI4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));`
6. Implement EXTI4_IRQHandler:
   - Declare uint8_t i; uint16_t alert_mask; i2c_status_t st; at top of function
   - Loop i=0 to i<4 (MOS channels only — devs[0] through devs[3]):
     - Call ina226_check_alert(&devs[i], &alert_mask) — reading the mask register also clears INA226 latch
     - If alert_mask has INA226_ALERT_SHUNT_OV bit set (0x8000): set fault_source_mask bit (1 << devs[i].channel)
   - Call dac8571_set_output(0) to zero the DAC immediately
   - Set fault_triggered = 1
   - Print diagnostic snapshot: printf("[FAULT] mask=0x%04X dac=0 addr=0x%02X-0x%02X-0x%02X-0x%02X\r\n", fault_source_mask, ...)
   - Call EXTI_ClearITPendingBit(EXTI_Line4)

Use `__disable_irq()` / `__enable_irq()` around the dac8571_set_output(0) call to prevent interrupt nesting during the DAC zeroing I2C transaction — ensures DAC goes to zero atomically even if another interrupt fires.

The diagnostic snapshot format should include: the fault source mask, the DAC value before zeroing (store in a volatile global `last_dac_value` that main.c will maintain), and the I2C addresses of all 4 MOS channels for traceability.
</action>

<acceptance_criteria>
- User/ch32v30x_it.c contains `EXTI4_IRQHandler` with WCH-Interrupt-fast attribute
- ISR loops through devs[0] through devs[3] calling ina226_check_alert on each
- ISR sets fault_triggered volatile flag after reading alert registers
- ISR calls dac8571_set_output(0) to zero DAC
- ISR prints diagnostic snapshot via printf with fault source mask
- ISR calls EXTI_ClearITPendingBit(EXTI_Line4) at end
- `volatile uint8_t fault_triggered` and `volatile uint16_t fault_source_mask` are declared at file scope
- User/ch32v30x_it.h includes ina226.h and dac8571.h
- Entire project compiles without errors (all Tasks 1-3 together)
</acceptance_criteria>
