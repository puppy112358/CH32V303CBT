# Codebase Structure

**Analysis Date:** 2026-05-28

## Directory Layout

```
CH32V303CBT/
├── .claude/                                  # AI assistant configuration (GSD agents, commands, hooks)
│   ├── agents/                               # GSD agent definitions (~30 agent files)
│   ├── commands/gsd/                         # GSD slash-command definitions
│   ├── get-shit-done/                        # GSD runtime (bin/, contexts/, references/, templates/, workflows/)
│   └── hooks/                                # Claude Code hooks
├── .planning/                                # GSD planning artifacts
│   └── codebase/                             # Codebase analysis documents (this file's location)
├── Core/                                     # Project-local RISC-V core access layer
│   ├── core_riscv.c                          # CSR access function implementations
│   └── core_riscv.h                          # RISC-V V4 core + NVIC/PFIC register definitions
├── Debug/                                    # Project-local debug support
│   ├── debug.c                               # UART printf, SDI printf, SysTick delay, heap _sbrk
│   └── debug.h                               # Debug config macros (DEBUG_UART1/2/3, SDI_PRINT)
├── example/                                  # Example applications and shared library sources
│   ├── SRC/                                  # ===== SHARED LIBRARY SOURCES =====
│   │   ├── Core/                             # RISC-V core access (duplicate of root Core/)
│   │   │   ├── core_riscv.c
│   │   │   └── core_riscv.h
│   │   ├── Debug/                            # Debug support (duplicate of root Debug/)
│   │   │   ├── debug.c
│   │   │   └── debug.h
│   │   ├── Ld/                               # Linker script
│   │   │   └── Link.ld                       # Memory layout: 288K FLASH + 32K RAM
│   │   ├── Peripheral/                       # Standard Peripheral Library (SPL)
│   │   │   ├── inc/                          # Peripheral header files (30+)
│   │   │   │   ├── ch32v30x.h               # Master header: IRQ numbers, register structs, base addresses
│   │   │   │   ├── ch32v30x_adc.h           # ADC driver interface
│   │   │   │   ├── ch32v30x_bkp.h           # Backup registers
│   │   │   │   ├── ch32v30x_can.h           # CAN controller
│   │   │   │   ├── ch32v30x_crc.h           # CRC calculation unit
│   │   │   │   ├── ch32v30x_dac.h           # DAC driver
│   │   │   │   ├── ch32v30x_dbgmcu.h        # Debug MCU (chip ID, etc.)
│   │   │   │   ├── ch32v30x_dma.h           # DMA controller
│   │   │   │   ├── ch32v30x_dvp.h           # Digital Video Port
│   │   │   │   ├── ch32v30x_eth.h           # Ethernet MAC
│   │   │   │   ├── ch32v30x_exti.h          # External interrupts
│   │   │   │   ├── ch32v30x_flash.h         # Flash memory controller
│   │   │   │   ├── ch32v30x_fsmc.h          # Flexible Static Memory Controller
│   │   │   │   ├── ch32v30x_gpio.h          # General Purpose I/O
│   │   │   │   ├── ch32v30x_i2c.h           # I2C interface
│   │   │   │   ├── ch32v30x_iwdg.h          # Independent Watchdog
│   │   │   │   ├── ch32v30x_misc.h          # Misc (NVIC priority groups)
│   │   │   │   ├── ch32v30x_opa.h           # Operational Amplifier
│   │   │   │   ├── ch32v30x_pwr.h           # Power control (sleep/stop/standby)
│   │   │   │   ├── ch32v30x_rcc.h           # Reset and Clock Control
│   │   │   │   ├── ch32v30x_rng.h           # Random Number Generator
│   │   │   │   ├── ch32v30x_rtc.h           # Real-Time Clock
│   │   │   │   ├── ch32v30x_sdio.h          # SDIO interface
│   │   │   │   ├── ch32v30x_spi.h           # SPI interface
│   │   │   │   ├── ch32v30x_tim.h           # Timer (advanced, general, basic)
│   │   │   │   ├── ch32v30x_usart.h         # USART/UART
│   │   │   │   ├── ch32v30x_usb.h           # USB (device/host/OTG)
│   │   │   │   └── ch32v30x_wwdg.h          # Window Watchdog
│   │   │   └── src/                          # Peripheral implementation files (25+)
│   │   │       ├── ch32v30x_adc.c
│   │   │       ├── ch32v30x_bkp.c
│   │   │       ├── ch32v30x_can.c
│   │   │       ├── ch32v30x_crc.c
│   │   │       ├── ch32v30x_dac.c
│   │   │       ├── ch32v30x_dbgmcu.c
│   │   │       ├── ch32v30x_dma.c
│   │   │       ├── ch32v30x_dvp.c
│   │   │       ├── ch32v30x_eth.c
│   │   │       ├── ch32v30x_exti.c
│   │   │       ├── ch32v30x_flash.c
│   │   │       ├── ch32v30x_fsmc.c
│   │   │       ├── ch32v30x_gpio.c
│   │   │       ├── ch32v30x_i2c.c
│   │   │       ├── ch32v30x_iwdg.c
│   │   │       ├── ch32v30x_misc.c
│   │   │       ├── ch32v30x_opa.c
│   │   │       ├── ch32v30x_pwr.c
│   │   │       ├── ch32v30x_rcc.c
│   │   │       ├── ch32v30x_rng.c
│   │   │       ├── ch32v30x_rtc.c
│   │   │       ├── ch32v30x_sdio.c
│   │   │       ├── ch32v30x_spi.c
│   │   │       ├── ch32v30x_tim.c
│   │   │       ├── ch32v30x_usart.c
│   │   │       └── ch32v30x_wwdg.c
│   │   └── Startup/                         # Assembly startup files
│   │       ├── startup_ch32v30x_D8.S        # For CH32V303Cx (128K flash variants)
│   │       └── startup_ch32v30x_D8C.S       # For CH32V305/307/317 (256K+ flash variants)
│   │
│   ├── ADC/                                  # ADC peripheral examples (13 sub-examples)
│   │   ├── ADC_DMA/                          # ADC with DMA transfer
│   │   │   └── User/                         # User application code
│   │   │       ├── main.c                   # Application entry point
│   │   │       ├── ch32v30x_conf.h          # Peripheral includes config
│   │   │       ├── ch32v30x_it.c            # Interrupt service routines
│   │   │       ├── ch32v30x_it.h            # ISR declarations
│   │   │       ├── system_ch32v30x.c        # Clock initialization (SystemInit)
│   │   │       └── system_ch32v30x.h        # System clock extern declaration
│   │   ├── AnalogWatchdog/
│   │   ├── Auto_Injection/
│   │   ├── Discontinuous_mode/
│   │   ├── DualADC_AlternateTrigger/
│   │   ├── DualADC_Combined_RegInjectionSimul/
│   │   ├── DualADC_FastInterleaved/
│   │   ├── DualADC_InjectionSimul/
│   │   ├── DualADC_RegSimul/
│   │   ├── DualADC_SlowInterleaved/
│   │   ├── ExtLines_Trigger/
│   │   ├── Internal_Temperature/
│   │   ├── Temperature_External_channel/
│   │   └── TIM_Trigger/
│   │
│   ├── APPLICATION/                          # Application-level examples
│   │   └── WS2812_LED/                       # WS2812 LED strip driver
│   ├── BKP/                                  # Backup register examples
│   ├── CAN/                                  # CAN bus examples (Networking, TestMode, Time-triggered)
│   ├── CRC/                                  # CRC calculation examples
│   ├── DAC/                                  # DAC examples (DMA, trigger, noise/triangle generation)
│   ├── DMA/                                  # DMA examples (mem-to-mem, peripheral-to-mem, etc.)
│   ├── DVP/                                  # Digital Video Port (camera) examples
│   ├── ETH/                                  # Ethernet (TCP/IP) examples
│   ├── EXTI/                                 # External interrupt examples
│   ├── FLASH/                                # Flash memory (program/erase/read) examples
│   ├── FPU/                                  # Floating-Point Unit usage examples
│   ├── FSMC/                                 # FSMC (external memory) examples
│   ├── FreeRTOS/                             # FreeRTOS port examples
│   ├── GPIO/                                 # GPIO examples (LED toggle, etc.)
│   ├── HarmonyOS/                            # HarmonyOS LiteOS-m port
│   ├── I2C/                                  # I2C communication examples
│   ├── I2S/                                  # I2S audio examples
│   ├── IAP/                                  # In-Application Programming (bootloader) examples
│   ├── INT/                                  # Interrupt controller examples
│   ├── IWDG/                                 # Independent watchdog examples
│   ├── OPA/                                  # Operational amplifier examples
│   ├── PMP/                                  # Physical Memory Protection examples
│   ├── PWR/                                  # Power management (sleep/stop/standby) examples
│   ├── RCC/                                  # Clock configuration examples
│   ├── RNG/                                  # Random number generator examples
│   ├── RT-Thread/                            # RT-Thread RTOS port
│   ├── RTC/                                  # Real-time clock examples
│   ├── SDIO/                                 # SD card interface examples
│   ├── SDI_Printf/                           # Serial Debug Interface printf examples
│   ├── SPI/                                  # SPI communication examples
│   ├── SYSTICK/                              # SysTick timer examples
│   ├── TIM/                                  # Timer examples (PWM, capture, encoder, etc.)
│   ├── TOUCHKEY/                             # Touch key (capacitive sensing) examples
│   ├── TencentOS/                            # TencentOS Tiny port
│   ├── USART/                                # USART communication examples
│   ├── USB/                                  # USB (device/host) examples
│   ├── VoiceRcgExam/                         # Voice recognition example
│   └── WWDG/                                 # Window watchdog examples
│
├── CH32V303CBT.launch                        # MounRiver Studio debug launch configuration
├── CH32V303CBT.wvproj                        # MounRiver Studio project file (XML, binary-encoded)
└── README.md                                 # Minimal project readme
```

