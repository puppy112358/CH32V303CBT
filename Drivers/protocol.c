/********************************** (C) COPYRIGHT  *******************************
* File Name          : protocol.c
* Author             : GSD Phase 03
* Version            : V1.0.0
* Date               : 2026/06/07
* Description        : Communication protocol implementation.
*                      USART1 (PA9=TX, PA10=RX) at 115200 bps 8N1+odd parity.
*                      Interrupt-driven 512-byte RX ring buffer with newline-
*                      delimited line extraction. Blocking TX for responses.
*******************************************************************************/
#include "../Drivers/protocol.h"
#include "debug.h"
#include "../Drivers/cjson/cJSON.h"
#include "../Drivers/temp_sensor.h"
#include <string.h>
#include <math.h>

/* --------------------------------------------------------------------------
 * External References (from main.c)
 * -------------------------------------------------------------------------- */
extern SystemMode  system_mode;
extern float       cv_target_voltage;
extern float       cc_target_current;
extern volatile uint16_t last_dac_value;
extern FaultRegister fault_reg;
extern void engage_cv(float target_voltage);
extern void engage_cc(float target_current);

/* Fan status (from Drivers/fan.c) — for telemetry */
extern volatile uint16_t fan_rpm;
extern volatile uint8_t  fan_stall;
extern uint32_t cycle_count;
ringbuffer ring_buffer = {{0}, 0, 0, 0};
USART_DMA_CTRL_  USART_DMA_CTRL = {
    .DMA_USE_BUFFER = 0,
    .Rx_Buffer      = {0},
};

/* --------------------------------------------------------------------------
 * cJSON Arena Allocator (4 KB static pool, 8-byte aligned)
 *
 * Alignment is CRITICAL on RV32IMAC: cJSON structs contain double fields
 * (valuedouble) which require 8-byte alignment.  An unaligned pool causes
 * a store/AMO access fault → HardFault → system reset.
 * -------------------------------------------------------------------------- */
#define CJSON_POOL_SIZE 4096
static uint8_t cjson_pool[CJSON_POOL_SIZE] __attribute__((aligned(8)));
static size_t  cjson_pool_used = 0;

static void *cjson_arena_alloc(size_t sz)
{
    void *ptr;
    size_t aligned_sz;

    /* Round up to 8-byte boundary so every allocation starts aligned */
    aligned_sz = (sz + 7) & ~((size_t)7);

    if (cjson_pool_used + aligned_sz > CJSON_POOL_SIZE)
    {
        return NULL;
    }
    ptr = &cjson_pool[cjson_pool_used];
    cjson_pool_used += aligned_sz;
    return ptr;
}

static void cjson_arena_free(void *ptr)
{
    (void)ptr; /* no-op — all memory reclaimed on arena reset */
}

/* --------------------------------------------------------------------------
 * Forward Declarations (static helpers)
 * -------------------------------------------------------------------------- */
static void send_error_ack(int code, const char *msg);
static const char *system_mode_to_string(SystemMode mode);

/* --------------------------------------------------------------------------
 * Line Buffer for protocol_poll() — static, retains partial lines across calls.
 * -------------------------------------------------------------------------- */
static char line_buf[LINE_BUF_SIZE];
static uint16_t line_pos = 0;

/*********************************************************************
 * @fn      protocol_init
 *
 * @brief   Initialize USART1 at 115200 bps 8N1+odd parity with RXNE
 *          interrupt enabled. Configure PA9 as AF push-pull (TX) and
 *          PA10 as input floating (RX).
 *          Zero ring buffer and error counters.
 *
 * @return  none
 */
