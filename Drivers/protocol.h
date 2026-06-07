/********************************** (C) COPYRIGHT  *******************************
* File Name          : protocol.h
* Author             : GSD Phase 03
* Version            : V1.0.0
* Date               : 2026/06/07
* Description        : Communication protocol header.
*                      Defines the public API for USART2-based cJSON command
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
 * Error Codes — Parse Phase (1xx)
 * -------------------------------------------------------------------------- */
#define PROTO_ERR_PARSE_SYNTAX      101   /* Invalid JSON syntax */
#define PROTO_ERR_PARSE_INCOMPLETE  102   /* Incomplete JSON */
#define PROTO_ERR_PARSE_OVERFLOW    103   /* RX buffer overflow */

/* --------------------------------------------------------------------------
 * Error Codes — Validation Phase (2xx)
 * -------------------------------------------------------------------------- */
#define PROTO_ERR_VAL_MISSING       201   /* Missing required field */
#define PROTO_ERR_VAL_RANGE         202   /* Value out of range */
#define PROTO_ERR_VAL_MODE          203   /* Invalid mode string */
#define PROTO_ERR_VAL_CMD           204   /* Unknown command */
#define PROTO_ERR_VAL_TYPE          205   /* Bad field type */

/* --------------------------------------------------------------------------
 * Error Codes — State Rejection (3xx)
 * -------------------------------------------------------------------------- */
#define PROTO_ERR_STATE_REJECTED    301   /* Command rejected in current mode */
#define PROTO_ERR_STATE_TRANSITION  302   /* Transition not allowed */
#define PROTO_ERR_STATE_FAULT       303   /* Fault active, clear first */

/* --------------------------------------------------------------------------
 * Hardware Limits
 * -------------------------------------------------------------------------- */
#define HW_MAX_VOLTAGE    30.0f   /* CV range maximum (volts) */
#define HW_MAX_CURRENT    10.0f   /* CC range maximum (amps) */
#define FW_VERSION        "1.0.0" /* Firmware version for get_info */
#define CONTROL_PERIOD_MS 100     /* Main loop cycle period in milliseconds */

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/* Initialize USART2 (PA2=TX, PA3=RX) at 115200 bps 8N1+odd parity,
 * zero the 512-byte RX ring buffer, enable RXNE interrupt at NVIC
 * priority 0x02, and prepare the cJSON parser hooks. */
void protocol_init(void);

/* Poll the RX ring buffer for a complete newline-delimited line.
 * If a full line is available, copies it (excluding '\n') into a
 * static buffer and returns a pointer to it.
 * Returns NULL if no complete line has been received. */
const char *protocol_poll(void);

/* Send a null-terminated JSON string over USART2, appending '\n'.
 * Blocks until the entire string has been transmitted (TC flag polling). */
void protocol_send(const char *json_str);

/* Assemble and send a telemetry packet over USART2.
 * Takes current sensor readings as parameters to avoid extern globals.
 * summary_v/i/p: summary INA226 values for the sum{} object
 * mos_i[4]: per-channel currents for the ch[] array
 * mos_v[4]: per-channel bus voltages for the ch[] array */
void protocol_send_telemetry(float summary_v, float summary_i, float summary_p,
                             float mos_i[4], float mos_v[4]);

/* Process a received command line: parse JSON, dispatch to handler,
 * send acknowledgment or error response. Called by main loop. */
void protocol_process_command(const char *line);

#ifdef __cplusplus
}
#endif

#endif /* __PROTOCOL_H */
