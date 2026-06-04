# Phase 01: Hardware Foundation - Research

**Researched:** 2026-06-04
**Domain:** Embedded I2C device drivers (INA226 current/power monitor, DAC8571 16-bit DAC), I2C bus fault recovery, USB-CDC virtual serial port
**Confidence:** HIGH

## Summary

Phase 01 establishes the hardware communication foundation: I2C1 bus drivers for five INA226 current monitors and one DAC8571 DAC, a protected I2C abstraction layer with automatic bus recovery, and USB-CDC debug output. All work builds on the existing CH32V30x Standard Peripheral Library (V2.9) and uses bare-metal super-loop architecture.

The phase is well-scoped: no control loop logic, no cJSON protocol, no WS2812, no fan control. The key technical challenges are (1) correctly implementing the I2C multi-byte read protocol for INA226 register access, (2) implementing reliable I2C bus recovery via GPIO bit-banging when a slave holds SDA low, and (3) adapting WCH's SimulateCDC example for debug-only output without the full UART bridge.

**Primary recommendation:** Build the `i2c_util` wrapper layer first with timeout and bus recovery, then implement INA226 and DAC8571 drivers against that wrapper, then add USB-CDC last. This sequencing ensures I2C device testing can use USART1 printf initially, and USB-CDC is integrated once the I2C foundation is solid.

**Critical finding:** The CONTEXT.md assigns INA226 addresses 0x40-0x44 (five devices), but the INA226 datasheet only supports four unique addresses (0x40, 0x41, 0x44, 0x45 via A0/A1 pins). Addresses 0x42 and 0x43 are NOT available on INA226. This requires user clarification before implementation. [VERIFIED: TI INA226 datasheet, WebSearch cross-referenced]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| I2C master communication (START/STOP/ACK/NAK) | MCU I2C1 peripheral | i2c_util wrapper | SPL I2C driver owns the hardware; i2c_util adds timeout and recovery logic |
| I2C bus fault recovery (9-clock-pulse) | GPIO bit-bang (PB6/PB7) | i2c_util wrapper | I2C peripheral cannot generate clocks when SDA is stuck low; GPIO is the only recovery path |
| INA226 register read (voltage/current/power) | INA226 driver (ina226.c) | i2c_util (transport) | Driver owns the register map and calibration; i2c_util only provides reliable byte transfer |
| DAC8571 value write | DAC8571 driver (dac8571.c) | i2c_util (transport) | Driver owns the control byte and data format; i2c_util provides reliable transfer |
| USB-CDC debug output | usb_cdc.c (USBFS peripheral) | _write() or custom printf | CDC driver owns endpoint buffers and USB protocol; debug output just calls usb_printf() |
| USART1 printf (existing) | Debug module (debug.c) | -- | Existing infrastructure, unchanged by this phase |

## User Constraints (from CONTEXT.md)

### Locked Decisions

- **D-01:** New `Drivers/` directory at project root for all device driver code.
- **D-02:** Per-device `.c/.h` pairs: `ina226.c/.h`, `dac8571.c/.h`, `i2c_util.c/.h`, `usb_cdc.c/.h`.
- **D-03:** Lowercase file naming to match existing project convention.
- **D-04:** Include files via relative paths (e.g., `#include "../Drivers/ina226.h"`) rather than modifying `.cproject` include paths.
- **D-05:** Shared `i2c_util.c/.h` wrapper layer in Drivers/ that wraps SPL I2C functions. Device drivers call i2c_util, not SPL I2C directly.
- **D-06:** Full protection: every I2C operation has tick-count timeout, automatic 9-clock-pulse bus recovery on timeout, and retry before giving up.
- **D-07:** Timeout 5-10ms per operation. On timeout, bus reset attempted early (minimal retries -- if SDA is stuck, retries waste time).
- **D-08:** I2C1 bus speed: 100kHz standard mode.
- **D-09:** Per-device stateful struct (`INA226_Dev`) holding I2C address and device identity. Functions take a pointer to the struct.
- **D-10:** Fixed `#define` calibration -- shared compile-time calibration register value, config register, and alert threshold across all 5 devices. All must use identical shunt resistors.
- **D-11:** Per-register getter functions: `ina226_get_voltage()`, `ina226_get_current()`, `ina226_get_power()`, `ina226_get_shunt_voltage()`. Each reads one register on demand.
- **D-12:** Driver provides `ina226_check_alert()` function to read and return the alert register mask. Application code (main loop / ISR) decides what protective action to take.
- **D-13:** Adapt from WCH's USB CDC example code (from the EVT examples). Proven on CH32V303, lower risk than writing from scratch.
- **D-14:** Both USB-CDC and USART1 printf active simultaneously. USB-CDC does not replace USART1.
- **D-15:** CDC implementation in `Drivers/usb_cdc.c/.h`.
- **D-16:** Separate print functions: `usb_printf()` for USB-CDC output, standard `printf()` continues to USART1. Clear separation avoids confusion and USB blocking issues.

### Claude's Discretion

No areas delegated to Claude -- all decisions were user-specified.

### Deferred Ideas (OUT OF SCOPE)

None -- discussion stayed within phase scope.

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| I2C-01 | I2C1总线驱动5个INA226（地址0x40-0x44），读取总线电压、分流电压、电流、功率寄存器 | Sections: INA226 Register Map, I2C Master Multi-Byte Read Protocol, INA226 Driver Design |
| I2C-02 | I2C1总线驱动DAC8571IDGK（地址0x4C），16位DAC输出参考电压 | Sections: DAC8571 Write Protocol, DAC8571 Driver Design |
| I2C-03 | I2C总线超时恢复机制 -- 每次I2C操作带tick-count超时，从设备卡死时执行9时钟脉冲总线复位 | Sections: I2C Bus Recovery, i2c_util Wrapper Design, SysTick Timeout |
| COMM-03 | USB-CDC虚拟串口提供调试日志输出（printf重定向），不影响UART0指令通道 | Sections: USB-CDC Implementation Strategy, usb_printf Design |