void protocol_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure  = {0};
    USART_InitTypeDef USART_InitStructure = {0};
    NVIC_InitTypeDef  NVIC_InitStructure  = {0};
    /* Enable peripheral clocks — USART1 is on APB2.
     * GPIOB clock also enabled per WCH USART example pattern. */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /* Configure PA9 (TX) — AF push-pull */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Configure PA10 (RX) — input floating */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* USART1: 115200 bps, 8 data bits, 1 stop bit, no parity, RX+TX */
    USART_InitStructure.USART_BaudRate            = 115200;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    /* Enable USART_IT_IDLE interrupt for byte-by-byte reception */
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel                   = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    /* Enable USART1 */
    USART_Cmd(USART1, ENABLE);

    /* Configure NVIC: USART1 at priority 0x02 (below EXTI4 at 0x01) */
    // NVIC_SetPriority(USART1_IRQn, 0x02);
    // NVIC_EnableIRQ(USART1_IRQn);

    /* Zero ring buffer state (ring_buffer is zero-initialized at definition) */

    /* USART1 TX test — send known string to verify terminal baud rate */
    {
        const char *test = "\r\n=== USART1 OK (115200,8N1) ===\r\n";
        while (*test)
        {
            while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}
            USART_SendData(USART1, (uint16_t)(*test));
            test++;
        }
    }

    /* Configure cJSON arena allocator hooks */
    {
        cJSON_Hooks hooks;
        hooks.malloc_fn = cjson_arena_alloc;
        hooks.free_fn   = cjson_arena_free;
        cJSON_InitHooks(&hooks);
    }

    printf("Protocol init: USART1 115200 8N1 (no parity), ring buffer %d bytes\r\n",
           RX_BUF_SIZE);
    printf("cJSON arena: %d bytes at 0x%08X\r\n",
           CJSON_POOL_SIZE, (uint32_t)(uintptr_t)cjson_pool);
}
/*********************************************************************
 * @fn      DMA_INIT
 *
 * @brief   Configures the DMA for USART1.
 *
 * @return  none
 */
void DMA_INIT(void)
{
    DMA_InitTypeDef  DMA_InitStructure  = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_DeInit(DMA1_Channel5);
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority           = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;

    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART1->DATAR);
    DMA_InitStructure.DMA_MemoryBaseAddr     = (u32)USART_DMA_CTRL.Rx_Buffer[0];
    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize         = RX_BUFFER_LEN;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);

    DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel                   = DMA1_Channel5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    DMA_Cmd(DMA1_Channel5, ENABLE);
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
}
/*********************************************************************
 * @fn      ring_buffer_push_huge
 *
 * @brief   Put a large amount of data into the ring buffer.
 *
 * @return  none
 */
void ring_buffer_push_huge(uint8_t *buffer, uint16_t len)
{
    const uint16_t bufferRemainCount = RING_BUFFER_LEN - ring_buffer.RemainCount;
    if (bufferRemainCount < len)
    {
        len = bufferRemainCount;
    }

    const uint16_t bufferSize = RING_BUFFER_LEN - ring_buffer.RecvPos;
    if (bufferSize >= len)
    {
        memcpy(&(ring_buffer.buffer[ring_buffer.RecvPos]), buffer, len);
        ring_buffer.RecvPos += len;
    }
    else
    {
        uint16_t otherSize = len - bufferSize;
        memcpy(&(ring_buffer.buffer[ring_buffer.RecvPos]), buffer, bufferSize);
        memcpy(ring_buffer.buffer, &(buffer[bufferSize]), otherSize);
        ring_buffer.RecvPos = otherSize;
    }
    ring_buffer.RemainCount += len;
}

/*********************************************************************
 * @fn      ring_buffer_pop
 *
 * @brief   Get a data from the ring buffer.
 *
 * @return  the Data
 */
uint8_t ring_buffer_pop()
{
    uint8_t data = ring_buffer.buffer[ring_buffer.SendPos];

    ring_buffer.SendPos++;
    if (ring_buffer.SendPos >= RING_BUFFER_LEN)
    {
        ring_buffer.SendPos = 0;
    }
    ring_buffer.RemainCount--;
    return data;
}


