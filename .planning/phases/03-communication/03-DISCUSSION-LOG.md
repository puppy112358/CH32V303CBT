# Phase 3: Communication - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-07
**Phase:** 3-Communication
**Areas discussed:** Command Protocol Scope, Telemetry Packet Design, Error Handling & Validation, Packet Framing & RX Strategy

---

## Command Protocol Scope

### Q1: What cJSON commands should UART0 accept beyond set_mode and clear_fault?

| Option | Description | Selected |
|--------|-------------|----------|
| Minimal — set_mode + clear_fault only | Just the two commands needed for core operation | |
| Practical — add status query + setpoint readback | Include get_status and get_info for PC-side debugging and connection validation | ✓ |
| Full — add disable, setpoint adjust, config readback | Include disable, set_setpoint, get_config commands | |

**User's choice:** Practical — 4 commands total: set_mode, clear_fault, get_status, get_info.
**Notes:** PC-side needs status query for connection validation and debugging. Config readback (PID coeffs, thresholds) not needed — those are compile-time constants.

### Q2: How should command acknowledgment work?

| Option | Description | Selected |
|--------|-------------|----------|
| Ack for every command | Every command gets {"ack":"ok"} or {"ack":"error",...} | ✓ |
| Ack only for queries | get_status/get_info return data; commands are fire-and-forget | |

**User's choice:** Ack for every command. PC needs confirmation that commands were received and acted on.

### Q3: What does set_mode accept?

| Option | Description | Selected |
|--------|-------------|----------|
| Volts/Amps with hardware-defined ranges | Value in real units (V/A), limits as #define constants | ✓ |
| Raw DAC value (0-65535) | PC sends raw 16-bit code | |

**User's choice:** Real engineering units (volts/amps). CV: 0.0–30.0V, CC: 0.0–10.0A. In-mode setpoint changes re-send set_mode with same mode + new value.

### Q4: What should get_status and get_info return?

| Option | Description | Selected |
|--------|-------------|----------|
| get_status: runtime state only | Mode, setpoint, DAC, fault_code, uptime | ✓ |
| get_status: everything including config | Also PID coeffs, protection thresholds, calibration | |

**User's choice:** get_status for runtime state, get_info for static system info. Clean separation.

---

## Telemetry Packet Design

### Q1: How should the telemetry JSON be structured?

| Option | Description | Selected |
|--------|-------------|----------|
| Nested arrays for channels | ch[] array with {v,i} objects, sum{} object | ✓ |
| Flat keys — one key per value | v0, i0, v1, i1, ... sum_v, sum_i, sum_p | |

**User's choice:** Nested arrays — cleaner for PC-side iteration over channels.

### Q2: Should telemetry synchronize with the control loop or run on an independent timer?

| Option | Description | Selected |
|--------|-------------|----------|
| Synchronized with control loop | Send after each PID cycle, ~100ms = ~10Hz | ✓ |
| Independent 10Hz timer | Separate timer, decoupled from control loop | |

**User's choice:** Synchronized — natural 10Hz cadence from existing 100ms cycle, data consistency guaranteed.

### Q3: Sequence number, timestamp, and temperature?

| Option | Description | Selected |
|--------|-------------|----------|
| Include seq + uptime ms, temp as 0.0 | Consistent schema from day one, temp placeholder | ✓ |
| No seq/timestamp, skip temperature | Add in Phase 4 when sensor exists | |

**User's choice:** Include seq + uptime for packet loss detection. Temperature field as 0.0 placeholder until Phase 4.

### Q4: Include DAC value and retry count in telemetry?

| Option | Description | Selected |
|--------|-------------|----------|
| Include DAC + retry count | Small integers, useful for debugging regulation and fault recovery | ✓ |
| Required fields only | Just channels, summary, mode, fault flags, temperature | |

**User's choice:** Include both — high diagnostic value for minimal packet size increase.

---

## Error Handling & Validation

### Q1: Error acknowledgment format?

| Option | Description | Selected |
|--------|-------------|----------|
| Error code + human message | {"ack":"error","code":101,"msg":"description"} | ✓ |
| Message only | {"ack":"error","msg":"description"}, no codes | |

**User's choice:** Numeric codes grouped by category (1xx=parse, 2xx=range, 3xx=state) + human-readable message.

### Q2: What should be validated?

| Option | Description | Selected |
|--------|-------------|----------|
| Full validation | JSON syntax, required fields, value ranges, mode validity, unknown commands | ✓ |
| Basic validation only | Only JSON syntax and required fields | |

**User's choice:** Full validation — better error messages for the PC operator.

### Q3: State transition gating?

| Option | Description | Selected |
|--------|-------------|----------|
| Strict state gating | set_mode rejected in FAULT, clear_fault rejected if not faulted | ✓ |
| Lenient — commands always accepted | set_mode in FAULT auto-clears fault first | |

**User's choice:** Strict gating — operator must explicitly clear faults before resuming operation.

### Q4: How to handle UART-level errors?

| Option | Description | Selected |
|--------|-------------|----------|
| Discard + report | Bad bytes discarded, error ack sent, control loop continues | ✓ |
| Silent discard | Drop bad data silently, no feedback to PC | |

**User's choice:** Discard + report. PC needs to know its data was rejected.

---

## Packet Framing & RX Strategy

### Q1: How should cJSON packets be delimited?

| Option | Description | Selected |
|--------|-------------|----------|
| Newline-delimited | \n terminator, JSON Lines format | ✓ |
| Length-prefixed binary frame | 2-byte length prefix before payload | |

**User's choice:** Newline-delimited — human-readable, testable with any terminal, standard format.

### Q2: RX strategy — interrupt-driven ring buffer or DMA?

| Option | Description | Selected |
|--------|-------------|----------|
| Interrupt-driven ring buffer | UART0 RXNE ISR pushes bytes, main loop scans for \n | ✓ |
| DMA circular buffer | DMA fills buffer, no per-byte interrupts | |

**User's choice:** Interrupt-driven ring buffer — simpler, proven on this MCU, no throughput issue at 115200.

### Q3: Baud rate and buffer size?

| Option | Description | Selected |
|--------|-------------|----------|
| 115200 bps, 512-byte ring buffer | Standard speed, holds 2-3 commands | ✓ |
| 230400 bps, 1024-byte ring buffer | Double speed, larger buffer | |

**User's choice:** 115200 bps, 512 bytes. Meets >115200 requirement, conservative SRAM usage.

### Q4: Command polling and TX strategy?

| Option | Description | Selected |
|--------|-------------|----------|
| Poll RX each control cycle, blocking TX | Parse at start of cycle, blocking USART_SendData for telemetry | ✓ |
| Interrupt-driven command processing + DMA TX | Parse in ISR, DMA-driven telemetry | |

**User's choice:** Poll in main loop + blocking TX. Simple, reliable, ~26ms TX time fits within 100ms cycle.

---

## Claude's Discretion

- cJSON library integration method (vendored source vs submodule)
- Ring buffer implementation (head/tail pointers, overflow handling)
- Exact error code numbering within 1xx/2xx/3xx categories
- cJSON memory management strategy in 32K SRAM
- UART0 GPIO pin assignment and RCC clock enable
- get_info exact values (rated_w, max_v, max_a) — verify against hardware spec
- Telemetry JSON key naming details
- Inter-byte timeout for incomplete JSON detection
- protocol.h API design

## Deferred Ideas

None — discussion stayed within phase scope.
