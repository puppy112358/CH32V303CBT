---
phase: 01-hardware-foundation
plan: 03
subsystem: debug
tags: [USB-CDC, CDC-ACM, CH32V303, dual-channel, WCH, USBFS]

# Dependency graph
requires:
  - phase: 01-02
    provides: INA226 current/voltage/power read loop, DAC8571 mid-scale output, USART1 printf data stream
provides:
  - USB-CDC virtual COM port debug output via usb_printf()
  - Dual-channel debug architecture (USART1 physical + USB-CDC virtual)
  - Simplified CDC ACM device driver (no UART bridge, EP3-only bulk IN)
  - Non-blocking usb_printf() with drop-on-busy semantics for super-loop safety
  - USBFS clock initialization (48MHz via PLLCLK_Div2 from 96MHz SystemCoreClock)
affects: [phase 03 — cJSON UART0 protocol, phase 04 — integration and final testing]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Dual-channel debug: printf() to USART1 (always-on) + usb_printf() to USB-CDC (connected-optional)"
    - "Simplified CDC ACM: EP1 (interrupt IN, notifications) + EP3 (bulk IN, debug data) — no EP2 OUT"
    - "Non-blocking USB output: usb_printf() checks USBFS_Endp_Busy[DEF_UEP3], drops silently when busy"
    - "Minimal WCH SimulateCDC adaptation: UART bridge, EP2 OUT, TIM2, DMA references removed"

key-files:
  created:
    - Drivers/usb_cdc.h — High-level USB-CDC API (usb_cdc_init, usb_printf with __attribute__((format(printf))))
    - Drivers/usb_cdc.c — usb_printf(): vsnprintf formatting, USBFS_Endp_Busy check, USBFS_Endp_DataUp(DEF_UEP3, ...)
    - Drivers/usb_desc.h — CDC descriptor defines (VID=0x1A86, PID=0xFE0C), endpoint sizes, extern declarations
    - Drivers/usb_desc.c — CDC ACM descriptors: config modified (EP2 OUT removed, total length 0x3C, bNumEndpoints=1)
    - Drivers/ch32v30x_usbfs_device.h — Endpoint definitions, buffer declarations, USBFS_Endp_DataUp API (no UART.h)
    - Drivers/ch32v30x_usbfs_device.c — Simplified USBFS driver: EP1 TX+EP3 TX only, IRQ handler without UART bridge
  modified:
    - User/main.c — Added usb_cdc_init() call, 9x usb_printf() calls alongside printf() for dual-channel output
    - User/ch32v30x_conf.h — Added #include "ch32v30x_usb.h" to enable USB peripheral
    - User/ch32v30x_it.c — Added USBFS_IRQHandler declaration (implementation in Drivers/)

key-decisions:
  - "Adapted WCH SimulateCDC with reduced footprint — removed UART bridge, EP2 OUT, TIM2, DMA, and UART struct to fit 32K SRAM budget"
  - "Used WCH default VID=0x1A86/PID=0xFE0C for development — no custom USB-IF vendor ID needed for debug interface"
  - "usb_printf() drops data silently when Endp_Busy — non-blocking by design, super-loop continues without USB involvement"
  - "CDC SET_LINE_CODING parsed but UART reconfiguration removed — host is ACK'd for COM port compatibility but no state is changed"
  - "Buffer size USB_CDC_BUF_SIZE=128 (2x USB max packet size) — balances stack usage vs throughput for ~2Hz data output"

patterns-established:
  - "Dual-channel debug: printf() to USART1 (always-on, PA9 pin) + usb_printf() to USB-CDC (virtual COM port, PA11/PA12 USB)"
  - "Simplified CDC ACM: EP1 (interrupt IN, CDC notifications) + EP3 (bulk IN, debug data) — EP2 OUT removed entirely"
  - "Non-blocking USB: usb_printf() checks USBFS_Endp_Busy[DEF_UEP3], drops when busy — no blocking, no buffering, super-loop-safe"
  - "Minimal WCH adaptation: started from SimulateCDC example, stripped UART bridge/TIM2/DMA/EP2 OUT, kept EP0 control transfers"

requirements-completed: [COMM-03]

# Metrics
duration: 12min (code implementation) + hardware verification (user time)
completed: 2026-06-05
---

