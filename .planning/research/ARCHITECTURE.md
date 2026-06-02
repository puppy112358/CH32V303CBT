# Architecture Research

**Domain:** Embedded Electronic Load Controller Firmware
**Researched:** 2026-06-02
**Confidence:** HIGH

## Standard Architecture

Electronic load controller firmware follows a **layered bare-metal super-loop with interrupt-driven I/O** pattern. The architecture is dominated by three concerns: (1) a deterministic control loop, (2) reliable sensor monitoring with hardware fault protection, and (3) a command/response protocol interface. Across open-source electronic load projects and commercial instruments, the same structural decomposition emerges consistently.

### System Overview

```
+====================================================================+
|                     APPLICATION / MAIN LOOP                         |
|  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐  |
|  │   Command Parser  │  │   Mode Manager   │  │  Status Reporter  │  |
|  │  (cJSON UART RX)  │  │  (CV/CC/Idle)    │  │  (10Hz UART TX)   │  |
|  └────────┬─────────┘  └────────┬─────────┘  └────────┬─────────┘  |
|           │                     │                     │             |
+===========+=====================+=====================+=============+
|                        CONTROL LAYER                                  |
|  ┌──────────────────────────────┐  ┌──────────────────────────────┐  |
|  │    CV/CC PID Controller      │  │    Fan PID Controller        │  |
|  │  (100ms period,              │  │  (periodic, tach feedback)   │  |
|  │   INA226 readings → PID      │  │   temp/power → PWM duty)     │  |
|  │   → DAC8571 setpoint)        │  │                              │  |
|  └──────────────┬───────────────┘  └──────────────┬───────────────┘  |
|                 │                                 │                  |
+=================+=================================+==================+
|                       DEVICE DRIVER LAYER                             |
|  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌─────────────┐  |
|  │ INA226 Driver │ │ DAC8571 Drv  │ │ WS2812 Driver│ │ Fan PWM Drv │  |
|  │ (5 instances) │ │ (1 instance) │ │ (TIM+PWM+DMA)│ │ (TIM PWM)   │  |
|  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘ └──────┬──────┘  |
|         │                │                │                │         |
+=========+================+================+================+=========+
|                       HARDWARE BUS LAYER                              |
|  ┌──────────────────────┐  ┌──────────────────────────────────────┐  |
|  │   I2C1 Bus Manager   │  │   UART0 / USB-CDC / EXTI Manager     │  |
|  │ (6 devices, 400kHz)  │  │ (RX ring buffer, TX queue, ISRs)     │  |
|  └──────────────────────┘  └──────────────────────────────────────┘  |
+======================================================================+
|                       MCU PERIPHERAL LAYER (SPL)                      |
|  GPIO | I2C1 | USART1 | USART2(CDC) | TIM1 | TIM2 | EXTI | DMA      |
+======================================================================+
|                       HARDWARE                                        |
|  CH32V303CBT6 (RV32IMAC, 128K Flash, 32K RAM)                        |
+======================================================================+
```

### Component Responsibilities

