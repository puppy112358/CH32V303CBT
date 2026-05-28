<!-- refreshed: 2026-05-28 -->
# Architecture

**Analysis Date:** 2026-05-28

## System Overview

This is an embedded bare-metal firmware project for the WCH CH32V303CBT microcontroller -- a RISC-V (RV32IMACF) MCU from Nanjing Qinheng Microelectronics. The project uses the WCH MounRiver Studio IDE with RISC-V GCC toolchain and WCH-Link debug probe. It is organized as a **Standard Peripheral Library (SPL) + User Application** architecture, closely modeling the CMSIS-like peripheral access pattern.

```text
+------------------------------------------------------------------+
|                        Application Layer                          |
|  `example/<peripheral>/<example>/User/main.c`                     |
|  (main() loop, peripheral init, business logic)                   |
+-------------------------------+----------------------------------+
                                |
                                v
+------------------------------------------------------------------+
|                      User Support Files                           |
|  `User/ch32v30x_conf.h`   - peripheral enable/disable includes   |
|  `User/ch32v30x_it.c/.h`  - interrupt service routine stubs      |
|  `User/system_ch32v30x.c/.h` - clock config, SystemInit()        |
+-------------------------------+----------------------------------+
                                |
                                v
+------------------------------------------------------------------+
|                Standard Peripheral Library (SPL)                  |
|  `example/SRC/Peripheral/src/ch32v30x_*.c`   - HAL implementation |
|  `example/SRC/Peripheral/inc/ch32v30x_*.h`   - HAL headers        |
|  `example/SRC/Peripheral/inc/ch32v30x.h`     - master header      |
+-------------------------------+----------------------------------+
                                |
                                v
+------------------------------------------------------------------+
|                   Core / CMSIS Layer                              |
|  `example/SRC/Core/core_riscv.c/.h`  - RISC-V CSR access, NVIC   |
+-------------------------------+----------------------------------+
                                |
                                v
+-----------+-----------+-----------+----------+-----------+
|  Startup  |  Linker   |   Debug   |  RTOS    | Extras   |
|  (asm)    |  Script   |  Support  |  Ports   |          |
| `SRC/     | `SRC/     | `SRC/     | FreeRTOS | DVP, ETH |
| Startup/  | Ld/       | Debug/    | RT-Thread| VoiceRcg |
| startup_  | Link.ld   | debug.c   | TencentOS|          |
| ch32v30x_ |           | debug.h   | HarmonyOS|          |
| D8C.S`    |           |           |          |          |
+-----------+-----------+-----------+----------+-----------+
```

## Component Responsibilities

| Component | Responsibility | Key Files |
|-----------|----------------|-----------|
| **Startup** | Reset handler, vector table, BSS zeroing, data section copy, CSR init | `example/SRC/Startup/startup_ch32v30x_D8C.S` |
| **Linker** | Memory layout, section placement, stack sizing | `example/SRC/Ld/Link.ld` |
| **Core/CMSIS** | RISC-V CSR access functions, NVIC/PFIC interrupt controller registers | `example/SRC/Core/core_riscv.c`, `example/SRC/Core/core_riscv.h` |
| **SPL Headers** | Peripheral register structs, bit definitions, function prototypes | `example/SRC/Peripheral/inc/ch32v30x*.h` |
| **SPL Sources** | Peripheral HAL implementations (init/deinit/cmd/ITconfig) | `example/SRC/Peripheral/src/ch32v30x_*.c` |
| **System Config** | Clock tree initialization, `SystemCoreClock` update | `User/system_ch32v30x.c/.h` |
| **Conf Header** | Selects which peripheral modules are compiled in | `User/ch32v30x_conf.h` |
| **ISR Stubs** | User-overridable interrupt handlers (weak defaults in startup) | `User/ch32v30x_it.c/.h` |
| **Debug** | UART/SDI printf, SysTick-based delay (us/ms) | `example/SRC/Debug/debug.c/.h` |
| **Application** | Main loop, peripheral configuration, business logic | `example/<peripheral>/<example>/User/main.c` |

## Pattern Overview

**Overall:** Bare-metal super-loop (with optional RTOS port layer)

**Key Characteristics:**
- **No dynamic memory management** in typical examples -- static allocation with global variables
- **Super-loop in main()** ending with `while(1);` for examples without RTOS
- **Polling-based** or **interrupt-driven** peripheral interaction
- **Direct register access** through memory-mapped struct pointers (e.g., `GPIOA->BSHR = ...`)
- **Weak-symbol interrupt handlers** allow user overrides without touching startup code
- **Compile-time peripheral selection** via `ch32v30x_conf.h` includes

## Layers

**Application Layer:**
- Purpose: User-written firmware logic, peripheral configuration, business rules
- Location: `example/<peripheral>/<example>/User/main.c`
- Contains: `main()` function, peripheral init helper functions
- Depends on: Debug (`debug.h`), SPL headers (via `ch32v30x_conf.h`)
- Used by: Nothing (top of stack)