# Phase 1 Plan 3: USB-CDC Summary

**Dual-channel debug output via simplified CDC ACM — USART1 printf (PA9) and USB-CDC usb_printf (virtual COM port) operate simultaneously, with non-blocking drop-on-busy semantics for super-loop safety**

## Performance

- **Duration:** ~3 min (code generation) + ~30 min (hardware verification by user)
- **Started:** 2026-06-05T07:40:00Z
- **Completed:** 2026-06-05T08:00:00Z
- **Tasks:** 3
- **Files created:** 6
- **Files modified:** 3

## Accomplishments
- Created 6 USB-CDC driver files in Drivers/ by adapting WCH SimulateCDC example — UART bridge, EP2 OUT, TIM2, and DMA dependencies all removed for minimal footprint
- Wired dual-channel debug output into main loop: 9 usb_printf() calls output INA226 voltage/current/power and DAC8571 status alongside existing USART1 printf() calls
- Hardware-verified: USB-CDC COM port enumerates on PC (VID=0x1A86, PID=0xFE0C), both USART1 and USB-CDC channels show identical INA226/DAC data at ~2Hz
- Verified disconnect/reconnect resilience: USART1 output continues uninterrupted when USB cable is disconnected; USB-CDC output resumes when reconnected

## Task Commits

Each task was committed atomically:

1. **Task 1: Create USB-CDC driver files — adapt WCH SimulateCDC to Drivers/** - `d50b95c` (feat)
2. **Task 2: Wire USB-CDC into project — main.c, ch32v30x_conf.h, ch32v30x_it.c** - `e0fc18b` (feat)
3. **Task 3: Verify USB-CDC enumeration and dual-channel output** — Hardware verified (checkpoint approved by user)

*Note: Commit hashes are from the worktree cherry-pick (original hashes: 64ab7e9 and d02d16e on worktree-agent-a61243e2e42b097af).*

## Files Created/Modified

### Created (Drivers/)
- `Drivers/usb_cdc.h` — USB_CDC_BUF_SIZE define (128 bytes), usb_cdc_init() and usb_printf() declarations with format attribute
- `Drivers/usb_cdc.c` — usb_printf(): vsnprintf formatting, USBFS_Endp_Busy check, USBFS_Endp_DataUp(DEF_UEP3), drop-on-busy
- `Drivers/usb_desc.h` — DEF_USB_VID=0x1A86, DEF_USB_PID=0xFE0C, endpoint size defines, extern descriptor declarations
- `Drivers/usb_desc.c` — CDC ACM descriptors: config modified (EP2 OUT removed, 0x3C total length, bNumEndpoints=1), all string descriptors preserved
- `Drivers/ch32v30x_usbfs_device.h` — DEF_UEP0-7, endpoint register macros, buffer declarations, USBFS_Endp_DataUp API (no UART.h)
- `Drivers/ch32v30x_usbfs_device.c` — USBFS_RCC_Init (48MHz PLLCLK_Div2), USBFS_Device_Endp_Init (EP1+EP3 only), USBFS_IRQHandler (no UART bridge, CDC_SET_LINE_CODING silently accepted)

### Modified
- `User/main.c` — #include "../Drivers/usb_cdc.h", usb_cdc_init() call before while(1), 9x usb_printf() calls alongside printf() for dual-channel output
- `User/ch32v30x_conf.h` — Added #include "ch32v30x_usb.h" to enable USB peripheral register definitions
- `User/ch32v30x_it.c` — Added USBFS_IRQHandler declaration (implementation in Drivers/ch32v30x_usbfs_device.c)

## Verification Results

All automated checks from the plan passed:

| Check | Result |
|-------|--------|
| 6 Driver files exist in Drivers/ | PASS |
| usb_printf calls in main.c (>=3) | PASS (9 calls) |
| ch32v30x_usb.h in ch32v30x_conf.h | PASS (1 occurrence) |
| USBFS_IRQHandler in ch32v30x_it.c | PASS (1 occurrence) |
| USBFS_Endp_DataUp in usb_cdc.c | PASS (2 occurrences) |
| USBFS_Endp_Busy in usb_cdc.c | PASS (1 occurrence) |
| No UART in ch32v30x_usbfs_device.c | PASS (only in comment, line 351) |
| USB_CDC_BUF_SIZE = 128 | PASS |
| Config descriptor 0x3C (EP2 OUT removed) | PASS |

Hardware verification (Task 3, approved by user):
- USB-CDC COM port enumerates on PC
- Both USART1 and USB-CDC terminals show identical INA226/DAC data
- USART1 continues uninterrupted when USB disconnected
- USB-CDC resumes output when USB reconnected

## Decisions Made

1. **Simplified CDC from WCH SimulateCDC** — Removed UART bridge, EP2 OUT endpoint, TIM2 timer, DMA channel, and UART struct. Kept EP1 (interrupt IN for CDC notifications) and EP3 (bulk IN for debug data). This reduces flash usage and eliminates any UART contention with UART0 (Phase 3 cJSON channel).

2. **Non-blocking usb_printf() with silent drop** — When USBFS_Endp_Busy[DEF_UEP3] is set, usb_printf() returns -1 without queuing or blocking. This is intentional: the super-loop must never stall waiting for USB. At ~2Hz output rate, occasional drops are invisible.

3. **CDC SET_LINE_CODING accepted silently** — The 7-byte line coding request is parsed from USBFS_EP0_Buf but no UART is reconfigured. The host receives ACK (required for COM port enumeration) but parameters are discarded. This avoids the SimulateCDC pattern of reconfiguring UART2 on every host-side terminal open.

4. **Dual-channel architecture** — printf() to USART1 (physical, always works) is the primary debug channel. usb_printf() to USB-CDC (virtual, USB-connected-dependent) is additive. Both channels output identical format strings for consistency.

## Deviations from Plan

### Minor Spec Variance

**1. usb_cdc.c line count (71 vs plan min_lines:80)**
- **Found during:** Task 1 execution
- **Issue:** Plan specified `min_lines: 80` for usb_cdc.c, but the implementation is 71 lines. All required functionality is present: vsnprintf formatting, USBFS_Endp_Busy check, USBFS_Endp_DataUp call, drop-on-busy return, and both usb_cdc_init() and usb_printf() functions.
- **Fix:** None needed — all acceptance criteria met. The 80-line estimate overstated the lines needed for a single-purpose printf-to-USB function with only 2 function bodies. Hardware verification confirmed correct operation.
- **Files modified:** N/A
- **Verification:** All 9 usb_printf calls in main.c deliver output correctly (hardware verified)
- **Committed in:** Already in Task 1 commit

---

**Total deviations:** 1 (minor spec variance — no functionality impact)

## Issues Encountered

- **Worktree reset on continuation:** When this continuation agent spawned, the worktree was reset to base commit 161f5c2 (before Task 1 and Task 2 commits). Both implementation commits were cherry-picked from worktree-agent-a61243e2e42b097af onto the current branch — no code was regenerated.
- **Plan line count estimate:** usb_cdc.c came in at 71 lines vs the plan's `min_lines: 80`. The estimate assumed more inline comments/whitespace than the clean implementation required. All functionality is present and hardware-verified.

## User Setup Required

None — USB-CDC uses standard CDC ACM class driver. Windows 10+ and Linux include built-in CDC ACM support. Windows 7/8 may require the WCH CDC driver from https://www.wch.cn/downloads/CH343SER_EXE.html (not needed for modern systems).

## Next Phase Readiness

- **Phase 2 ready:** USB-CDC debug channel is fully operational. The dual-channel architecture (USART1 + USB-CDC) provides redundancy for Phase 2 development. UART0 (PA2/PA3) remains available for the cJSON command protocol in Phase 3.
- **No blockers:** USB-CDC does not consume UART0 pins or conflict with the Phase 3 cJSON command channel.

## Threat Flags

No new threat surfaces beyond those documented in the plan's threat model. All 8 threats (T-03-01 through T-03-08) were addressed as designed: descriptors are compile-time const, buffer is bounded by vsnprintf, busy-check prevents blocking, SET_LINE_CODING is parsed but no UART is reconfigured, and USB disconnect/reconnect is handled gracefully.

---
*Phase: 01-hardware-foundation*
*Plan: 03 — USB-CDC*
*Completed: 2026-06-05*
