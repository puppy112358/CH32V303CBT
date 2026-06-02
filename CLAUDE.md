<!-- GSD:project-start source:PROJECT.md -->
## Project

**电子负载控制器 (Electronic Load Controller)**

基于CH32V303CBT6 RISC-V MCU的电子负载控制固件。通过UART接收cJSON格式的恒压(CV)/恒流(CC)指令，控制外置DAC8571输出参考电压驱动恒压恒流环路。I2C1总线上挂载5个INA226（4路MOS电流检测+1路汇总电流检测）实时监测，数据以10Hz频率通过UART上报。WS2812灯带显示负载状态，风扇PID控速散热。USB-CDC提供调试日志输出。

**Core Value:** **精确、安全的电子负载控制** —— 可靠的CV/CC双模式运行，每路MOS独立过流保护，实时数据监控和上报。

### Constraints

- **MCU**: CH32V303CBT6 — 128K Flash, 32K SRAM，需注意内存使用
- **I2C总线**: I2C1单总线挂载6个设备，需合理分配地址，注意总线速率和冲突
- **实时性**: 10Hz数据上报 + 100ms级控制回路 + 风扇PID，需注意时序调度
- **保护**: 4路INA226 ALARM中断需可靠响应，过流保护不能有遗漏
- **UART0**: cJSON协议 >115200bps + 奇校验，注意cJSON内存分配和解析性能
- **内存**: cJSON库在32K SRAM下的使用需注意动态内存碎片
<!-- GSD:project-end -->

<!-- GSD:stack-start source:codebase/STACK.md -->
## Technology Stack

## Languages
- C (GNU99 / C99) - All application code, peripheral drivers, system initialization
- Assembly (RISC-V) - Startup code and vector table in `Startup/startup_ch32v30x_D8.S`
## Runtime
- Bare-metal (no OS) - confirmed by `.template` line 13: `RTOS=NoneOS`
- RISC-V V4 Core (WCH Proprietary) with PFIC (Programmable Fast Interrupt Controller)
- CPU: RV32IMAC (RISC-V 32-bit Integer + Multiply/Divide + Atomics + Compressed instructions)
- ABI: ILP32 (32-bit integer only, no hardware floating-point ABI)
- Model: CH32V303CBT6 (WCH / Nanjing Qinheng Microelectronics)
- Series: CH32V303, variant CH32V30x_D8
- Flash: 128 KB (address range `0x00000000` - `0x00020000`)
- SRAM: 32 KB (address range `0x20000000` - `0x20008000`)
- Max clock: 144 MHz (configured for 96 MHz in this project via `SYSCLK_FREQ_96MHz_HSE`)
- HSE oscillator: 8 MHz (defined in `Peripheral/inc/ch32v30x.h` line 29)
- DVP, Ethernet, and USB OTG peripherals available (CH32V30x_D8C variants excluded)
- Not applicable (no package manager in use - bare-metal embedded project)
- newlib-nano (linked via `--specs=nano.specs` and `--specs=nosys.specs` in `.cproject` lines 80-81)
- Math library: libm (linked in `.cproject` line 85)
## Frameworks
- CH32V30x Standard Peripheral Library (Version 2.9) - Hardware abstraction and peripheral drivers
- RISC-V Core Support Layer: `Core/core_riscv.h` and `Core/core_riscv.c`
- Not detected - no test framework present. This is a bare-metal embedded project verified through hardware debugging.
- MounRiver Studio IDE (Eclipse-based, CDT managed build)
- RISC-V Cross GCC Toolchain (`riscv-none-embed-` prefix)
- Linker: GNU RISC-V Cross C Linker, using custom linker script `Ld/Link.ld`
- Debugger: OpenOCD + GDB via WCH-Link (config in `CH32V303CBT.launch`)
- Binary output: ELF executable, with flash image (.hex) creation and extended listing support
- Build output directory: `obj/`
## Key Dependencies
- CH32V30x Standard Peripheral Library (V2.9) - All MCU hardware interaction flows through this library
- RISC-V Core Layer (`Core/core_riscv.h`) - NVIC/PFIC, SysTick, atomic operations, CSR access
- System initialization (`User/system_ch32v30x.c`) - Clock tree configuration, system init
- Debug module (`Debug/debug.c`, `Debug/debug.h`) - USART printf (debug output on PA9/USART1 at 115200 baud), delay functions
- Startup code (`Startup/startup_ch32v30x_D8.S`) - Vector table, reset handler, interrupt stubs
- Linker script (`Ld/Link.ld`) - Memory layout, section placement, 2 KB stack allocation
## Configuration
- No environment variable files detected (no `.env` files present)
- HSE clock: 8 MHz external oscillator (hardware-defined, set in `Peripheral/inc/ch32v30x.h` line 29)
- System clock configured via macro in `User/system_ch32v30x.c` line 24: `SYSCLK_FREQ_96MHz_HSE` = 96 MHz
- UART debug baud rate: 115200 (set in `User/main.c` line 43)
- Debug UART selection: `DEBUG_UART1` (PA9 TX) via `Debug/debug.h` line 30
- Project configuration: `.cproject` (Eclipse CDT managed build)
- IDE settings: `.project` (Eclipse project definition)
- Device/flash programming: `.template` (MCU type, flash config, programmer settings)
- Debug configuration: `CH32V303CBT.launch` (OpenOCD/GDB launch config)
- Linker script: `Ld/Link.ld`
- Compiler options (from `.cproject`):
- Include paths: `Startup/`, `Debug/`, `Core/`, `User/`, `Peripheral/inc/`
## Platform Requirements
- MounRiver Studio IDE (WCH-provided Eclipse-based IDE for RISC-V MCU development)
- RISC-V GCC toolchain (`riscv-none-embed-*`)
- WCH-Link debug probe (hardware) for flashing and debugging
- Target hardware: CH32V303CBT6 development board
- Binary flashed to CH32V303CBT6 MCU internal flash at address `0x08000000` (as per `.template` line 3)
- No external dependencies at runtime - fully self-contained embedded firmware
- Erase All + Program + Verify flash programming workflow
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