/*********************************************************************
 * @fn      protocol_poll
 *
 * @brief   Poll the DMA+IDLE ring buffer for a complete newline-
 *          delimited line. Bytes are consumed one-at-a-time via
 *          ring_buffer_pop(). When '\n' or '\r' is found, the
 *          accumulated line (excluding the delimiter) is null-
 *          terminated and returned.
 *
 *          Handles \r\n and \n\r line-ending pairs transparently.
 *          Returns NULL if no complete line is available yet.
 *
 * @return  Pointer to null-terminated line string, or NULL.
 */
const char *protocol_poll(void)
{
    uint8_t byte;

    while (ring_buffer.RemainCount > 0)
    {
        byte = ring_buffer_pop();

        if (byte == '\n' || byte == '\r')
        {
            /* Consume paired \r\n or \n\r */
            if (ring_buffer.RemainCount > 0)
            {
                uint8_t next = ring_buffer.buffer[ring_buffer.SendPos];
                if ((byte == '\r' && next == '\n') ||
                    (byte == '\n' && next == '\r'))
                {
                    ring_buffer_pop();
                }
            }

            line_buf[line_pos] = '\0';
            line_pos = 0;

            /* Return non-empty lines only (skip blank lines) */
            if (line_buf[0] != '\0')
            {
                return line_buf;
            }
        }
        else if (line_pos < (LINE_BUF_SIZE - 1))
        {
            line_buf[line_pos++] = (char)byte;
        }
        /* else: byte discarded — line exceeds LINE_BUF_SIZE;
         * line_pos reset at next delimiter */
    }

    return NULL;
}

/*********************************************************************
 * @fn      protocol_send
 *
 * @brief   Send a null-terminated JSON string over USART1, appending
 *          '\n' as a line terminator. Blocks on USART_FLAG_TC until
 *          each byte has been transmitted.
 *
 * @param   json_str - Pointer to null-terminated string to send
 *
 * @return  none
 */
void protocol_send(const char *json_str)
{
    while (*json_str)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
        {
            /* Wait for TX complete */
        }
        USART_SendData(USART1, (uint16_t)(*json_str));
        json_str++;
    }

    /* Append line terminator */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
    {
        /* Wait for TX complete */
    }
    USART_SendData(USART1, (uint16_t)'\n');
}

/*********************************************************************
 * @fn      system_mode_to_string
 *
 * @brief   Convert system mode enum to string for get_status response.
 *
 * @param   mode - Current SystemMode value
 *
 * @return  String literal: "IDLE", "CV", "CC", or "FAULT"
 */
static const char *system_mode_to_string(SystemMode mode)
{
    switch (mode)
    {
        case MODE_IDLE:  return "IDLE";
        case MODE_CV:    return "CV";
        case MODE_CC:    return "CC";
        case MODE_FAULT: return "FAULT";
        default:         return "?";
    }
}

/*********************************************************************
 * @fn      send_error_ack
 *
 * @brief   Build and send a standard JSON error acknowledgment.
 *          Format: {"ack":"error","code":N,"msg":"description"}
 *
 * @param   code - Numeric error code (1xx/2xx/3xx)
 * @param   msg  - Human-readable error description
 *
 * @return  none
 */
static void send_error_ack(int code, const char *msg)
{
    cJSON *err;
    char *str;

    err = cJSON_CreateObject();
    if (err == NULL) return;

    cJSON_AddStringToObject(err, "ack", "error");
    cJSON_AddNumberToObject(err, "code", code);
    cJSON_AddStringToObject(err, "msg", msg);

    str = cJSON_PrintUnformatted(err);
    if (str != NULL)
    {
        protocol_send(str);
    }

    cJSON_Delete(err);
}

/*********************************************************************
 * @fn      protocol_process_command
 *
 * @brief   Parse a received JSON command line, validate fields, check
 *          state gates, dispatch to engage_cv/engage_cc/fault_clear,
 *          and send an acknowledgment or error response.
 *
 *          Supported commands:
 *            set_mode    — switch to CV or CC with engineering-unit value
 *            clear_fault — transition FAULT→IDLE (fault_clear)
 *            get_status  — return runtime state as JSON
 *            get_info    — return static system info as JSON
 *
 * @param   line - Null-terminated command string (JSON)
 *
 * @return  none
 */
