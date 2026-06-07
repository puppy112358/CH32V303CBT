# Phase 3: Communication - Context

**Gathered:** 2026-06-07
**Status:** Ready for planning

## Phase Boundary

The electronic load is remotely controllable via cJSON commands over UART0 and reports real-time telemetry at 10Hz. A new `Drivers/protocol.c/.h` module handles UART0 interrupt-driven RX (ring buffer), newline-delimited cJSON parsing, command dispatch into the existing PID/state machine, and telemetry packet assembly + transmission. cJSON library is integrated into the build. No WS2812 LEDs (Phase 4), no fan control (Phase 4).

The control loop (100ms SysTick-gated super-loop from Phase 2) is extended to: poll for incoming commands at the start of each cycle → parse + dispatch → read sensors → PID compute → DAC update → fault checks → assemble + send telemetry.

## Implementation Decisions

### Command Protocol

- **D-01:** Four cJSON commands supported: `set_mode`, `clear_fault`, `get_status`, `get_info`.
- **D-02:** Every command receives a JSON acknowledgment: `{"ack":"ok"}` on success, `{"ack":"error","code":N,"msg":"description"}` on failure.
- **D-03:** `set_mode` uses real engineering units: `{"cmd":"set_mode","mode":"CV","value":5.0}` (volts) or `{"cmd":"set_mode","mode":"CC","value":2.0}` (amps). Hardware limits defined as `#define` constants (CV: 0.0–30.0V, CC: 0.0–10.0A). In-mode setpoint change: re-send set_mode with same mode + new value — no separate set_setpoint command needed.
- **D-04:** `get_status` returns runtime state only: `{"mode":"CV","setpoint":5.0,"dac":32768,"fault_code":0,"uptime_s":120}`. Real-time operating data.
- **D-05:** `get_info` returns static system info: `{"fw_ver":"1.0.0","chip_id":"ABCD1234","rated_w":150.0,"max_v":30.0,"max_a":10.0}`.
- **D-06:** `clear_fault` resets permanent latch from Phase 2 D-01 — transitions FAULT→IDLE.

### Telemetry Packet Design

- **D-07:** Nested JSON structure: `ch[]` array for 4 MOS channels (each with `v`/`i`), `sum{}` object for summary (`v`/`i`/`p`), flat meta fields: `seq`, `uptime`, `mode`, `fault`, `dac`, `retry`, `temp`.
- **D-08:** Telemetry sent once per control cycle (synchronized with PID update, ~100ms = ~10Hz). No separate timer needed.
- **D-09:** Packet includes sequence number (`seq` — rolls over at 65535) and uptime in milliseconds (`uptime`). Enables PC-side packet loss detection and timing.
- **D-10:** `dac` field (current DAC output 0–65535) and `retry` field (fault retry counter) included for debugging.
- **D-11:** `temp` field included as `0.0` placeholder — temperature sensor (NTC/ADC) added in Phase 4. Consistent schema from day one.

### Error Handling & Validation

- **D-12:** Error response format: `{"ack":"error","code":N,"msg":"human-readable description"}`. Numeric codes grouped by category: 1xx = parse errors, 2xx = range/validation, 3xx = state transition violations.
- **D-13:** Full validation on every incoming command: (1) valid cJSON syntax, (2) required fields present (`cmd` always required; `mode`/`value` for set_mode), (3) value ranges against hardware limits, (4) valid mode strings (`"CV"` or `"CC"` only), (5) unknown command strings → error.
- **D-14:** Strict state gating: `set_mode` rejected while in FAULT (must `clear_fault` first); `clear_fault` rejected if not in FAULT; `set_mode` to current mode + same setpoint accepted but idempotent (no-op). Valid transitions: IDLE→CV, IDLE→CC, CV↔CC, FAULT→IDLE (via clear_fault).
- **D-15:** UART-level errors (parity error, framing error, buffer overflow) → discard affected byte(s), log warning via debug printf, send error ack to PC. Incomplete JSON (missing closing brace after inter-byte timeout) → discard and send parse error. Control loop never pauses or blocks on UART errors.

### Packet Framing & RX/TX Strategy

