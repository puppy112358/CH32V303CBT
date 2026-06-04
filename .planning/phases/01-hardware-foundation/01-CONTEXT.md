# Phase 1: Hardware Foundation - Context

**Gathered:** 2026-06-04
**Status:** Ready for planning

## Phase Boundary

All I2C1 devices (5x INA226 at 0x40-0x44, 1x DAC8571 at 0x4C) respond correctly at their assigned addresses. I2C bus automatically recovers from slave-stuck faults via 9-clock-pulse reset. USB-CDC virtual serial port provides debug output alongside existing USART1 printf. No control loop, no cJSON protocol, no WS2812, no fan control — those are Phases 2-4.

## Implementation Decisions

### Driver File Organization
- **D-01:** New `Drivers/` directory at project root for all device driver code.
- **D-02:** Per-device `.c/.h` pairs: `ina226.c/.h`, `dac8571.c/.h`, `i2c_util.c/.h`, `usb_cdc.c/.h`.
- **D-03:** Lowercase file naming to match existing project convention.
- **D-04:** Include files via relative paths (e.g., `#include "../Drivers/ina226.h"`) rather than modifying `.cproject` include paths.

### I2C Abstraction Layer
- **D-05:** Shared `i2c_util.c/.h` wrapper layer in Drivers/ that wraps SPL I2C functions. Device drivers call i2c_util, not SPL I2C directly.
- **D-06:** Full protection: every I2C operation has tick-count timeout, automatic 9-clock-pulse bus recovery on timeout, and retry before giving up.
- **D-07:** Timeout 5-10ms per operation. On timeout, bus reset attempted early (minimal retries — if SDA is stuck, retries waste time).
- **D-08:** I2C1 bus speed: 100kHz standard mode.

### INA226 Driver Design
- **D-09:** Per-device stateful struct (`INA226_Dev`) holding I2C address and device identity. Functions take a pointer to the struct.
- **D-10:** Fixed `#define` calibration — shared compile-time calibration register value, config register, and alert threshold across all 5 devices. All must use identical shunt resistors.
- **D-11:** Per-register getter functions: `ina226_get_voltage()`, `ina226_get_current()`, `ina226_get_power()`, `ina226_get_shunt_voltage()`. Each reads one register on demand.
- **D-12:** Driver provides `ina226_check_alert()` function to read and return the alert register mask. Application code (main loop / ISR) decides what protective action to take.

### USB-CDC Approach
- **D-13:** Adapt from WCH's USB CDC example code (from the EVT examples). Proven on CH32V303, lower risk than writing from scratch.
- **D-14:** Both USB-CDC and USART1 printf active simultaneously. USB-CDC does not replace USART1.
- **D-15:** CDC implementation in `Drivers/usb_cdc.c/.h`.
- **D-16:** Separate print functions: `usb_printf()` for USB-CDC output, standard `printf()` continues to USART1. Clear separation avoids confusion and USB blocking issues.

### Claude's Discretion
No areas deferred to Claude — all decisions were user-specified.

## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Planning & Requirements
- `.planning/ROADMAP.md` — Phase 1 goal, success criteria, and phase dependencies
- `.planning/PROJECT.md` — Project constraints (32K SRAM, 128K Flash, bare-metal), key decisions, evolution rules
- `.planning/REQUIREMENTS.md` — Full requirements traceability (I2C-01, I2C-02, I2C-03, COMM-03 map to Phase 1)

### SPL I2C (foundation for all device communication)
- `Peripheral/inc/ch32v30x_i2c.h` — I2C master API: Init, Cmd, SendData, ReceiveData, GenerateSTART/STOP, flag checking
- `Peripheral/src/ch32v30x_i2c.c` — I2C implementation (SPL V2.9)

### SPL USB (foundation for CDC)
- `Peripheral/inc/ch32v30x_usb.h` — USB peripheral register definitions and structures
- WCH USB CDC EVT example — Reference implementation to adapt (D-13)

### Existing Debug Infrastructure
- `Debug/debug.c` — `_write()` syscall override, `USART_Printf_Init()`, delay functions
- `Debug/debug.h` — Debug macros, UART baud rate config

### I2C Device Datasheets
- INA226 datasheet — Register map (0x00 Config, 0x01 Shunt Voltage, 0x02 Bus Voltage, 0x03 Power, 0x04 Current, 0x05 Calibration, 0x06 Alert), calibration formula, I2C timing
- DAC8571 datasheet — I2C write format (address byte + 2 data bytes MSB-first), output range, power-on behavior

## Existing Code Insights

### Reusable Assets
- **SPL I2C driver** (`Peripheral/src/ch32v30x_i2c.c`): Full I2C master mode — Init, Cmd, SendData, ReceiveData, GenerateSTART/STOP, status flag polling. Called through i2c_util wrapper (D-05).
- **Debug module** (`Debug/debug.c`): `USART_Printf_Init()`, `_write()` syscall for printf, `Delay_Ms()`/`Delay_Us()` for timing.
- **SPL GPIO/RCC**: Pin configuration and clock enable for I2C1 (PB6=SCL, PB7=SDA) and USB (PA11=DM, PA12=DP).
- **SysTick** (`Delay_Init`): Already initialized in main.c — usable for I2C timeout tick counting.

### Established Patterns
- **SPL init struct pattern**: `XXX_InitTypeDef` with zero-init `= {0}`, config struct fields, `XXX_Init(periph, &struct)`, `XXX_Cmd(periph, ENABLE)`. Driver init functions should follow this.
- **Global variables for device state**: 5x INA226 struct instances as file-scope or global arrays.
- **Weak interrupt handlers**: Override in `ch32v30x_it.c` for I2C1 event/error interrupts and USB interrupts.
- **Super-loop in main()**: No RTOS — all operations are blocking or interrupt-flagged.
- **RCC clock before access**: Always enable peripheral clocks before touching registers.

### Integration Points
- **I2C1 GPIO pins**: PB6 (SCL) and PB7 (SDA) — configure as alternate function open-drain in main.c GPIO init
- **USB pins**: PA11 (DM) and PA12 (DP) — configure for USB peripheral
- **`ch32v30x_conf.h`**: Enable I2C and USB peripheral include blocks
- **`ch32v30x_it.c`**: Add I2C1_EV_IRQHandler, I2C1_ER_IRQHandler for bus error detection; USB HP IRQ handler for CDC
- **`main.c`**: Call driver init functions after system init, enter super-loop that reads INA226 data and prints via both debug channels

## Specific Ideas

No specific UI/UX references — this is an embedded hardware validation phase. Output is register values printed to serial console.

## Deferred Ideas

None — discussion stayed within phase scope.

---

*Phase: 1-Hardware Foundation*
*Context gathered: 2026-06-04*
