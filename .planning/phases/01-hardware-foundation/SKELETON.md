# Walking Skeleton — Electronic Load Controller (电子负载控制器)

**Phase:** 1 — Hardware Foundation
**Generated:** 2026-06-04

## Capability Proven End-to-End

A developer compiles firmware, flashes to CH32V303CBT6 via WCH-Link, and sees real INA226 bus voltage data printed on BOTH a USART1 hardware serial terminal (PA9, 115200 8N1) and a USB-CDC virtual COM port simultaneously. The DAC8571 outputs a measurable analog voltage at mid-scale, and the I2C bus automatically recovers from slave-stuck faults via GPIO bit-bang recovery.

## Architectural Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| MCU Platform | CH32V303CBT6 (RISC-V RV32IMAC, 128K Flash, 32K SRAM, 96MHz) | Project hardware constraint; WCH Standard Peripheral Library V2.9 provides full HAL |
| Runtime Model | Bare-metal super-loop (no RTOS) | 32K SRAM insufficient for RTOS overhead; deterministic polling adequate for 100ms control loop |
| I2C Bus Topology | I2C1 at 100kHz standard mode, PB6 (SCL) / PB7 (SDA), alternate-function open-drain | Single bus serves 6 devices (5x INA226 + 1x DAC8571); 100kHz chosen for reliability over mixed-load bus |
| I2C Abstraction | i2c_util wrapper over SPL I2C with tick-count timeout + GPIO bit-bang bus recovery | SPL I2C examples lack timeout protection; wrapper adds fault tolerance without rewriting SPL state machine |
| I2C Bus Recovery | GPIO bit-bang 9 SCL pulses with SDA as high-impedance input, then STOP condition, then peripheral re-init | Industry-standard method (Linux kernel, Espressif, Atmel); I2C_SoftwareResetCmd alone does not release stuck SDA |
| Driver Organization | `Drivers/` directory at project root, per-device `.c/.h` pairs, lowercase file names, relative includes (no .cproject changes) | Isolates device code from SPL; relative includes keep MounRiver IDE project configuration unchanged |
| Debug Channel Architecture | Dual output: USART1 printf (PA9, 115200bps, always active) + USB-CDC usb_printf (virtual COM port, available when USB connected) | USART1 provides reliable always-on debug; USB-CDC provides convenient PC-side logging without extra hardware |
| USB-CDC Implementation | Adapted from WCH SimulateCDC example, simplified to device-only (no UART bridge) | Removes ~3KB of UART bridge buffers (UART2, TIM2, DMA); keeps EP1 (CDC notifications) and EP3 (bulk IN for debug output); EP2 OUT removed |
| USBFS Clock | 48MHz via PLLCLK_Div2 from 96MHz SystemCoreClock | Required for USB Full Speed (12Mbps) operation; verified in WCH SimulateCDC RCC init |
| Build System | MounRiver Studio IDE, riscv-none-embed-gcc, newlib-nano, custom Link.ld | Existing project toolchain; no modifications to .cproject or Link.ld needed |
| Memory Strategy | Static allocation only: global INA226_Dev array (5 structs = ~10 bytes), USB-CDC format buffer (128 bytes stack), endpoint buffers (64 bytes each × 3 endpoints = 192 bytes static) | 32K SRAM constraint; no malloc/heap usage in Phase 1; total Phase 1 memory addition ~500 bytes |
| INA226 Calibration | Compile-time #define calibration register value, shared across all devices, identical shunt resistor assumption | Simplifies init; single calibration write per device; all devices must use same R_SHUNT on PCB |

## Stack Touched in Phase 1

- [x] Project scaffold — `Drivers/` directory with 4 driver modules (i2c_util, ina226, dac8571, usb_cdc)
- [x] I2C communication — I2C1 master read/write with timeout protection
- [x] I2C device drivers — INA226 (voltage/current/power read) and DAC8571 (16-bit DAC write)
- [x] I2C bus fault recovery — Automatic 9-clock-pulse GPIO recovery on timeout
- [x] Debug output — Dual-channel: USART1 printf (existing) + USB-CDC usb_printf (new)
- [x] USB device stack — CDC ACM class with simplified descriptors (no UART bridge)
- [x] Dev deployment — Firmware builds in MounRiver IDE and flashes via WCH-Link

## Out of Scope (Deferred to Later Phases)

- CV/CC control loop and PID controller — Phase 2
- INA226 ALARM EXTI interrupt handling and over-current protection — Phase 2
- INA226 calibration register periodic re-write verification — Phase 2
- cJSON protocol over UART0 for remote commands — Phase 3
- 10Hz telemetry data reporting — Phase 3
- WS2812 LED strip status indication — Phase 4
- Fan PWM control and PID cooling — Phase 4
- Any dynamic I2C address scanning or hot-plug detection
- I2C non-blocking interrupt-driven transfers (all Phase 1 I2C is blocking with timeout)
- USB-CDC host-to-device data (EP2 OUT removed — debug output only)

## Subsequent Phase Plan

Each later phase adds one vertical capability on top of this skeleton without altering its architectural decisions:

- **Phase 2**: Developer can set CV or CC mode and the load maintains the setpoint with overload protection — PID controller, INA226 alarm ISR, soft-start ramp, DAC output driven by control loop
- **Phase 3**: Developer can remotely control the load via cJSON commands over UART0 and receive 10Hz telemetry — cJSON parser, UART0 with odd parity, telemetry packet assembly, non-blocking serial
- **Phase 4**: Developer can see load status on WS2812 LED strip and hear the fan respond to temperature — WS2812 DMA driver, fan PID control, tachometer RPM measurement
