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

/* --------------------------------------------------------------------------
 * Ring Buffer Constants
 * -------------------------------------------------------------------------- */
#define RX_BUF_SIZE   512
#define LINE_BUF_SIZE 256

/* --------------------------------------------------------------------------
 * Ring Buffer Static Variables
 * -------------------------------------------------------------------------- */
static volatile uint8_t  rx_buf[RX_BUF_SIZE];
static volatile uint16_t rx_head         = 0;
static volatile uint16_t rx_tail         = 0;
static volatile uint8_t  rx_overflow     = 0;
static volatile uint8_t  rx_parity_err   = 0;
static volatile uint8_t  rx_framing_err  = 0;

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

    printf("Protocol init: USART2 115200 8N1+odd parity, ring buffer %d bytes\r\n",
           RX_BUF_SIZE);
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
 * @fn      protocol_send_telemetry
 *
 * @brief   Assemble and send a telemetry packet over USART2.
 *          Stub — full implementation in Plan 03.
 *
 * @return  none
 */
void protocol_send_telemetry(void)
{
    /* TODO: Plan 03 */
}
