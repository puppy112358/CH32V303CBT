---
phase: 03-communication
plan: 01
subsystem: communication
tags: [usart, ring-buffer, interrupt, protocol, uart, ch32v303]

# Dependency graph
requires:
  - phase: 02
    provides: "Fault state machine, system mode enum, EXTI4 overcurrent protection — protocol must coexist with fault ISR at priority 0x01"
provides:
  - "USART2 driver at 115200 bps 8N1+odd parity with RXNE interrupt"
  - "512-byte volatile ring buffer with overflow/flush semantics"
  - "Newline-delimited line extraction via protocol_poll()"
  - "Blocking TX via protocol_send() with TC flag polling"
  - "protocol_send_telemetry() stub for Plan 03 wiring"
affects: [03-02 (cJSON parser), 03-03 (command dispatch)]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "USART2 interrupt-driven RX with ring buffer (ISR pushes bytes, main loop extracts lines)"
    - "Drivers/ module header convention: include guard, C++ extern block, docblock-style declarations"
    - "NVIC priority ladder: EXTI4@0x01 (highest) > USART2@0x02 > SysTick"
    - "Protocol error code ranges: 1xx=parse, 2xx=validation, 3xx=state rejection"

key-files:
  created:
    - "Drivers/protocol.h"
    - "Drivers/protocol.c"
  modified:
    - "User/ch32v30x_it.h"
    - "User/ch32v30x_it.c"
    - "User/main.c"

key-decisions:
  - "Ring buffer variables are non-static (global linkage) so the ISR in ch32v30x_it.c can access them via extern — C89 inline access, no function call overhead in ISR"
  - "RX_BUF_SIZE and LINE_BUF_SIZE defined in protocol.h for both protocol.c and ISR consumption"
  - "Parity/framing errors counted per-event in ISR (not filtered) — main loop can report cumulative stats"

patterns-established:
  - "Drivers/ module header pattern: __PROTOCOL_H guard, C++ extern, error codes as #defines, function docblocks"
  - "ISR-to-driver coupling: extern volatile variables shared between protocol.c and ch32v30x_it.c"

requirements-completed:
  - "COMM-01"

# Metrics
duration: 15min
completed: 2026-06-07
---

# Phase 03 Plan 01: Protocol Module Scaffold + USART2 Init + Ring Buffer + ISR

**USART2 driver at 115200 bps 8N1+odd parity with interrupt-driven 512-byte RX ring buffer and newline-delimited line extraction**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-06-07T12:22:10Z
- **Completed:** 2026-06-07T12:37:00Z
- **Tasks:** 4
- **Files modified:** 5

## Accomplishments
- Drivers/protocol.h with 11 error code #defines (1xx/2xx/3xx ranges), 4 public API declarations, and C++ extern block
- Drivers/protocol.c with USART2 init (PA2=TX, PA3=RX, 115200 bps, odd parity), 512-byte volatile ring buffer, protocol_poll() line extraction, protocol_send() blocking TX
- USART2_IRQHandler ISR in ch32v30x_it.c with RXNE byte push, wrap-around indexing, overflow/flush on buffer full, and PE/FE/ORE error counting
- protocol_init() wired into main.c after USB-CDC init and before PID init — system now starts in MODE_IDLE (engage_cv removed)

## Task Commits

Each task was committed atomically:

1. **Task 1: Create protocol.h** — `7a276df` (feat(03-01): create protocol.h with error codes, API declarations, and C++ extern block)
2. **Task 2: Create protocol.c** — `bb416f5` (feat(03-01): create protocol.c with USART2 init, 512B ring buffer, and polling/send functions)
3. **Task 3: USART2_IRQHandler ISR** — `d29f941` (feat(03-01): add USART2_IRQHandler ISR with RX ring buffer push and error handling)
4. **Task 4: Wire into main.c** — `aa64d4b` (feat(03-01): wire protocol_init() into main.c, remove Phase 2 test engage)

## Files Created/Modified
- `Drivers/protocol.h` — 11 error codes (1xx/2xx/3xx), RX_BUF_SIZE/LINE_BUF_SIZE, 4 API declarations
- `Drivers/protocol.c` — USART2 init, 512B ring buffer, protocol_poll(), protocol_send(), telemetry stub
- `User/ch32v30x_it.h` — Added protocol.h include
- `User/ch32v30x_it.c` — USART2_IRQHandler ISR with RXNE/PE/FE/ORE handling, extern ring buffer references
- `User/main.c` — protocol_init() call after USB-CDC, engage_cv(5.0f) removed (MODE_IDLE start)

## Decisions Made
- Ring buffer variables are non-static (global linkage) so the ISR can access them via extern — avoids function call overhead inside the interrupt handler
- RX_BUF_SIZE added to protocol.h (not just protocol.c) so the ISR can use the symbolic constant instead of magic number 512
- USART2 NVIC priority set to 0x02 (below EXTI4 at 0x01) so hardware fault protection always preempts UART ISR
- System now boots into MODE_IDLE — awaits cJSON commands via USART2 rather than auto-engaging CV mode

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

None.

## Next Phase Readiness
- Physical communication channel established — USART2 RX/TX functional
- COMM-01 RX path foundation ready for cJSON parsing (Plan 03-02)
- Line-oriented protocol (newline delimiter) ready for command framing

---
*Phase: 03-communication*
*Completed: 2026-06-07*
