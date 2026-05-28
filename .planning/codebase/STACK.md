# Technology Stack

**Analysis Date:** 2026-05-28

## Languages

**Primary:**
- C (GNU99 / C99) - All application code, peripheral drivers, system initialization
  - Language standard set in `.cproject` line 68: `ilg.gnumcueclipse.managedbuild.cross.riscv.option.c.compiler.std.gnu99`

**Secondary:**
- Assembly (RISC-V) - Startup code and vector table in `Startup/startup_ch32v30x_D8.S`

## Runtime

**Environment:**
- Bare-metal (no OS) - confirmed by `.template` line 13: `RTOS=NoneOS`
- RISC-V V4 Core (WCH Proprietary) with PFIC (Programmable Fast Interrupt Controller)
- CPU: RV32IMAC (RISC-V 32-bit Integer + Multiply/Divide + Atomics + Compressed instructions)
- ABI: ILP32 (32-bit integer only, no hardware floating-point ABI)

**Target MCU:**
- Model: CH32V303CBT6 (WCH / Nanjing Qinheng Microelectronics)
- Series: CH32V303, variant CH32V30x_D8
- Flash: 128 KB (address range `0x00000000` - `0x00020000`)
- SRAM: 32 KB (address range `0x20000000` - `0x20008000`)
- Max clock: 144 MHz (configured for 96 MHz in this project via `SYSCLK_FREQ_96MHz_HSE`)
- HSE oscillator: 8 MHz (defined in `Peripheral/inc/ch32v30x.h` line 29)
- DVP, Ethernet, and USB OTG peripherals available (CH32V30x_D8C variants excluded)

**Package Manager:**
- Not applicable (no package manager in use - bare-metal embedded project)

**Standard C Library:**
- newlib-nano (linked via `--specs=nano.specs` and `--specs=nosys.specs` in `.cproject` lines 80-81)
- Math library: libm (linked in `.cproject` line 85)

## Frameworks

**Core:**
- CH32V30x Standard Peripheral Library (Version 2.9) - Hardware abstraction and peripheral drivers
  - Version defined at `Peripheral/inc/ch32v30x.h` line 38-41: `__CH32V30x_STDPERIPH_VERSION = 0x0209`
  - Peripheral version from `.template` line 17: `PeripheralVersion=2.9`
  - Contains `Peripheral/inc/` (headers for ADC, BKP, CAN, CRC, DAC, DMA, DVP, ETH, EXTI, FLASH, FSMC, GPIO, I2C, IWDG, OPA, PWR, RCC, RNG, RTC, SDIO, SPI, TIM, USART, USB, WWDG)
  - Contains `Peripheral/src/` (matching implementations)

- RISC-V Core Support Layer: `Core/core_riscv.h` and `Core/core_riscv.c`
  - Provides inline NVIC (PFIC) functions, SysTick control, atomic operations (AMO), RISC-V CSR accessors
  - WCH custom interrupt attribute: `__attribute__((interrupt("WCH-Interrupt-fast")))`
  - Vector table base at `0x00000000`
  - Uses WCH-specific hardware stacking for fast interrupt preemption

**RTOS Support (example code only, not in main project):**
  - FreeRTOS examples available in `example/FreeRTOS/FreeRTOS_Core/`
  - RT-Thread examples available in `example/RT-Thread/rt-thread/`
  - Huawei LiteOS examples available in `example/HarmonyOS/LiteOS_m/`
  - TencentOS examples available in `example/TencentOS/TencentOS/`

**Testing:**
- Not detected - no test framework present. This is a bare-metal embedded project verified through hardware debugging.

**Build/Dev:**
- MounRiver Studio IDE (Eclipse-based, CDT managed build)
  - `.project` identifies this as an Eclipse CDT project with `org.eclipse.cdt.managedbuilder.core.managedBuildNature`
  - `.cproject` defines build configuration managed by `ilg.gnumcueclipse` plugin
