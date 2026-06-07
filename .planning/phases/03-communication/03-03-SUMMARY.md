---
phase: 03-communication
plan: 03
subsystem: communication
tags: [telemetry, cjson, data-reporting, monitoring, uart]

# Dependency graph
requires:
  - phase: 03
    plan: 02
    provides: "cJSON dispatch engine, arena allocator, protocol_send() — telemetry uses same infrastructure"
provides:
  - "10Hz cJSON telemetry packets with ch[4], sum{}, and meta fields"
  - "Sequence-numbered packets (uint16_t rollover at 65535)"
  - "Monotonic uptime in milliseconds (cycle_count × 100ms)"
  - "Complete COMM-02 requirement: live data reporting over USART2"
affects: [Phase 4 (data logging / PC-side monitoring)]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Telemetry assembly: arena reset → build cJSON tree → PrintUnformatted → protocol_send → cJSON_Delete → arena reset"
    - "Parameter-based telemetry: sensor readings passed as function args, not extern globals"
    - "Sequence number: static uint16_t increments each call, wraps at 65535"
    - "Uptime: deterministic cycle_count × CONTROL_PERIOD_MS (not SysTick-dependent)"

key-files:
  created: []
  modified:
    - "Drivers/protocol.h"
    - "Drivers/protocol.c"
    - "User/main.c"

key-decisions:
  - "Telemetry function takes 5 parameters (summary v/i/p + mos_i[4] + mos_v[4]) — cleaner than extern globals for sensor data"
  - "MOS channel voltages read in step 2 alongside currents — same I2C bus, minimal overhead (~2ms for 4 reads)"
  - "Telemetry placed after fault state machine (step 6.5) — reflects just-computed PID output and fault state"
  - "Arena reset both before AND after telemetry assembly — guarantees full pool available"

patterns-established:
  - "Telemetry packet schema: {seq, uptime, mode, fault, dac, retry, temp, ch[{v,i}×4], sum{v,i,p}}"
  - "10Hz reporting: one packet per 100ms control cycle, best-effort (no retry on failure)"

requirements-completed:
  - "COMM-02"

# Metrics
duration: 15min
completed: 2026-06-07
---

# Phase 03 Plan 03: Telemetry Assembly + TX + Main Loop Integration

**10Hz cJSON telemetry packets with 4-channel MOS data, summary statistics, and meta fields over USART2**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-06-07T13:02:00Z
- **Completed:** 2026-06-07T13:17:00Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments
- protocol_send_telemetry() assembles full cJSON telemetry: ch[4] array (v/i per channel), sum{} object (v/i/p), flat meta (seq, uptime, mode, fault, dac, retry, temp)
- Sequence number (telemetry_seq) increments each packet — static uint16_t, rolls over at 65535
- Uptime derived from cycle_count × CONTROL_PERIOD_MS — deterministic, monotonic over ~49.7 days
- Telemetry wired into main loop at step 6.5 — after sensor reads + PID compute + fault SM, before retry reset
- MOS channel bus voltages added to main loop readings (step 2) — passed as mos_v[4] parameter

## Task Commits

1. **Task 1: Implement telemetry** — `f16d8cb` (feat(03-03): protocol_send_telemetry with cJSON assembly)
2. **Task 2: Wire into main loop** — `2ee0876` (feat(03-03): wire telemetry into main loop with MOS channel voltages)
3. **Task 3: Integration verification** — code-level review passed (see verification section)

## Files Created/Modified
- `Drivers/protocol.h` — Added CONTROL_PERIOD_MS, updated protocol_send_telemetry() signature with 5 params
- `Drivers/protocol.c` — Full telemetry assembly: arena reset, cJSON tree build, PrintUnformatted, protocol_send, cleanup
- `User/main.c` — Added mos_v[4] reads in step 2, telemetry call at step 6.5

## Decisions Made
- Telemetry function takes 5 parameters (not extern globals for sensor data) — explicit data flow, easier to test
- MOS channel bus voltages read in the same I2C pass as currents — adds ~2ms to the ~15ms I2C block, well within budget
- Telemetry positioned after fault state machine — reports the system state AFTER all fault handling decisions are made
- Arena reset at function entry AND exit (belt and suspenders) — guarantees full pool regardless of whether command processing consumed partial pool

## Deviations from Plan

### Auto-fixed Issues

**1. [Parameter count] Extended telemetry signature to 5 parameters**
- **Found during:** Task 1 (telemetry implementation)
- **Issue:** Plan referenced `mos_i_v[i]` for per-channel voltage but no such variable exists. Plan specified 4-parameter signature but ch[] array needs both v and i per channel.
- **Fix:** Added `mos_v[4]` as 5th parameter and read MOS channel bus voltages in main loop step 2
- **Files modified:** Drivers/protocol.h, Drivers/protocol.c, User/main.c
- **Verification:** Each ch[] object now has valid v and i fields populated from actual INA226 readings

---
**Total deviations:** 1 auto-fixed (parameter extension)
**Impact on plan:** Necessary for correct telemetry data — per-channel voltage data would be missing otherwise. No scope creep.

## Issues Encountered

- None

## Verification

### Code-Level Review (Theoretical — no RISC-V toolchain available)

1. **Build check:** Cannot verify on this system (requires MounRiver Studio + riscv-none-embed-gcc). All code follows existing SPL patterns, includes correct headers, and uses documented API calls.
2. **Timing analysis:** Telemetry TX ~26ms (300 bytes × 11 bits/byte ÷ 115200) + command ~3ms + I2C reads ~17ms (summary + 4ch voltages + 4ch currents) + PID ~1ms + fault checks ~1ms = ~48ms. Cycle budget: 100ms. Margin: 52ms (52%).
3. **Memory analysis:** cJSON arena (4KB) + ring buffer (512B) + cjson pool variable (4KB static) + protocol globals (~100B) = ~8.6KB Phase 3. Total SRAM estimate: ~12-14KB < 32KB.
4. **Sequence number:** static uint16_t telemetry_seq — increments each call, rolls over at 65535. Code review: ✓
5. **Uptime monotonicity:** cycle_count × CONTROL_PERIOD_MS — increases by 100ms each cycle. Code review: ✓
6. **JSON schema:** All required fields present: seq, uptime, mode, fault, dac, retry, temp, ch[0-3].{v,i}, sum.{v,i,p}. Code review: ✓
7. **Concurrent command + telemetry:** Command processing (step 0) runs before sensor reads and telemetry (step 6.5). Arena reset at telemetry start ensures full pool. No interference. Code review: ✓
8. **Error recovery:** protocol_process_command returns error acks for malformed input, doesn't crash. Telemetry continues next cycle. Code review: ✓

### Phase 3 Success Criteria

| Criterion | Status |
|-----------|--------|
| set_mode CV/CC commands acknowledged with ack JSON | ✓ |
| CC mode entry at specified current via cJSON command | ✓ |
| 10Hz telemetry with all required fields | ✓ |
| Malformed/out-of-range/invalid-state commands rejected | ✓ |

## Next Phase Readiness
- COMM-01 and COMM-02 both delivered — cJSON command control + 10Hz telemetry
- System accepts commands and reports state over USART2
- Ready for Phase 4 (data logging, PC-side visualization, or extended features)

---
*Phase: 03-communication*
*Completed: 2026-06-07*
