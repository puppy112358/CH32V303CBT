# External Integrations

**Analysis Date:** 2026-05-28

## APIs & External Services

**No external APIs or cloud services detected.** This is a bare-metal embedded firmware project running on a CH32V303CBT6 microcontroller. All functionality is self-contained on-chip. There are no HTTP clients, SDK imports, or network service integrations.

## Data Storage

**On-Chip Storage:**
- Internal Flash: 128 KB at `0x08000000` - Program code and read-only data
  - Flash controller driver: `Peripheral/src/ch32v30x_flash.c`, `Peripheral/inc/ch32v30x_flash.h`
  - IAP (In-Application Programming) examples available in `example/IAP/`
- Internal SRAM: 32 KB at `0x20000000` - Runtime data, stack, heap
- Backup Registers (BKP): Battery-backed registers for persistent data across resets
  - Driver: `Peripheral/src/ch32v30x_bkp.c`, `Peripheral/inc/ch32v30x_bkp.h`

**External Storage (peripheral support, not in main application):**
- SDIO interface support for SD cards: `Peripheral/inc/ch32v30x_sdio.h`, `example/SDIO/`
- FSMC (Flexible Static Memory Controller) for external SRAM/NOR/NAND: `Peripheral/inc/ch32v30x_fsmc.h`, `example/FSMC/`
  - Note: CH32V303CBT6 in 48-pin LQFP package has limited FSMC pin availability

**File Storage:**
- Not applicable - no filesystem in the current main application

**Caching:**
- Not applicable - no caching layer. The MCU has no data cache.

## Communication Interfaces (On-Chip Peripherals)

These are hardware interfaces supported by the MCU peripheral library. The current `main.c` only uses USART1 for debug output.

**USART (Serial):**
- Driver: `Peripheral/src/ch32v30x_usart.c`, `Peripheral/inc/ch32v30x_usart.h`
- Current usage: USART1 (PA9 TX) for printf debug output at 115200 baud (`User/main.c` line 43)
- Debug module: `Debug/debug.c` provides `USART_Printf_Init()` and `printf` redirection
- Examples in `example/USART/`

**SPI:**
- Driver: `Peripheral/src/ch32v30x_spi.c`, `Peripheral/inc/ch32v30x_spi.h`
- Examples in `example/SPI/`

**I2C:**
- Driver: `Peripheral/src/ch32v30x_i2c.c`, `Peripheral/inc/ch32v30x_i2c.h`
- Examples in `example/I2C/`

**CAN:**
- Driver: `Peripheral/src/ch32v30x_can.c`, `Peripheral/inc/ch32v30x_can.h`
- Examples in `example/CAN/` (Networking, TestMode, Time-triggered)

**USB:**
- Driver: `Peripheral/inc/ch32v30x_usb.h` (header only)
- Examples in `example/USB/`

**Ethernet:**
- Driver: `Peripheral/src/ch32v30x_eth.c`, `Peripheral/inc/ch32v30x_eth.h`
- Examples in `example/ETH/`
- Note: CH32V303CBT6 may not expose full Ethernet MAC pins on 48-pin package

**SDI Printf (Serial Wire Debug):**
- Support for printf output over SWD debug interface (`Debug/debug.h` lines 34-39, controlled by `SDI_PRINT` macro)
- Currently disabled (`SDI_PR_CLOSE`)
- Examples in `example/SDI_Printf/`

## Authentication & Identity

**No authentication providers.** This is a headless embedded system.

**Chip Identification:**
- `DBGMCU_GetCHIPID()` function reads a unique chip ID (printed in `User/main.c` line 45 via `printf("ChipID:%08x\r\n",...)`)
- CRC hardware module available for data integrity checks: `Peripheral/src/ch32v30x_crc.c`

## Monitoring & Observability

**Error Tracking:**
- None - no external error tracking service. Errors are handled locally.

**Logs:**
- Debug printf via USART1 (PA9) at 115200 baud - the primary observability mechanism
- SDI printf via SWD debug interface (disabled by default)
- Output examples from `main.c`: system clock frequency, chip ID, and custom messages

**Debug Interface:**
- WCH-Link debug probe via SWD (Serial Wire Debug)
- OpenOCD server with GDB client for step-through debugging
- Hardware breakpoints and watchpoints supported

**Built-in Debug/Trace Peripherals:**
- DBGMCU (Debug MCU) registers for freeze control during debugging: `Peripheral/inc/ch32v30x_dbgmcu.h`

## CI/CD & Deployment

**Hosting:**
- Not applicable - firmware runs on MCU hardware, not a hosted service

**CI Pipeline:**
- No CI/CD configuration detected (no `.github/workflows`, `.gitlab-ci.yml`, Jenkinsfile, etc.)

**Build output:**
- `.hex` file for flash programming (target path: `obj/CH32V303CBT.hex`, per `.template` line 2)
- ELF binary for debugging
- Extended listing (.lst) for disassembly inspection

**Flash Programming:**
- Tool: WCH-Link hardware probe
- Protocol: WCH proprietary SWD-based programming
- Options: Erase All, Program, Verify (per `.template` lines 4-7)
- Config file: `wch-riscv.cfg` (referenced in `CH32V303CBT.launch` line 30)

## Environment Configuration

**Required env vars:**
- None - no environment variables are required or used by this embedded C project

**Secrets location:**
- Not applicable - no secrets, API keys, or credentials exist in this codebase

## Webhooks & Callbacks

**Incoming:**
- None - the MCU has no network stack in the current main application configuration

**Outgoing:**
- None - no outbound network communication in the current main application configuration

**Interrupt Callbacks (Internal):**
- Interrupt handlers are defined as weak aliases in the startup vector table (`Startup/startup_ch32v30x_D8.S`)
- User override handlers in `User/ch32v30x_it.c` (currently only NMI_Handler and HardFault_Handler overridden)
- Interrupt attribute: `__attribute__((interrupt("WCH-Interrupt-fast")))` for hardware-stacked fast interrupts

## RTOS Integration Examples (Reference Only)

The `example/` directory contains reference projects showing integration with third-party RTOS kernels. These are **not part of the main application** but demonstrate the platform's RTOS capability:

| RTOS | Example Location |
|------|-----------------|
| FreeRTOS | `example/FreeRTOS/FreeRTOS_Core/` |
| RT-Thread | `example/RT-Thread/rt-thread/` |
| Huawei LiteOS | `example/HarmonyOS/LiteOS_m/` |
| TencentOS Tiny | `example/TencentOS/TencentOS/` |

---

*Integration audit: 2026-05-28*