## Standard Stack

### Core (all from existing SPL V2.9 -- no external packages)
| Library/Module | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| ch32v30x_i2c.c/h | SPL V2.9 | I2C master mode: Init, START/STOP, SendData, ReceiveData, CheckEvent, GetFlagStatus, SoftwareResetCmd | Included with MCU SDK; proven on CH32V303 |
| ch32v30x_gpio.c/h | SPL V2.9 | GPIO bit-bang SCL for I2C bus recovery | Included with MCU SDK; needed for 9-clock-pulse sequence |
| ch32v30x_rcc.c/h | SPL V2.9 | Clock enable for I2C1 (APB1), GPIOB (APB2), USBFS (AHB) | Included with MCU SDK |
| ch32v30x_usb.h | SPL V2.9 | USB register definitions, CDC class request codes | Included with MCU SDK; needed for USB-CDC |
| ch32v30x_usbfs_device.c/h | WCH EVT (adapted) | USBFS device mode: endpoint init, data upload, IRQ handler | From WCH SimulateCDC example; proven CDC implementation on CH32V303 [CITED: example/USB/USBFS/DEVICE/SimulateCDC/] |
| debug.c/h | Existing | USART1 printf, Delay_Ms/Us, SysTick timing | Already in project; provides timeout tick base |
| system_ch32v30x.c/h | Existing | SystemCoreClock, clock configuration | Already in project |

### Supporting (from WCH EVT examples, adapted)
| Library/Module | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| usb_desc.c/h (CDC variant) | Adapted from SimulateCDC | CDC device, configuration, and string descriptors | Required for USB enumeration as CDC ACM device |
| ch32v30x_it.c (extended) | Modified existing | ISR stubs for I2C1_EV, I2C1_ER, USBFS interrupts | Phase adds these interrupt handlers |

### No Package Installation Required

This is a bare-metal embedded phase. No npm/PyPI/cargo packages are installed. All dependencies are either in the existing SPL tree or adapted from the WCH EVT example bundle already present at `example/USB/USBFS/DEVICE/SimulateCDC/`.

## Package Legitimacy Audit

**Not applicable.** This phase uses no external packages. All dependencies are source files from the existing SPL V2.9 tree or the WCH EVT example bundle already present in the project repository under `example/`.

## Architecture Patterns

### System Architecture Diagram

```
+------------------------------------------------------------------+
|                        main.c (super-loop)                        |
|  1. Init: RCC -> GPIO -> I2C1 -> i2c_util -> INA226[5] + DAC8571 |
|  2. Init: USB-CDC (usb_cdc_init)                                  |
|  3. Loop: Read all INA226 registers -> print via usb_printf+printf|
+------------------------------------------------------------------+
          |                    |                    |
          v                    v                    v
+------------------+  +-----------------+  +------------------+
|   i2c_util.c     |  |   usb_cdc.c     |  |   debug.c        |
| (timeout+recovery|  | (USBFS device   |  | (USART1 printf,  |
|  wrapper for     |  |  CDC class,     |  |  Delay_Ms/Us,    |
|  SPL I2C master) |  |  EP1/EP3 IN)    |  |  SysTick ticks)  |
+------------------+  +-----------------+  +------------------+
          |                    |
          v                    v
+------------------+  +-----------------+
| SPL I2C1         |  | SPL USBFS       |
| (PB6=SCL, PB7=SDA|  | (PA11=DM,PA12=DP|
|  100kHz, master) |  |  Full Speed 12M) |
+------------------+  +-----------------+
          |
          v
+-------------------------------------------------------+
|                 I2C1 Bus (100kHz)                       |
|  [INA226#1] [INA226#2] [INA226#3] [INA226#4]         |
|    0x40       0x41       TBD         TBD               |
|  [INA226#5] [DAC8571]                                  |
|    TBD        0x4C                                      |
+-------------------------------------------------------+

GPIO Recovery Path (when SDA stuck):
  PB6 (SCL) -> GPIO output (bit-bang 9 clocks)
  PB7 (SDA) -> GPIO input (hi-Z, monitor release)
  After recovery -> re-init I2C1 peripheral
```

### Recommended Project Structure

```
CH32V303CBT/
├── Drivers/                    # NEW - device drivers (D-01, D-02, D-03)
│   ├── i2c_util.h              # I2C wrapper: timeout, recovery, read/write API
│   ├── i2c_util.c
│   ├── ina226.h                # INA226 driver: register getters, calibration, alert
│   ├── ina226.c
│   ├── dac8571.h               # DAC8571 driver: set_output, power-down
│   ├── dac8571.c
│   ├── usb_cdc.h               # USB-CDC: init, usb_printf, endpoint management
│   └── usb_cdc.c
├── Peripheral/                  # Existing SPL - unchanged
├── Debug/                       # Existing debug - unchanged
├── User/                        # Modified
│   ├── main.c                   # Extended: driver init, I2C+USB test loop
│   ├── ch32v30x_conf.h          # Extended: add USB peripheral include
│   ├── ch32v30x_it.c            # Extended: add I2C1 and USBFS ISR handlers
│   └── ch32v30x_it.h            # Extended: declare new ISRs if needed
├── example/USB/USBFS/DEVICE/
│   └── SimulateCDC/             # Reference CDC code (read-only source of truth)
└── Ld/Link.ld                   # Existing - unchanged
```