- RISC-V Cross GCC Toolchain (`riscv-none-embed-` prefix)
  - C Compiler: `riscv-none-embed-gcc`
  - C++ Compiler: `riscv-none-embed-g++`
  - Assembler with preprocessor support
  - GNU Make for build orchestration (`make` with parallel build enabled, `optimal` parallel jobs)
- Linker: GNU RISC-V Cross C Linker, using custom linker script `Ld/Link.ld`
  - `-nostartfiles` (no standard CRT)
  - `--gc-sections` (remove unused sections)
  - `-lm` (link math library)
- Debugger: OpenOCD + GDB via WCH-Link (config in `CH32V303CBT.launch`)
  - GDB Server: OpenOCD with `wch-riscv.cfg` configuration
  - GDB Client: `riscv-none-embed-gdb` targeting `riscv:rv32`
  - Flash programmer: WCH-Link
- Binary output: ELF executable, with flash image (.hex) creation and extended listing support
- Build output directory: `obj/`

## Key Dependencies

**Critical:**
- CH32V30x Standard Peripheral Library (V2.9) - All MCU hardware interaction flows through this library
  - Headers at `Peripheral/inc/ch32v30x_*.h` (27 peripheral headers)
  - Sources at `Peripheral/src/ch32v30x_*.c` (27 peripheral implementations)
- RISC-V Core Layer (`Core/core_riscv.h`) - NVIC/PFIC, SysTick, atomic operations, CSR access
- System initialization (`User/system_ch32v30x.c`) - Clock tree configuration, system init

**Infrastructure:**
- Debug module (`Debug/debug.c`, `Debug/debug.h`) - USART printf (debug output on PA9/USART1 at 115200 baud), delay functions
- Startup code (`Startup/startup_ch32v30x_D8.S`) - Vector table, reset handler, interrupt stubs
- Linker script (`Ld/Link.ld`) - Memory layout, section placement, 2 KB stack allocation

## Configuration

**Environment:**
- No environment variable files detected (no `.env` files present)
- HSE clock: 8 MHz external oscillator (hardware-defined, set in `Peripheral/inc/ch32v30x.h` line 29)
- System clock configured via macro in `User/system_ch32v30x.c` line 24: `SYSCLK_FREQ_96MHz_HSE` = 96 MHz
- UART debug baud rate: 115200 (set in `User/main.c` line 43)
- Debug UART selection: `DEBUG_UART1` (PA9 TX) via `Debug/debug.h` line 30

**Build:**
- Project configuration: `.cproject` (Eclipse CDT managed build)
- IDE settings: `.project` (Eclipse project definition)
- Device/flash programming: `.template` (MCU type, flash config, programmer settings)
- Debug configuration: `CH32V303CBT.launch` (OpenOCD/GDB launch config)
- Linker script: `Ld/Link.ld`
- Compiler options (from `.cproject`):
  - `-Os` (optimize for size: `optimization.level.size`)
  - `-fmessage-length=0`
  - `-fsigned-char` (char is signed)
  - `-ffunction-sections`, `-fdata-sections`
  - `-Wunused`, `-Wuninitialised`
  - RV32I with M, A, C extensions + WCH XW extension + saverestore
- Include paths: `Startup/`, `Debug/`, `Core/`, `User/`, `Peripheral/inc/`

## Platform Requirements

**Development:**
- MounRiver Studio IDE (WCH-provided Eclipse-based IDE for RISC-V MCU development)
- RISC-V GCC toolchain (`riscv-none-embed-*`)
- WCH-Link debug probe (hardware) for flashing and debugging
- Target hardware: CH32V303CBT6 development board

**Production:**
- Binary flashed to CH32V303CBT6 MCU internal flash at address `0x08000000` (as per `.template` line 3)
- No external dependencies at runtime - fully self-contained embedded firmware
- Erase All + Program + Verify flash programming workflow

---

*Stack analysis: 2026-05-28*
