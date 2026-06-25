/********************************** (C) COPYRIGHT  *******************************
* File Name          : protocol.h
* Author             : GSD Phase 03
* Version            : V1.0.0
* Date               : 2026/06/07
* Description        : Communication protocol header.
*                      Defines the public API for USART1-based cJSON command
*                      reception and telemetry transmission. Declares error
*                      codes for parse, validation, and state rejection.
*******************************************************************************/
#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* --------------------------------------------------------------------------
 * Ring Buffer Configuration
 * -------------------------------------------------------------------------- */
#define RX_BUF_SIZE   512   /* RX ring buffer size in bytes */
#define LINE_BUF_SIZE 256   /* Max extracted line length */

/* --------------------------------------------------------------------------
 * Frame Format — <chip_id,cmd_id>{json}
 * -------------------------------------------------------------------------- */
#define PROTO_FRAME_PREFIX     '<'   /* Frame start marker */
#define PROTO_FRAME_SUFFIX     '>'   /* Frame header end marker */
#define PROTO_FRAME_SEPARATOR  ','   /* chip_id / cmd_id separator */
#define PROTO_CHIP_ID_LEN      8     /* 8-char hex chip ID string */

/* --------------------------------------------------------------------------
 * Command Codes — 1xxx = Downlink (下发), 2xxx = Uplink (上报)
 * -------------------------------------------------------------------------- */
#define PROTO_CMD_POWER_SET   101    /* Power command: {"I":"...","V":"..."}  */
#define PROTO_CMD_MODE_SET    102    /* Mode command:  {"mode":"..."}         */

#define PROTO_CMD_RESPONSE    200    /* Response:      {"command":"..."}      */

/* --------------------------------------------------------------------------
 * Mode Codes — for command 102 "mode" field value (string)
 * -------------------------------------------------------------------------- */
#define PROTO_MODE_CC         11     /* Constant Current mode                  */
#define PROTO_MODE_CV         12     /* Constant Voltage mode                  */
#define PROTO_MODE_TYPEC_ONLY 20     /* Type-C trigger only (not implemented) */
#define PROTO_MODE_TYPEC_CC   21     /* Type-C trigger + CC (not implemented) */

/* --------------------------------------------------------------------------
 * Error Codes — Parse Phase (1xx)
 * -------------------------------------------------------------------------- */
#define PROTO_ERR_PARSE_SYNTAX      101   /* Invalid JSON syntax */
#define PROTO_ERR_PARSE_INCOMPLETE  102   /* Incomplete JSON */
#define PROTO_ERR_PARSE_OVERFLOW    103   /* RX buffer overflow */
#define PROTO_ERR_PARSE_FRAME       110   /* Invalid frame format <...> */
#define PROTO_ERR_PARSE_CHIPID      111   /* Invalid chip ID in frame */
#define PROTO_ERR_PARSE_CMDID       112   /* Invalid command ID in frame */
#define USART_RX_CH         DMA1_Channel5

/* --------------------------------------------------------------------------
 * Error Codes — Validation Phase (2xx)
 * -------------------------------------------------------------------------- */
#define PROTO_ERR_VAL_MISSING       201   /* Missing required field */
#define PROTO_ERR_VAL_RANGE         202   /* Value out of range */
#define PROTO_ERR_VAL_MODE          203   /* Invalid mode string */
#define PROTO_ERR_VAL_CMD           204   /* Unknown command */
#define PROTO_ERR_VAL_TYPE          205   /* Bad field type */
#define PROTO_ERR_VAL_NOT_IMPL      206   /* Feature not implemented */

/* --------------------------------------------------------------------------
 * Error Codes — State Rejection (3xx)
 * -------------------------------------------------------------------------- */
#define PROTO_ERR_STATE_REJECTED    301   /* Command rejected in current mode */
#define PROTO_ERR_STATE_TRANSITION  302   /* Transition not allowed */
#define PROTO_ERR_STATE_FAULT       303   /* Fault active, clear first */
#define RING_BUFFER_LEN     (1024u)
#define RX_BUFFER_LEN       (128u)

/* --------------------------------------------------------------------------
 * Hardware Limits
 * -------------------------------------------------------------------------- */
#define HW_MAX_VOLTAGE    30.0f   /* CV range maximum (volts) */
#define HW_MAX_CURRENT    10.0f   /* CC range maximum (amps) */
#define FW_VERSION        "1.0.0" /* Firmware version for get_info */
#define CONTROL_PERIOD_MS 100     /* Main loop cycle period in milliseconds */
typedef struct
{
    uint8_t           buffer[RING_BUFFER_LEN];
    volatile uint16_t RecvPos;  //
    volatile uint16_t SendPos;  //
    volatile uint16_t RemainCount;

} ringbuffer;
typedef struct
{
    volatile uint8_t DMA_USE_BUFFER;
    uint8_t          Rx_Buffer[2][RX_BUFFER_LEN];

} USART_DMA_CTRL_;


/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/* Initialize USART1 (PA9=TX, PA10=RX) at 115200 bps 8N1+odd parity,
 * zero the 512-byte RX ring buffer, enable RXNE interrupt at NVIC
 * priority 0x02, and prepare the cJSON parser hooks. */
void protocol_init(void);

/* Poll the RX ring buffer for a complete newline-delimited line.
 * If a full line is available, copies it (excluding '\n') into a
 * static buffer and returns a pointer to it.
 * Returns NULL if no complete line has been received. */
const char *protocol_poll(void);

/* Send a null-terminated JSON string over USART1, appending '\n'.
 * Blocks until the entire string has been transmitted (TC flag polling). */
void protocol_send(const char *json_str);

/* Assemble and send a telemetry packet over USART1.
 * Takes current sensor readings as parameters to avoid extern globals.
 * summary_v/i/p: summary INA226 values for the sum{} object
 * mos_i[4]: per-channel currents for the ch[] array
 * mos_v[4]: per-channel bus voltages for the ch[] array */
void protocol_send_telemetry(float summary_v, float summary_i, float summary_p,
                             float mos_i[4], float mos_v[4]);

/* Process a received command line: parse JSON, dispatch to handler,
 * send acknowledgment or error response. Called by main loop. */
void protocol_process_command(const char *line);
uint8_t ring_buffer_pop();
void ring_buffer_push_huge(uint8_t *buffer, uint16_t len);
void DMA_INIT(void);
void cdc_send_telemetry(void);

/* Return the local chip ID as an 8-character hex string (null-terminated).
 * Initialized by protocol_init() from DBGMCU_GetCHIPID(). */
const char *protocol_get_local_chip_id(void);


#ifdef __cplusplus
}
#endif

#endif /* __PROTOCOL_H */