| Component | Responsibility | Typical Implementation |
|-----------|----------------|------------------------|
| **SysTick Timer** | Global 1ms tick, scheduling heartbeat for control/monitor loops | SysTick_Handler incrementing `global_tick_ms` |
| **UART0 RX ISR + Ring Buffer** | Non-blocking byte capture into circular buffer, line detection (`\r\n`) | ISR writes to static `uint8_t rx_ring[]`, sets flag on delimiter |
| **Command Parser / Dispatcher** | cJSON deserialization, command routing to handlers (CV setpoint, CC setpoint, mode switch, status query) | Static command table: `{"cmd_name", handler_fn}`; zero dynamic alloc |
| **Mode Manager** | Operating mode state machine: Idle → CV → CC → Fault. Encapsulates mode-specific behavior | Strategy pattern: mode struct vtable with `enter()`, `exit()`, `control_tick()` |
| **INA226 Driver** | I2C read/write to 5x INA226: config init, shunt/bus voltage read, alarm threshold set, alert latch clear | Per-instance struct with I2C address + calibration registers; latch-mode alert (LEN=1) |
| **DAC8571 Driver** | I2C write to 1x DAC8571: 16-bit setpoint value | Write-only I2C transaction, idempotent (redundant writes harmless) |
| **CV/CC PID Controller** | Software PID: INA226 measurement → error → P+I term → DAC output. Period: ~100ms | `pid_compute(&pid, setpoint, measurement, dt)` → clamp → DAC write |
| **Protection Monitor** | INA226 ALARM pin EXTI interrupt → fault latch → emergency shutdown (DAC→0, mode→Fault) | EXTI ISR reads Mask/Enable reg to clear latch, sets fault flag, main loop acts |
| **Status Reporter** | 10Hz cJSON data packet: 4-ch MOS current/voltage + summary + temperature + mode + faults | `status_builder()` fills static JSON buffer, UART0 TX DMA sends |
| **WS2812 Driver** | TIM PWM + DMA: color buffer → bitstream → LED strip. Updates on mode/fault change | TIM1_CH1 (PA0) 800kHz PWM, DMA circular or single-shot for buffer output |
| **Fan PID Controller** | Fan PWM duty cycle from PID (temperature/power setpoint), tachometer input capture for speed feedback | TIM2 CH for PWM out, TIM input capture for tach, PID period ~500ms |
| **USB-CDC Debug** | printf() redirect to USART2 via USB-CDC virtual COM port | `_write()` syscall override → USART2 TX |

## Recommended Project Structure

```
User/
├── main.c                    # Super-loop: init + scheduler ticks
├── app/
│   ├── app_scheduler.c/.h    # Cooperative tick scheduler (1ms heartbeat)
│   ├── app_command.c/.h      # cJSON UART command parser + dispatcher
│   ├── app_mode.c/.h         # Operating mode manager (CV/CC/Idle/Fault)
│   ├── app_control.c/.h      # CV/CC PID control loop
│   └── app_report.c/.h       # 10Hz status reporter (cJSON build + send)
├── drivers/
│   ├── drv_i2c_bus.c/.h      # I2C1 bus manager (init, read, write, scan)
│   ├── drv_ina226.c/.h       # INA226 driver (5 instances)
│   ├── drv_dac8571.c/.h      # DAC8571 driver (1 instance)
│   ├── drv_ws2812.c/.h       # WS2812 LED strip driver (TIM+PWM+DMA)
│   └── drv_fan.c/.h          # Fan PWM + tachometer driver
├── hal/
│   ├── hal_uart.c/.h         # UART0 RX ring buffer + TX DMA, USB-CDC printf
│   └── hal_systick.c/.h      # SysTick 1ms timer + delay helpers
├── lib/
│   └── pid.c/.h              # Generic PID controller struct + compute
├── ch32v30x_conf.h           # SPL peripheral enable/disable
├── ch32v30x_it.c/.h          # ISR implementations
└── system_ch32v30x.c/.h      # Clock config, SystemInit()
```

### Structure Rationale

- **`app/`:** All application logic lives here. Each module is independently testable. The scheduler coordinates timing; nothing else knows about hardware.
- **`drivers/`:** Device-specific I2C and PWM code. Each driver exposes an init + operation API. I2C drivers go through the bus manager, never touch I2C registers directly.
- **`hal/`:** Thin wrappers over SPL for UART and SysTick. Isolates SPL dependency; application layer never includes SPL headers.
- **`lib/`:** Pure computation (PID). No hardware dependencies.
- **`ch32v30x_it.c`:** ISR implementations centralized in one file. Keeps all interrupt code visible and auditable.

## Architectural Patterns

### Pattern 1: Cooperative Time-Triggered Scheduler

**What:** A SysTick-driven 1ms tick counter drives periodic tasks. The super-loop checks elapsed time per task and runs them at their cadence. No RTOS required.

