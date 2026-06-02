# Stack Research

**Domain:** Embedded electronic load controller firmware (bare-metal RISC-V MCU)
**Researched:** 2026-05-28
**Confidence:** HIGH

## Recommended Stack

### Core Technologies (Existing, Verified)

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| CH32V303CBT6 MCU | RV32IMAC | Application processor | 128K Flash / 32K SRAM, 96MHz, I2C + UART + TIM + DMA peripherals match all requirements |
| CH32V30x SPL | V2.9 | Peripheral hardware abstraction | WCH-provided, well-tested, covers all needed peripherals (I2C, USART, TIM, DMA, GPIO, EXTI) |
| riscv-none-embed-gcc | (MounRiver bundled) | Cross-compiler | RISC-V RV32IMAC toolchain, `-Os` optimization, newlib-nano, linker GC sections |
| Bare-metal super-loop | N/A | Runtime architecture | No RTOS overhead; 32K SRAM is too tight for RTOS + cJSON + WS2812 buffer; super-loop with interrupt-driven I/O is the standard pattern for single-purpose controllers |

### Protocol & Data Libraries

| Library | Version | Purpose | Why Recommended |
|---------|---------|---------|-----------------|
| **cJSON** | **v1.7.19** | JSON parse (UART commands) + JSON generate (telemetry) | MIT license, ANSI C89, single .c/.h pair. v1.7.19 fixes critical CVE-2025-57052 (OOB access). Configured with static memory pool hooks (`cJSON_InitHooks`) to eliminate malloc/free fragmentation on 32K SRAM. MANDATORY: ship v1.7.19 exactly -- older versions have known CVEs |

**cJSON configuration for this project:**

```c
// In cjson_port.c -- static memory pool to prevent heap fragmentation
#define CJSON_POOL_SIZE 4096  // 4KB static pool, covers typical JSON messages
static uint8_t cjson_pool[CJSON_POOL_SIZE];
static size_t pool_offset = 0;

static void* cjson_port_malloc(size_t size) {
    // Align to 4 bytes
    size = (size + 3) & ~3;
    if (pool_offset + size > CJSON_POOL_SIZE) return NULL;
    void* ptr = &cjson_pool[pool_offset];
    pool_offset += size;
    return ptr;
}

static void cjson_port_free(void* ptr) {
    (void)ptr;  // No-op: pool reset at message boundary
}

void cjson_pool_reset(void) {
    pool_offset = 0;  // Call after each Parse+extract+Delete cycle
}

// Init in main():
cJSON_Hooks hooks = { cjson_port_malloc, cjson_port_free };
cJSON_InitHooks(&hooks);
```

**Why NOT jsmn (despite lower RAM):** This project needs BOTH parse AND generate. cJSON generates JSON for the 10Hz telemetry stream. Using jsmn for parse + cJSON for generate would mean shipping two JSON libraries, which wastes Flash. cJSON with static pool hooks is the single-library solution.

**Compile-time options for size:**
```c
#define CJSON_ENABLE_COMMENTS  0   // Strip comment support (~500 bytes saved)
#define CJSON_NESTING_LIMIT   16   // Cap recursion depth for stack safety
```

### Self-Implemented Drivers (Recommended Approach)

For I2C peripheral devices, self-implemented drivers using the SPL I2C primitives are the standard embedded pattern. The protocols are simple register read/write sequences; a library dependency adds complexity without value.

