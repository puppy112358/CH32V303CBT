# Coding Conventions

**Analysis Date:** 2026-05-28

## Naming Patterns

**Files:**
- All lowercase with underscore separators: `ch32v30x_gpio.c`, `ch32v30x_gpio.h`, `core_riscv.h`
- Subsystem prefix convention: `ch32v30x_` for MCU peripheral drivers, `core_` for RISC-V core layer
- Single-word utility names: `debug.h` (in `Debug/`), `main.c` (in `User/`)
- Configuration files: `ch32v30x_conf.h` (in `User/`)
- Linker script: `Link.ld` (in `Ld/`)

**Functions:**
- PascalCase with underscore-prefixed subsystem module: `GPIO_Init()`, `USART_Printf_Init()`, `ADC_Cmd()`, `DMA_DeInit()`
- Peripheral functions all follow `{PERIPH}_{Verb}{Noun}`: `GPIO_WriteBit()`, `RCC_APB2PeriphClockCmd()`, `FLASH_ErasePage()`
- Private/static functions: `static` keyword, same PascalCase convention: `SetSysClock()`, `SetSysClockTo96_HSE()`
- NVIC/PFIC functions: Snake_case prefix `NVIC_` then PascalCase: `NVIC_SetPriority()`, `NVIC_EnableIRQ()`
- RISC-V inline functions in `core_riscv.h` use `__` prefix + UPPER_CASE: `__enable_irq()`, `__NOP()`, `__WFI()`

**Variables:**
- Global/module-level: PascalCase or UPPER_CASE prefix: `SystemCoreClock`, `TxBuf`, `Calibrattion_Val`, `AHBPrescTable`
- Local variables (inside functions): lowercase camelCase or single-style: `tmpreg`, `currentmode`, `pinpos`, `StartUpCounter`, `HSEStatus`
- Local loop counters: single letters: `i`, `t`, `tmp`
- `static` file-scope arrays: PascalCase: `AHBPrescTable`

**Types (typedefs, structs, enums):**
- Struct typedefs: PascalCase with `_TypeDef` suffix: `GPIO_InitTypeDef`, `ADC_InitTypeDef`, `DMA_InitTypeDef`
- Peripheral register structs: PascalCase with `_Type` suffix: `PFIC_Type`, `SysTick_Type`, `GPIO_TypeDef`
- Enum typedefs: PascalCase then `_TypeDef` suffix: `GPIOSpeed_TypeDef`, `GPIOMode_TypeDef`
- Simple enums: PascalCase: `BitAction`, `ErrorStatus`, `FunctionalState`, `FlagStatus`, `ITStatus`

**Defines / Macros:**
- UPPER_CASE with subsystem prefix: `GPIO_Pin_0`, `RCC_APB2Periph_GPIOA`, `FLASH_FLAG_BSY`, `RCC_HSEON`
- Bit masks: UPPER_CASE as above: `ECR_PORTPINCONFIG_MASK`, `LSB_MASK`
- Pin definitions: `GPIO_Pin_x` where x is 0-15 and `GPIO_Pin_All` for OR'd all pins
- Function-like macros: UPPER_CASE: `RV_STATIC_INLINE`

**Inclusion guards:**
- Format: `__UPPER_FILENAME_H` or `__UPPER_FILENAME_H__`
- Examples: `#ifndef __CH32V30x_GPIO_H`, `#define __CORE_RISCV_H__`
- Guard is always the first directive after the file header comment
- Closing: `#endif /* __CH32V30x_GPIO_H */` with filename comment

## Code Style