- **D-16:** Newline-delimited JSON (`\n`), standard JSON Lines format. PC terminates each command with `\n`; firmware buffers bytes until `\n` found, then parses. Telemetry packets also `\n`-terminated.
- **D-17:** UART0 at 115200 bps, 8N1 with odd parity. Meets the >115200 requirement.
- **D-18:** Interrupt-driven RX ring buffer, 512 bytes. UART0 RXNE ISR pushes each byte; main loop scans for `\n` at start of each control cycle. 512 bytes holds ~2-3 full commands before processing.
- **D-19:** Command processing happens at the start of each 100ms control cycle (poll ring buffer for complete line → parse → dispatch). No JSON parsing in ISR context.
- **D-20:** Telemetry TX is blocking within the cycle: `USART_SendData` loop with TC flag wait. A ~300-byte telemetry packet at 115200 bps takes ~26ms to transmit, well within the 100ms window.

### Claude's Discretion

- cJSON library integration method (vendored source vs submodule, include path setup)
- Ring buffer implementation details (head/tail pointers, overflow behavior)
- Exact error code numbering within the 1xx/2xx/3xx categories
- cJSON memory management strategy in 32K SRAM (static pre-allocation, pool, or careful malloc/free)
- UART0 GPIO pin assignment and RCC clock enable
- `get_info` values — researcher to confirm rated_w, max_v, max_a against hardware spec
- Telemetry packet exact field naming within the nested structure (key names like `ch` vs `channels`, `sum` vs `summary`)
- Inter-byte timeout value for incomplete JSON detection
- `protocol.h` API design (init, poll, send_telemetry function signatures)

## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Planning & Requirements
- `.planning/ROADMAP.md` — Phase 3 goal, success criteria (4 items), phase dependencies (Phase 2)
- `.planning/PROJECT.md` — Project constraints (32K SRAM, 128K Flash, bare-metal), key decisions, evolution rules
- `.planning/REQUIREMENTS.md` — Full requirements: COMM-01 (UART0 cJSON commands), COMM-02 (10Hz telemetry)

### Prior Phase Context (decisions this phase depends on)
- `.planning/phases/01-hardware-foundation/01-CONTEXT.md` — Drivers/ directory structure (D-01 to D-04), I2C abstraction via i2c_util (D-05 to D-08), INA226 per-device structs (D-09 to D-12), USB-CDC debug output (D-13 to D-16)
- `.planning/phases/02-control-loop-protection/02-CONTEXT.md` — State machine IDLE/CV/CC/FAULT (D-15), PID instances + anti-windup (D-08, D-09), soft-start ramp (D-05, D-06), fault handler with auto-retry + permanent latch (D-01 to D-04), EXTI4 ISR for ALARM (D-11, D-12), calibration re-validation (D-14)

### Existing Driver APIs (Phase 1)
- `Drivers/ina226.h` — INA226 API: init, get_bus_voltage, get_shunt_voltage, get_current, get_power, check_alert. Used for telemetry data gathering.
- `Drivers/dac8571.h` — DAC8571 API: init, set_output(uint16_t). Command dispatch calls this for set_mode.
- `Drivers/i2c_util.h` — I2C wrapper: i2c_util_write, i2c_util_read, i2c_util_bus_recovery.
- `Drivers/pid.h` — PID API: pid_init, pid_compute, pid_set_integral. Command dispatch sets targets on the existing pid_cv/pid_cc instances.
- `Drivers/fault.h` — Fault API: fault_init, fault_handler_hw, fault_handler_opp, fault_state_machine, fault_print_snapshot, fault_reg, SystemMode enum. clear_fault command integrates with this.

### SPL Peripherals (for UART0)
- `Peripheral/inc/ch32v30x_usart.h` — USART/UART API: Init (baud, parity, stop bits), Cmd, SendData, ReceiveData, flag checking (RXNE, TC, TXE), ITConfig for RXNE interrupt
- `Peripheral/inc/ch32v30x_gpio.h` — GPIO pin config for UART0 TX/RX alternate function pins
- `Peripheral/inc/ch32v30x_rcc.h` — RCC clock enable for UART0 peripheral

### Application Integration Points
- `User/main.c` — Current super-loop with 100ms SysTick gating (lines 285-408). Phase 3 extends this with command poll + telemetry TX. Existing PID instances (`pid_cv`, `pid_cc`), state machine (`system_mode`, `cv_target_voltage`, `cc_target_current`), fault handler all used directly.
- `User/ch32v30x_it.c` — Where UART0 RXNE interrupt handler is added
- `User/ch32v30x_conf.h` — Enable USART peripheral include block for UART0