## Directory Purposes

**`Core/` (project root):**
- Purpose: Project-local copy of RISC-V core peripheral access layer
- Contains: CSR accessor functions (`core_riscv.c`), NVIC/PFIC definitions and inline functions (`core_riscv.h`)
- Key files: `core_riscv.h` (NVIC API, SysTick struct, atomic ops, VTF interrupt config), `core_riscv.c` (CSR get/set implementations)

**`Debug/` (project root):**
- Purpose: Project-local copy of debug support module
- Contains: `printf()` backend (`_write()`), heap allocator (`_sbrk()`), SysTick-based microsecond/millisecond delays
- Key files: `debug.h` (DEBUG_UARTx selection macros, SDI_PRINT toggle), `debug.c` (USART_Printf_Init, Delay_Us, Delay_Ms, SDI_Printf_Enable)

**`example/SRC/` (shared library):**
- Purpose: Shared source files linked by all example projects -- the Standard Peripheral Library (SPL) plus CMSIS-like core
- Contains: Peripheral drivers (inc + src), Core HAL, Debug module, Linker script, Startup assembly
- Key files: `Ld/Link.ld` (memory layout and section placement), `Startup/startup_ch32v30x_D8C.S` (reset handler + vector table), `Peripheral/inc/ch32v30x.h` (master header with all IRQ numbers and register bases)