**Why for this project:** The project has well-defined timing: 10Hz reporting (100ms), ~100ms PID loop, ~500ms fan PID. A cooperative scheduler is simpler than an RTOS, uses zero task stack overhead, and guarantees deterministic behavior. This is the dominant pattern in open-source electronic load firmware (KennyBurfeindt DC-programmable-load, Arduino-based constant loads).

**Trade-offs:** If any task blocks too long (e.g., slow I2C transaction), timing drifts. Mitigation: I2C reads must use non-blocking timeout patterns; all tasks complete in <1ms. Not suitable for complex multi-rate control, but fine for this project's single-loop architecture.

**Example:**
```c
// sys_tick.c
volatile uint32_t g_tick_ms = 0;
void SysTick_Handler(void) { g_tick_ms++; }

// app_scheduler.c
static uint32_t last_control_tick = 0;
static uint32_t last_report_tick = 0;
static uint32_t last_fan_tick = 0;

void scheduler_run(void) {
    uint32_t now = g_tick_ms;

    if (now - last_control_tick >= 100) {
        app_control_tick();        // CV/CC PID, 100ms
        last_control_tick = now;
    }
    if (now - last_report_tick >= 100) {
        app_report_send();         // 10Hz status
        last_report_tick = now;
    }
    if (now - last_fan_tick >= 500) {
        drv_fan_pid_tick();        // Fan PID, 500ms
        last_fan_tick = now;
    }

    app_command_process();         // Non-blocking: process pending UART bytes
    app_fault_check();             // Non-blocking: check fault flags

    __WFI();                       // Sleep until next interrupt
}
```

### Pattern 2: INA226 Latch-Mode Alert with EXTI → Main Loop Deferred Processing

**What:** The INA226 is configured with `LEN=1` (latch mode) and `APOL` set appropriately. The ALARM pin connects to an MCU EXTI line. The EXTI ISR is minimal: clear the INA226 latch by reading the Mask/Enable register, set a fault flag, and return. The main loop detects the flag and performs the heavy processing (read fault source, disable DAC, log event, transition mode).

**Why for this project:** The INA226 datasheet and Linux kernel driver both recommend latch mode to guarantee alerts are never missed. Keeping ISRs short prevents nested interrupt priority inversion and keeps the main loop in control of complex state transitions. This is the pattern used in PX4 firmware, Zephyr INA226 driver, and the Linux `ina2xx.c` hwmon driver.

**When to use:** Always for fault alerts (SOL/SUL/BOL/BUL/POL). For Conversion Ready (CNVR) mode, latch is optional depending on whether you want each conversion to trigger.

**Example:**
```c
// In ch32v30x_it.c — EXTI ISR for INA226 ALARM pin
void EXTI9_5_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line5) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line5);
        // Minimal: clear latch by reading Mask/Enable reg
        uint16_t mask = drv_ina226_read(addr_overcurrent_source, INA226_REG_MASK);
        // Set fault flag for main loop
        g_fault_flags |= FAULT_OVERCURRENT;
    }
}

// In main loop scheduler
void app_fault_check(void) {
    if (g_fault_flags & FAULT_OVERCURRENT) {
        dac8571_write(0x0000);         // Kill DAC output
        app_mode_transition(MODE_FAULT);
        ws2812_set_color(COLOR_FAULT);  // Red LED
        g_fault_flags &= ~FAULT_OVERCURRENT;
    }
}
```

### Pattern 3: Strategy Pattern for Operating Modes

**What:** Each operating mode (CV, CC, Idle, Fault) is represented by a struct containing function pointers (`enter`, `exit`, `control_tick`). Mode transitions swap the active strategy pointer rather than scattering `if (mode == ...)` throughout the code.

**Why for this project:** Documented in the element14 programmable electronic load project firmware. Prevents mode-check spaghetti code. Adding a new mode (e.g., Constant Resistance CR, Constant Power CP) requires only writing three new functions. Currently only CV/CC needed, but the structure future-proofs.

