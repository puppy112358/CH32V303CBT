# Codebase Concerns

**Analysis Date:** 2026-05-28

## Tech Debt

### Hardcoded Clock Configuration in Source File

- Issue: The system clock frequency is selected via a `#define` hardcoded near the top of `User/system_ch32v30x.c` (line 24: `#define SYSCLK_FREQ_96MHz_HSE  96000000`). Changing the clock requires editing the source file and recompiling. There is no configuration header or build-system flag for this.
- Files: `User/system_ch32v30x.c`
- Impact: Switching between clock speeds (e.g., for power tuning or different board revisions) forces source-level changes, increasing the risk of forgetting to update it across branches.
- Fix approach: Move the `#define` to a standalone configuration header (e.g., `User/clock_config.h`) that is included before `system_ch32v30x.c`, or pass it as a compiler `-D` flag from `.cproject`.

### Massive Code Duplication in Clock Setup Functions

- Issue: Each of the twelve `SetSysClockToXX_YYY()` functions (lines 281-1036 in `User/system_ch32v30x.c`) repeats identical HSE startup polling, timeout, and PLL enable/switch logic. Approximately 700+ lines of near-identical code that differ only in the PLL multiplier and a few register writes.
- Files: `User/system_ch32v30x.c`
- Impact: Any bug fix to the HSE startup or PLL sequence must be replicated across 12 functions. Risk of missing a fix in one variant.
- Fix approach: Refactor into a single parameterized function accepting multiplier and divider arguments. The compile-time `#ifdef` branches would then be trivial wrappers.

### Empty Error Handlers for HSE Startup Failure

- Issue: Every clock initialization function contains an `else` branch for HSE startup failure (e.g., lines 321-326, 392-398, etc.) that is completely empty with only a comment saying "User can add here some code to deal with this error."
- Files: `User/system_ch32v30x.c` (12 occurrences)
- Impact: If the external HSE crystal fails or is unpopulated, the MCU will silently continue with an uninitialized or misconfigured clock system, leading to undefined behavior.
- Fix approach: At minimum, fall back to HSI as a safe clock source. At best, signal the failure via a GPIO/LED or log via UART. Implement a common error handler called from all clock setup functions.

### No .gitignore File

- Issue: The repository has no `.gitignore` file. Build output directories (`Debug/`, `obj/`, any `.o`, `.elf`, `.hex`, `.map` files) are not excluded.
- Files: Repository root
- Impact: Build artifacts, IDE user-specific files, and potentially debug binaries may be accidentally committed, bloating the repository and causing merge conflicts.
- Fix approach: Create a `.gitignore` file excluding at minimum `Debug/`, `obj/`, `*.o`, `*.elf`, `*.hex`, `*.map`, `*.bin`, `*.lst`, and IDE metadata.

### Slow-Scope Peripheral Library Sources

- Issue: The `Peripheral/src/` directory contains full driver source files (e.g., `ch32v30x_adc.c`, `ch32v30x_can.c`, etc.) but these are not explicitly tracked as versioned dependencies. They appear to be copied from the WCH SDK without clear version metadata in the file.
- Files: `Peripheral/src/ch32v30x_*.c` (20+ files), `Peripheral/inc/ch32v30x_*.h` (20+ files)
- Impact: Difficult to determine which SDK version was used. Upgrading the peripheral library is manual and error-prone. No package management or submodule tracking.
- Fix approach: Document the SDK version in a file (e.g., `SDK_VERSION`) or manage the Peripheral directory as a git submodule pointing to the official WCH SDK repository. Alternatively, record the source URL and version in `README.md`.

### Dual Startup File Ambiguity

- Issue: Two startup files exist: `Startup/startup_ch32v30x_D8.S` (for CH32V303) and `Startup/startup_ch32v30x_D8C.S` (for CH32V305/CH32V307). The `.cproject` build configuration must include only the correct one, but there is no documentation about which is in use.
- Files: `Startup/startup_ch32v30x_D8.S`, `Startup/startup_ch32v30x_D8C.S`
- Impact: A developer unfamiliar with the hardware variants could accidentally switch startup files and produce a binary targeting the wrong chip's vector table.
- Fix approach: Document which startup file is active in `README.md`. Consider deleting the unused variant, or using a build configuration flag to select the correct one.

### IDE-Only Build System

- Issue: The project relies entirely on Eclipse CDT with the GNU MCU Eclipse plugin (`.cproject`). There is no `Makefile`, `CMakeLists.txt`, or command-line build script.
- Files: `.cproject`, `.project`
- Impact: Builds can only be performed within the Eclipse IDE. This blocks CI/CD pipelines, automated testing, and headless builds. New developers must install specific IDE tooling.
- Fix approach: Generate or write a standalone `Makefile` from the `.cproject` settings. Tools like `eclipse-make` can extract build commands. Alternatively, adopt CMake as a cross-platform build system.