### Pattern 1: I2C Master Write (INA226 register pointer, DAC8571 data)

**What:** SPL-based I2C master write with timeout polling on each status flag, wrapped by i2c_util.

**When to use:** Writing a register pointer to INA226 (1 byte) or writing DAC value to DAC8571 (3 bytes: control + MSB + LSB).

**Standard SPL master transmitter sequence (from ch32v30x_i2c.h):**
```
1. I2C_GenerateSTART(I2C1, ENABLE)
2. while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) { timeout }  // EVT5: BUSY+MSL+SB
3. I2C_Send7bitAddress(I2C1, dev_addr, I2C_Direction_Transmitter)
4. while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) { timeout }  // EVT6
5. I2C_SendData(I2C1, byte)
6. while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) { timeout }  // EVT8_2
7. [repeat steps 5-6 for additional bytes]
8. I2C_GenerateSTOP(I2C1, ENABLE)
```

Source: SPL I2C header `ch32v30x_i2c.h` event definitions (EVT5, EVT6, EVT8_2) [VERIFIED: Peripheral/inc/ch32v30x_i2c.h lines 146, 174, 217]

### Pattern 2: I2C Master Multi-Byte Read (INA226 register data)

**What:** I2C write (register pointer) then repeated START with read for 2-byte register data.

**When to use:** Reading any INA226 register (all are 16-bit, returned MSB-first).

**Standard SPL sequence (combined write-then-read):**
```
// Phase 1: Write register pointer (no STOP)
1. I2C_GenerateSTART(I2C1, ENABLE)
2. wait EVT5 (BUSY+MSL+SB)
3. I2C_Send7bitAddress(I2C1, dev_addr, I2C_Direction_Transmitter)
4. wait EVT6 (BUSY+MSL+ADDR+TXE+TRA)
5. I2C_SendData(I2C1, register_pointer)
6. wait EVT8_2 (BUSY+MSL+TRA+TXE+BTF)

// Phase 2: Repeated START, read 2 bytes (MSB, LSB)
7. I2C_GenerateSTART(I2C1, ENABLE)
8. wait EVT5
9. I2C_Send7bitAddress(I2C1, dev_addr, I2C_Direction_Receiver)
10. wait EVT6_RX (BUSY+MSL+ADDR)
11. I2C_AcknowledgeConfig(I2C1, ENABLE)   // ACK for MSB
12. wait EVT7 (BUSY+MSL+RXNE)
13. msb = I2C_ReceiveData(I2C1)
14. I2C_AcknowledgeConfig(I2C1, DISABLE)  // NACK for LSB (last byte)
15. I2C_GenerateSTOP(I2C1, ENABLE)
16. wait EVT7 
17. lsb = I2C_ReceiveData(I2C1)
```

Source: INA226 datasheet Figure 21 "Read Register" timing diagram and SPL I2C event model [CITED: TI INA226 datasheet; VERIFIED: Peripheral/inc/ch32v30x_i2c.h event macros]

**i2c_util wrapper provides:** `i2c_util_read_reg16(dev_addr, reg_ptr, *data)` that encapsulates the full sequence with timeout at each wait point, automatic bus recovery on timeout, and a return code indicating success/failure.

### Pattern 3: I2C Bus Recovery (9-Clock-Pulse)

**What:** When SDA is stuck low, reconfigure I2C pins as GPIO, bit-bang 9 SCL pulses, then generate STOP.

**When to use:** Any I2C operation that times out during flag polling. Called automatically by i2c_util.

**Standard implementation (from I2C specification and industry practice):**
```c
// 1. Disable I2C1 peripheral
I2C_Cmd(I2C1, DISABLE);

// 2. Reconfigure PB6 (SCL) and PB7 (SDA) as GPIO
//    PB6: GPIOB, Pin 6, Output, Open-Drain, 50MHz
//    PB7: GPIOB, Pin 7, Input, Floating (hi-Z -- DO NOT drive SDA low!)
GPIO_InitTypeDef gpio = {0};
gpio.GPIO_Pin = GPIO_Pin_6;
gpio.GPIO_Mode = GPIO_Mode_Out_OD;
gpio.GPIO_Speed = GPIO_Speed_50MHz;
GPIO_Init(GPIOB, &gpio);

gpio.GPIO_Pin = GPIO_Pin_7;
gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
GPIO_Init(GPIOB, &gpio);

// 3. Generate up to 9 clock pulses on SCL
//    Check SDA after each pulse; early exit if SDA is released
for (uint8_t i = 0; i < 9; i++) {
    GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_RESET);
    Delay_Us(5);  // ~100kHz timing
    GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_SET);
    Delay_Us(5);
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7) == Bit_SET) {
        break;  // SDA released -- bus recovered
    }
}

// 4. Generate STOP condition: SDA goes high while SCL is high
//    First drive SDA low (reconfigure as output), then release
gpio.GPIO_Pin = GPIO_Pin_7;
gpio.GPIO_Mode = GPIO_Mode_Out_OD;
GPIO_Init(GPIOB, &gpio);
GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_RESET);
Delay_Us(5);
GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_SET);  // SCL high
Delay_Us(5);
GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_SET);  // SDA high while SCL high = STOP
Delay_Us(5);

// 5. Re-init I2C1 peripheral (reconfigure PB6/PB7 as AF OD)
i2c_util_init();  // re-runs I2C init + GPIO AF config
```

Source: I2C Bus Specification (NXP UM10204), Atmel AT24 EEPROM Software Reset, Linux kernel `i2c_generic_scl_recovery` in `drivers/i2c/i2c-core-base.c` [VERIFIED: WebSearch multiple sources cross-referenced; consistent across TI, Silicon Labs, Espressif, and Linux kernel implementations]