**Formatting:**
- No linter or formatter configuration detected in the project (no `.clang-format`, `.editorconfig`, `.prettierrc`)
- Indentation: 4 spaces, consistently applied throughout
- Bracing: Allman style -- opening brace on its own line at same indentation level:
```c
void GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_InitStruct)
{
    uint32_t currentmode = 0x00, currentpin = 0x00, pinpos = 0x00, pos = 0x00;
    // ...
}
```
- `if`/`else` blocks always use braces even for single statements
- Spaces around binary operators: `-`, `+`, `*`, `>>`, `<<`, `|`, `&`, `!=`, `==`, `=`
- No space between function name and opening paren: `GPIO_Init(GPIOA, &GPIO_InitStructure);`
- Space after comma in parameter lists: `(GPIOA, &GPIO_InitStructure)`
- Space after `if`, `while`, `for` keywords before opening paren: `if (x == 0)`, `while(1)`
- Casts: space after cast: `(uint32_t) RCC_HSEON`

**Comments:**
- File headers: Large asterisk block with metadata on separate lines:
```
/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : Main program body.
*********************************************************************************
```
- Function docblocks: Block comment style `/** ... */` with structured tags:
```c
/*********************************************************************
 * @fn      GPIO_Init
 *
 * @brief   Initializes the GPIOx peripheral according to the specified parameters.
 *
 * @param   GPIOx - where x can be (A..G) to select the GPIO peripheral.
 *          GPIO_InitStruct - pointer to a GPIO_InitTypeDef structure.
 *
 * @return  none
 */
```
- Inline comments: `/* comment text */` on their own line or inline
- End-of-line comments: `// description` occasionally used
- Section dividers: Comments like `/* Global define */`, `/* Global Variable */` separate source sections
- Preprocessor condition comments: trailing `/* comment */` after `#endif` to identify the block

**Section organization within .c files:**
1. File header copyright block
2. `#include` directives
3. `/* MASK */` or `/* define */` section for preprocessor macros
4. `/* Global typedef */` (rare in .c, more common in .h)
5. `/* Global define */`
6. `/* Global Variable */`
7. Function implementations, each preceded by a docblock

**Enum patterns:**
```c
typedef enum {NoREADY = 0, READY = !NoREADY} ErrorStatus;
typedef enum {DISABLE = 0, ENABLE = !DISABLE} FunctionalState;
typedef enum {RESET = 0, SET = !RESET} FlagStatus, ITStatus;
typedef enum
{
    Bit_RESET = 0,
    Bit_SET
}BitAction;
```

## Import Organization

**Order:**
1. Standard library headers: `#include "stdio.h"`
2. Core/MCU headers: `#include "ch32v30x.h"` (the main MCU header)
3. Peripheral driver headers: `#include "ch32v30x_gpio.h"`, `#include "ch32v30x_rcc.h"`
4. Debug utilities: `#include "debug.h"`

**Convention:**
- All includes use double-quote style: `#include "debug.h"` (not angle brackets)
- User code (main.c) typically includes only `#include "debug.h"` which transitively pulls in `ch32v30x.h` and `stdio.h`
- Peripheral `.c` files include their own `.h` first, then any additional subsystem headers needed
- Conf file (`ch32v30x_conf.h`) aggregates all peripheral headers via `#include`
- No path aliases -- all includes are flat (headers assumed on include path)

**Path Aliases:**
- Not detected. All includes are flat file names; the build system (MounRiver IDE / GCC toolchain) manages include search paths.

## Error Handling

**Error reporting pattern:**
- Functions return status enums rather than throwing or aborting:
  - `ErrorStatus` (`NoREADY` / `READY`): Used for peripheral ready checks
  - `FunctionalState` (`DISABLE` / `ENABLE`): Used for enabling/disabling features
  - `FlagStatus` / `ITStatus` (`RESET` / `SET`): Check if flags are set
  - `FLASH_Status` (`FLASH_COMPLETE` etc.): Flash operation result
- Function return pattern: `void` for most init and config functions; typed return for status queries

**Known gap:**
- HSE crystal failure is detected but produces only a comment: `/* If HSE fails to start-up, the application will have wrong clock configuration. User can add here some code to deal with this error */` in `User/system_ch32v30x.c` (lines 323-326, 392-398, and similar). No backoff to HSI or error recovery is implemented.