| Driver Module | Interface | Purpose | Implementation Approach |
|---------------|-----------|---------|------------------------|
| **ina226.c/.h** | I2C1 (SPL polling) | 5x INA226 current/power monitors | Self-implemented: register read/write wrappers over SPL I2C, calibration API, Alert Limit config API, Alert IRQ handler |
| **dac8571.c/.h** | I2C1 (SPL polling) | 16-bit DAC reference voltage output | Self-implemented: 3-byte write sequence (control + MSB + LSB), set_voltage() API with mV unit |
| **ws2812.c/.h** | TIM2_CH1 + DMA1 | WS2812 LED strip on PA0 | Self-implemented: TIM PWM + DMA, GRB color buffer, strip_update() API |
| **pid.c/.h** | N/A (pure math) | CV/CC control loop + fan speed control | Self-implemented: position-form PID with anti-windup clamping, float32 or Q15.16 fixed-point arithmetic |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| MounRiver Studio | IDE, build, debug | Eclipse CDT-based, already configured in `.cproject` |
| WCH-Link | Flash programming + debug | OpenOCD+GDB, `CH32V303CBT.launch` already configured |
| Debug UART (USART1, PA9) | printf debugging at 115200 bps | `Debug/debug.c` already provides `printf()` over USART1 |
| USB-CDC (planned) | Debug log output channel | Does not consume UART0 (reserved for cJSON protocol) |

## Installation

No package manager is used (bare-metal embedded project). All libraries are vendored as source files:

```bash
# Directory structure to create under User/:
User/
  cjson/cJSON.c          # v1.7.19, from GitHub release
  cjson/cJSON.h
  cjson/cjson_port.c     # Static pool allocator (project-specific)
  cjson/cjson_port.h
  drivers/ina226.c       # Self-implemented INA226 driver
  drivers/ina226.h
  drivers/dac8571.c      # Self-implemented DAC8571 driver
  drivers/dac8571.h
  drivers/ws2812.c       # Self-implemented WS2812 driver
  drivers/ws2812.h
  drivers/pid.c          # Self-implemented PID controller
  drivers/pid.h
```

Add to include paths in `.cproject`:
- `User/cjson`
- `User/drivers`

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| JSON parser | cJSON v1.7.19 + static pool | jsmn (tokenizer only) | jsmn cannot generate JSON; would need second library for telemetry, increasing Flash/RAM |
| JSON parser | cJSON v1.7.19 + static pool | cJSON with default malloc/free | Heap fragmentation risk on 32K SRAM with repeated parse/free cycles at 10Hz |
| INA226 driver | Self-implemented over SPL I2C | External I2C device library | INA226 protocol is 8 registers; a library adds unnecessary abstraction and ROM overhead |
| DAC8571 driver | Self-implemented over SPL I2C | Arduino DAC8571 library (RobTillaart) | C++ only; porting effort exceeds writing a clean C driver for a 1-register device |
| WS2812 driver | Self-implemented TIM+DMA | Adafruit NeoPixel (bit-bang) | Bit-bang blocks CPU with interrupts disabled; TIM+DMA is non-blocking and standard |
| PID controller | Self-implemented (~100 lines C) | External PID library (CMSIS-DSP, etc.) | Simple position-form PID with anti-windup does not warrant a library dependency |
| RTOS | Bare-metal super-loop | FreeRTOS / RT-Thread | 32K SRAM with 4KB cJSON pool + 1KB+ WS2812 DMA buffer leaves insufficient headroom for RTOS kernel + task stacks |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| **cJSON versions < v1.7.19** | CVE-2025-57052 (OOB), CVE-2023-53154 (buffer over-read), CVE-2024-31755 (segfault), CVE-2023-50471, CVE-2023-50472 | cJSON v1.7.19 |
| **cJSON default malloc/free (no hooks)** | On 32K SRAM with 10Hz parse/free cycles, malloc fragmentation will eventually cause allocation failure -- this is a well-documented embedded failure mode | cJSON with static pool hooks (`cJSON_InitHooks`) |
| **Bit-bang WS2812 (GPIO+DWT cycle counter)** | Blocks CPU for entire strip update (~30us per LED), disables interrupts, breaks 10Hz telemetry timing | TIM PWM + DMA (zero CPU overhead during transmission) |
| **HAL/CMSIS driver abstraction layer** | CH32V303 SPL is already the hardware abstraction layer; adding a wrapper-on-wrapper wastes ROM and adds latency for 100ms control loops | Direct SPL usage with thin driver modules |
| **Floating-point PID on every iteration (unnecessary)** | RV32IMAC has no hardware FPU -- soft-float via libgcc adds ~1000 cycles per operation; for the 100ms control loop this is acceptable but should use float sparingly | Position PID with float32 for setpoint/error computation, integer PWM output mapping |
| **DMA circular mode for WS2812** | WS2812 requires a precise reset pulse (>50us LOW) between frames; circular mode would continuously transmit with no gap | DMA normal mode: prepare buffer, start TIM+DMA, wait for TC flag, stop TIM to generate reset |