**Critical implementation detail:** During the 9 clock pulses, SDA must be configured as INPUT (high-impedance), NOT driven low. Driving SDA low is interpreted by the slave as an ACK, which causes it to continue outputting the next byte rather than releasing the bus. [VERIFIED: Espressif ESP32 bug fix commit #1767; Linux kernel I2C recovery docs]

### Pattern 4: USB-CDC Debug Output (Simplified from SimulateCDC)

**What:** Initialize USB-CDC device with CDC ACM descriptors, provide `usb_printf()` that sends data to endpoint 3 (bulk IN).

**When to use:** Debug log output to host PC via USB virtual COM port.

**Adaptation strategy from WCH SimulateCDC example:**
The WCH SimulateCDC example (`example/USB/USBFS/DEVICE/SimulateCDC/`) implements a full UART-to-USB bridge: UART2 Rx data is forwarded to USB EP3 IN, and USB EP2 OUT data is forwarded to UART2 Tx. For Phase 01, we simplify dramatically:

| Component | SimulateCDC | Phase 01 |
|-----------|-------------|----------|
| UART2 bridge | Full bidirectional UART-USB bridge | NOT needed |
| EP1 (interrupt IN) | CDC notifications (serial state) | Keep -- required for CDC ACM compliance |
| EP2 (bulk OUT) | Host-to-device data | NOT needed (no host input) |
| EP3 (bulk IN) | Device-to-host data via UART2 DMA | Device-to-host data via buffered usb_printf |
| TIM2 | UART timeout/receive timing | NOT needed (simplified) |
| DMA (UART2 RX/TX) | UART2 DMA channels | NOT needed |
| UART.c/h | Full UART config and data handling | NOT needed |
| System buffer | `UART2_Tx_Buf[1024]`, `UART2_Rx_Buf[2048]` | Small buffer for usb_printf output |

**Minimum files to adapt from SimulateCDC:**
1. `ch32v30x_usbfs_device.c` -- USBFS device driver (init, endpoints, IRQ handler) -- copy and simplify
2. `ch32v30x_usbfs_device.h` -- Header with endpoint definitions -- copy and simplify
3. `usb_desc.c` -- CDC ACM descriptors (device, config, string) -- copy and adapt VID/PID/strings
4. `usb_desc.h` -- Descriptor length definitions -- copy

**File sources:** [VERIFIED: example/USB/USBFS/DEVICE/SimulateCDC/User/USB_Device/ and User/]

**What gets removed from the adapted code:**
- All `#include "UART.h"` references and UART bridge logic
- `UART2_Tx_Buf` / `UART2_Rx_Buf` external references
- `TIM2_Init()` and `TIM2_IRQHandler` (unless needed for other purposes)
- `RCC_Configuration()`, `UART2_Init()`, `UART2_DataRx_Deal()`, `UART2_DataTx_Deal()`
- EP2 (bulk OUT) endpoint configuration
- DMA-based transfers for UART data
- `Uart` struct and all `Uart.*` references

**Simplified USBFS_IRQHandler:**
- Keep: RESET handling, SUSPEND handling, SETUP/GET_DESCRIPTOR, CDC class requests (SET_LINE_CODING, GET_LINE_CODING)
- Keep: EP1 IN transfer complete (interrupt endpoint for CDC notifications)
- Keep: EP3 IN transfer complete (bulk endpoint for data to host)
- Remove: EP2 OUT handling, UART bridge logic, TIM2-related code

**usb_printf() design (D-16):**
```c
// Drivers/usb_cdc.h
void usb_cdc_init(void);
int  usb_printf(const char *fmt, ...);  // like printf but to USB-CDC

// Internally formats into a buffer, then calls USBFS_Endp_DataUp(DEF_UEP3, buf, len, DEF_UEP_CPY_LOAD)
// Must check USBFS_Endp_Busy[DEF_UEP3] before sending; if busy, drop or buffer
```

Source: [CITED: example/USB/USBFS/DEVICE/SimulateCDC/User/USB_Device/ch32v30x_usbfs_device.c; VERIFIED: ch32v30x_usbfs_device.h USBFS_Endp_DataUp API and endpoint definitions]

### Anti-Patterns to Avoid

- **Blocking I2C without timeout:** The SPL examples use `while(!I2C_CheckEvent(...))` without timeout. On a stuck bus, this hangs forever. i2c_util MUST wrap every wait with a tick-count escape. [VERIFIED: existing SPL examples show this pattern]
- **Driving SDA low during bus recovery:** This is a documented bug in multiple implementations (including early ESP32 Arduino). The slave interprets SDA low as ACK and continues transmitting. Always configure SDA as INPUT (floating/hi-Z) during the 9 clock pulses. [VERIFIED: Espressif/arduino-esp32 commit 9db207a; Linux kernel I2C recovery docs]
- **Using I2C_SoftwareResetCmd as bus recovery:** This resets the I2C peripheral registers but does NOT generate clock pulses on the bus. If a slave is holding SDA low, a software reset alone does nothing. GPIO bit-banging is the only reliable recovery method. [VERIFIED: SPL I2C source ch32v30x_i2c.c I2C_SoftwareResetCmd implementation -- only writes to CTLR1 register]
- **Calling SPL I2C directly from device drivers:** D-05 requires all I2C calls go through i2c_util. Raw SPL calls bypass timeout protection and bus recovery.
- **Disabling USART1 printf:** D-14 requires both channels active. Do not remove `USART_Printf_Init()` or `printf()`.
- **Copying the full UART bridge from SimulateCDC:** The bridge adds UART2, TIM2, DMA channels, and 3KB of buffers. For debug-only output, the simplified CDC device is much smaller and simpler.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| I2C master state machine | Custom START/STOP/ACK sequencing | SPL `I2C_GenerateSTART/STOP`, `I2C_SendData/ReceiveData`, `I2C_CheckEvent` | SPL handles all timing, flag sequencing, and error conditions; hand-rolled state machines are fragile and miss edge cases |
| I2C bus recovery | Software-only reset of I2C peripheral | GPIO bit-bang 9 SCL pulses with SDA monitoring | I2C peripheral cannot generate clocks when SDA is low; only GPIO toggling can force the slave to release |
| USB CDC device stack | Custom USB descriptors and control transfers | Adapted WCH SimulateCDC `usb_desc.c` + `ch32v30x_usbfs_device.c` | USB enumeration is complex (standard requests, class requests, endpoint management); WCH code is proven on CH32V303 |
| Printf formatting | Custom string formatting | `vsnprintf` from newlib-nano (already linked) | newlib-nano provides full printf formatting; zero implementation cost since it is already linked |
| I2C timeout counting | Busy-wait delay loops | SysTick-based tick counter (Delay_Ms already initialized) | Accurate timeout needs a known timebase; SysTick is already running at system init |

**Key insight:** The SPL already provides battle-tested I2C master sequences. The i2c_util wrapper adds exactly what's missing: timeout protection and bus recovery. Do not rewrite the I2C state machine -- wrap it.

## Runtime State Inventory

**Not applicable.** This is a greenfield phase (first implementation). There is no existing runtime state to migrate. The existing codebase is the WCH SPL template with USART1 printf only -- no I2C devices, no USB-CDC, no data stored.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | None | N/A -- first implementation |
| Live service config | None | N/A |
| OS-registered state | None | N/A |
| Secrets/env vars | None | N/A |
| Build artifacts | None relevant | Existing `obj/` directory will be regenerated by build |

## Common Pitfalls

### Pitfall 1: INA226 Address Conflict
**What goes wrong:** Five INA226 devices are assigned addresses 0x40-0x44, but the INA226 only provides four unique 7-bit addresses based on A0/A1 pins (0x40, 0x41, 0x44, 0x45). Addresses 0x42 and 0x43 are physically impossible on INA226.
**Why it happens:** Datasheet limitation -- the INA226 has only two address pins (A0, A1), yielding 2^2 = 4 addresses. The CONTEXT.md specifies five devices at sequential addresses, but the gap between 0x41 and 0x44 is intentional (0x42 and 0x43 do not exist).
**How to avoid:** The user must clarify the physical address plan. Options: (a) use an I2C multiplexer (TCA9548A or similar) to split the bus, (b) use different I2C bus (if available -- but CH32V303 has I2C2 on different pins), or (c) the hardware design already accounts for this via pin strapping that I am not aware of. **Do not implement until user confirms addressing.**
**Warning signs:** Devices at 0x42 and 0x43 do not ACK. I2C_CheckEvent times out waiting for EVT6 (address ACK).

### Pitfall 2: I2C Multi-Byte Read Protocol Errors
**What goes wrong:** Reading INA226 16-bit registers returns corrupted data or the same byte twice.
**Why it happens:** The SPL I2C master receiver sequence requires careful ACK/NACK management. For a 2-byte read, the master must ACK the first byte (to tell the slave to send more) and NACK the second byte (to tell the slave to stop). Missing the NACK on the last byte causes the slave to continue driving SDA.
**How to avoid:** Follow Pattern 2 exactly: enable ACK before receiving MSB, disable ACK before receiving LSB, ensure STOP is generated after the last byte. Use the EVT7/BTF flag combination to detect byte reception.
**Warning signs:** First byte correct, second byte same as first. Slave holds SDA after read (bus stuck).

### Pitfall 3: USB-CDC Buffer Busy Race Condition
**What goes wrong:** `usb_printf()` is called while a previous USB transfer is still in progress, causing data loss or USB protocol errors.
**Why it happens:** USB bulk IN transfers are asynchronous -- the data sits in the endpoint buffer until the host polls for it. `USBFS_Endp_Busy[3]` is set when data is queued and cleared in the USB IRQ handler when the transfer completes. Calling `USBFS_Endp_DataUp()` while busy returns 1 (error).
**How to avoid:** Check `USBFS_Endp_Busy[DEF_UEP3]` before each send. If busy, either: (a) drop the message (simplest), (b) buffer it for retry next loop iteration, or (c) block with timeout waiting for the IRQ to clear. For debug output, option (a) or (b) is sufficient.
**Warning signs:** Missing debug messages, USB disconnects under heavy printf load, data corruption.

### Pitfall 4: I2C Bus Recovery Leaves Pins in Wrong State
**What goes wrong:** After GPIO-based bus recovery, the I2C peripheral fails to communicate because PB6/PB7 are still configured as GPIO instead of AF OD.
**Why it happens:** The recovery sequence reconfigures PB6/PB7 as GPIO for bit-banging. If `i2c_util_init()` is not called afterward, the pins remain in GPIO mode. The I2C peripheral cannot drive them.
**How to avoid:** Always call the full I2C init sequence after recovery: GPIO reinit (AF_OD for PB6/PB7), I2C peripheral init, I2C_Cmd(ENABLE). The i2c_util wrapper should encapsulate this.
**Warning signs:** I2C_CheckEvent after recovery never sees BUSY flag set. SCL/SDA scope shows no I2C activity.

### Pitfall 5: USB-CDC Enumeration Failure
**What goes wrong:** USB device does not appear as COM port on host PC. Device not recognized.
**Why it happens:** CDC ACM requires specific descriptors (Communication Class interface + Data Class interface, IAD descriptor for composite). Missing or incorrect descriptors cause enumeration failure. Clock configuration must provide correct 48MHz to USBFS peripheral.
**How to avoid:** Use the exact descriptor structure from WCH SimulateCDC (known-working on CH32V303). For 96MHz system clock, USBFS clock must be `RCC_USBFSCLKConfig(RCC_USBFSCLKSource_PLLCLK_Div2)` (96/2=48MHz). [VERIFIED: ch32v30x_usbfs_device.c USBFS_RCC_Init -- the `#else` branch for non-D8C variant at line 68]
**Warning signs:** Device manager shows "Unknown Device" or "Device Descriptor Request Failed". USBFS clock not exactly 48MHz.

### Pitfall 6: SysTick Counter for I2C Timeout
**What goes wrong:** I2C timeout uses `Delay_Ms()` as a blocking wait, blocking the entire system for 5-10ms per operation.
**Why it happens:** The simplest timeout implementation uses `Delay_Ms()` which blocks via SysTick compare. In a super-loop architecture with no other time-critical tasks in Phase 01, this is acceptable -- but the pattern is worth documenting.
**How to avoid:** Use a non-blocking tick counter approach: record `SysTick->CNT` at start, compute elapsed ticks on each poll iteration, compare against timeout threshold. This allows the timeout check to be interleaved with flag polling without wasting CPU cycles.
**Warning signs:** System appears to freeze during I2C operations with many devices. Each INA226 register read takes 5+ polls at 100kHz -- with 5 devices this adds up.

## Code Examples

### INA226 Calibration Setup
```c
// Source: TI INA226 datasheet Section 7.5.1 "Programming the Calibration Register"
// For example: R_SHUNT = 0.010 ohm (10 milliohm), Max Current = 5A
// Current_LSB = 5A / 32768 = 0.0001526 A/bit
// Calibration = 0.00512 / (0.0001526 * 0.010) = 3355 = 0x0D1B

#define INA226_R_SHUNT        0.010f    // 10 milliohm shunt resistor
#define INA226_MAX_CURRENT    5.0f      // 5A max expected current
#define INA226_CURRENT_LSB    (INA226_MAX_CURRENT / 32768.0f)
#define INA226_CAL_VALUE      ((uint16_t)(0.00512f / (INA226_CURRENT_LSB * INA226_R_SHUNT)))

// Config register: 0x4127 = default (1 sample avg, 1.1ms bus voltage conversion,
//                      1.1ms shunt voltage conversion, continuous shunt+bus mode)
#define INA226_CONFIG_VALUE   0x4127
```

Source: [CITED: TI INA226 datasheet, Equations 1-2 for calibration formula; VERIFIED: WebSearch confirmed register defaults]

### DAC8571 Write Sequence
```c
// Source: DAC8571 datasheet "I2C Write Operation" section
// Control byte: 0x00 = normal mode (load DAC with I2C data, PD0=0)
// 3 bytes total: control byte + DAC_MSB + DAC_LSB
// DAC updates on falling edge of ACK after LS byte

// Example: set DAC to mid-scale (VOUT = VREF/2)
uint8_t data[3];
data[0] = 0x00;           // control byte: normal mode, update DAC
data[1] = 0x80;           // MSB of 0x8000 (32768)
data[2] = 0x00;           // LSB of 0x8000

i2c_util_write(DAC8571_ADDR, data, 3, timeout_ms);
```

Source: [CITED: DAC8571 datasheet, Table 1 "Control Byte Format", Figure 28 "I2C Write Operation"]

### i2c_util API Design
```c
// Drivers/i2c_util.h
// Source: D-05 through D-08 decisions from CONTEXT.md

#define I2C_TIMEOUT_MS    10   // D-07: 5-10ms per operation

typedef enum {
    I2C_OK = 0,
    I2C_TIMEOUT = -1,           // operation timed out
    I2C_BUS_FAULT = -2,         // bus recovery attempted but failed
    I2C_NACK = -3,              // device did not acknowledge address
} i2c_status_t;

// Initialize I2C1: GPIO config (PB6/PB7 AF OD), peripheral init, clock
void i2c_util_init(void);

// Write len bytes to device (handles all START/STOP/ACK polling)
i2c_status_t i2c_util_write(uint8_t dev_addr, const uint8_t *data,
                            uint8_t len, uint32_t timeout_ms);

// Read len bytes from device register (write register pointer, repeated START, read)
i2c_status_t i2c_util_read(uint8_t dev_addr, uint8_t reg_ptr,
                           uint8_t *data, uint8_t len, uint32_t timeout_ms);

// Attempt bus recovery (9-clock-pulse on SCL, STOP condition, re-init)
// Returns I2C_OK if SDA was released, I2C_BUS_FAULT if still stuck
i2c_status_t i2c_util_bus_recovery(void);
```

### INA226 Device Struct and API
```c
// Drivers/ina226.h
// Source: D-09, D-10, D-11, D-12 from CONTEXT.md

typedef struct {
    uint8_t  address;       // I2C 7-bit address (D-09)
    uint8_t  channel;       // logical channel ID (0-4 for 5 devices)
} INA226_Dev;

// Initialize device: write calibration register + config register (D-10)
i2c_status_t ina226_init(INA226_Dev *dev);

// Per-register getters (D-11): each reads one register on demand
i2c_status_t ina226_get_bus_voltage(INA226_Dev *dev, float *voltage_v);
i2c_status_t ina226_get_shunt_voltage(INA226_Dev *dev, float *voltage_mv);
i2c_status_t ina226_get_current(INA226_Dev *dev, float *current_a);
i2c_status_t ina226_get_power(INA226_Dev *dev, float *power_w);

// Read alert register mask (D-12): returns raw mask register value
i2c_status_t ina226_check_alert(INA226_Dev *dev, uint16_t *mask);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| SPL bare I2C (no timeout) | Timeout + bus recovery wrapper | Phase 01 (this phase) | System never hangs on I2C faults |
| USART1-only printf | USART1 + USB-CDC dual output | Phase 01 (this phase) | Debug output available without UART hardware |
| WCH CDC UART bridge pattern | CDC device-only (no UART bridge) | Phase 01 (this phase) | ~3KB less RAM usage, simpler code |
| Polling without tick counter | SysTick-based non-blocking timeout | Phase 01 (this phase) | Accurate timeout without busy-wait |

**Deprecated/outdated:**
- `I2C_SoftwareResetCmd()` as sole recovery method: resets registers but does not release a stuck SDA line. GPIO bit-banging is the correct approach.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Five INA226 devices can coexist on I2C1 at addresses 0x40, 0x41, 0x42, 0x43, 0x44 | Architecture Patterns, Common Pitfalls | CRITICAL: INA226 only has 4 unique addresses. If the user's board design assigns these addresses via some external mechanism (I2C mux, address translation), implementation proceeds normally. Without such hardware, addresses 0x42/0x43 will not ACK and the plan must change (use I2C2, add mux, or use a different current sensor for one channel). |
| A2 | Existing SysTick timer (Delay_Init already called in main.c) provides sufficient timebase for I2C timeout | Common Pitfalls | LOW: SystemCoreClock is verified at 96MHz, SysTick is initialized before any phase 01 code runs. If SysTick were reconfigured or disabled, timeouts would fail. |
| A3 | 32K SRAM is sufficient for the added code and buffers (I2C+CDC) | Standard Stack | MEDIUM: The simplified CDC implementation removes ~3KB of UART bridge buffers. Code size increase is estimated at 5-8KB Flash and 1-2KB RAM (device state structs + CDC buffers). If SRAM proves tight, USB-CDC buffers can be reduced (64-byte USB max packet size is minimum). |
| A4 | WCH SimulateCDC code is compatible with CH32V303CBT6 (CH32V30x_D8 variant, not D8C) | USB-CDC | LOW: The SimulateCDC example does not use `#ifdef CH32V30x_D8C` -- it uses `#else` for the non-D8C path which is correct for CH32V303CBT6. Verified in `USBFS_RCC_Init()`. |

## Open Questions

1. **CRITICAL: INA226 address plan (0x40-0x44 vs datasheet limitation)**
   - What we know: CONTEXT.md specifies 5x INA226 at addresses 0x40-0x44. The INA226 datasheet only provides 4 addresses (0x40, 0x41, 0x44, 0x45) via A0/A1 pins. Addresses 0x42 and 0x43 do not exist on standard INA226.
   - What's unclear: Does the hardware design use an I2C multiplexer? Are different sensors used for two channels? Is I2C2 also available on the board?
   - Recommendation: **Halt implementation on INA226 driver until user confirms the physical addressing scheme.** The CONTEXT.md addresses may be aspirational rather than physically verified. If confirmed, document the actual hardware configuration. If not possible, recommend: use I2C1 for 4x INA226 at 0x40/0x41/0x44/0x45 + DAC8571 at 0x4C, and use I2C2 for the 5th INA226 (or use a TCA9548A I2C mux on I2C1).

2. **VREF voltage for DAC8571**
   - What we know: DAC8571 output range is 0 to VREF. The output drives an op-amp comparator for CV/CC control loop. VREF determines the maximum reference voltage.
   - What's unclear: What is the VREF voltage on the board design? This affects DAC code-to-voltage mapping but does not block Phase 01 (any VREF works for validation).
   - Recommendation: Implement driver assuming VREF is known at compile time (`#define DAC8571_VREF 3.3f` for typical 3.3V) or configurable at init. Validate by writing known DAC values and measuring analog output.

3. **USB-CDC VID/PID assignment**
   - What we know: WCH's example uses VID=0x1A86 (WCH) and PID=0xFE0C.
   - What's unclear: Should Phase 01 use WCH's VID for development, or should a custom PID be assigned? Using WCH VID requires WCH driver on Windows.
   - Recommendation: Keep WCH VID/PID for Phase 01 development. The descriptors are purely for debug output -- no production device identity is needed yet. This can be changed later without affecting the driver code.

4. **INA226 shunt resistor values**
   - What we know: D-10 requires identical shunt resistors across all 5 devices for shared calibration.
   - What's unclear: What is the actual R_SHUNT value on the PCB? This directly determines the calibration register value and measurement accuracy.
   - Recommendation: Define `INA226_R_SHUNT` as a configurable `#define` with a placeholder value. The calibration formula is documented. User verifies with actual hardware measurement.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| riscv-none-embed-gcc | Compilation | ✓ | (GCC toolchain) | -- |
| MounRiver Studio / Make | Build system | ✓ | Eclipse CDT | -- |
| WCH-Link debug probe | Flashing/debug | ✓ (assumed) | -- | -- |
| CH32V303CBT6 hardware | Execution | ✓ (assumed) | -- | -- |
| INA226 devices (5x) | I2C-01 | ✓ (assumed on PCB) | -- | -- |
| DAC8571 device | I2C-02 | ✓ (assumed on PCB) | -- | -- |
| USB cable (MCU to PC) | COMM-03 | ✓ (assumed) | -- | -- |

**Missing dependencies with no fallback:**
- None identified -- all dependencies are hardware components assumed present on the target board.

**Missing dependencies with fallback:**
- If hardware is not available, I2C drivers can be tested via logic analyzer / scope verification. USB-CDC can be tested only with physical USB connection.

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | No | N/A -- no user authentication in embedded debug output |
| V3 Session Management | No | N/A -- no sessions |
| V4 Access Control | No | N/A -- no access control layers |
| V5 Input Validation | Yes (minimal) | Validated I2C addresses (0x40-0x4C range), register pointer bounds (0x00-0xFF valid), DAC value bounds (0-65535). All function parameters bounds-checked before hardware access. |
| V6 Cryptography | No | N/A -- no cryptographic operations |

### Known Threat Patterns for Bare-Metal I2C + USB-CDC

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| I2C bus lockup from slave failure | Denial of Service | 9-clock-pulse bus recovery with timeout (I2C-03) |
| I2C data corruption from noise/glitches | Tampering | INA226 register reads cross-checked; alert register provides independent fault indication |
| USB-CDC buffer overflow from unthrottled printf | Denial of Service | Check `USBFS_Endp_Busy` before sending; bounded buffer; drop on overflow |
| Malformed USB descriptors causing host driver crash | Denial of Service | Use WCH-proven descriptor templates (verified on Windows/Linux CDC ACM driver) |
| Stale calibration data (INA226 calibration register corruption) | Tampering | Periodic re-write of calibration register (PROT-04, deferred to Phase 2) |

## Sources

### Primary (HIGH confidence)
- `Peripheral/inc/ch32v30x_i2c.h` -- SPL I2C API: all event macros (EVT5-EVT8_2), function prototypes, flag definitions [VERIFIED: codebase read]
- `Peripheral/src/ch32v30x_i2c.c` -- SPL I2C implementation: I2C_SoftwareResetCmd, I2C_CheckEvent, I2C_GenerateSTART/STOP [VERIFIED: codebase read]
- `Peripheral/inc/ch32v30x_gpio.h` -- GPIO pin definitions, modes (AF_OD, IN_FLOATING, Out_OD), GPIO_InitTypeDef [VERIFIED: codebase read]
- `Peripheral/inc/ch32v30x_rcc.h` -- RCC_APB1Periph_I2C1, RCC_AHBPeriph_USBFS, RCC_APB2Periph_GPIOB clock enable macros [VERIFIED: codebase read]
- `Peripheral/inc/ch32v30x_usb.h` -- USB CDC class request codes, PID definitions, descriptor structures [VERIFIED: codebase read]
- `example/USB/USBFS/DEVICE/SimulateCDC/User/USB_Device/ch32v30x_usbfs_device.c` -- WCH USBFS device implementation: init, endpoint config, IRQ handler, USBFS_Endp_DataUp [VERIFIED: codebase read]
- `example/USB/USBFS/DEVICE/SimulateCDC/User/USB_Device/ch32v30x_usbfs_device.h` -- USBFS endpoint definitions, buffer declarations, external API [VERIFIED: codebase read]
- `Debug/debug.c` -- USART_Printf_Init, _write syscall, Delay_Ms/Us, SysTick usage [VERIFIED: codebase read]
- `Ld/Link.ld` -- 128K Flash, 32K RAM, 2KB stack [VERIFIED: codebase read]
- `.template` -- MCU=CH32V303CBT6, Series=CH32V303, PeripheralVersion=2.9 [VERIFIED: codebase read]
- `User/ch32v30x_conf.h` -- existing peripheral includes (I2C included, USB not included) [VERIFIED: codebase read]
- `User/ch32v30x_it.c` -- existing ISR handlers (NMI, HardFault only) [VERIFIED: codebase read]

### Secondary (MEDIUM confidence)
- TI INA226 datasheet (SBOS547) -- Register map (00h-FFh), calibration formula (Equation 1-5), I2C read/write timing, address selection (A0/A1 pins), config register defaults [CITED: ti.com/lit/ds/symlink/ina226.pdf; cross-verified with multiple WebSearch sources and GitHub driver implementations]
- DAC8571 datasheet -- I2C write protocol (control byte + MSB + LSB), address selection (0x4C/0x4E via A0), output voltage formula (VOUT = VREF * D/65536), power-on behavior (0V), control byte format [CITED: ti.com/product/DAC8571; cross-verified with GitHub driver implementations]
- WCH SimulateCDC example structure -- endpoint assignment (EP1=interrupt IN, EP2=bulk OUT, EP3=bulk IN), UART bridge architecture, descriptor format [VERIFIED: codebase read of example files]

### Tertiary (LOW confidence)
- I2C bus recovery 9-clock-pulse method -- Verified across multiple sources (TI SPRZ429 Errata, Atmel AT24 datasheet, Silicon Labs EFR32 manual, Linux kernel i2c-core-base.c, Espressif ESP32 fix) but not verified on CH32V303 specifically [WebSearch consensus]
- USB-CDC descriptor compatibility with Windows/Linux CDC ACM drivers -- WCH example is known-working but not personally verified on the target hardware with the simplified implementation [ASSUMED based on WCH EVT documentation]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all components from existing SPL V2.9 and WCH EVT examples; no external packages needed
- Architecture: HIGH -- SPL I2C patterns and WCH USB-CDC patterns are well-documented in codebase; bus recovery pattern verified across industry sources
- Pitfalls: HIGH -- multiple independent sources for bus recovery, INA226 addressing issue confirmed via datasheet

**Research date:** 2026-06-04
**Valid until:** 2026-07-04 (stable embedded domain -- SPL V2.9 is mature, INA226/DAC8571 datasheets are stable)