**Exception handlers:**
- `NMI_Handler`: Captures NMI exception and enters infinite `while(1)` loop (no recovery)
- `HardFault_Handler`: Triggers `NVIC_SystemReset()` then enters infinite `while(1)` loop
- Both use `__attribute__((interrupt("WCH-Interrupt-fast")))` attribute for the WCH hardware vectoring

**Application-level error patterns:**
```c
FLASHStatus = FLASH_ErasePage(addr);
if(FLASHStatus != FLASH_COMPLETE)
{
    printf("FLASH Erase Fail\r\n");
    return;  // Early exit pattern
}
```

## Logging

**Framework:** No dedicated logging framework. The primary debug output mechanism is UART printf.

**Patterns:**
- `printf()` over USART via `USART_Printf_Init(baudrate)` from `Debug/debug.h`
- Initialization pattern in every `main()`:
```c
SystemCoreClockUpdate();
Delay_Init();
USART_Printf_Init(115200);
printf("SystemClk:%d\r\n", SystemCoreClock);
printf("ChipID:%08x\r\n", DBGMCU_GetCHIPID());
```
- Status messages use `\r\n` (CR+LF) line endings
- Test pass/fail messages: `"Memory Program PASS!\r\n"` / `"Memory Program FAIL!\r\n"`
- No log levels (INFO/DEBUG/WARN/ERROR) -- all output uses the same `printf()` channel
- SDI printf is also available with `SDI_Printf_Enable()` (for non-UART debug probes), configured via `SDI_PRINT` define

## Function Design

**Size:** Functions vary widely. Peripheral driver init functions are typically 60-200 lines. Clock configuration functions in `system_ch32v30x.c` are ~50 lines each (duplicated pattern across clock speeds). User application functions are typically short -- 10-50 lines.

**Parameters:**
- Peripheral init functions take a pointer to a config struct (STM32-style pattern):
```c
void GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_InitStruct);
void ADC_Init(ADC_TypeDef *ADCx, ADC_InitTypeDef *ADC_InitStruct);
```
- Command functions take peripheral base address + state:
```c
void ADC_Cmd(ADC_TypeDef *ADCx, FunctionalState NewState);
```
- Custom application functions use similar patterns or positional params

**Return Values:**
- Config/init functions: `void` return
- Status query functions: return typed status (`ErrorStatus`, `FlagStatus`, `uint8_t`, etc.)
- All local variables declared at top of function (C89 block-scope convention)
- Struct initialization uses zero-initialization: `GPIO_InitTypeDef GPIO_InitStructure = {0};`

**Static functions:**
- Clock config helpers declared `static void`: `SetSysClockTo96_HSE()` etc.
- Used for internal implementation details not exposed through headers

## Module Design

**Exports:**
- Header files declare only public API functions with `extern`
- Struct and typedef definitions always in headers
- Register base address defines and peripheral pointers in headers

**Barrel Files:**
- `ch32v30x.h`: The master MCU header that includes `core_riscv.h` and defines all peripheral base addresses, register structures, and bit definitions
- `ch32v30x_conf.h`: Aggregates all peripheral driver headers; treated as the single include point for application code to pull in the full HAL
- `debug.h`: Aggregates stdio, ch32v30x, and provides delay/printf init functions

**C++ guards:**
- All public headers have `#ifdef __cplusplus` / `extern "C" {` / `}` / `#endif` wrappers for C++ compatibility even though the project is pure C

**Volatile usage:**
- All memory-mapped register pointers use `__IO`, `__I`, or `__O` volatile qualifiers defined in `Core/core_riscv.h`

## Struct Initialization Pattern

When using init structs in application code, the canonical pattern is:
```c
GPIO_InitTypeDef GPIO_InitStructure = {0};
GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
GPIO_Init(GPIOA, &GPIO_InitStructure);
```

Important: the struct must be zero-initialized with `= {0}` before setting individual fields.

---

*Convention analysis: 2026-05-28*
