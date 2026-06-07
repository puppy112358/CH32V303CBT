/********************************** (C) COPYRIGHT  *******************************
* File Name          : protocol.c
* Author             : GSD Phase 03
* Version            : V1.0.0
* Date               : 2026/06/07
* Description        : Communication protocol implementation.
*                      USART2 (PA2=TX, PA3=RX) at 115200 bps 8N1+odd parity.
*                      Interrupt-driven 512-byte RX ring buffer with newline-
*                      delimited line extraction. Blocking TX for responses.
*******************************************************************************/
#include "../Drivers/protocol.h"
#include "debug.h"
#include "../Drivers/cjson/cJSON.h"
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

/* --------------------------------------------------------------------------
 * cJSON Arena Allocator (4 KB static pool)
 * -------------------------------------------------------------------------- */
#define CJSON_POOL_SIZE 4096
static uint8_t cjson_pool[CJSON_POOL_SIZE];
static size_t  cjson_pool_used = 0;

static void *cjson_arena_alloc(size_t sz)
{
    void *ptr;
    if (cjson_pool_used + sz > CJSON_POOL_SIZE)
    {
        return NULL;
    }
    ptr = &cjson_pool[cjson_pool_used];
    cjson_pool_used += sz;
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
 * Ring Buffer Variables (non-static — shared with ch32v30x_it.c ISR)
 * -------------------------------------------------------------------------- */
volatile uint8_t  rx_buf[RX_BUF_SIZE];
volatile uint16_t rx_head         = 0;
volatile uint16_t rx_tail         = 0;
volatile uint8_t  rx_overflow     = 0;
volatile uint8_t  rx_parity_err   = 0;
volatile uint8_t  rx_framing_err  = 0;

/* --------------------------------------------------------------------------
 * Line Buffer for protocol_poll()
 * -------------------------------------------------------------------------- */
static char line_buf[LINE_BUF_SIZE];

/*********************************************************************
 * @fn      protocol_init
 *
 * @brief   Initialize USART2 at 115200 bps 8N1+odd parity with RXNE
 *          interrupt enabled. Configure PA2 as AF push-pull (TX) and
 *          PA3 as input floating (RX). Zero ring buffer and error
 *          counters.
 *
 * @return  none
 */
void protocol_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    /* Enable peripheral clocks */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* Configure PA2 (TX) — AF push-pull */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Configure PA3 (RX) — input floating */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* USART2: 115200 bps, 8 data bits, 1 stop bit, odd parity, RX+TX */
    USART_InitStructure.USART_BaudRate            = 115200;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_Odd;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    /* Enable RXNE interrupt */
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    /* Enable USART2 */
    USART_Cmd(USART2, ENABLE);

    /* Configure NVIC: USART2 at priority 0x02 (below EXTI4 at 0x01) */
    NVIC_SetPriority(USART2_IRQn, 0x02);
    NVIC_EnableIRQ(USART2_IRQn);

    /* Zero ring buffer and error flags */
    rx_head        = 0;
    rx_tail        = 0;
    rx_overflow    = 0;
    rx_parity_err  = 0;
    rx_framing_err = 0;

    /* Configure cJSON arena allocator hooks */
    {
        cJSON_Hooks hooks;
        hooks.malloc_fn = cjson_arena_alloc;
        hooks.free_fn   = cjson_arena_free;
        cJSON_InitHooks(&hooks);
    }

    printf("Protocol init: USART2 115200 8N1+odd parity, ring buffer %d bytes\r\n",
           RX_BUF_SIZE);
    printf("cJSON arena: %d bytes at 0x%08X\r\n",
           CJSON_POOL_SIZE, (uint32_t)(uintptr_t)cjson_pool);
}

/*********************************************************************
 * @fn      protocol_poll
 *
 * @brief   Poll the RX ring buffer for a complete newline-delimited
 *          line. If a '\n' byte is found, copies the line (excluding
 *          '\n') into line_buf, null-terminates, advances rx_head past
 *          the delimiter, and returns a pointer to line_buf.
 *          Handles overflow by flushing the ring buffer.
 *
 * @return  Pointer to null-terminated line string, or NULL if no
 *          complete line is available.
 */
const char *protocol_poll(void)
{
    uint16_t head;
    uint16_t tail;
    uint16_t pos;
    uint16_t len;
    uint16_t i;

    head = rx_head;
    tail = rx_tail;

    /* Buffer empty */
    if (head == tail)
    {
        return NULL;
    }

    /* Overflow condition: flush ring buffer and report */
    if (rx_overflow)
    {
        rx_head     = 0;
        rx_tail     = 0;
        rx_overflow = 0;
        return NULL;
    }

    /* Scan from head for '\n' */
    pos = head;
    len = 0;
    while (pos != tail)
    {
        if (rx_buf[pos] == '\n')
        {
            /* Found delimiter — copy line (excluding '\n') */
            i = 0;
            while (head != pos && i < (LINE_BUF_SIZE - 1))
            {
                line_buf[i] = (char)rx_buf[head];
                i++;
                head++;
                if (head >= RX_BUF_SIZE)
                {
                    head = 0;
                }
            }
            line_buf[i] = '\0';

            /* Advance rx_head past the '\n' */
            head++;
            if (head >= RX_BUF_SIZE)
            {
                head = 0;
            }
            rx_head = head;

            /* If line was truncated (didn't fit), return NULL */
            if (i >= (LINE_BUF_SIZE - 1) && rx_buf[pos] != '\n')
            {
                return NULL;
            }

            return line_buf;
        }

        pos++;
        if (pos >= RX_BUF_SIZE)
        {
            pos = 0;
        }
        len++;
        if (len >= LINE_BUF_SIZE)
        {
            /* Line too long — discard until '\n' or buffer empty */
            while (pos != tail && rx_buf[pos] != '\n')
            {
                pos++;
                if (pos >= RX_BUF_SIZE)
                {
                    pos = 0;
                }
            }
            if (pos != tail)
            {
                /* Skip past the '\n' */
                pos++;
                if (pos >= RX_BUF_SIZE)
                {
                    pos = 0;
                }
            }
            rx_head = pos;
            return NULL;
        }
    }

    /* No '\n' found */
    return NULL;
}

/*********************************************************************
 * @fn      protocol_send
 *
 * @brief   Send a null-terminated JSON string over USART2, appending
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
        while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
        {
            /* Wait for TX complete */
        }
        USART_SendData(USART2, (uint16_t)(*json_str));
        json_str++;
    }

    /* Append line terminator */
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
    {
        /* Wait for TX complete */
    }
    USART_SendData(USART2, (uint16_t)'\n');
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
        send_error_ack(PROTO_ERR_PARSE_SYNTAX, "Invalid JSON syntax");
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
 * @brief   Assemble and send a cJSON telemetry packet over USART2.
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
    cJSON_AddNumberToObject(root, "temp", 0.0); /* Placeholder per D-11 */

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
