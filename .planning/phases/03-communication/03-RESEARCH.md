# Phase 03: Communication — Research

**Date:** 2026-06-07
**Status:** Complete

---

## Research Overview

This research covers the four areas needed to plan Phase 3: (1) hardware UART mapping — which CH32V303 peripheral serves as "UART0", (2) cJSON library integration strategy for 32K SRAM, (3) ring buffer + ISR architecture patterns already in the codebase, and (4) memory budget analysis. Each section ends with a concrete planning recommendation.

---

## 1. Hardware UART Mapping: "UART0" = USART2

### Finding

The CONTEXT.md uses the logical name "UART0" for the command/telemetry channel. On CH32V303CBT6, the USART/UART peripherals are numbered USART1, USART2, USART3, UART4, UART5, UART6, UART7, UART8. There is no literal "UART0".

**USART1 is already in use** for debug printf (`Debug/debug.c` line 98–104, `DEBUG == DEBUG_UART1`): PA9 as TX (AF push-pull), 115200 bps, APB2 peripheral. This is hard-wired into the `_write()` syscall and must not be repurposed.

**USART2 is the correct mapping for "UART0":**

| Attribute | Value | Source |
|-----------|-------|--------|
| Peripheral | USART2 | `ch32v30x.h` line 1261 |
| Base address | `APB1PERIPH_BASE + 0x4400` | `ch32v30x.h` line 1171 |
| Clock bus | APB1 (`RCC_APB1Periph_USART2`) | `ch32v30x.h` line 5765 |
| IRQ number | 54 (`USART2_IRQn`) | `ch32v30x.h` line 95 |
| ISR name | `USART2_IRQHandler` | `startup_ch32v30x_D8.S` line 78, 176, 261 |
| TX pin | PA2 (AF push-pull) | `debug.c` line 110–113 (existing USART2 pattern) |
| RX pin | PA3 (input / AF) | SPL convention — PA3 is USART2 RX default |
| Reset bit | `RCC_USART2RST` | `ch32v30x.h` line 5706 |

**Pin availability check:** PA2 and PA3 are not used by I2C1 (PB6/PB7), EXTI4 (PA4), USART1 debug (PA9), or DAC8571 (I2C address only). No conflicts.

### Baud Rate Accuracy at 96 MHz

The SPL USART driver computes the divider automatically from `SystemCoreClock`:
- `IntegerDivider = PCLK1 / (16 × BaudRate)` = 96,000,000 / (16 × 115,200) ≈ 52.08
- Fractional divider = 0.08 × 16 ≈ 1.3 → rounded to 1
- Actual baud rate = 96,000,000 / (16 × 52.0625) ≈ 115,246 bps
- Error = (115,246 − 115,200) / 115,200 ≈ 0.04% — **well within tolerance**

### Recommendation for Planning

Map "UART0" → USART2 at 115200 bps, 8N1 + odd parity (`USART_Parity_Odd`), RX+TX mode (`USART_Mode_Rx | USART_Mode_Tx`). Enable RXNE interrupt via `USART_ITConfig(USART2, USART_IT_RXNE, ENABLE)`. Add `USART2_IRQHandler` to `User/ch32v30x_it.c`.

---

## 2. cJSON Library Integration

### Finding