### External Library
- cJSON library (https://github.com/DaveGamble/cJSON) — Single-header/source MIT-licensed JSON parser for C. Researcher to determine integration approach (vendored source vs submodule) and memory management strategy for 32K SRAM.

## Existing Code Insights

### Reusable Assets
- **SPL USART driver** (`Peripheral/src/ch32v30x_usart.c`): Full UART API — Init, Cmd, SendData, ReceiveData, ITConfig, flag polling. UART0 is a separate peripheral instance from USART1 (debug printf).
- **Debug printf** (`Debug/debug.c`): `USART_Printf_Init()`, `_write()` syscall. Remains on USART1 — UART0 is the dedicated command/telemetry channel.
- **USB-CDC** (`Drivers/usb_cdc.c/.h`): `usb_printf()` for USB debug output. Remains active alongside UART0.
- **SysTick** (`Delay_Init`, SysTick->CNT): Already provides the 100ms control period gating. Phase 3 leverages the same timestamp for telemetry timing.
- **INA226 driver** (`Drivers/ina226.c/.h`): All getter functions already called each control cycle — telemetry assembly reads from the same variables.
- **PID instances** (`pid_cv`, `pid_cc`): `cv_target_voltage` and `cc_target_current` globals are the setpoint targets — command dispatch writes to these.
- **Fault handler** (`Drivers/fault.c/.h`): `fault_reg`, `fault_state_machine()`, `FAULT_FREE_RESET_MS`. `clear_fault` command integrates with the existing auto-retry/permanent-latch logic.

### Established Patterns
- **SPL init struct pattern**: `USART_InitTypeDef usart_init = {0}` with config struct, `USART_Init(USARTx, &usart_init)`. UART0 init follows the same pattern as existing USART1 init.
- **Weak interrupt handlers**: Override in `ch32v30x_it.c` for `USART0_IRQHandler`. RXNE interrupt pushes byte into ring buffer.
- **Super-loop with blocking I2C**: Current main.c reads all 5 INA226 with blocking I2C each cycle. Phase 3 adds blocking UART0 TX (~26ms) after the control computation — total cycle time remains well under 100ms.
- **Global state for ISR communication**: Ring buffer head/tail pointers as `volatile` globals — ISR updates tail, main loop updates head. Pattern matches existing `fault_triggered`/`fault_source_mask` volatile flags.
- **Static allocation**: Ring buffer, command parsing buffer, telemetry assembly buffer all as static arrays — no malloc needed for protocol runtime (cJSON may use heap for parse trees).
- **Drivers/ module pattern**: New `protocol.c/.h` in `Drivers/` with lowercase naming, relative `#include "../Drivers/protocol.h"`. Matches Phase 1 D-01 to D-04.

### Integration Points
- **main.c super-loop**: Insert at the start of each 100ms cycle (before sensor reads): poll ring buffer → if complete line found, parse cJSON → validate → dispatch (set_mode → engage_cv/cc, clear_fault → fault_state_machine, get_status → build response, get_info → build response). After fault checks + state machine dispatch, assemble telemetry from the just-read sensor values and send via UART0.
- **ch32v30x_it.c**: Add `USART0_IRQHandler` — read USART0->RDR, push byte into ring buffer, handle parity/framing errors.
- **UART0 GPIO pins**: Configure TX and RX alternate function pins. Exact pin mapping depends on CH32V303CBT6 pinout — researcher to determine from datasheet.
- **ch32v30x_conf.h**: Ensure USART/UART peripheral block is enabled (may already be for USART1).
- **Build system**: Add `Drivers/protocol.c` and cJSON source to compilation. May need `.cproject` include path updates if cJSON is placed outside the existing include paths.
- **cJSON integration**: Researcher to determine: vendored source in `Drivers/cjson/` (simpler, no external deps) vs git submodule. Memory management: cJSON uses `cJSON_malloc`/`cJSON_free` hooks — researcher to evaluate static pool vs newlib-nano malloc on 32K SRAM.

## Specific Ideas

No specific UI/UX references — this is an embedded communication protocol phase. Wire format follows standard JSON Lines convention, and the command/telemetry structures are defined in the decisions above.

## Deferred Ideas

None — discussion stayed within phase scope.

---

*Phase: 3-Communication*
*Context gathered: 2026-06-07*