**When to use:** When the system has 2+ distinct operating states with different behavior for the same events (setpoint change, control tick, fault).

**Example:**
```c
typedef struct {
    void (*enter)(void);
    void (*exit)(void);
    void (*control_tick)(void);
    void (*on_setpoint)(float value);
} eload_mode_t;

static const eload_mode_t mode_cv = { .enter = cv_enter, .control_tick = cv_tick, ... };
static const eload_mode_t mode_cc = { .enter = cc_enter, .control_tick = cc_tick, ... };
static const eload_mode_t mode_fault = { .enter = fault_enter, ... };

static const eload_mode_t *g_active_mode = &mode_idle;

void app_mode_transition(const eload_mode_t *new_mode) {
    if (g_active_mode->exit) g_active_mode->exit();
    g_active_mode = new_mode;
    if (g_active_mode->enter) g_active_mode->enter();
}
```

### Pattern 4: Static Ring Buffer + Zero-Copy JSON Parsing

**What:** UART RX ISR pushes bytes into a preallocated ring buffer. The command parser scans for `\r\n` delimiters without copying. cJSON parses the line in-place (or from a static scratch buffer). No `malloc`/`free` — all storage is static.

**Why for this project:** 32K SRAM is the dominant constraint. cJSON uses `malloc` by default which fragments heap. The solution: preallocate a pool of cJSON nodes statically, or use `cJSON_InitHooks()` with a bump allocator from a static arena. This is the standard embedded JSON approach documented by Espressif, STM32, and many IoT firmware projects.

**Example:**
```c
#define UART_RX_RING_SIZE 512
static volatile uint8_t  rx_ring[UART_RX_RING_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;
static volatile uint8_t  rx_line_ready = 0;
static          char     rx_line_buf[256];

void USART1_IRQHandler(void) {
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t ch = USART_ReceiveData(USART1);
        uint16_t next = (rx_head + 1) % UART_RX_RING_SIZE;
        if (next != rx_tail) {  // not full
            rx_ring[rx_head] = ch;
            rx_head = next;
            if (ch == '\n') rx_line_ready = 1;
        }
    }
}
```

## Data Flow

### Command Path (UART RX to DAC Setpoint)

```
UART0 RX pin (PA10)
    │
    ▼ [USART1 RXNE interrupt]
  rx_ring[] (ISR pushes bytes)
    │
    ▼ [main loop polls rx_line_ready]
  app_command_process()
    ├─ Line extraction from ring buffer → rx_line_buf[]
    ├─ cJSON_Parse(rx_line_buf) → cJSON tree (static arena)
    ├─ Command dispatch: {"cmd":"cv_set", "voltage":3.30}
    ├─ Validate setpoint (range check, mode check)
    └─ app_mode_on_setpoint(value)
         │
         ▼
  app_mode.c: update setpoint variable (g_setpoint_voltage)
         │
         ▼ [control tick, 100ms]
  app_control_tick() (CV PID)
    ├─ drv_ina226_read(channel, &current, &voltage)  → measured values
    ├─ pid_compute(&g_pid_cv, g_setpoint_voltage, measured_voltage, 0.1)
    └─ drv_dac8571_write(pid_output_16bit)
         │
         ▼
  I2C1 SDA/SCL (PB6/PB7) → DAC8571 (0x4C)
         │
         ▼
  DAC8571 VOUT → Op-Amp Comparator → MOS Gate → Constant Voltage on Load
```

### Monitoring Path (INA226 Reading to UART TX)

```
Scheduler tick (100ms)
    │
    ▼
  app_report_send()
    ├─ drv_ina226_read_all(ch_measurements[])
    │   └─ I2C1 reads: CH1(0x40) CH2(0x41) CH3(0x44) CH4(0x45) SUM(0x4F)
    ├─ Aggregate: total_current, total_power, per_channel_checks
    ├─ cJSON build (static buffer)
    │   {
    │     "ch1": {"v":12.34, "i":1.50},
    │     "ch2": {"v":12.31, "i":1.48},
    │     ...
    │     "sum": {"v":12.33, "i":5.98, "p":73.8},
    │     "mode": "cv",
    │     "temp": 42.5
    │   }
    └─ hal_uart_send(json_buf, len) → USART1 TX DMA → UART0 TX pin (PA9)
```