**User Support Layer:**
- Purpose: Project-specific configuration glue
- Location: `example/<peripheral>/<example>/User/`
- Contains: `ch32v30x_conf.h` (peripheral includes), `ch32v30x_it.c/.h` (ISRs), `system_ch32v30x.c/.h` (clock init)
- Depends on: SPL master header `ch32v30x.h`
- Used by: Application layer

**Standard Peripheral Library (SPL) Layer:**
- Purpose: Hardware abstraction for all on-chip peripherals (GPIO, ADC, TIM, USART, SPI, I2C, USB, ETH, CAN, DMA, RTC, etc.)
- Location: `example/SRC/Peripheral/inc/` (headers), `example/SRC/Peripheral/src/` (implementations)
- Contains: 25+ peripheral drivers with Init/DeInit/Cmd/ITConfig/Flag pattern
- Depends on: Core layer (`core_riscv.h`), master header `ch32v30x.h`
- Used by: Application + Support layers

**Core / CMSIS Layer:**
- Purpose: RISC-V privileged architecture access, NVIC (PFIC) interrupt controller interface, SysTick timer
- Location: `example/SRC/Core/core_riscv.c/.h`
- Contains: CSR accessor functions (mstatus, mepc, mcause, etc.), NVIC inline functions, atomic operations, VTF (vector table-free) fast interrupt support
- Depends on: `<stdint.h>`, compiler intrinsics
- Used by: SPL, Debug, startup code

## Data Flow

### Primary Request Path (Peripheral Example)

1. **Reset** (`startup_ch32v30x_D8C.S:326-372`): CPU starts at `_start`, jumps to `handle_reset` which sets up GP, SP, copies `.data` to RAM, zeros `.bss`, configures interrupt nesting, hardware stack, and FPU, then calls `SystemInit()` followed by `main()`.
2. **SystemInit** (`system_ch32v30x.c`): Configures the clock tree (HSE -> PLL -> SYSCLK -> AHB/APB1/APB2 prescalers), sets `SystemCoreClock`.
3. **main()** (`User/main.c:125`): Calls `SystemCoreClockUpdate()`, `Delay_Init()`, `USART_Printf_Init()`, then initializes peripherals and enters the application loop.
4. **Peripheral Init** (e.g., `ADC_Function_Init()` in `main.c:35`): Configures RCC clocks, GPIO alternate functions, peripheral registers via SPL struct init patterns.
5. **Peripheral Operation**: Polling loops (e.g., `while(flag == RESET)`) or DMA/interrupt-driven data transfer.

### Interrupt Path

1. Hardware interrupt triggers, CPU vectors to `mtvec` (configured by `handle_reset` to `_vector_base` address + mode 3 = vectored mode).
2. Vector table in `startup_ch32v30x_D8C.S:21-127` maps to weak-default handlers.
3. User overrides weak handler in `ch32v30x_it.c` (e.g., `DMA1_Channel1_IRQHandler`).
4. Handler clears interrupt flag, processes data, signals completion to main loop via global flag variables.

**State Management:**
- **Global variables** for state sharing between ISRs and main loop (e.g., `TxBuf[1024]`, `Calibrattion_Val` in `main.c:24-25`)
- **Volatile struct pointers** for hardware register access (memory-mapped at fixed addresses)
- **No RTOS in typical examples** -- all synchronization is disable-interrupt / flag-poll based

## Key Abstractions

**Standard Peripheral Init Pattern:**
- Purpose: Consistent init/deinit/cmd interface for all peripherals
- Examples: `GPIO_Init()`, `ADC_Init()`, `USART_Init()`, `DMA_Init()`, `TIM_TimeBaseInit()`
- Pattern: Each peripheral has `XXX_InitTypeDef` struct, `XXX_Init(periph, &struct)`, `XXX_Cmd(periph, ENABLE)`, `XXX_DeInit(periph)`.

**RCC Clock Configuration Functions:**
- Purpose: Enable/disable clocks to individual peripherals
- Examples: `RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE)`
- Pattern: Clock must be enabled before any peripheral register access.

**Debug Output Abstraction:**
- Purpose: `printf()` over UART or SDI (Serial Debug Interface) for development
- Configurable via `DEBUG` macro (UART1/2/3) and `SDI_PRINT` macro in `debug.h`
- Implementation: Overrides `_write()` syscall; also provides `_sbrk()` for heap.

**NVIC Interrupt Management:**
- Purpose: Inline functions for enable/disable/priority/pending management
- Pattern: `NVIC_EnableIRQ(IRQn)`, `NVIC_SetPriority(IRQn, prio)`, `NVIC_ClearPendingIRQ(IRQn)`
- All defined as `__attribute__((always_inline)) static inline` in `core_riscv.h`

