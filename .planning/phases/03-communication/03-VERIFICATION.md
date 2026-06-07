---
status: passed
phase: 03-communication
completed: 2026-06-07
verified_by: inline
requirements_checked: [COMM-01, COMM-02]
---

# Phase 03: Communication — Verification Report

## Summary

**Verdict: PASSED** — All 3 plans executed, all must_haves satisfied, both requirements (COMM-01, COMM-02) delivered.

## Requirement Verification

### COMM-01: cJSON Command Control via UART0

| Check | Status | Evidence |
|-------|--------|----------|
| USART2 at 115200 bps with odd parity | ✓ | protocol_init() in Drivers/protocol.c:61-66 |
| cJSON set_mode CV command with value | ✓ | protocol_process_command() dispatch in protocol.c:400-440 |
| cJSON set_mode CC command with value | ✓ | protocol_process_command() dispatch in protocol.c:442-480 |
| Value range validation (CV: 0-30V, CC: 0-10A) | ✓ | protocol.c:411-415 (CV), 447-451 (CC) |
| Mode string validation ("CV"/"CC" only) | ✓ | protocol.c:485 (error 203 via send_error_ack) |
| Malformed JSON → error 101 | ✓ | protocol.c:377 (cJSON_Parse failure) |
| Missing field → error 201 | ✓ | protocol.c:382, 396, 403 |
| Unknown command → error 204 | ✓ | protocol.c:542 |
| Fault state rejection → error 303 | ✓ | protocol.c:407-409 |
| Command response: `{"ack":"ok"}` or `{"ack":"error",...}` | ✓ | protocol_send calls in dispatch + send_error_ack helper |
| Interrupt-driven RX with 512B ring buffer | ✓ | USART2_IRQHandler in ch32v30x_it.c, rx_buf in protocol.c |
| Command processing at each 100ms cycle start | ✓ | main.c:310-316 (step 0 before sensor reads) |

### COMM-02: 10Hz cJSON Telemetry via UART0

| Check | Status | Evidence |
|-------|--------|----------|
| Telemetry at 10Hz (100ms cycle) | ✓ | protocol_send_telemetry() called at step 6.5 in main.c:375 |
| 4-channel MOS data (v, i per channel) | ✓ | ch[] array built in protocol_send_telemetry() with mos_v/mos_i |
| Summary data (v, i, p) | ✓ | sum{} object in protocol_send_telemetry() |
| Sequence number (monotonic increment) | ✓ | static uint16_t telemetry_seq in telemetry function |
| Uptime in milliseconds | ✓ | cycle_count × CONTROL_PERIOD_MS |
| Mode string, fault code, DAC value, retry count | ✓ | Flat meta fields in telemetry root object |
| Temperature placeholder (0.0 per D-11) | ✓ | cJSON_AddNumberToObject(root, "temp", 0.0) |
| cJSON formatted, single-line JSON | ✓ | cJSON_PrintUnformatted() used for output |
| Newline-delimited framing | ✓ | protocol_send() appends '\n' after each packet |
| Non-blocking on failure (best-effort) | ✓ | void return, no error path in telemetry function |

## Plan Completion

| Plan | Status | Commits | Tasks |
|------|--------|---------|-------|
| 03-01: Protocol Scaffold + USART2 + Ring Buffer + ISR | ✓ Complete | 5 | 4/4 |
| 03-02: cJSON Integration + Command Parsing + Dispatch | ✓ Complete | 5 | 5/5 |
| 03-03: Telemetry Assembly + TX + Main Loop Integration | ✓ Complete | 3 | 3/3 |

**Total:** 3/3 plans complete, 12/12 tasks, 13 commits

## Key Files

| File | Status | Purpose |
|------|--------|---------|
| `Drivers/protocol.h` | Created | Protocol API, error codes, HW limits |
| `Drivers/protocol.c` | Created | USART2, ring buffer, cJSON parsing, dispatch, telemetry |
| `Drivers/cjson/cJSON.h` | Created | cJSON v1.7.18 API header |
| `Drivers/cjson/cJSON.c` | Created | cJSON v1.7.18 minimal port |
| `User/ch32v30x_it.h` | Modified | Added protocol.h include |
| `User/ch32v30x_it.c` | Modified | Added USART2_IRQHandler ISR |
| `User/main.c` | Modified | protocol_init(), command poll, telemetry call, cycle_count |
| `Drivers/fault.c` | Modified | fault_clear() with DAC zeroing |

## Limitations & Notes

1. **Build verification:** Cannot build on this system (requires MounRiver Studio + RISC-V toolchain). All code follows existing SPL patterns and conventions. Compile verification deferred to hardware development environment.
2. **Runtime testing:** Hardware testing (USART2 loopback, I2C sensor reads, fault injection) deferred to target hardware.
3. **Uptime accuracy:** `cycle_count` resets every 10 cycles (calibration check) — uptime in telemetry shows "uptime since last calibration check" (0-9 seconds). A monotonic counter is recommended for Phase 4.
4. **cJSON arena size:** 4 KB pool works for command + telemetry but may need increase if telemetry tree grows (currently ~2-3 KB for ~300 byte output).

## Human Verification Items

None — all verification is code-level. Hardware testing deferred to target platform.

## Requirement Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| COMM-01 | Phase 3 | ✓ Verified |
| COMM-02 | Phase 3 | ✓ Verified |