### Protection Path (INA226 ALARM to Fault State)

```
INA226 Alert pin (nALERT, open-drain, pulled up)
    │  Device detects shunt over-voltage
    │  Alert pin pulls LOW (latch mode: stays low until cleared)
    ▼
MCU EXTI line (e.g., PB5, falling edge trigger)
    │
    ▼ [EXTI ISR]
  app_fault_flag |= FAULT_OVERCURRENT_CH1;
    │  (ISR returns immediately — no heavy processing)
    ▼ [main loop, next scheduler iteration]
  app_fault_check()
    ├─ Read INA226 Mask/Enable register → clears latch, identifies source
    ├─ drv_dac8571_write(0x0000)     → DAC output to 0V
    ├─ app_mode_transition(MODE_FAULT)
    ├─ ws2812_set_color(COLOR_RED)    → visual alert
    └─ report_fault_event()           → JSON error over UART
```

### LED Status Path

```
Mode change / fault event
    │
    ▼
  ws2812_set_color(color_enum)
    ├─ Color map lookup: MODE_CV→GREEN, MODE_CC→BLUE, MODE_FAULT→RED, ...
    ├─ Build bitstream buffer (24 bits per LED × N LEDs × duty cycle values)
    ├─ DMA start: buffer → TIM1 CCR1 (800kHz PWM)
    └─ DMA TC ISR: TIM1 stop, DMA stop
```

### Debug Path (printf → USB-CDC)

```
Any module: printf("debug message %d\r\n", val)
    │
    ▼
  _write() syscall override (in debug.c / hal_uart.c)
    └─ USART2 TX byte → USB-CDC virtual COM port → PC terminal
```

### State Management

```
┌──────────────────────────────────────────────────────┐
│                   Global State Map                    │
│  (all static, no heap allocation)                    │
├──────────────────────────────────────────────────────┤
│  g_tick_ms: uint32_t            — 1ms counter        │
│  g_eload_mode: eload_mode_enum  — IDLE/CV/CC/FAULT   │
│  g_setpoint: float              — CV: volts, CC: amps│
│  g_pid_cv: pid_state_t          — CV PID state       │
│  g_pid_cc: pid_state_t          — CC PID state       │
│  g_pid_fan: pid_state_t         — Fan PID state      │
│  g_fault_flags: uint16_t        — bitmask of faults  │
│  g_ina226[5]: ina226_state_t    — calibration + cache│
│  g_ws2812_colors[N]: uint32_t   — LED color buffer   │
│  g_command_arena: uint8_t[512]  — cJSON static pool   │
└──────────────────────────────────────────────────────┘
```

Interaction rule: ISRs may only write to `volatile` globals and set flags. The main loop is the sole writer to all other state. This eliminates race conditions without locks.

## Scaling Considerations

This is an embedded single-MCU system. "Scaling" here means increasing feature count or I2C device count, not user load.

| Scale | Architecture Adjustments |
|-------|--------------------------|
| Current: 6 I2C devices | Single I2C1 bus at 400kHz. Bus manager with timeout per-transaction. Fine. |
| Add more INA226 devices (up to 16) | I2C address conflict possible (INA226 has 16 unique addresses max via A0/A1 pins). Use I2C multiplexer (TCA9548A) if exceeding. |
| Add display (OLED/LCD) | I2C bus bandwidth shared with sensors. Move display to separate I2C2 bus or SPI. |
| Faster control loop (<10ms) | Move PID to ISR context triggered by INA226 Conversion Ready. Current 100ms from SysTick is fine for thermal load. |
| RTOS migration | If adding WiFi/BLE stack: migrate to FreeRTOS. Control task priority = highest, comm = medium, display = low. Use message queues for setpoint updates. |
| Flash calibration storage | Store calibration constants in last 2K of Flash using SPL Flash API. Version-stamped records for firmware update safety. |