**`example/<PERIPHERAL>/` (example groups):**
- Purpose: Each subdirectory demonstrates a specific on-chip peripheral
- Contains: One or more example projects per peripheral (e.g., ADC has 13 different usage patterns)
- Each example is a standalone compile target with its own `User/` application files

**`example/<PERIPHERAL>/<example>/User/` (application code):**
- Purpose: Project-specific user code for a single example
- Contains: `main.c` (entry point), `ch32v30x_conf.h` (peripheral selection), `ch32v30x_it.c/.h` (interrupt handlers), `system_ch32v30x.c/.h` (clock configuration)
- Each `User/` directory is unique to its example and not shared

**`.claude/` (AI tooling):**
- Purpose: GSD (Get Shit Done) development workflow automation
- Contains: Agent definitions, slash-command handlers, runtime binaries, templates
- Key files: `agents/` (30+ specialized AI agent definitions), `commands/gsd/` (50+ slash commands)
- Generated: No, manually installed via GSD

**`.planning/` (GSD planning artifacts):**
- Purpose: Stores codebase analysis, phase plans, and other planning documents
- Contains: `codebase/` (architecture, structure, conventions, concerns docs)
- Generated: Yes, created by `/gsd:map-codebase` and other GSD commands

## Key File Locations

**Entry Points:**
- `example/SRC/Startup/startup_ch32v30x_D8C.S`: CPU reset vector (`_start` -> `handle_reset` -> `SystemInit` -> `main`)
- `example/<peripheral>/<example>/User/main.c`: Application `main()` function
- `example/SRC/Startup/startup_ch32v30x_D8.S`: Alternate startup for smaller flash variants (128K)

