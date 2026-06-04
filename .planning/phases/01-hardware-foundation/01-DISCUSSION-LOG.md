# Phase 1: Hardware Foundation - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-04
**Phase:** 1-Hardware Foundation
**Areas discussed:** Driver file organization, I2C abstraction layer, INA226 driver design, USB-CDC approach

---

## Driver File Organization

| Option | Description | Selected |
|--------|-------------|----------|
| User/ directory (flat) | Put all new driver .c/.h files alongside main.c in User/ | |
| New Drivers/ directory | New Drivers/ directory at project root | ✓ |
| User/Hardware/ subdirectory | Hardware-specific code under User/Hardware/ | |

| Option | Description | Selected |
|--------|-------------|----------|
| Per-device: ina226.c/.h, dac8571.c/.h | One .c/.h pair per device | ✓ |
| Single file: i2c_devices.c/.h | All I2C devices in one file | |
| Layered: i2c/, sensors/, actuators/ | Split by layer with subdirectories | |

| Option | Description | Selected |
|--------|-------------|----------|
| Lowercase: ina226.c, dac8571.c | Match existing project convention | ✓ |
| Uppercase: INA226.c, DAC8571.c | Match datasheet capitalization | |
| Prefixed: el_ina226.c, el_dac8571.c | Prefixed to avoid collisions | |

| Option | Description | Selected |
|--------|-------------|----------|
| Update .cproject include path | Add Drivers/ to Eclipse CDT includes | |
| Relative paths in #include | Use relative paths like ../Drivers/ina226.h | ✓ |
| Compiler flag only | -IDrivers flag, IDE may show false errors | |

**User's choice:** New Drivers/ directory with per-device lowercase files, relative includes.
**Notes:** Clean separation from SPL and application code. No .cproject modification needed.

---

## I2C Abstraction Layer

| Option | Description | Selected |
|--------|-------------|----------|
| Shared i2c_util wrapper | Wrapper functions for timeout, recovery, retry | ✓ |
| Direct SPL calls per driver | Each driver calls SPL I2C directly | |
| Lightweight macros in header | Inline helper macros, SPL direct calls | |

| Option | Description | Selected |
|--------|-------------|----------|
| Full: timeout + recovery + retry | Timeout on every op, auto bus reset, retry | ✓ |
| Medium: timeout + manual recovery | Timeout + status checking, manual reset | |
| Minimal: timeout only | Just timeout, manual recovery | |

| Option | Description | Selected |
|--------|-------------|----------|
| 10ms timeout, 3 retries, then reset | Conservative retry before reset | |
| 5-10ms timeout, reset early | Fast failure detection, minimal retries | ✓ |
| 50ms timeout, 5 retries | Very conservative, slow fault detection | |

| Option | Description | Selected |
|--------|-------------|----------|
| 100kHz standard mode | Maximum compatibility, robust with 6 devices | ✓ |
| 400kHz fast mode | Faster reads for 5 INA226s | |
| Datasheet-driven (research) | Let research determine optimal speed | |

**User's choice:** Shared i2c_util wrapper with full protection (timeout + auto-recovery + retry), 5-10ms per-op timeout with early bus reset, 100kHz bus speed.
**Notes:** I2C-03 requirement explicitly needs automatic bus recovery. The 9-clock-pulse SDA reset sequence is implemented once in i2c_util.

---

## INA226 Driver Design

| Option | Description | Selected |
|--------|-------------|----------|
| Per-device struct (stateful) | Struct with addr, calibration, cached values | ✓ |
| Stateless with addr param | Functions take I2C addr as first param | |
| Hybrid: config struct + stateless reads | Config in struct, runtime reads stateless | |

| Option | Description | Selected |
|--------|-------------|----------|
| Calibration computed from shunt + max current | Dynamic per-device calibration | |
| Fixed #define calibration | Compile-time shared calibration register value | ✓ |
| Full config struct | All config fields in init struct | |

| Option | Description | Selected |
|--------|-------------|----------|
| Bulk read: all values at once | One burst read per device | |
| Per-register getters | Individual getters for voltage/current/power | ✓ |
| Both bulk + individual getters | Bulk for loop, individual for spot checks | |

| Option | Description | Selected |
|--------|-------------|----------|
| Driver configures; app handles ISR | Alert configured at init, ISR in app layer | |
| Driver provides alert check function | ina226_check_alert() returns mask | ✓ |
| Driver: register access only | Only read/write, no alert logic | |

**User's choice:** Stateful per-device struct, fixed #define calibration shared across all 5 devices, per-register getter API, driver provides alert check function.
**Notes:** All 5 INA226s must use identical shunt resistors with the fixed calibration approach. Application calls ina226_check_alert() from ISR or polling loop.

---

## USB-CDC Approach

| Option | Description | Selected |
|--------|-------------|----------|
| Adapt WCH CDC example | Use WCH EVT USB CDC example as reference | ✓ |
| Custom minimal CDC | Write from scratch using SPL USB registers | |
| TinyUSB open-source stack | Integrate TinyUSB (CH32V30x support) | |

| Option | Description | Selected |
|--------|-------------|----------|
| Replace: USB-CDC only for printf | USB-CDC replaces USART1 printf | |
| Both: USB-CDC + USART1 printf | Both channels active simultaneously | ✓ |
| Compile-time switchable | Macro to select at build time | |

| Option | Description | Selected |
|--------|-------------|----------|
| Drivers/usb_cdc.c/.h | CDC in Drivers/ alongside other device code | ✓ |
| Extend Debug/debug.c | Add USB CDC path to existing debug module | |
| Drivers/usb/ subdirectory | Separate dir for USB stack files | |

| Option | Description | Selected |
|--------|-------------|----------|
| Dual-output in _write() | Single printf to both channels | |
| Separate print functions | usb_printf() + regular printf() | ✓ |
| USB primary, USART fallback | USB default, USART for critical errors | |

**User's choice:** Adapt WCH CDC example, both channels active, Drivers/usb_cdc.c/.h, separate print functions.
**Notes:** usb_printf() for CDC output, printf() for USART1. Avoids USB blocking issues affecting USART1 debug output.

---

## Claude's Discretion

No decisions deferred to Claude. All choices were user-specified.

## Deferred Ideas

None — discussion stayed within phase scope.
