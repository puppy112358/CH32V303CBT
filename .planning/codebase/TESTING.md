# Testing Patterns

**Analysis Date:** 2026-05-28

## Test Framework

**Runner:**
- No test framework for the core CH32V30x SDK/Peripheral library code. The project is an embedded C MCU project where testing is primarily manual verification via UART printf output on physical hardware.
- The HarmonyOS LiteOS_M example (`example/HarmonyOS/LiteOS_m/`) includes a dedicated test suite framework (`osTest.h`) for LiteOS kernel validation, but this is an imported third-party RTOS test infrastructure, not part of the CH32V30x SDK itself.

**Assertion Library (LiteOS_M tests):**
- Custom assertion macros defined in the LiteOS_m test framework:
  - `ICUNIT_ASSERT_EQUAL_VOID(actual, expected, retVal)` -- Assert equality, return void on failure
  - `ICUNIT_GOTO_EQUAL(actual, expected, retVal, label)` -- Assert equality, goto label on failure
  - `ICUNIT_ASSERT_EQUAL(actual, expected, retVal)` -- Assert equality, return on failure

**Test Registration (LiteOS_M):**
```c
TEST_ADD_CASE("TestCaseName", TestFunction, TEST_LOS, TEST_EVENT, TEST_LEVEL1, TEST_FUNCTION);
```
Parameters:
- Test case name (string)
- Test function pointer
- Test module category (e.g., `TEST_LOS`)
- Test feature category (e.g., `TEST_EVENT`)
- Test level (e.g., `TEST_LEVEL1`)
- Test type (e.g., `TEST_FUNCTION`)

**Run Commands:**
- Not applicable for the CH32V30x SDK itself. No `make test` or similar target exists.
- Tests are compiled into the firmware binary and run on-target via the RTOS scheduler.

## Test File Organization

**Location (CH32V30x SDK):**
- No dedicated test directories for the SDK code itself. Verification is done within example `main.c` files as application-level demonstrations.

**Location (LiteOS_M example tests):**
- `example/HarmonyOS/LiteOS_m/LiteOS/testsuits/sample/kernel/{module}/` -- One directory per kernel feature (e.g., `event/`, `hwi/`, `mem/`)
- Test files follow naming: `It_los_{module}.c` (suite aggregator) and `It_los_{module}_{NNN}.c` (individual test cases)

**Naming (LiteOS_M):**
- Suite aggregator: `It_los_{module}.c` with function `ItSuiteLos{Module}()`
- Individual tests: `It_los_{module}_{NNN}.c` with function `ItLos{Module}{NNN}()`
- Example: `It_los_event_001.c` -> function `ItLosEvent001()`
- Test function comments document the pattern: `// IT_Layer_ModuleORFeature_No`

**Structure (LiteOS_M):**
```
example/HarmonyOS/LiteOS_m/LiteOS/testsuits/
├── sample/
│   └── kernel/
│       ├── event/
│       │   ├── It_los_event.c         # Suite aggregator: ItSuiteLosEvent()
│       │   ├── It_los_event_001.c     # Test: ItLosEvent001()
│       │   ├── It_los_event_002.c     # Test: ItLosEvent002()
│       │   └── ...                    # 43 event test cases total
│       ├── hwi/
│       │   ├── it_los_hwi.c           # Suite aggregator
│       │   └── it_los_hwi_001.c ...
│       ├── mem/
│       │   └── ...
│       └── ...
├── Makefile
└── unittest/
    └── fuzz/
        └── src/
            └── stdarg/
                └── test_stdarg_fuzz.c  # Fuzz test
```

## Test Structure

**Suite Organization (LiteOS_M):**
```c
// It_los_event.c - Suite aggregator
EVENT_CB_S g_pevent;  // Global test fixture

VOID ItSuiteLosEvent()
{
    ItLosEvent001();
    ItLosEvent002();
    ItLosEvent003();
    // ...
#if (LOS_KERNEL_HWI_TEST == 1)
    ItLosEvent023();  // Conditionally compiled tests
    ItLosEvent024();
#endif
    // ...
}
```