## Known Bugs

### Delay Functions Break SysTick for Other Uses

- Symptoms: After calling `Delay_Us()` or `Delay_Ms()`, the SysTick peripheral is left disabled (line 56 and 81 of `Debug/debug.c`: `SysTick->CTLR &= ~(1 << 0)`). Any code that relies on `SysTick_Handler` for a system tick (scheduling, timeouts) will stop working.
- Files: `Debug/debug.c` (lines 43-82)
- Trigger: Call `Delay_Us()` or `Delay_Ms()` from any context. The SysTick counter is used for the delay and then shut down.
- Workaround: Re-initialize SysTick after every delay call if a system tick is needed. Alternatively, use a hardware timer (TIM2-TIM7) for delays instead of SysTick.

### SDI Printf Reads Past Buffer on Last Chunk

- Symptoms: In `_write()` with `SDI_PRINT == SDI_PR_OPEN`, the code at lines 205-206 always reads 7 bytes from the input buffer (`*(buf+i+3)` through `*(buf+i+6)`) even when fewer than 7 bytes remain (`writeSize < 7`).
- Files: `Debug/debug.c` (lines 205-206)
- Trigger: Call `printf()` with a string whose length is not a multiple of 7 when SDI_PRINT is enabled. The function reads beyond the allocated buffer.
- Workaround: Pad printf strings to multiples of 7 bytes, or disable SDI_PRINT (`#define SDI_PRINT SDI_PR_CLOSE`).

## Security Considerations

### HardFault Handler Immediately Resets Without Diagnostics

- Risk: In `HardFault_Handler` (line 38-44 in `User/ch32v30x_it.c`), the MCU simply calls `NVIC_SystemReset()` with no stack dump, register capture, or fault logging. This makes debugging production failures nearly impossible and could mask ongoing memory corruption.
- Files: `User/ch32v30x_it.c` (lines 38-44)
- Current mitigation: None. The system reboots silently.
- Recommendations: Before resetting, save crash context (mepc, mcause, mtval, stack pointer) to a preserved RAM region (noinit section) that survives reset. Log or report on next boot. At minimum, toggle a GPIO or blink an LED pattern to signal fault occurrence.

### NMI Handler Deadlocks Without Recovery

- Risk: `NMI_Handler` is an infinite empty loop (lines 24-29 in `User/ch32v30x_it.c`). A non-maskable interrupt will lock the system permanently.
- Files: `User/ch32v30x_it.c` (lines 24-29)
- Current mitigation: None. The CPU hangs forever.
- Recommendations: At minimum, implement the same reset behavior as `HardFault_Handler`. Better: log the NMI occurrence and then reset, since NMIs typically indicate hardware-level faults that require restart.

### Default Weak Interrupt Handlers Jump to Infinite Loop

- Risk: The startup assembly (`Startup/startup_ch32v30x_D8.S` and `Startup/startup_ch32v30x_D8C.S`) provides weak default handlers for all ~90 interrupt vectors. Every default handler is a `j 1b` (infinite spin loop at lines 320-321). If any peripheral interrupt is accidentally enabled without a proper handler installed, the system deadlocks.
- Files: `Startup/startup_ch32v30x_D8.S` (lines 226-321), `Startup/startup_ch32v30x_D8C.S` (lines 226-321)
- Current mitigation: None. The infinite loop has no timeout or watchdog.
- Recommendations: Replace the default infinite-loop handler with one that logs the unexpected IRQ number and either resets or disables the offending interrupt. Alternatively, implement a default handler that captures the `mcause` register.

### No Stack Overflow Protection

- Risk: The linker script (`Ld/Link.ld`) places the stack at `ORIGIN(RAM) + LENGTH(RAM) - __stack_size` (2048 bytes). The heap (via `_sbrk` in `Debug/debug.c`) grows upward from `_end`. There is no guard page, MPU protection, or runtime check to detect stack/heap collision.
- Files: `Ld/Link.ld` (stack definition), `Debug/debug.c` (lines 239-250, `_sbrk`)
- Current mitigation: None. With only 32KB of RAM, collision is a real risk under moderate heap usage combined with deep call stacks or interrupt nesting.
- Recommendations: Add a stack canary/watermark at the top of `.bss` and check it periodically. Use the MPU to protect the stack guard region. Alternatively, avoid dynamic memory allocation (`malloc`) entirely and use only static allocation, which is a common pattern for safety-critical embedded firmware.

### No MPU or Memory Protection Configured