**Configuration:**
- `example/<peripheral>/<example>/User/ch32v30x_conf.h`: Peripheral module enable/disable (include the specific `ch32v30x_*.h` headers needed)
- `example/<peripheral>/<example>/User/system_ch32v30x.c`: Clock tree configuration (HSE/HSI/PLL frequency selection)
- `example/SRC/Ld/Link.ld`: Flash/RAM sizes, stack size (2048 bytes), section layout

**Core Logic (SPL):**
- `example/SRC/Peripheral/inc/ch32v30x.h`: Master header -- IRQ enum, register struct typedefs, peripheral base address macros, standard types (`vu32`, `FunctionalState`, etc.)
- `example/SRC/Peripheral/src/ch32v30x_rcc.c`: Clock configuration implementation (largest SPL file)
- `example/SRC/Peripheral/src/ch32v30x_gpio.c`: GPIO implementation
- `example/SRC/Core/core_riscv.h`: NVIC inline functions, atomic ops, CSR access declarations

**Debug/Delay:**
- `example/SRC/Debug/debug.c`: `printf()` backend, `_sbrk()` heap, `Delay_Us()`/`Delay_Ms()`
- `example/SRC/Debug/debug.h`: Debug UART selection (`DEBUG_UART1/2/3`) and SDI print toggle

## Naming Conventions

**Files:**
- SPL headers: `ch32v30x_<peripheral>.h` (lowercase, underscore separated)
- SPL sources: `ch32v30x_<peripheral>.c`
- User files: `ch32v30x_<purpose>.c/.h` (e.g., `ch32v30x_conf.h`, `ch32v30x_it.c`)
- Startup: `startup_ch32v30x_<variant>.S` (uppercase .S for preprocessed assembly)
- Example directories: `<PERIPHERAL>` (uppercase) or `<PERIPHERAL>_<SubExample>` (PascalCase with underscores)

**Directories:**
- Peripheral example groups: UPPERCASE (e.g., `ADC/`, `GPIO/`, `USART/`)
- Sub-examples: PascalCase with underscores (e.g., `ADC_DMA/`, `DualADC_FastInterleaved/`)
- Library source: UPPERCASE acronyms (e.g., `SRC/`, `Core/`, `Ld/`)