cJSON (https://github.com/DaveGamble/cJSON) is a single `.c` + `.h` MIT-licensed JSON parser. Key attributes for this project:

**Integration approach — vendored source:**
- Place `cJSON.c` and `cJSON.h` in `Drivers/cjson/`
- Add `Drivers/cjson/cJSON.c` to the MounRiver Studio build (right-click → Add existing file, or edit `.cproject`)
- Include path: `Drivers/cjson/` is already within the project tree; include as `#include "../Drivers/cjson/cJSON.h"` from other Drivers/ files, or `#include "cJSON.h"` if include path is added to `.cproject`

**Memory management — static pool with hooks (recommended):**

cJSON allocates/frees tree nodes via `cJSON_malloc`/`cJSON_free` function pointers. The default hooks call `malloc`/`free` from newlib-nano. In 32K SRAM, this risks fragmentation over long runs (>hours of telemetry assembly every 100ms).

**Recommended approach:** Override hooks with a static arena:

```c
/* Static pool for cJSON — avoids heap fragmentation */
#define CJSON_POOL_SIZE 4096  /* 4 KB pool */
static uint8_t cjson_pool[CJSON_POOL_SIZE];
static size_t  cjson_pool_used = 0;

static void *cjson_arena_alloc(size_t sz) {
    void *ptr = &cjson_pool[cjson_pool_used];
    if (cjson_pool_used + sz > CJSON_POOL_SIZE) return NULL;
    cjson_pool_used += sz;
    return ptr;
}

static void cjson_arena_free(void *ptr) {
    /* no-op — arena resets when pool_used reset to 0 */
}
```

This is a **bump allocator**: allocations never fragment, memory is reclaimed by resetting `cjson_pool_used = 0` after each parse/assembly cycle. Simple, deterministic, no fragmentation.

**Alternative (if heap is preferred):** Keep default `malloc`/`free` but validate that newlib-nano's allocator handles the pattern of `cJSON_Parse()` → use tree → `cJSON_Delete()` without fragmentation. This is riskier for long-running operation.

**Parse tree size estimates:**
- Command JSON (~50–100 bytes on wire) → parse tree ~1–2 KB (each key/value/object/array is a `cJSON` struct, ~56 bytes each)
- Telemetry assembly (~300 bytes on wire) → building tree from scratch each cycle → tree ~4–6 KB during assembly, freed after `cJSON_Print()` + TX
- **Worst case:** 4 KB pool covers both command and telemetry trees if never held simultaneously. If a command arrives mid-telemetry assembly, the pool doubles. **Recommend 4 KB pool + process commands before telemetry assembly** (no overlap, per D-19).

**cJSON_Print buffer:** `cJSON_Print()` allocates the output string via `cJSON_malloc`. A 300-byte telemetry packet needs a ~350-byte allocation (some overhead for escapes). With the arena, this comes from the same pool and is freed after TX.

### Build Integration

The `.cproject` file manages include paths and source files in MounRiver Studio. Two changes needed:
1. Add `Drivers/cjson/cJSON.c` to the source file list
2. Ensure `Drivers/cjson/` is on the include path (or use relative includes)

### Recommendation for Planning

Vendor cJSON into `Drivers/cjson/`. Implement static arena allocator with 4 KB pool. Override `cJSON_InitHooks()` in `protocol_init()`. Process commands at the start of each cycle (before telemetry assembly) to avoid concurrent cJSON tree allocations.

---

## 3. Ring Buffer and ISR Architecture

### Finding

The codebase already has a proven ISR-to-main-loop communication pattern:

**Existing pattern (EXTI4 ISR → main loop):**
```c
/* ch32v30x_it.c */
volatile uint8_t  fault_triggered = 0;    /* ISR sets, main loop reads+clears */
volatile uint16_t fault_source_mask = 0;  /* ISR writes channel bits */
volatile uint16_t last_dac_value = 0;     /* ISR captures for diagnostic print */
```

The ISR does minimal work (read registers, set flags), and the main loop handles heavyweight processing (state machine, diagnostic prints). This is the correct pattern for UART RX as well.

**Ring buffer design for UART RX:**

```c
#define RX_BUF_SIZE  512

static volatile uint8_t rx_buf[RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;  /* main loop consumer — next byte to read */
static volatile uint16_t rx_tail = 0;  /* ISR producer — next write position */
```

- **ISR (`USART2_IRQHandler`):** On RXNE, read `USART2->RDR`, write to `rx_buf[rx_tail]`, increment `rx_tail` (wrap at `RX_BUF_SIZE`). If `rx_tail + 1 == rx_head`, buffer full → discard byte + set overflow flag.
- **Main loop consumer (`protocol_poll()`):** Called at start of each control cycle. Scans from `rx_head` for `\n`. When found: copy line to command buffer, advance `rx_head` past `\n`, return pointer to command buffer. If no `\n` found, return NULL.
- **Overflow handling:** If the ring buffer fills (PC sends data faster than 10Hz processing), set `rx_overflow` flag. On next poll, flush buffer (reset head=tail=0) and send error ack: `{"ack":"error","code":103,"msg":"RX buffer overflow"}`.

**Error handling in ISR:**
- Parity error (`USART_FLAG_PE`): read RDR to clear, discard byte, set `rx_parity_err` flag
- Framing error (`USART_FLAG_FE`): read RDR to clear, discard byte
- Overrun error (`USART_FLAG_ORE`): read SR then RDR to clear
- Per D-15: errors logged via debug printf **from main loop**, not ISR (printf in ISR is dangerous)

**512-byte ring buffer capacity:** At 115200 bps, one byte ≈ 87 µs. 512 bytes = ~45 ms of continuous RX before overflow. With 100ms processing interval, this holds ~4–5 typical commands (50–100 bytes each). Adequate.

### USART2_IRQHandler Priority

USART2 IRQ should be lower priority than EXTI4 (0x01):
- `NVIC_SetPriority(USART2_IRQn, 0x02);` — preempted by EXTI4 ALARM (critical safety)
- USART2 RXNE latency: ~87 µs between bytes at 115200. ISR execution time ~2–5 µs (read RDR, write ring buffer, increment tail). Well within margin.

### Recommendation for Planning

Implement 512-byte ring buffer with `volatile` head/tail on USART2. ISR pushes bytes and handles errors (parity/framing/overrun → discard). Main loop `protocol_poll()` scans for `\n` delimiter at start of each control cycle. Follow the existing `fault_triggered` pattern for ISR-to-main communication.

---

## 4. Memory Budget Analysis

### Finding

Current SRAM usage (Phase 1 + 2):
| Item | Size | Location |
|------|------|----------|
| Global data (.data + .bss) | ~4 KB | RAM |
| Stack (linker script) | 2 KB | Top of RAM |
| Heap (_sbrk — newlib-nano) | Variable | Between _end and _heap_end |
| INA226 readings (5 × float) | 20 bytes | .bss |
| PID instances (2 × PID_Instance) | ~128 bytes | .bss |
| Fault register + flags | ~10 bytes | .bss |
| Debug printf TxBuf | implicit | stack |

**Phase 3 additions:**
| Item | Size | Notes |
|------|------|-------|
| Ring buffer (`rx_buf`) | 512 bytes | Static array in `protocol.c` |
| cJSON arena pool | 4096 bytes | Static array — bump allocator arena |
| Command line buffer | 256 bytes | `protocol_poll()` copies line here |
| Telemetry assembly buffer | 512 bytes | `cJSON_Print()` output (or stack) |
| `tx_buf` for telemetry TX | 512 bytes | Optional: pre-formatted for DMA (future). Blocking TX doesn't need this. |
| Ring buffer indices + flags | ~16 bytes | head, tail, overflow, parity_err, etc. |
| Sequence number | 2 bytes | `uint16_t seq` |
| **Phase 3 total** | **~5.3 KB** | |
| **Estimated total SRAM** | **~9–11 KB** | ~4 KB existing + ~5.3 KB new |

**32 KB SRAM — 11 KB used = 21 KB free.** Well within budget. The cJSON arena pool (4 KB) is the largest single consumer; it can be reduced to 2 KB if memory pressure arises (2 KB covers a 300-byte telemetry tree + a 100-byte command tree sequentially).

### Heap Considerations

The `_sbrk()` implementation (debug.c line 239–250) provides heap between `_end` and `_heap_end` (start of stack). If using the static arena for cJSON, **no heap is needed for Phase 3**. The default `malloc`/`free` in newlib-nano remain available but unused for protocol operations.

### Flash Budget

| Item | Size (estimate) | Notes |
|------|-----------------|-------|
| cJSON library | ~10–12 KB | Full parser + printer, all features |
| `protocol.c` | ~3–5 KB | Command dispatch, telemetry assembly, ring buffer ops |
| Total Phase 3 | ~15 KB | |
| **Estimated total Flash** | **~50–60 KB** | Phase 1 (~25 KB) + Phase 2 (~15 KB) + Phase 3 (~15 KB) |

**128 KB Flash — 60 KB used = 68 KB free.** Plenty of room.

### Recommendation for Planning

Static allocation for all Phase 3 buffers. Use 4 KB cJSON arena pool. Monitor actual Flash usage after build — if cJSON is too large, disable unused features (`cJSON_Utils.c` not needed, `cJSON_GetErrorPtr()` optional).

---

## 5. Baud Rate Parity: ">115200 with odd parity"

### Finding

CONTEXT.md D-17 specifies >115200 bps. The standard baud rates above 115200 are:
- 230400 (error ~0.08% at 96 MHz)
- 460800 (error ~0.16%)
- 921600 (error ~0.78%)

However, the CONTEXT says "115200 bps" and the success criteria mention ">115200bps", meaning ≥115200. The D-17 decision settles at 115200 exactly. **Standard 115200 is sufficient** — it is ~12 KB/s, which means a 300-byte telemetry packet transmits in ~26ms, well within the 100ms budget. Commands at 50–100 bytes take ~4–9ms.

Higher baud rates increase error margin and are unnecessary for 10Hz telemetry. Stick with 115200.

---

## 6. USART2 Pin Initialization Pattern

### Finding

The SPL USART init follows a consistent pattern (from `debug.c` USART2 example at lines 106–113, 137–139):

```c
/* Clock enable */
RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

/* TX pin: PA2, AF push-pull */
GPIO_InitTypeDef GPIO_InitStructure = {0};
GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
GPIO_Init(GPIOA, &GPIO_InitStructure);

/* RX pin: PA3, floating input (UART RX must be input) */
GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
GPIO_Init(GPIOA, &GPIO_InitStructure);

/* USART init with parity */
USART_InitTypeDef USART_InitStructure = {0};
USART_InitStructure.USART_BaudRate = 115200;
USART_InitStructure.USART_WordLength = USART_WordLength_8b;
USART_InitStructure.USART_StopBits = USART_StopBits_1;
USART_InitStructure.USART_Parity = USART_Parity_Odd;    /* odd parity per D-17 */
USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  /* both RX and TX */
USART_Init(USART2, &USART_InitStructure);

/* Enable RXNE interrupt */
USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

/* Enable USART */
USART_Cmd(USART2, ENABLE);

/* NVIC */
NVIC_SetPriority(USART2_IRQn, 0x02);
NVIC_EnableIRQ(USART2_IRQn);
```

**Note on parity:** With `USART_WordLength_8b` + `USART_Parity_Odd`, the UART sends 1 start + 8 data + 1 parity + 1 stop = 11 bits per byte. The PC must match (8N1 + odd parity, or equivalently 8 bits + parity). This is what the requirements call for.

### Recommendation for Planning

Follow the existing `debug.c` USART2 init pattern exactly, adding parity (odd), RX mode, RXNE interrupt, and NVIC configuration. The TX blocking pattern (`while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET); USART_SendData(USART2, byte);`) is used for telemetry — same as `_write()` in debug.c.

---

## 7. Protocol Module API Design

### Finding

Following the Drivers/ module pattern established in Phase 1 (D-01 to D-04):
- New files: `Drivers/protocol.c` + `Drivers/protocol.h`
- Lowercase naming, include guard `__PROTOCOL_H`
- Extern C wrapper for C++ compatibility
- `#include "../Drivers/protocol.h"` from main.c (matching existing pattern at main.c:21–27)

### Proposed API

```c
/* protocol.h */
#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <stdint.h>

/* Initialize UART0 (USART2), ring buffer, cJSON hooks */
void protocol_init(void);

/* Poll ring buffer for complete \n-delimited line. Called at start of each
 * control cycle. Returns pointer to null-terminated command string if a
 * complete line was found, NULL otherwise. The returned pointer is valid
 * until the next call to protocol_poll(). */
const char *protocol_poll(void);

/* Send a null-terminated JSON string via USART2. Blocks until transmission
 * complete (~26ms for a 300-byte packet). */
void protocol_send(const char *json_str);

/* Send telemetry packet. Reads current values from globals (bus_v, bus_i,
 * etc.) and INA226 getter results, assembles JSON, and transmits. */
void protocol_send_telemetry(void);

#endif /* __PROTOCOL_H */
```

### Recommendation for Planning

Keep API minimal — four functions. `protocol_init()` encapsulates USART2 init + ring buffer + cJSON hooks. `protocol_poll()` is the only RX entry point for the main loop. `protocol_send()` is the generic TX. `protocol_send_telemetry()` is the convenience wrapper that assembles the telemetry JSON from current sensor readings.

---

## 8. Command Dispatch Integration

### Finding

The command parsing and dispatch flow per D-19:
1. `protocol_poll()` returns a complete line (or NULL)
2. `cJSON_Parse(line)` → `cJSON *root`
3. Extract `cmd` field: `cJSON_GetObjectItem(root, "cmd")->valuestring`
4. Dispatch via `strcmp`:
   - `"set_mode"` → extract `mode` + `value`, validate, call `engage_cv()`/`engage_cc()`, send ack
   - `"clear_fault"` → call `fault_clear()`, send ack
   - `"get_status"` → build status JSON from globals, `protocol_send()`
   - `"get_info"` → build info JSON from `#define`s, `protocol_send()`
   - Unknown → send error ack (code 105)
5. `cJSON_Delete(root)` to free parse tree
6. If parsing or validation fails → send error ack with appropriate code

### Existing integration points (from main.c):
- `engage_cv(float target)` at line 282 — called directly, sets `system_mode = MODE_CV`
- `engage_cc(float target)` — same pattern, sets `system_mode = MODE_CC`
- `fault_clear()` — declared in fault.h:118, stub with TODO for Phase 3
- `system_mode`, `cv_target_voltage`, `cc_target_current` — globals in main.c
- `last_control_tick` — 100ms timing in main loop (line 296–301)

### State gating (D-14):
- `set_mode` → allowed in IDLE, CV, CC; rejected in FAULT (must clear_fault first)
- `clear_fault` → allowed only in FAULT
- `set_mode` with same mode + same value → idempotent, accepted but no-op

### Recommendation for Planning

Implement `protocol_process_command()` in `protocol.c` as the dispatch function. It receives the parsed `cJSON *root`, handles dispatch, sends responses, and cleans up. `protocol_poll()` calls it internally. The main loop only calls `protocol_poll()` at the start of each cycle — no other changes to the control loop structure.

---

## 9. Telemetry Packet Assembly

### Finding

Per D-07, the telemetry JSON structure is:
```json
{
  "seq": 1234,
  "uptime": 120000,
  "mode": "CV",
  "fault": 0,
  "dac": 32768,
  "retry": 0,
  "temp": 0.0,
  "ch": [
    {"v": 12.34, "i": 2.50},
    {"v": 12.35, "i": 2.48},
    {"v": 12.33, "i": 2.51},
    {"v": 12.36, "i": 2.49}
  ],
  "sum": {"v": 12.34, "i": 10.00, "p": 123.4}
}
```

**Assembly flow each 100ms cycle:**
1. `cJSON_CreateObject()` → root
2. Add flat fields: `cJSON_AddNumberToObject(root, "seq", seq++)`
3. Build `ch` array: 4 × `cJSON_CreateObject()` with `v`/`i` from MOS current readings
4. Build `sum` object: `v`/`i`/`p` from summary INA226
5. `cJSON_Print(root)` → output string
6. `protocol_send(output_str)` → blocking TX via USART2
7. Reset cJSON arena pool (`cjson_pool_used = 0`) — all cJSON memory reclaimed at once

**Timing:** cJSON tree assembly ~0.5–1ms at 96 MHz. Blocking TX ~26ms. Total telemetry overhead ~27ms. Command processing (if a command arrived) ~1–3ms. **Total Phase 3 overhead ~30ms, well within 100ms budget.**

### Sequence number and uptime
- `seq`: `uint16_t`, increments each telemetry packet, rolls over at 65535
- `uptime`: `uint32_t` milliseconds since boot, derived from `SysTick->CNT` and cycle count

### Recommendation for Planning

Implement `protocol_send_telemetry()` as a single function that reads from the same sensor variables used by the control loop. Call after the state machine dispatch (between current main.c steps 6 and 9). The sequence number is a static variable in `protocol.c`. Uptime is computed from `cycle_count * CONTROL_PERIOD_MS`.

---

## 10. Error Code Numbering

### Finding

Per D-12, error codes are grouped by category:

| Range | Category | Codes |
|-------|----------|-------|
| 100 | Parse errors | 101 = invalid JSON syntax, 102 = incomplete JSON, 103 = buffer overflow |
| 200 | Validation errors | 201 = missing required field, 202 = value out of range, 203 = invalid mode string, 204 = unknown command, 205 = bad field type |
| 300 | State errors | 301 = command rejected in current mode, 302 = transition not allowed, 303 = fault active (clear first) |

### Recommendation for Planning

Define error codes as `#define` constants in `protocol.h`. Keep the human-readable `msg` strings as `static const char *` arrays in `protocol.c`.

---

## 11. Risks & Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| cJSON memory fragmentation with default malloc | Medium | High — eventual heap exhaustion | Static arena allocator — no fragmentation by design |
| UART RX overrun at high command rate | Low | Low — missed command, error ack sent | 512B ring buffer holds ~4 commands. PC should wait for ack before next command |
| Telemetry TX blocks too long | Low | Medium — missed control deadline | 300B at 115200 = 26ms. If packets grow larger, switch to interrupt-driven TX. Monitor. |
| cJSON parse failure on malformed input | High (expected) | Low — error ack, no crash | Validate `cJSON_Parse()` return before accessing tree. Robust error path. |
| `clear_fault` while not in FAULT | Low | Low — rejected with error ack | State gating in dispatch before calling `fault_clear()` |
| `set_mode` race with fault | Medium | Medium — command overridden by fault | Command processing before sensor read in cycle. If fault_triggered set during command processing, fault takes priority (already in main loop). |
| USART2 baud rate divider error | Very Low | Low | SPL computes divider from SystemCoreClock = 96 MHz. Error < 0.05%. Clock stability from HSE crystal. |

---

## 12. Open Questions

1. **`get_info` chip_id:** The CONTEXT.md shows `"chip_id":"ABCD1234"` as an example. The actual chip ID is available from `DBGMCU_GetCHIPID()` (already called in `main()` line 199, returns `uint32_t`). Should format as 8-char hex string. **Resolved: use `DBGMCU_GetCHIPID()` formatted as `"XXXXXXXX"`.**

2. **cJSON `cJSON_Print()` vs `cJSON_PrintUnformatted()`:** `cJSON_Print()` adds whitespace (newlines, indentation). For machine-to-machine protocol, use `cJSON_PrintUnformatted()` (compact, single line, no unnecessary whitespace). Saves ~20–50 bytes per telemetry packet. **Resolved: use `cJSON_PrintUnformatted()` for both telemetry and command responses.**

3. **Inter-byte timeout for incomplete JSON:** CONTEXT.md mentions this but no specific value. With 10Hz polling, the worst case is ~100ms between polls. If PC sends partial command then stalls, the incomplete line sits in the ring buffer until the next `\n`. No timeout needed — just wait for `\n`. If the ring buffer wraps before `\n` arrives, the partial line is lost (overflow). **Resolved: no inter-byte timeout. Relies on newline framing and PC sending complete lines.**

---

## 13. Recommendations for Planning

1. **Plan structure:** Split into 3 plans:
   - Plan 01: `Drivers/protocol.c/.h` scaffolding + USART2 init + ring buffer + ISR
   - Plan 02: cJSON integration + command parsing + dispatch (`set_mode`, `clear_fault`, `get_status`, `get_info`)
   - Plan 03: Telemetry assembly + TX + main loop integration

2. **Wave 1** (Plans 01, 02): Plan 01 has no dependencies. Plan 02 depends on Plan 01 (needs `protocol_poll()`). Can run sequentially in Wave 1.

3. **Wave 2** (Plan 03): Depends on Plan 02 (needs the cJSON arena, command dispatch verified). Telemetry is the final integration step.

4. **Key files to modify:**
   - NEW: `Drivers/protocol.c`, `Drivers/protocol.h`
   - NEW: `Drivers/cjson/cJSON.c`, `Drivers/cjson/cJSON.h` (vendored)
   - MODIFY: `User/main.c` — add `protocol_init()`, `protocol_poll()` call, telemetry TX
   - MODIFY: `User/ch32v30x_it.c` — add `USART2_IRQHandler`
   - MODIFY: `User/ch32v30x_it.h` — add protocol.h include
   - MODIFY: `.cproject` — add protocol.c + cJSON.c to build

5. **Files to read before planning:**
   - `Drivers/fault.c` — `fault_clear()` stub implementation
   - `Startup/startup_ch32v30x_D8.S` — USART2 weak handler placement
   - `Peripheral/src/ch32v30x_usart.c` — USART_Init, USART_SendData, USART_ReceiveData patterns

---

## RESEARCH COMPLETE