## Naming Patterns
- All lowercase with underscore separators: `ch32v30x_gpio.c`, `ch32v30x_gpio.h`, `core_riscv.h`
- Subsystem prefix convention: `ch32v30x_` for MCU peripheral drivers, `core_` for RISC-V core layer
- Single-word utility names: `debug.h` (in `Debug/`), `main.c` (in `User/`)
- Configuration files: `ch32v30x_conf.h` (in `User/`)
- Linker script: `Link.ld` (in `Ld/`)
- PascalCase with underscore-prefixed subsystem module: `GPIO_Init()`, `USART_Printf_Init()`, `ADC_Cmd()`, `DMA_DeInit()`
- Peripheral functions all follow `{PERIPH}_{Verb}{Noun}`: `GPIO_WriteBit()`, `RCC_APB2PeriphClockCmd()`, `FLASH_ErasePage()`
- Private/static functions: `static` keyword, same PascalCase convention: `SetSysClock()`, `SetSysClockTo96_HSE()`
- NVIC/PFIC functions: Snake_case prefix `NVIC_` then PascalCase: `NVIC_SetPriority()`, `NVIC_EnableIRQ()`
- RISC-V inline functions in `core_riscv.h` use `__` prefix + UPPER_CASE: `__enable_irq()`, `__NOP()`, `__WFI()`
- Global/module-level: PascalCase or UPPER_CASE prefix: `SystemCoreClock`, `TxBuf`, `Calibrattion_Val`, `AHBPrescTable`
- Local variables (inside functions): lowercase camelCase or single-style: `tmpreg`, `currentmode`, `pinpos`, `StartUpCounter`, `HSEStatus`
- Local loop counters: single letters: `i`, `t`, `tmp`
- `static` file-scope arrays: PascalCase: `AHBPrescTable`
- Struct typedefs: PascalCase with `_TypeDef` suffix: `GPIO_InitTypeDef`, `ADC_InitTypeDef`, `DMA_InitTypeDef`
- Peripheral register structs: PascalCase with `_Type` suffix: `PFIC_Type`, `SysTick_Type`, `GPIO_TypeDef`
- Enum typedefs: PascalCase then `_TypeDef` suffix: `GPIOSpeed_TypeDef`, `GPIOMode_TypeDef`
- Simple enums: PascalCase: `BitAction`, `ErrorStatus`, `FunctionalState`, `FlagStatus`, `ITStatus`
- UPPER_CASE with subsystem prefix: `GPIO_Pin_0`, `RCC_APB2Periph_GPIOA`, `FLASH_FLAG_BSY`, `RCC_HSEON`
- Bit masks: UPPER_CASE as above: `ECR_PORTPINCONFIG_MASK`, `LSB_MASK`
- Pin definitions: `GPIO_Pin_x` where x is 0-15 and `GPIO_Pin_All` for OR'd all pins
- Function-like macros: UPPER_CASE: `RV_STATIC_INLINE`
- Format: `__UPPER_FILENAME_H` or `__UPPER_FILENAME_H__`
- Examples: `#ifndef __CH32V30x_GPIO_H`, `#define __CORE_RISCV_H__`
- Guard is always the first directive after the file header comment
- Closing: `#endif /* __CH32V30x_GPIO_H */` with filename comment
## Code Style
- No linter or formatter configuration detected in the project (no `.clang-format`, `.editorconfig`, `.prettierrc`)
- Indentation: 4 spaces, consistently applied throughout
- Bracing: Allman style -- opening brace on its own line at same indentation level:
- `if`/`else` blocks always use braces even for single statements
- Spaces around binary operators: `-`, `+`, `*`, `>>`, `<<`, `|`, `&`, `!=`, `==`, `=`
- No space between function name and opening paren: `GPIO_Init(GPIOA, &GPIO_InitStructure);`
- Space after comma in parameter lists: `(GPIOA, &GPIO_InitStructure)`
- Space after `if`, `while`, `for` keywords before opening paren: `if (x == 0)`, `while(1)`
- Casts: space after cast: `(uint32_t) RCC_HSEON`
- File headers: Large asterisk block with metadata on separate lines:
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : Main program body.
- Function docblocks: Block comment style `/** ... */` with structured tags:
- Inline comments: `/* comment text */` on their own line or inline
- End-of-line comments: `// description` occasionally used
- Section dividers: Comments like `/* Global define */`, `/* Global Variable */` separate source sections
- Preprocessor condition comments: trailing `/* comment */` after `#endif` to identify the block
## Import Organization
- All includes use double-quote style: `#include "debug.h"` (not angle brackets)
- User code (main.c) typically includes only `#include "debug.h"` which transitively pulls in `ch32v30x.h` and `stdio.h`
- Peripheral `.c` files include their own `.h` first, then any additional subsystem headers needed
- Conf file (`ch32v30x_conf.h`) aggregates all peripheral headers via `#include`
- No path aliases -- all includes are flat (headers assumed on include path)
- Not detected. All includes are flat file names; the build system (MounRiver IDE / GCC toolchain) manages include search paths.
## Error Handling
- Functions return status enums rather than throwing or aborting:
- Function return pattern: `void` for most init and config functions; typed return for status queries
- HSE crystal failure is detected but produces only a comment: `/* If HSE fails to start-up, the application will have wrong clock configuration. User can add here some code to deal with this error */` in `User/system_ch32v30x.c` (lines 323-326, 392-398, and similar). No backoff to HSI or error recovery is implemented.
- `NMI_Handler`: Captures NMI exception and enters infinite `while(1)` loop (no recovery)
- `HardFault_Handler`: Triggers `NVIC_SystemReset()` then enters infinite `while(1)` loop
- Both use `__attribute__((interrupt("WCH-Interrupt-fast")))` attribute for the WCH hardware vectoring
## Logging
- `printf()` over USART via `USART_Printf_Init(baudrate)` from `Debug/debug.h`
- Initialization pattern in every `main()`:
- Status messages use `\r\n` (CR+LF) line endings
- Test pass/fail messages: `"Memory Program PASS!\r\n"` / `"Memory Program FAIL!\r\n"`
- No log levels (INFO/DEBUG/WARN/ERROR) -- all output uses the same `printf()` channel
- SDI printf is also available with `SDI_Printf_Enable()` (for non-UART debug probes), configured via `SDI_PRINT` define
## Function Design
- Peripheral init functions take a pointer to a config struct (STM32-style pattern):
- Command functions take peripheral base address + state:
- Custom application functions use similar patterns or positional params
- Config/init functions: `void` return
- Status query functions: return typed status (`ErrorStatus`, `FlagStatus`, `uint8_t`, etc.)
- All local variables declared at top of function (C89 block-scope convention)
- Struct initialization uses zero-initialization: `GPIO_InitTypeDef GPIO_InitStructure = {0};`
- Clock config helpers declared `static void`: `SetSysClockTo96_HSE()` etc.
- Used for internal implementation details not exposed through headers
## Module Design
- Header files declare only public API functions with `extern`
- Struct and typedef definitions always in headers
- Register base address defines and peripheral pointers in headers
- `ch32v30x.h`: The master MCU header that includes `core_riscv.h` and defines all peripheral base addresses, register structures, and bit definitions
- `ch32v30x_conf.h`: Aggregates all peripheral driver headers; treated as the single include point for application code to pull in the full HAL
- `debug.h`: Aggregates stdio, ch32v30x, and provides delay/printf init functions
- All public headers have `#ifdef __cplusplus` / `extern "C" {` / `}` / `#endif` wrappers for C++ compatibility even though the project is pure C
- All memory-mapped register pointers use `__IO`, `__I`, or `__O` volatile qualifiers defined in `Core/core_riscv.h`
## Struct Initialization Pattern
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