- Risk: The startup code and `SystemInit` do not configure the RISC-V Physical Memory Protection (PMP) unit. Any code can write to any memory-mapped peripheral or RAM region.
- Files: `Startup/startup_ch32v30x_D8.S` (handle_reset), `User/system_ch32v30x.c` (SystemInit)
- Current mitigation: None. There is no separation between application and system memory.
- Recommendations: Configure PMP to make unused flash/RAM regions inaccessible, and to mark critical configuration registers as read-only after initialization. This is especially important if the application ever runs untrusted code or handles safety-critical functions.

## Performance Bottlenecks

### Blocking Delay Functions

- Problem: `Delay_Us()` and `Delay_Ms()` are fully blocking busy-wait loops that poll the SysTick status flag. The CPU is 100% utilized during any delay.
- Files: `Debug/debug.c` (lines 43-82)
- Cause: The functions use a simple polling loop (`while((SysTick->SR & (1 << 0)) != (1 << 0))`) without any sleep or yield mechanism.
- Improvement path: For longer delays (tens of milliseconds), use a timer interrupt-based delay that allows the CPU to enter WFI sleep. For sub-millisecond delays where busy-wait is acceptable, document the trade-off clearly.

### printf Blocking with No Buffer/Interrupt-Driven TX

- Problem: The `_write()` function (lines 174-230 in `Debug/debug.c`) transmits each character synchronously, waiting for `USART_FLAG_TC` (transmission complete) before sending the next byte. At 115200 baud, each character takes ~87us, blocking the CPU for the full printf duration.
- Files: `Debug/debug.c` (lines 215-227)
- Cause: No TX buffer, no DMA, no interrupt-driven transmission.
- Improvement path: Implement a ring buffer with interrupt-driven TX. The `_write()` function would copy data to the buffer and return immediately. The USART TXE interrupt would drain the buffer asynchronously. This is standard practice for non-trivial embedded firmware.

### No DMA Usage for Common Peripherals

- Problem: The project configures USART for print-debug but does not use DMA for any peripheral. Blocking CPU-polled I/O is used throughout.
- Files: `Debug/debug.c`, `User/main.c`
- Cause: Template/skeleton project with minimal functionality.
- Improvement path: As peripherals (ADC, SPI, I2C) are enabled, use DMA for data transfers larger than a few bytes to free the CPU for application logic.

## Fragile Areas

### SysTick Usage in Delays

- Files: `Debug/debug.c` (lines 43-82)
- Why fragile: The delay functions reconfigure SysTick with custom compare values and then disable it. Any ISR that fires during a delay and also tries to use SysTick will see corrupted state. Any RTOS or scheduling layer that expects SysTick to generate periodic interrupts will malfunction.
- Safe modification: If delays are needed alongside a system tick, use a free-running hardware timer (TIM6 or TIM7) for delays and leave SysTick exclusively for the OS/scheduler. Alternatively, convert delays to non-blocking state-machine-based timing.
- Test coverage: None - no test files exist in the project.

### _sbrk Heap Implementation

- Files: `Debug/debug.c` (lines 239-250)
- Why fragile: The `_sbrk` implementation uses a simple `curbrk` pointer that advances toward `_heap_end`. There is no thread safety (no lock), no fragmentation management, and the check `curbrk + incr > _heap_end` only validates the current allocation, not total free space. On a system with only 32KB RAM, this is a high-risk area.
- Safe modification: If dynamic allocation is truly needed, replace with a lightweight allocator (e.g., TLSF or a fixed-block pool). Strongly consider eliminating `malloc`/`free` entirely and using only static allocation.
- Test coverage: None.

### USART Printf Initialization Assumes Fixed Pins

- Files: `Debug/debug.c` (lines 93-146)
- Why fragile: The `USART_Printf_Init` function hardcodes GPIO pins (PA9 for UART1, PA2 for UART2, PB10 for UART3). If the board layout remaps these pins, the function silently uses the wrong pins. There is no validation that these pins are actually connected to a UART transceiver.
- Safe modification: Make pin assignments configurable via `#define` or a configuration struct. Add a compile-time or runtime check that the selected UART peripheral is actually usable.
- Test coverage: None.

## Scaling Limits

### RAM Capacity: 32KB

- Current capacity: The linker script is configured for 128KB Flash / 32KB RAM (CH32V303CB variant).
- Limit: 32KB is shared between `.data`, `.bss`, heap, and a 2KB stack. Global variables, printf buffers, and peripheral driver state will quickly consume this. The alternate configuration for larger chips (128KB+ RAM) is commented out in the linker script.
- Scaling path: If the application grows, evaluate using a CH32V303RB (64KB RAM) or CH32V303RC/Vx variants (128KB+). Minimize heap usage; prefer stack and global allocation. Profile RAM usage with linker map analysis.

### Flash Capacity: 128KB