### Resource Budget (Current Architecture)

| Resource | Budget | Used (Estimated) | Notes |
|----------|--------|-------------------|-------|
| Flash 128K | 100% | ~60K | SPL (~25K) + cJSON (~6K) + app (~15K) + drivers (~10K) + buffer |
| RAM 32K | 100% | ~16K | Stack 2K + WS2812 buffer ~2K + cJSON arena ~1K + driver state + UART rings |
| I2C bus time | ~2.5ms/sensor read | 5× INA226 reads ~12.5ms per 100ms tick = 12.5% utilization | Well within 400kHz budget |
| CPU time | 100ms tick budget | PID calc ~0.1ms + cJSON build ~1ms + I2C reads ~12.5ms = ~14% | Remaining 86% for command parsing and idle |

## Anti-Patterns

### Anti-Pattern 1: UART Blocking in Main Loop

**What people do:** Process UART bytes with blocking `while(USART_GetFlagStatus(...) == RESET)` loops for each byte.

**Why it's wrong:** Blocks the control loop. If UART is down or peer is slow, the DAC setpoint freezes, potentially destroying hardware. In an electronic load, a frozen DAC output with changing load conditions can cause thermal runaway.

**Do this instead:** Interrupt-driven ring buffer. The main loop processes complete lines only. If no line is ready, skip parsing entirely. The super-loop must never block for more than a handful of microseconds.

### Anti-Pattern 2: I2C Blocking Without Timeout

**What people do:** `while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))` with no timeout.

**Why it's wrong:** If an I2C device is physically disconnected or holding SDA low, the MCU hangs forever. On an electronic load that's actively sinking current, the control loop stops and the load can overheat.

**Do this instead:** Every I2C operation uses a tick-count timeout:
```c
uint32_t timeout = g_tick_ms + 10;  // 10ms timeout
while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) {
    if (g_tick_ms > timeout) {
        drv_i2c_bus_reset();  // Bit-bang clock to release SDA
        return I2C_ERR_TIMEOUT;
    }
}
```

### Anti-Pattern 3: Heavy Processing in ISR

**What people do:** In the INA226 ALARM EXTI ISR: read status registers, disable DAC, transition mode, send UART message, all inside the ISR.

**Why it's wrong:** Long ISR execution disables lower-priority interrupts, including the SysTick that drives scheduling. Timer drift accumulates. Nested interrupt complexity on CH32V30x (hardware stack limited to a few levels).

**Do this instead:** ISR sets `volatile` flags and returns. Main loop processes flags in a known order with controlled preemption.

### Anti-Pattern 4: malloc/free in cJSON on 32K RAM

**What people do:** `cJSON_Parse()` using default hooks (calls `malloc` for every JSON node).

**Why it's wrong:** 32K RAM is tight. cJSON allocates dozens of small nodes per parse. Fragmentation builds up after hours of operation. Eventually `malloc` returns NULL during a critical command, crashing the parser (or silently failing to parse a protection command).

**Do this instead:** Use `cJSON_InitHooks()` to redirect allocations to a static bump allocator from a preallocated arena. Alternatively, preallocate a pool of `cJSON` nodes and use a pool allocator. Reset the arena/pool after each command parse completes.

### Anti-Pattern 5: WS2812 Bit-Banging with Delay Loops

**What people do:** Toggle GPIO with `Delay_Us()` to generate WS2812 timing. The naive Arduino pattern.

**Why it's wrong:** Disables interrupts for the entire LED update (~30us per LED × N LEDs). On an electronic load, the INA226 ALARM interrupt can fire during this window and be missed. The DAC control loop also freezes for the duration.