**Functions:**
- SPL functions: `<PERIPHERAL>_<Action>()` -- PascalCase with underscore separator (e.g., `GPIO_Init()`, `ADC_SoftwareStartConvCmd()`, `RCC_APB2PeriphClockCmd()`)
- SPL init structs: `<PERIPHERAL>_InitTypeDef` (e.g., `GPIO_InitTypeDef`, `ADC_InitTypeDef`)
- Core functions: `__<verb>_<REGISTER>()` -- double underscore C-stdlib style (e.g., `__get_MSTATUS()`, `__enable_irq()`)
- NVIC functions: `NVIC_<Action><Target>()` (e.g., `NVIC_EnableIRQ()`, `NVIC_SetPriority()`)
- User functions: PascalCase with underscores (e.g., `ADC_Function_Init()`, `DMA_Tx_Init()`, `Get_ConversionVal()`)

**Types:**
- StdPeriph types: `vu32`, `vu16`, `vu8`, `u32`, `u16`, `u8`, `s32`, `s16`, `s8` (defined in `core_riscv.h`)
- Enums: `FlagStatus` (SET/RESET), `FunctionalState` (ENABLE/DISABLE), `ErrorStatus` (READY/NoREADY), `ITStatus` (SET/RESET)

**Macros:**
- Peripheral base address macros: UPPERCASE (e.g., `GPIOA`, `USART1`, `ADC1`) -- cast to struct pointers
- Bit flag macros: `<PERIPHERAL>_<Feature>_<Value>` (e.g., `GPIO_Pin_1`, `GPIO_Mode_AIN`, `ADC_Mode_Independent`, `RCC_APB2Periph_GPIOA`)
- Include guards: `__CH32V30x_<MODULE>_H` (e.g., `__CH32V30x_GPIO_H`)

## Where to Add New Code

**New bare-metal application:**
- Primary code: `example/<PERIPHERAL>/<NewExample>/User/main.c`
- Interrupt handlers: `example/<PERIPHERAL>/<NewExample>/User/ch32v30x_it.c`
- Config: `example/<PERIPHERAL>/<NewExample>/User/ch32v30x_conf.h` (include only needed peripheral headers)

**New peripheral driver:**
- Header: `example/SRC/Peripheral/inc/ch32v30x_<periph>.h`
- Implementation: `example/SRC/Peripheral/src/ch32v30x_<periph>.c`
- Also update: `example/SRC/Peripheral/inc/ch32v30x.h` (add IRQn entries if needed)
- User inclusion: Add `#include "ch32v30x_<periph>.h"` to `ch32v30x_conf.h`

**New hardware abstraction module:**
- Implementation: `example/SRC/Peripheral/src/` (follow SPL naming and pattern conventions)
- Tests: Not applicable (this is an example library -- testing is done via example projects)

**Utilities/shared helpers:**
- Shared helpers: Add to `example/SRC/` under appropriate subdirectory or create new peripheral driver
- Debug/delay: Extend `example/SRC/Debug/debug.c/.h`

**RTOS application:**
- User code: Follow the pattern in `example/FreeRTOS/`, `example/RT-Thread/`, or `example/TencentOS/`
- RTOS kernel: Already provided as ported source in respective example directories

**New IDE targets:**
- Create `.wvproj` and `.launch` files in the new example directory
- Use existing example as template

## Special Directories

**`.claude/`:**
- Purpose: GSD AI development framework installation (agents, commands, runtime, hooks)
- Generated: No (installed by GSD tooling)
- Committed: Yes

**`.planning/`:**
- Purpose: GSD planning documents (codebase maps, phase plans, etc.)
- Generated: Yes (by GSD commands like `/gsd:map-codebase`)
- Committed: Yes

**`Core/` and `Debug/` (project root):**
- Purpose: Duplicate copies of `example/SRC/Core/` and `example/SRC/Debug/` for the root-level project
- Generated: No (manually placed)
- Committed: Yes
- Note: These appear to be local copies for the root `CH32V303CBT.wvproj` project. The canonical (most up-to-date) versions are in `example/SRC/`.

**`example/SRC/`:**
- Purpose: Shared library -- single source of truth for all SPL, Core, Debug, Startup, and Linker files
- Generated: No (provided by WCH SDK and shared across all example projects)
- Committed: Yes

---

*Structure analysis: 2026-05-28*
