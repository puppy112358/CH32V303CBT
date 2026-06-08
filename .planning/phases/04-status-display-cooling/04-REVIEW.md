---
status: clean
phase: 04-status-display-cooling
depth: standard
files_reviewed: 11
review_date: 2026-06-08
critical: 0
warning: 0
info: 3
total: 3
---

# Phase 04: Status Display & Cooling — Code Review

**Standard-depth review of 11 source files across 3 plans (WS2812 LED, NTC temperature sensor, Fan PWM+tacho+PID+stall).**

## Review Scope

| File | Plan | Type | Lines |
|------|------|------|-------|
| Drivers/ws2812.h | 04-01 | Header | 83 |
| Drivers/ws2812.c | 04-01 | Implementation | 263 |
| Drivers/temp_sensor.h | 04-02 | Header | 83 |
| Drivers/temp_sensor.c | 04-02 | Implementation | 192 |
| Drivers/fan.h | 04-03 | Header | 115 |
| Drivers/fan.c | 04-03 | Implementation | 272 |
| User/ch32v30x_it.h | All | Header | 22 |
| User/ch32v30x_it.c | All | ISR | 248 |
| Drivers/fault.h | 04-03 | Header | 131 |
| Drivers/protocol.c | 04-02,03 | Protocol | 607 |
| User/main.c | All | Application | 443 |

## Findings

### Critical (0)

No critical issues found.

### Warning (0)

No warnings found.

### Info (3)

#### IN-01: Blocking ADC poll in temp_sensor_read() — accepted design choice
- **File:** Drivers/temp_sensor.c:87-90
- **Finding:** `while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);` is a blocking spin-wait. If ADC clock fails, this loops forever.
- **Assessment:** Same pattern as existing blocking I2C calls in the codebase (i2c_util, DAC8571). ADC conversion at 12MHz completes in ~30μs. The project has no RTOS, so a blocking call can't starve other tasks. The plan's threat model (PRD Section 5) acknowledges this and defers a tick-count timeout to v2.
- **Risk:** Low. Same failure mode as all other peripheral waits in the codebase.

#### IN-02: Shared tachometer variables between ISR and main loop — accepted with volatile
- **File:** Drivers/fan.c:34-37, User/ch32v30x_it.c:230-245
- **Finding:** `tacho_last_capture`, `tacho_period_ticks`, `tacho_valid`, `tacho_timeout` are written by TIM3_IRQHandler and read/written by fan_update(). All are declared `volatile` which ensures compiler doesn't optimize away reads. However, multi-byte (32-bit) accesses on RV32IMAC are not guaranteed atomic — a 32-bit read could be interrupted mid-access.
- **Assessment:** Acceptable for this use case. The values are non-critical (monitoring only). A partially-read `tacho_period_ticks` would produce a transient wrong RPM value for one 100ms cycle, which self-corrects on the next cycle. The risk of actual corruption on a 32-bit bus with aligned access is extremely low.
- **Risk:** Very low. Self-correcting within one cycle.

#### IN-03: ws2812_dma_busy flag check-then-set not atomic — safe in single-threaded context
- **File:** Drivers/ws2812.c:165-169
- **Finding:** `if (ws2812_dma_busy) return;` followed by `ws2812_dma_busy = 1;` has a TOCTOU window. If an interrupt could preempt between these lines and clear the flag, a race would occur.
- **Assessment:** Safe because: (1) ws2812_dma_trigger() is only called from the main loop (single-threaded super-loop, no RTOS preemption). (2) The ISR only clears the flag (never sets it). (3) The DMA transfer completes in 60μs, and the next call is guaranteed >100ms later. The check-then-set is effectively atomic from the main loop's perspective.
- **Risk:** None in current architecture. Would need attention if RTOS preemption is added.

## Security Review

| Threat | Assessed | Status |
|--------|----------|--------|
| Pin conflicts between Phase 04 modules | PA0, PA5, PA6, PA7 — all verified free on LQFP48 | ✓ CLEAR |
| ISR execution time exceeding budget | All Phase 04 ISRs are fast-path (<10μs each) | ✓ CLEAR |
| Stack overflow from nested ISRs | 2KB stack, 3 new ISRs × ~150 bytes = <500 bytes | ✓ CLEAR |
| DMA bus contention | DMA1 shared: only CH5 (WS2812, 60μs per 100ms cycle). I2C1 uses no DMA. | ✓ CLEAR |
| Fault register bitfield layout | fan_stall_flag at bit 14, reserved2 at bit 15 — no overlap | ✓ CLEAR |
| Uninitialized variables in ISR context | All globals zero-initialized at file scope or in init() | ✓ CLEAR |

## Code Quality Assessment

- **Project convention compliance:** All new files follow the established Drivers/ module pattern (lowercase filenames, SPL init struct pattern, C89 variable declarations at function top, Allman bracing, 4-space indent). ✓
- **Comment quality:** All functions have docblocks in `/** */` or `/*****/` style matching project convention. ✓
- **Magic numbers:** PWM frequencies, PID coefficients, lookup table values — all extracted as named #define constants. ✓
- **Error handling:** Follows project pattern (void-return for init, typed return for queries). ADC clamping provides safe defaults. ✓
- **Include hygiene:** Each module includes only what it needs. No circular dependencies detected. ✓

## Summary

All 11 Phase 04 source files pass review at standard depth. The three informational findings are accepted design choices consistent with the existing bare-metal embedded codebase patterns. No blocking issues, no security vulnerabilities, no code quality regressions.

---
*Review: 04-status-display-cooling*
*Date: 2026-06-08*