void protocol_process_command(const char *line)
{
    cJSON *root;
    cJSON *cmd_item;
    cJSON *mode_item;
    cJSON *value_item;
    cJSON *response;
    char *response_str;
    const char *cmd;
    float value;

    root     = NULL;
    cmd_item = NULL;
    mode_item = NULL;
    value_item = NULL;
    response   = NULL;
    response_str = NULL;
    cmd = NULL;
    value = 0.0f;

    /* Step 1: Parse JSON */
    root = cJSON_Parse(line);
    if (root == NULL)
    {
        /* Silently ignore parse failures — they are almost always caused
         * by line noise on a floating RX pin, not by real commands.
         * Diagnostics go to CDC (printf), not UART1, to keep the command
         * channel clean. */
        printf("[PROTO] parse err (len=%u): ", (unsigned)strlen(line));
        printf("%s\r\n", line);
        return; /* no cJSON_Delete needed — root is NULL */
    }

    /* Step 2: Extract "cmd" field */
    cmd_item = cJSON_GetObjectItem(root, "cmd");
    if (cmd_item == NULL || !(cmd_item->type & cJSON_String))
    {
        send_error_ack(PROTO_ERR_VAL_MISSING, "Missing required field: cmd");
        goto cleanup;
    }
    cmd = cmd_item->valuestring;

    /* Step 3: Dispatch by command name */
    if (strcmp(cmd, "set_mode") == 0)
    {
        /* State gate: reject in FAULT */
        if (system_mode == MODE_FAULT)
        {
            send_error_ack(PROTO_ERR_STATE_FAULT, "Fault active — clear fault first");
            goto cleanup;
        }

        /* Extract mode field */
        mode_item = cJSON_GetObjectItem(root, "mode");
        if (mode_item == NULL || !(mode_item->type & cJSON_String))
        {
            send_error_ack(PROTO_ERR_VAL_MISSING, "Missing required field: mode");
            goto cleanup;
        }

        /* Extract value field */
        value_item = cJSON_GetObjectItem(root, "value");
        if (value_item == NULL || !(value_item->type & cJSON_Number))
        {
            send_error_ack(PROTO_ERR_VAL_MISSING, "Missing required field: value");
            goto cleanup;
        }
        value = (float)value_item->valuedouble;

        /* Validate mode string */
        if (strcmp(mode_item->valuestring, "CV") == 0)
        {
            /* Range check */
            if (value < 0.0f || value > HW_MAX_VOLTAGE)
            {
                send_error_ack(PROTO_ERR_VAL_RANGE, "Value out of range [0.0, 30.0]V");
                goto cleanup;
            }

            /* Idempotency: same mode + same value → no-op ack */
            if (system_mode == MODE_CV && fabsf(value - cv_target_voltage) < 0.01f)
            {
                protocol_send("{\"ack\":\"ok\"}");
                goto cleanup;
            }

            /* Dispatch CV engagement */
            engage_cv(value);
            protocol_send("{\"ack\":\"ok\"}");
        }
        else if (strcmp(mode_item->valuestring, "CC") == 0)
        {
            /* Range check */
            if (value < 0.0f || value > HW_MAX_CURRENT)
            {
                send_error_ack(PROTO_ERR_VAL_RANGE, "Value out of range [0.0, 10.0]A");
                goto cleanup;
            }

            /* Idempotency: same mode + same value → no-op ack */
            if (system_mode == MODE_CC && fabsf(value - cc_target_current) < 0.01f)
            {
                protocol_send("{\"ack\":\"ok\"}");
                goto cleanup;
            }

            /* Dispatch CC engagement */
            engage_cc(value);
            protocol_send("{\"ack\":\"ok\"}");
        }
        else
        {
            send_error_ack(PROTO_ERR_VAL_MODE, "Invalid mode — use CV or CC");
        }
    }
    else if (strcmp(cmd, "clear_fault") == 0)
    {
        /* State gate: only valid in FAULT */
        if (system_mode != MODE_FAULT)
        {
            send_error_ack(PROTO_ERR_STATE_TRANSITION, "Not in fault state");
            goto cleanup;
        }

        fault_clear();
        protocol_send("{\"ack\":\"ok\"}");
    }
    else if (strcmp(cmd, "get_status") == 0)
    {
        response = cJSON_CreateObject();
        if (response == NULL) goto cleanup;

        cJSON_AddStringToObject(response, "mode",
            system_mode_to_string(system_mode));
        cJSON_AddNumberToObject(response, "setpoint",
            (system_mode == MODE_CV) ? (double)cv_target_voltage
                                     : (double)cc_target_current);
        cJSON_AddNumberToObject(response, "dac", last_dac_value);
        cJSON_AddNumberToObject(response, "fault_code", fault_reg.raw);

        response_str = cJSON_PrintUnformatted(response);
        if (response_str != NULL)
        {
            protocol_send(response_str);
        }
    }
    else if (strcmp(cmd, "get_info") == 0)
    {
        response = cJSON_CreateObject();
        if (response == NULL) goto cleanup;

        cJSON_AddStringToObject(response, "fw_ver", FW_VERSION);
        cJSON_AddNumberToObject(response, "chip_id",
            (double)DBGMCU_GetCHIPID());
        cJSON_AddNumberToObject(response, "rated_w", RATED_WATTAGE);
        cJSON_AddNumberToObject(response, "max_v", HW_MAX_VOLTAGE);
        cJSON_AddNumberToObject(response, "max_a", HW_MAX_CURRENT);

        response_str = cJSON_PrintUnformatted(response);
        if (response_str != NULL)
        {
            protocol_send(response_str);
        }
    }
    else
    {
        send_error_ack(PROTO_ERR_VAL_CMD, "Unknown command");
    }

cleanup:
    if (root != NULL)     cJSON_Delete(root);
    if (response != NULL) cJSON_Delete(response);
    cjson_pool_used = 0;  /* Reset arena for next cycle */
}