- Current capacity: 128KB flash. The Peripheral library alone is substantial (20+ C files).
- Limit: With all peripheral drivers compiled in and full optimization (`-Os`), application code space may be tight. Full debug builds (`-O0`) may not fit.
- Scaling path: Use `--gc-sections` and `-ffunction-sections`/`-fdata-sections` (already configured in `.cproject`). Only include peripheral sources actually needed by the application. Strip unused functions.

## Dependencies at Risk

### WCH Proprietary RISC-V Toolchain

- Risk: The build system uses `riscv-none-embed-gcc` with custom WCH extensions (`-march=rv32imacxw`). The `xw` extension is WCH-specific (hardware stack, fast interrupt). This toolchain is only distributed by WCH and not part of mainline RISC-V GCC.
- Files: `.cproject` (ISA options), `Startup/startup_ch32v30x_D8.S` (custom CSRs like 0x804, 0xbc0)
- Impact: Cannot build with standard `riscv32-unknown-elf-gcc`. If WCH stops distributing or updating the toolchain, the project becomes unbuildable. Bug fixes in upstream GCC do not automatically apply.
- Migration plan: Keep the WCH toolchain installer archived in the repository or as a documented dependency. For long-term viability, track whether the WCH extensions are being upstreamed to mainline GCC/RISC-V specs.

### No SDK Version Pinning

- Risk: The WCH Peripheral Library files (`Peripheral/inc/` and `Peripheral/src/`) exist as raw source copies without version metadata. The header comment in `Peripheral/inc/ch32v30x.h` (the main chip header) does not declare a library version.
- Files: All files in `Peripheral/inc/` and `Peripheral/src/`
- Impact: If WCH releases a new SDK with breaking changes or bug fixes, there is no way to diff against the baseline or know what version was originally used.
- Migration plan: Tag the initial import with a git tag (e.g., `sdk-v2.9`). Document the SDK version in `README.md`. On future SDK updates, commit the changes as a separate commit with the new version documented.

## Missing Critical Features

### No Watchdog Configuration

- Problem: The Independent Watchdog (IWDG) and Window Watchdog (WWDG) are not configured or enabled. A firmware hang (e.g., infinite loop in an interrupt handler) will not be recovered.
- Blocks: Production deployment. Any field-deployed device without a watchdog can permanently lock up.
- Fix approach: Enable IWDG in `main()` after system initialization, and periodically refresh it. For a bare-metal system, refresh in the main loop. Consider the WWDG for more precise timing violation detection.

### No Clock Security System (CSS)

- Problem: The Clock Security System is not enabled. If the HSE crystal fails during operation, the MCU will continue running with a potentially corrupted or stopped clock.
- Blocks: Safety-critical applications that require reliable clock operation.
- Fix approach: Enable CSS in `SystemInit` after configuring HSE. The CSS will automatically switch to HSI and trigger an NMI if HSE fails. Update `NMI_Handler` to handle this event.

### No Power Management

- Problem: The project has no sleep/deep-sleep support. The main loop (`while(1){}` in `User/main.c`) is a bare spin loop that wastes power.
- Blocks: Battery-powered or energy-sensitive applications.
- Fix approach: Call `__WFI()` in the main loop to enter sleep mode when no work is pending. For deeper sleep, configure `PWR_EnterSTOPMode()` or `PWR_EnterSTANDBYMode()` and wake via external interrupt or RTC alarm.

### No Bootloader or Firmware Update Mechanism

- Problem: There is no OTA (over-the-air) or serial bootloader support. Firmware updates require physical access to the SWD/JTAG programmer.
- Blocks: Field-deployed devices that need firmware updates without disassembly.
- Fix approach: Implement a simple UART-based bootloader in a protected flash sector, or use WCH's built-in ISP bootloader with a custom host-side flashing tool.

## Test Coverage Gaps

### No Test Files Exist

- What's not tested: The entire codebase. There are no test files, no test framework, no unit tests, and no integration tests.
- Files: All source files.
- Risk: Any change to the clock configuration, peripheral drivers, delay functions, or interrupt handlers is completely untested. Regressions will only be discovered on hardware.
- Priority: Medium. For an MCU project, full hardware-in-the-loop testing is the gold standard, but at minimum, host-compiled unit tests for pure-logic functions (clock calculation, delay math, buffer management) can catch regressions early.

### No Hardware Abstraction Layer Testing

- What's not tested: The peripheral driver layer (`Peripheral/src/ch32v30x_*.c`). These are vendor-supplied files, but any bug introduced by SDK updates or local modifications goes undetected.
- Files: `Peripheral/src/ch32v30x_*.c`
- Risk: Register-level bugs in peripheral initialization or configuration can cause subtle hardware misbehavior that is difficult to diagnose.
- Priority: Low (vendor library), but any locally modified driver files should receive targeted testing.

---

*Concerns audit: 2026-05-28*