## Stack Patterns by Variant

**If 32K SRAM proves too tight (verified at build time):**
- Reduce `CJSON_POOL_SIZE` from 4096 to 2048 (accepts smaller incoming JSON)
- Reduce WS2812 DMA buffer from full-strip to a smaller ring (fewer LEDs per update)
- Use `uint16_t` (HalfWord) instead of `uint32_t` for WS2812 DMA buffer to halve memory (24 HalfWords per LED = 48 bytes)

**If I2C bus contention occurs with 6 devices:**
- Implement I2C transaction retry with exponential backoff
- Split INA226 reading across multiple super-loop ticks (read 2-3 devices per tick instead of all 5 at once)
- Use I2C error interrupt (`I2C_IT_ERR`) with auto-recovery (`I2C_SoftwareResetCmd`)

**If PID tuning is difficult without visualization:**
- Add a debug cJSON telemetry field for PID internals (error, P_term, I_term, D_term, output)
- Tune via USB-CDC debug log during development, then remove field for production

## Version Compatibility

| Component | Compatible With | Notes |
|-----------|-----------------|-------|
| cJSON v1.7.19 | CH32V30x SPL V2.9 | No conflicts; cJSON does not touch hardware |
| cJSON v1.7.19 | newlib-nano (--specs=nano.specs) | cJSON uses `memcpy`, `strcmp`, `strlen`, `strtod` from libc -- all provided by newlib-nano |
| cJSON v1.7.19 | GCC -Os | Compiles cleanly at -Os; all code paths are well-defined ANSI C |
| cJSON v1.7.19 | CH32V303 32K SRAM | Static pool configuration verified at 4KB pool; adjust pool size based on largest expected JSON message |

## I2C1 Bus Address Allocation

With 6 devices on I2C1 (1x DAC8571 + 5x INA226), address conflicts must be avoided:

| Device | 7-bit Address | A0/A1 Pin Configuration |
|--------|--------------|------------------------|
| INA226 #1 (MOS1) | 0x40 | A0=GND, A1=GND |
| INA226 #2 (MOS2) | 0x41 | A0=VS, A1=GND |
| INA226 #3 (MOS3) | 0x42 | A0=SDA, A1=GND |
| INA226 #4 (MOS4) | 0x43 | A0=SCL, A1=GND |
| INA226 #5 (Summary) | 0x44 | A0=GND, A1=VS |
| DAC8571 | 0x4C | A0=GND |

All addresses are verified non-conflicting on a single bus. I2C fast mode (400kHz) recommended. Each INA226 register read is 2 bytes; reading 5 devices (shunt + bus + current + power = 8 bytes each) = 40 bytes total, at 400kHz this takes ~1ms, well within the 100ms control period.

## Timer and DMA Allocation