/*********************************************************************
 * @fn      protocol_send_telemetry
 *
 * @brief   Assemble and send a cJSON telemetry packet over USART1.
 *          Contains: 4-channel MOS data in ch[] array, summary data
 *          in sum{} object, and flat meta fields (seq, uptime, mode,
 *          fault, dac, retry, temp). Transmitted at 10Hz via the
 *          100ms control cycle (COMM-02).
 *
 * @param   summary_v - Summary bus voltage (volts)
 * @param   summary_i - Summary total current (amps)
 * @param   summary_p - Summary total power (watts)
 * @param   mos_i     - Per-channel MOS currents array [4]
 * @param   mos_v     - Per-channel MOS bus voltages array [4]
 *
 * @return  none
 */
void protocol_send_telemetry(float summary_v, float summary_i, float summary_p,
                             float mos_i[4], float mos_v[4])
{
    cJSON *root;
    cJSON *ch_array;
    cJSON *ch_obj;
    cJSON *sum_obj;
    char *json_str;
    static uint16_t telemetry_seq = 0;
    uint8_t i;

    root     = NULL;
    ch_array = NULL;
    ch_obj   = NULL;
    sum_obj  = NULL;
    json_str = NULL;

    /* Reset cJSON arena before assembly — reclaim all memory */
    cjson_pool_used = 0;

    /* Create root object */
    root = cJSON_CreateObject();
    if (root == NULL) return;

    /* Flat meta fields */
    cJSON_AddNumberToObject(root, "seq", (double)telemetry_seq++);
    cJSON_AddNumberToObject(root, "uptime", (double)(cycle_count * CONTROL_PERIOD_MS));
    cJSON_AddStringToObject(root, "mode", system_mode_to_string(system_mode));
    cJSON_AddNumberToObject(root, "fault", fault_reg.raw);
    cJSON_AddNumberToObject(root, "dac", last_dac_value);
    cJSON_AddNumberToObject(root, "retry", fault_reg.bits.retry_count);
    cJSON_AddNumberToObject(root, "temp", (double)heatsink_temp_c); /* NTC heatsink temperature — Phase 4 D-04 */
    cJSON_AddNumberToObject(root, "fan_rpm", (double)fan_rpm);   /* Fan RPM — Phase 4 FAN-02 */
    cJSON_AddNumberToObject(root, "fan_stall", (double)fan_stall); /* Fan stall flag — Phase 4 FAN-02 */

    /* Build ch[] array — 4 MOS channels */
    ch_array = cJSON_CreateArray();
    for (i = 0; i < 4; i++)
    {
        ch_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(ch_obj, "v", (double)mos_v[i]);
        cJSON_AddNumberToObject(ch_obj, "i", (double)mos_i[i]);
        cJSON_AddItemToArray(ch_array, ch_obj);
    }
    cJSON_AddItemToObject(root, "ch", ch_array);

    /* Build sum{} object */
    sum_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(sum_obj, "v", (double)summary_v);
    cJSON_AddNumberToObject(sum_obj, "i", (double)summary_i);
    cJSON_AddNumberToObject(sum_obj, "p", (double)summary_p);
    cJSON_AddItemToObject(root, "sum", sum_obj);

    /* Print to compact JSON and transmit */
    json_str = cJSON_PrintUnformatted(root);
    if (json_str != NULL)
    {
        protocol_send(json_str); /* includes trailing '\n' */
    }

    /* Cleanup */
    cJSON_Delete(root);
    cjson_pool_used = 0; /* Reset arena for next cycle */
}