## System Overview
```text
|                        Application Layer                          |
|  `example/<peripheral>/<example>/User/main.c`                     |
|  (main() loop, peripheral init, business logic)                   |
|                      User Support Files                           |
|  `User/ch32v30x_conf.h`   - peripheral enable/disable includes   |
|  `User/ch32v30x_it.c/.h`  - interrupt service routine stubs      |
|  `User/system_ch32v30x.c/.h` - clock config, SystemInit()        |
|                Standard Peripheral Library (SPL)                  |
|  `example/SRC/Peripheral/src/ch32v30x_*.c`   - HAL implementation |
|  `example/SRC/Peripheral/inc/ch32v30x_*.h`   - HAL headers        |
|  `example/SRC/Peripheral/inc/ch32v30x.h`     - master header      |
|                   Core / CMSIS Layer                              |
|  `example/SRC/Core/core_riscv.c/.h`  - RISC-V CSR access, NVIC   |
|  Startup  |  Linker   |   Debug   |  RTOS    | Extras   |
|  (asm)    |  Script   |  Support  |  Ports   |          |
| `SRC/     | `SRC/     | `SRC/     | FreeRTOS | DVP, ETH |
| Startup/  | Ld/       | Debug/    | RT-Thread| VoiceRcg |
| startup_  | Link.ld   | debug.c   | TencentOS|          |
| ch32v30x_ |           | debug.h   | HarmonyOS|          |
| D8C.S`    |           |           |          |          |
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
- **No dynamic memory management** in typical examples -- static allocation with global variables
- **Super-loop in main()** ending with `while(1);` for examples without RTOS
- **Polling-based** or **interrupt-driven** peripheral interaction
- **Direct register access** through memory-mapped struct pointers (e.g., `GPIOA->BSHR = ...`)
- **Weak-symbol interrupt handlers** allow user overrides without touching startup code
- **Compile-time peripheral selection** via `ch32v30x_conf.h` includes
## Layers
- Purpose: User-written firmware logic, peripheral configuration, business rules
- Location: `example/<peripheral>/<example>/User/main.c`
- Contains: `main()` function, peripheral init helper functions
- Depends on: Debug (`debug.h`), SPL headers (via `ch32v30x_conf.h`)
- Used by: Nothing (top of stack)
- Purpose: Project-specific configuration glue
- Location: `example/<peripheral>/<example>/User/`
- Contains: `ch32v30x_conf.h` (peripheral includes), `ch32v30x_it.c/.h` (ISRs), `system_ch32v30x.c/.h` (clock init)
- Depends on: SPL master header `ch32v30x.h`
- Used by: Application layer
- Purpose: Hardware abstraction for all on-chip peripherals (GPIO, ADC, TIM, USART, SPI, I2C, USB, ETH, CAN, DMA, RTC, etc.)
- Location: `example/SRC/Peripheral/inc/` (headers), `example/SRC/Peripheral/src/` (implementations)
- Contains: 25+ peripheral drivers with Init/DeInit/Cmd/ITConfig/Flag pattern
- Depends on: Core layer (`core_riscv.h`), master header `ch32v30x.h`
- Used by: Application + Support layers
- Purpose: RISC-V privileged architecture access, NVIC (PFIC) interrupt controller interface, SysTick timer
- Location: `example/SRC/Core/core_riscv.c/.h`
- Contains: CSR accessor functions (mstatus, mepc, mcause, etc.), NVIC inline functions, atomic operations, VTF (vector table-free) fast interrupt support
- Depends on: `<stdint.h>`, compiler intrinsics
- Used by: SPL, Debug, startup code
## Data Flow
### Primary Request Path (Peripheral Example)
### Interrupt Path
- **Global variables** for state sharing between ISRs and main loop (e.g., `TxBuf[1024]`, `Calibrattion_Val` in `main.c:24-25`)
- **Volatile struct pointers** for hardware register access (memory-mapped at fixed addresses)
- **No RTOS in typical examples** -- all synchronization is disable-interrupt / flag-poll based
## Key Abstractions
- Purpose: Consistent init/deinit/cmd interface for all peripherals
- Examples: `GPIO_Init()`, `ADC_Init()`, `USART_Init()`, `DMA_Init()`, `TIM_TimeBaseInit()`
- Pattern: Each peripheral has `XXX_InitTypeDef` struct, `XXX_Init(periph, &struct)`, `XXX_Cmd(periph, ENABLE)`, `XXX_DeInit(periph)`.
- Purpose: Enable/disable clocks to individual peripherals
- Examples: `RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE)`
- Pattern: Clock must be enabled before any peripheral register access.
- Purpose: `printf()` over UART or SDI (Serial Debug Interface) for development
- Configurable via `DEBUG` macro (UART1/2/3) and `SDI_PRINT` macro in `debug.h`
- Implementation: Overrides `_write()` syscall; also provides `_sbrk()` for heap.
- Purpose: Inline functions for enable/disable/priority/pending management
- Pattern: `NVIC_EnableIRQ(IRQn)`, `NVIC_SetPriority(IRQn, prio)`, `NVIC_ClearPendingIRQ(IRQn)`
- All defined as `__attribute__((always_inline)) static inline` in `core_riscv.h`
## Entry Points
- Location: `example/SRC/Startup/startup_ch32v30x_D8C.S:16` (`_start`)
- Triggers: Power-on reset, external reset, system reset
- Responsibilities: Initialize CPU state, copy data, zero BSS, configure interrupts, call `SystemInit()` then `main()`
- Location: `example/<peripheral>/<example>/User/main.c:125`
- Triggers: Called by startup code after system initialization
- Responsibilities: Configure peripherals, enter application logic loop
- Location: `example/<peripheral>/<example>/User/system_ch32v30x.c`
- Triggers: Called by `handle_reset` in startup code
- Responsibilities: Configure HSE/PLL clock tree, set `SystemCoreClock` global
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
### Global Variable State Sharing
### Magic Register Offsets in Debug
## Error Handling
- **Calibration validation:** `Get_ConversionVal()` bounds-checks ADC samples (clamps to [0, 4095])
- **Blocking wait loops:** `while(ADC_GetResetCalibrationStatus(ADC1))` -- blocks forever if hardware fails
- **Null pointer guard in _sbrk():** Checks heap bounds before extending, returns `NULL-1` on overflow
- **NMI handler:** Infinite loop (traps debugger, no recovery)
- **HardFault handler:** System reset via `NVIC_SystemReset()`
## Cross-Cutting Concerns
<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->
## Project Skills

No project skills found. Add skills to any of: `.claude/skills/`, `.agents/skills/`, `.cursor/skills/`, `.github/skills/`, or `.codex/skills/` with a `SKILL.md` index file.
<!-- GSD:skills-end -->

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->



<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
