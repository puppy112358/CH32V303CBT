---
status: partial
phase: 01-hardware-foundation
source: [01-VERIFICATION.md]
started: 2026-06-05T12:00:00Z
updated: 2026-06-05T12:00:00Z
---

## Current Test

[awaiting human testing]

## Tests

### 1. Project Compilation
expected: MounRiver Studio IDE Build All — 0 errors, 0 warnings. All 6 new Drivers/ source files included in build.
result: [pending]

### 2. System Initialization Output
expected: Connect PA9 (USART1 TX) to serial terminal at 115200 8N1. After reset, system info banner and I2C device init status appear. Each of 5 INA226 and 1 DAC8571 show "OK" or "FAIL".
result: [pending]

### 3. INA226 Data Reading
expected: Terminal shows per-channel readings: bus voltage (V), shunt voltage (mV), current (A), power (W) for each of 5 channels at ~2Hz rate. Non-zero values on channels with connected hardware.
result: [pending]

### 4. USB-CDC Enumeration
expected: USB cable from MCU to PC → COM port appears in Device Manager. Open terminal on COM port — same INA226/DAC data as USART1. Disconnect USB — USART1 continues, USB-CDC stops. Reconnect — USB-CDC resumes. No crashes/hangs.
result: [pending]

### 5. DAC8571 Analog Output
expected: Multimeter on DAC8571 VOUT pin reads approximately VREF/2 (mid-scale) with 0x8000 test code. If VREF=3.3V, expect ~1.65V.
result: [pending]

## Summary

total: 5
passed: 0
issues: 0
pending: 5
skipped: 0
blocked: 0

## Gaps