/*********************************************************************
 * @fn      cdc_send_telemetry
 *
 * @brief   Assemble and send a lightweight cJSON telemetry packet
 *          over USB-CDC (via printf) at 10 Hz.
 *
 *          Packet fields:
 *            seq      - monotonic sequence number
 *            mode     - "IDLE" / "CV" / "CC" / "FAULT"
 *            setpoint - target voltage (CV) or current (CC)
 *            v        - bus voltage (placeholder until INA226 active)
 *            i        - bus current (placeholder until INA226 active)
 *            p        - bus power   (placeholder until INA226 active)
 *
 * @return  none
 */
void cdc_send_telemetry(void)
{
    cJSON *root;
    char *json_str;
    static uint16_t cdc_seq = 0;
    float setpoint;

    /* Reset cJSON arena before assembly */
    cjson_pool_used = 0;

    root = cJSON_CreateObject();
    if (root == NULL) return;

    cJSON_AddNumberToObject(root, "seq", (double)cdc_seq++);

    if (system_mode == MODE_CV)
    {
        cJSON_AddStringToObject(root, "mode", "CV");
        setpoint = cv_target_voltage;
    }
    else if (system_mode == MODE_CC)
    {
        cJSON_AddStringToObject(root, "mode", "CC");
        setpoint = cc_target_current;
    }
    else if (system_mode == MODE_FAULT)
    {
        cJSON_AddStringToObject(root, "mode", "FAULT");
        setpoint = 0.0f;
    }
    else
    {
        cJSON_AddStringToObject(root, "mode", "IDLE");
        setpoint = 0.0f;
    }

    cJSON_AddNumberToObject(root, "setpoint", (double)setpoint);

    /* Placeholder values — INA226 not yet active */
    cJSON_AddNumberToObject(root, "v", 0.0);
    cJSON_AddNumberToObject(root, "i", 0.0);
    cJSON_AddNumberToObject(root, "p", 0.0);

    json_str = cJSON_PrintUnformatted(root);
    if (json_str != NULL)
    {
        printf("%s\r\n", json_str);
    }

    cJSON_Delete(root);
    cjson_pool_used = 0; /* Reset arena for next cycle */
}