## Entry Points

**Reset Vector:**
- Location: `example/SRC/Startup/startup_ch32v30x_D8C.S:16` (`_start`)
- Triggers: Power-on reset, external reset, system reset
- Responsibilities: Initialize CPU state, copy data, zero BSS, configure interrupts, call `SystemInit()` then `main()`

**main():**
- Location: `example/<peripheral>/<example>/User/main.c:125`
- Triggers: Called by startup code after system initialization
- Responsibilities: Configure peripherals, enter application logic loop

**SystemInit():**
- Location: `example/<peripheral>/<example>/User/system_ch32v30x.c`
- Triggers: Called by `handle_reset` in startup code
- Responsibilities: Configure HSE/PLL clock tree, set `SystemCoreClock` global

**Interrupt Vector Table:**
- Location: `example/SRC/Startup/startup_ch32v30x_D8C.S:19` (`_vector_base`)
- Triggers: Hardware interrupt events
- Contains: 80+ entries for system exceptions and peripheral interrupts

## Architectural Constraints

- **Threading:** Single-threaded super-loop (no preemptive multitasking in bare-metal examples). RTOS ports exist for FreeRTOS, RT-Thread, TencentOS, and HarmonyOS LiteOS-m.
- **Global state:** Heavy use of module-level globals (e.g., `TxBuf`, `Calibrattion_Val`, `p_us`, `p_ms`). No thread-safety mechanisms in bare-metal examples.
- **Circular imports:** Not detected. Headers use `#ifndef` include guards consistently. Dependency chain is strictly top-down (app -> SPL -> core).
- **Interrupt nesting:** Enabled by default via CSR 0x804 setting in `handle_reset`. Hardware stack configured for nested interrupts.
- **Memory layout:** Fixed by linker script (`Link.ld`). Flash at 0x00000000 (288K), RAM at 0x20000000 (32K), stack at top of RAM.
- **No MPU:** Memory Protection Unit not present on CH32V303 (`__MPU_PRESENT = 0`).
- **Toolchain:** RISC-V GNU Embedded GCC (riscv-none-embed-gcc), MounRiver Studio IDE (Eclipse-based).

## Anti-Patterns

### Weak Handler Infinite Loop

**What happens:** Default weak interrupt handlers in `startup_ch32v30x_D8C.S` fall through to an infinite loop (`j 1b` at line 321).
**Why it's wrong:** If an unexpected interrupt fires without a user override, the MCU silently hangs. There is no error logging or recovery.
**Do this instead:** Provide a user handler in `ch32v30x_it.c` for every enabled interrupt source, even if it only logs an error. At minimum, the handler should clear the interrupt flag and return.

### Global Variable State Sharing

**What happens:** ISRs and main-loop code share state via unprotected global variables (e.g., `TxBuf` in the ADC_DMA example). No `volatile` qualifier on many shared globals.
**Why it's wrong:** Compiler optimizations may cache values in registers, causing ISR writes to be invisible to the main loop. Race conditions possible with nested interrupts.
**Do this instead:** Declare all ISR-shared globals as `volatile`. For multi-byte shared state, use critical sections (`__disable_irq()`/`__enable_irq()`) around reads/writes in the main loop.

### Magic Register Offsets in Debug

**What happens:** `debug.c` uses hard-coded magic addresses `0xE0000380` and `0xE0000384` for SDI printf data registers (lines 18-19).
**Why it's wrong:** No symbolic name or documentation for these registers. If the hardware changes, the magic numbers must be found and replaced manually.
**Do this instead:** Define named macros or struct members in the PFIC/Core header for SDI communication registers.

## Error Handling

**Strategy:** Minimal error handling typical of embedded examples. Most functions return void.

**Patterns:**
- **Calibration validation:** `Get_ConversionVal()` bounds-checks ADC samples (clamps to [0, 4095])
- **Blocking wait loops:** `while(ADC_GetResetCalibrationStatus(ADC1))` -- blocks forever if hardware fails
- **Null pointer guard in _sbrk():** Checks heap bounds before extending, returns `NULL-1` on overflow
- **NMI handler:** Infinite loop (traps debugger, no recovery)
- **HardFault handler:** System reset via `NVIC_SystemReset()`

## Cross-Cutting Concerns

**Logging:** `printf()` over UART1/2/3 (compile-time selected via `DEBUG` macro) or SDI interface. Default baud rate is 115200.

**Validation:** None beyond basic bounds checks in conversion functions. No API parameter validation in SPL functions (assumes correct usage).

**Authentication:** Not applicable (bare-metal MCU firmware with no network authentication).

**Power Management:** PWR peripheral support (`ch32v30x_pwr.c/.h`) for sleep/stop/standby modes. `__WFI()` and `__WFE()` primitives in core headers.

---

*Architecture analysis: 2026-05-28*