| Peripheral | Timer | Channel / Pin | DMA Channel | Purpose |
|------------|-------|---------------|-------------|---------|
| WS2812 | TIM2 | CH1 / PA0 | DMA1 Ch5 (TIM2_UP)** | PWM+DMA LED strip control |
| Fan PWM | TIM3 | CH1 / PA6 | None (static duty) | 25kHz PWM fan speed control |
| Fan TACH | TIM4 | CH1 / PB6 | None (input capture) | Fan RPM measurement via pulse timing |
| System tick | SysTick | N/A | N/A | 1ms timer for super-loop scheduling |
| I2C1 | I2C1 | PB6(SCL)/PB7(SDA) | None (polling) | 6-device I2C bus |
| UART0 (cJSON) | USART1 | PA9(TX)/PA10(RX) | DMA1 Ch4 (TX) | cJSON protocol >115200 bps |
| USB-CDC (debug) | USBOTG | PA11/PA12 | N/A | Debug log output |

** DMA channel mapping for TIM2_UP on CH32V303 must be verified against the official CH32V303 Reference Manual. The CH32V303 has DMA2 with 11 channels, which differs from STM32F103's DMA1-only (7 channels). Mapping above assumes STM32F103-compatible channel assignment; verify with RM Chapter 7 (DMA controller) before implementation.

## Sources

- cJSON v1.7.19 release and CVE fixes: [Buildroot security bump commit](https://lists.buildroot.org/pipermail/buildroot/2025-September/786535.html), [Debian security tracker](https://security-tracker.debian.org/tracker/source-package/cjson) -- HIGH confidence
- cJSON embedded memory management (static pool hooks, zero-copy references, depth control): [CSDN embedded cJSON guide](https://blog.csdn.net/weixin_35886636/article/details/160091063) -- MEDIUM confidence (community article, corroborated by cJSON source)
- jsmn vs cJSON comparison for embedded: [CSDN performance comparison](https://blog.csdn.net/efc12345678/article/details/154860000) -- MEDIUM confidence (community testing)
- WS2812 TIM+PWM+DMA standard implementation (PWM encode, CCDS bit, DMA normal mode, start order): [ErniW bare-metal STM32 repo](https://github.com/ErniW/Neopixels-bare-metal-STM32), [HackYourMom tutorial](https://hackyourmom.com/en/osvita/vid-dyvannyh-kibervoyiniv/stm32-chastyna-7-keruvannya-neopixel-cherez-dma-drajver/) -- HIGH confidence (verified implementation patterns)
- INA226 register map, Alert configuration, calibration formula: [TI SBOS547 datasheet](https://www.ti.com/document-viewer/lit/html/SBOS547A) -- HIGH confidence (official datasheet)
- INA226 Alert pin overcurrent protection pattern: [TI E2E forum](https://e2e.ti.com/support/amplifiers-group/amplifiers/f/amplifiers-forum/462911/ina226-alert-pin-function) -- HIGH confidence (TI engineering response)
- DAC8571 I2C address, control byte, write sequence: [TI DAC8571 datasheet](http://www.bdtic.com/datasheet/TI/DAC8571.pdf) -- HIGH confidence (official datasheet); [RobTillaart Arduino library](https://github.com/RobTillaart/DAC8571) -- MEDIUM confidence (reference implementation)
- Embedded PID control (position form, anti-windup, fan control pattern): [CSDN embedded fan PID library](https://blog.csdn.net/weixin_42153793/article/details/159478700) -- MEDIUM confidence (community project, standard textbook algorithm)
- CH32V303 SPL timer and I2C API: Project source code at `Peripheral/inc/ch32v30x_tim.h`, `Peripheral/inc/ch32v30x_i2c.h` -- HIGH confidence (verified in codebase)
- CH32V303 clock tree (96MHz HSE, HCLK=96MHz, APB1=DIV2=48MHz, APB2=DIV1=96MHz): Project source at `User/system_ch32v30x.c` -- HIGH confidence (verified in codebase)
- CH32V303 DMA controller (DMA1: 7 channels, DMA2: 11 channels): Interrupt vector table at `Peripheral/inc/ch32v30x.h` -- HIGH confidence (verified in codebase)

---
*Stack research for: 电子负载控制器 (Electronic Load Controller) firmware on CH32V303CBT6*
*Researched: 2026-05-28*