**Do this instead:** TIM PWM + DMA. The CPU sets up the buffer and starts the transfer. The TIM+DMA hardware generates the bitstream autonomously. The CPU is free to run the control loop and respond to interrupts during the transfer. On CH32V303, this requires TIM1 CH1 at 800kHz with DMA channel for CCR1 updates.

## Integration Points

### External Interfaces

| Interface | Protocol | Notes |
|-----------|----------|-------|
| PC/Host → MCU | UART0, cJSON over text, 115200+ baud, odd parity | Commands: CV setpoint, CC setpoint, mode switch, status query |
| MCU → PC/Host | UART0, cJSON text, 10Hz | Status telemetry: per-channel V/I, sum V/I/P, mode, temperature, faults |
| MCU → PC (debug) | USB-CDC (USART2), printf text | All debug output. Must not interfere with UART0 command channel |
| INA226 ALARM → MCU | EXTI (falling edge) | 4× ALARM pins (one per MOS channel INA226). 5th summary INA226 may or may not use alarm |
| Fan Tach → MCU | TIM Input Capture | Frequency measurement for RPM feedback |

### I2C1 Bus Topology

```
                    CH32V303CBT6 (Master)
                    I2C1: PB6(SCL) / PB7(SDA)
                           │
              ┌────────────┼────────────┬────────────┬────────────┬────────────┐
              │            │            │            │            │            │
           INA226       INA226       INA226       INA226       INA226       DAC8571
          CH1 MOS       CH2 MOS      CH3 MOS      CH4 MOS       SUM          DAC
          Addr: 0x40    Addr: 0x41   Addr: 0x44   Addr: 0x45   Addr: 0x4F   Addr: 0x4C
          ALARM→PB5     ALARM→PB8    ALARM→PB9    ALARM→PB10   (optional)   (no alert)
```

**I2C address notes (HIGH confidence, from INA226 datasheet):**
- INA226 7-bit address range: 0x40–0x4F (A0 and A1 pins set the two LSBs)
- DAC8571 7-bit address: 0x4C (A0 pin grounded)
- Address conflict risk: 0x4C overlaps with INA226 address range. The five INA226 must avoid 0x4C (A0=0, A1=1 pin config).

### Internal Boundaries

| Boundary | Communication | Considerations |
|----------|---------------|----------------|
| Command parser → Mode manager | Direct function call, setpoint variable write | cJSON tree discarded after parse. Setpoint copied to global state. |
| Mode manager → PID controller | Direct function call, PID state struct passed by pointer | PID state persists across mode transitions. Gains are mode-specific. |
| PID controller → DAC8571 driver | `drv_dac8571_write(uint16_t value)` | DAC is write-only. Idempotent — redundant writes harmless. |
| Scheduler → All app modules | `g_tick_ms` global, elapsed-time comparisons | Scheduler is the sole timing authority. Modules never read hardware timers. |
| Fault ISR → Main loop | `volatile uint16_t g_fault_flags` bitmask | Only ISR sets. Only main loop clears (after processing). |
| INA226 driver → I2C bus manager | `drv_i2c_read(addr, reg, buf, len)` | Bus manager handles arbitration, timeout, bus reset. Drivers are bus-agnostic. |
| WS2812 driver → DMA | Memory-to-Peripheral DMA: buffer → TIM1 CCR1 | One-shot or circular. Circular can cause visual artifacts if buffer races. |

## Build Order Implications

Based on dependency analysis, the recommended build order is:

```
Phase 1: Foundation
  ┌──────────────────────────────┐
  │ SysTick 1ms timer            │
  │ UART0 printf (skeleton)      │
  │ USB-CDC debug printf         │
  │ GPIO basics (LED heartbeat)  │
  └──────────┬───────────────────┘
             │ (needed by: everything)
             ▼
Phase 2: I2C + Sensors
  ┌──────────────────────────────┐
  │ I2C1 bus manager             │
  │ INA226 driver (1 device)     │
  │ INA226 driver (5 devices)    │
  │ DAC8571 driver               │
  └──────────┬───────────────────┘
             │ (needed by: control loop, reporting)
             ▼
Phase 3: Control Loop
  ┌──────────────────────────────┐
  │ PID library                  │
  │ CV/CC control loop (100ms)   │
  │ Mode manager (CV/CC/Idle)    │
  │ Manual DAC setpoint test     │
  └──────────┬───────────────────┘
             │ (needed by: command parser validates setpoints)
             ▼
Phase 4: Command Interface
  ┌──────────────────────────────┐
  │ UART0 RX ring buffer         │
  │ cJSON static allocator       │
  │ Command parser + dispatcher  │
  │ Status reporter (10Hz JSON)  │
  │ Full protocol integration    │
  └──────────┬───────────────────┘
             │ (parallelizable after Phase 2)
             ▼
Phase 5: Protection
  ┌──────────────────────────────┐
  │ EXTI config (INA226 ALARM)   │
  │ Fault ISR → flag pattern     │
  │ Main loop fault processing   │
  │ Fault → DAC disable + mode   │
  └──────────┬───────────────────┘
             │
             ▼
Phase 6: Peripherals
  ┌──────────────────────────────┐
  │ WS2812 TIM+PWM+DMA driver    │
  │ Fan PWM driver               │
  │ Fan tachometer input capture │
  │ Fan PID control loop         │
  └──────────────────────────────┘
```

**Phase ordering rationale:**
- Phase 1 must be first: all other phases need SysTick for timing and a debug channel to observe behavior.
- Phase 2 is the hardware interface layer: you cannot control or measure anything without I2C/I2C devices. The INA226 and DAC8571 are pure I2C devices.
- Phase 3 depends on Phase 2's sensors and actuator. You need measured current/voltage and DAC output before you can close the control loop.
- Phase 4 depends on Phase 3: the command parser processes setpoint changes, which are validated through the mode manager. But UART RX infrastructure starts in Phase 1.
- Phase 5 (protection) conceptually depends on Phase 2's INA226 driver but is logically independent. It can be developed alongside Phase 3/4 but should be tested before Phase 4's protocol goes live (unsafe to accept commands without protection).
- Phase 6 (LED, fan) has no dependencies on the command or control logic beyond Phase 2. Can be developed in parallel.

**Parallel opportunities:**
- Phase 5 (protection) can start as soon as Phase 2 is done
- Phase 6 (fan + LED) can start as soon as Phase 2 is done
- Phase 3 and Phase 4 can share development after Phase 2 (Phase 3 needs hardware, Phase 4 needs UART from Phase 1)

## Sources

- WCH CH32V303 Reference Manual and SPL documentation (project-local)
- INA226 datasheet (Texas Instruments SBOS547) — registers, latch mode, alert configuration. HIGH confidence.
- DAC8571 datasheet (Texas Instruments) — I2C address 0x4C, 16-bit write protocol. HIGH confidence.
- WS2812 protocol specification — 800kHz timing, GRB 24-bit format. HIGH confidence.
- Linux kernel ina2xx.c hwmon driver (Guenter Roeck, 2024 patch for latch mode) — verified pattern for INA226 alert handling. HIGH confidence.
- PX4 autopilot firmware INA226 driver — interrupt-driven alert handling pattern. MEDIUM confidence.
- element14 Programmable Electronic Load (community project) — strategy pattern for operating modes, hardware/software modularity. MEDIUM confidence.
- Michael J. Pont, "Patterns for Time-Triggered Embedded Systems" — cooperative scheduler design. MEDIUM confidence.
- STM32 WS2812 PWM+DMA driver (MaJerle, mucahitcinkilic) — verified TIM+PWM+DMA pattern for LED strip control. HIGH confidence.
- CH32V003 WS2812 driver (fabian-bxr) — CH32-specific porting reference for TIM PWM+DMA on WCH MCUs. HIGH confidence.

---
*Architecture research for: 电子负载控制器 (Electronic Load Controller)*
*Researched: 2026-06-02*