**Individual Test Case Pattern (LiteOS_M):**
```c
#include "osTest.h"
#include "It_los_event.h"

static VOID TaskF01(VOID)
{
    UINT32 ret;
    g_testCount++;
    ret = LOS_EventRead(&g_pevent, 0x11, LOS_WAITMODE_AND, LOS_WAIT_FOREVER);
    ICUNIT_ASSERT_EQUAL_VOID(ret, g_pevent.uwEventID, ret);
    ICUNIT_ASSERT_EQUAL_VOID(g_pevent.uwEventID, 0x11, g_pevent.uwEventID);
    g_testCount++;
    ret = LOS_TaskDelete(g_testTaskID01);
    ICUNIT_ASSERT_EQUAL_VOID(ret, LOS_OK, ret);
    return;
}

static UINT32 Testcase(VOID)
{
    UINT32 ret;
    TSK_INIT_PARAM_S task1;
    (void)memset_s(&task1, sizeof(TSK_INIT_PARAM_S), 0, sizeof(TSK_INIT_PARAM_S));
    task1.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskF01;
    task1.pcName = "EventTsk1";
    task1.uwStackSize = TASK_STACK_SIZE_TEST;
    task1.usTaskPrio = TASK_PRIO_TEST - 2;
    task1.uwResved = LOS_TASK_STATUS_DETACHED;

    g_testCount = 0;
    g_pevent.uwEventID = 0;
    LOS_EventInit(&g_pevent);

    ret = LOS_TaskCreate(&g_testTaskID01, &task1);
    ICUNIT_GOTO_EQUAL(ret, LOS_OK, ret, EXIT);

    ret = LOS_EventWrite(&g_pevent, 0x10);
    ICUNIT_GOTO_EQUAL(ret, LOS_OK, ret, EXIT);
    ICUNIT_GOTO_EQUAL(g_testCount, 1, g_testCount, EXIT);

    ret = LOS_EventWrite(&g_pevent, 0x1);
    ICUNIT_GOTO_EQUAL(ret, LOS_OK, ret, EXIT);
    ICUNIT_ASSERT_EQUAL(g_testCount, 2, g_testCount);

    return LOS_OK;
EXIT:
    ret = LOS_TaskDelete(g_testTaskID01);
    ICUNIT_ASSERT_EQUAL(ret, LOS_OK, ret);
    return LOS_OK;
}

VOID ItLosEvent001(VOID) // IT_Layer_ModuleORFeature_No
{
    TEST_ADD_CASE("ItLosEvent001", Testcase, TEST_LOS, TEST_EVENT, TEST_LEVEL1, TEST_FUNCTION);
}
```

**Patterns:**
- Each test file: static helper tasks/functions + a static `Testcase()` function + a public `VOID ItLos{Module}{NNN}(VOID)` entry point
- `Testcase()`: Sets up test state, creates RTOS tasks, runs assertions, cleans up on exit path
- `EXIT:` label pattern for cleanup on test failure (goto-based error handling typical in C)
- Conditional compilation guards: `#if (LOS_KERNEL_HWI_TEST == 1)` for hardware-interrupt-dependent tests
- Test tasks use `LOS_TaskDelete()` to self-terminate when done
- Shared test globals: `g_pevent`, `g_testCount`, `g_testTaskID01` declared in aggregator file

## Mocking

**Framework:**
- Not applicable. No mocking framework detected in this bare-metal embedded C project.
- The code interacts directly with hardware registers. Testing depends on real hardware (or simulator, if available).

**Patterns:**
- Not applicable. No mock or stub patterns observed in the CH32V30x SDK or LiteOS_M tests.

## Fixtures and Factories

**Test Data (LiteOS_M):**
- Global variables in the suite aggregator file serve as shared test fixtures:
```c
// It_los_event.c
EVENT_CB_S g_pevent;  // Reused across all event tests
```
- Each test initializes its own state within `Testcase()`:
```c
g_testCount = 0;
g_pevent.uwEventID = 0;
LOS_EventInit(&g_pevent);
```

**Location:**
- Suite-level globals: defined in `It_los_{module}.c` aggregator file
- Test-local data: declared as local variables inside `Testcase()`

**CH32V30x example pattern (not formal tests):**
```c
// FLASH example - manual test data
#define Fadr    (0x08020000)
#define Fsize   ((((256*4))>>2))
u32 buf[Fsize];
// buf is populated with test pattern: for(i=0; i<Fsize; i++){ buf[i] = i; }
```

## Coverage

**Requirements:**
- No code coverage enforcement. No coverage tooling detected (no `gcov`, `lcov`, or embedded coverage tools).
- LiteOS_M tests use `#if (LOS_KERNEL_HWI_TEST == 1)` and `#if (LOS_KERNEL_TEST_NOT_SMOKE == 1)` compile-time flags to categorize test levels (smoke vs full), but no runtime coverage measurement.

**View Coverage:**
- Not applicable. No coverage generation command available.

## Test Types

**Unit Tests:**
- Not practiced for the CH32V30x SDK code. The peripheral drivers are tested implicitly through the example applications.
- LiteOS_M has extensive kernel unit tests (43 for event module alone, 35+ for HWI module) running on-target as RTOS tasks.

**Integration Tests:**
- The `example/` directory serves as integration-level verification. Each example (GPIO Toggle, ADC DMA, USART Printf, etc.) demonstrates a specific peripheral feature working end-to-end on real hardware.
- Example `main()` functions follow a pattern: init peripherals, exercise functionality, print results via UART, enter infinite loop.
- No automated pass/fail -- output must be visually inspected on a serial terminal.

**E2E Tests:**
- Not applicable in the traditional CI sense. The closest analog is the full example applications that combine multiple peripherals (e.g., `FSMC/LCD`, `ETH/WebServer`).

**Application-level self-test (CH32V30x examples):**
Some examples include inline verification with PASS/FAIL reporting:
```c
// FLASH example - self-test pattern
TestStatus MemoryEraseStatus = PASSED;
TestStatus MemoryProgramStatus = PASSED;
// ... perform operation, check result ...
if(MemoryProgramStatus == FAILED)
    printf("Memory Program FAIL!\r\n");
else
    printf("Memory Program PASS!\r\n");
```
This is the informal "test pattern" for the CH32V30x examples.

## Common Patterns

**Async Testing (LiteOS_M):**
- RTOS tasks created within test cases execute concurrently. Tests use event/semaphore waits to synchronize:
```c
ret = LOS_TaskCreate(&g_testTaskID01, &task1);
ICUNIT_GOTO_EQUAL(ret, LOS_OK, ret, EXIT);
ret = LOS_EventWrite(&g_pevent, 0x10);  // Signal the task
// Task reads event with LOS_WAIT_FOREVER, increments counter, deletes itself
```

**Error Testing (LiteOS_M):**
- Goto-based cleanup pattern with labeled exit:
```c
static UINT32 Testcase(VOID)
{
    // ... setup and assertions ...
    return LOS_OK;
EXIT:
    // Cleanup: delete tasks, free resources
    ret = LOS_TaskDelete(g_testTaskID01);
    ICUNIT_ASSERT_EQUAL(ret, LOS_OK, ret);
    return LOS_OK;
}
```

**Unit test boilerplate (LiteOS_M):**
- Every test file: includes `osTest.h` and the matching `It_los_{module}.h`
- Test function signature: `VOID ItLos{Module}{NNN}(VOID)` with one-line `TEST_ADD_CASE(...)` body
- Task entry signatures: `static VOID TaskF{NN}(VOID)` or `static UINT32 TaskF{NN}(VOID)`

---

*Testing analysis: 2026-05-28*
