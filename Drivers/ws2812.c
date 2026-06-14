/********************************** (C) COPYRIGHT  *******************************
* File Name          : ws2812.c
* Author             : GSD Phase 04
* Version            : V1.0.0
* Date               : 2026/06/08
* Description        : WS2812 LED driver implementation.
*                      Drives 2 WS2812 LEDs via TIM2 CH1 PWM (PA0) + DMA1
*                      Channel 5 at 800kHz using NRZ bitstream encoding.
*                      Each bit maps to a PWM CCR value: T0H=25 (bit 0),
*                      T1H=50 (bit 1) at ARR=89. GRB color ordering, MSB-first
*                      per WS2812B V5 datasheet. 50% brightness via R/2, G/2, B/2.
*******************************************************************************/
#include "../Drivers/ws2812.h"
#include "debug.h"

/* --------------------------------------------------------------------------
 * Module Constants
 * -------------------------------------------------------------------------- */
#define WS2812_LED_COUNT        2
#define WS2812_BITS_PER_LED     24
#define WS2812_BUF_SIZE         (WS2812_LED_COUNT * WS2812_BITS_PER_LED)  /* = 48 */
#define WS2812_CCR_T0H          25   /* PWM CCR for NRZ bit 0 (~28% duty at ARR=89) */
#define WS2812_CCR_T1H          50   /* PWM CCR for NRZ bit 1 (~56% duty at ARR=89) */
#define WS2812_BRIGHTNESS_DIV   2    /* 50% brightness divisor */

/* --------------------------------------------------------------------------
 * Module Static Variables
 * -------------------------------------------------------------------------- */
static uint8_t  ws2812_bitstream[WS2812_BUF_SIZE];  /* 48-byte DMA source buffer */
static SystemMode ws2812_last_mode = (SystemMode)(-1); /* Force first update always */
volatile uint8_t ws2812_dma_busy = 0;               /* 1 = DMA transfer active */

/* --------------------------------------------------------------------------
 * Static Helper Declarations (defined in Task 3)
 * -------------------------------------------------------------------------- */
static void ws2812_dma_trigger(void);

/*********************************************************************
 * @fn      ws2812_set_color
 *
 * @brief   Encode RGB color into WS2812 GRB bitstream and trigger DMA.
 *
 *          Each RGB byte is divided by 2 for 50% brightness.
 *          Color byte order is GRB (green first per WS2812 protocol).
 *          Bit order within each byte is MSB-first.
 *          Both LEDs receive identical color data.
 *
 * @param   r - Red channel value (0-255, halved internally)
 * @param   g - Green channel value (0-255, halved internally)
 * @param   b - Blue channel value (0-255, halved internally)
 *
 * @return  none
 */
void ws2812_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t led_index;
    uint8_t byte_i;
    int8_t  bit_i;
    uint8_t colors[3];
    uint8_t offset;
    uint8_t buf_idx;
    uint8_t color_byte;

    /* Apply 50% brightness: integer divide each channel by 2 */
    colors[0] = g / WS2812_BRIGHTNESS_DIV;  /* WS2812 GRB order: green first */
    colors[1] = r / WS2812_BRIGHTNESS_DIV;  /* red second */
    colors[2] = b / WS2812_BRIGHTNESS_DIV;  /* blue third */

    /* Encode both LEDs with identical color */
    for (led_index = 0; led_index < WS2812_LED_COUNT; led_index++)
    {
        offset = led_index * WS2812_BITS_PER_LED;

        for (byte_i = 0; byte_i < 3; byte_i++)
        {
            color_byte = colors[byte_i];

            for (bit_i = 7; bit_i >= 0; bit_i--)
            {
                buf_idx = offset + (byte_i * 8) + (7 - (uint8_t)bit_i);

                if (color_byte & (1 << (uint8_t)bit_i))
                {
                    ws2812_bitstream[buf_idx] = WS2812_CCR_T1H;
                }
                else
                {
                    ws2812_bitstream[buf_idx] = WS2812_CCR_T0H;
                }
            }
        }
    }

    /* Trigger DMA transfer (hardware init in ws2812_init, Task 3) */
    ws2812_dma_trigger();
}

/*********************************************************************
 * @fn      ws2812_update_from_mode
 *
 * @brief   Map SystemMode to RGB color and drive LED update.
 *
 *          Skips recomputation when mode is unchanged since last call
 *          (except FAULT mode which always re-triggers for visibility).
 *
 * @param   mode - Current system operating mode
 *
 * @return  none
 */
void ws2812_update_from_mode(SystemMode mode)
{
    /* Skip if mode unchanged (optimization: avoid unnecessary DMA transfer).
     * FAULT mode always re-triggers for guaranteed visibility. */
    if (mode == ws2812_last_mode && mode != MODE_FAULT)
    {
        return;
    }

    ws2812_last_mode = mode;

    switch (mode)
    {
    case MODE_IDLE:
        ws2812_set_color(0, 32, 0);     /* Green at 50% brightness */
        break;

    case MODE_CV:
        ws2812_set_color(0, 32, 32);    /* Cyan at 50% brightness */
        break;

    case MODE_CC:
        ws2812_set_color(0, 0, 32);     /* Blue at 50% brightness */
        break;

    case MODE_FAULT:
        ws2812_set_color(32, 0, 0);     /* Red at 50% brightness */
        break;

    default:
        ws2812_set_color(0, 32, 0);     /* Safe default: green (IDLE) */
        break;
    }
}

/*********************************************************************
 * @fn      ws2812_dma_trigger
 *
 * @brief   Arm and trigger DMA1 Channel 5 to stream bitstream to TIM2 CH1 CCR.
 *
 *          Safety features:
 *          - Guards against overlapping DMA transfers (busy flag check)
 *          - Disables DMA before reconfiguring CNTR/MADDR (atomic reconfigure)
 *          - Resets TIM2 counter to start PWM cycle cleanly from rising edge
 *
 *          Precondition: ws2812_bitstream[] populated with 48 CCR values.
 *          Postcondition: DMA TC ISR clears ws2812_dma_busy after transfer.
 *
 * @return  none
 */
static void ws2812_dma_trigger(void)
{
    /* Guard: skip if previous DMA transfer still in progress */
    if (ws2812_dma_busy)
    {
        return;
    }

    ws2812_dma_busy = 1;

    /* Atomic reconfigure: disable → update → re-enable */
    DMA_Cmd(DMA1_Channel5, DISABLE);

    /* Set transfer count to 48 bytes (2 LEDs × 24 bits/LED) */
    DMA1_Channel5->CNTR = WS2812_BUF_SIZE;

    /* Point memory address to bitstream buffer */
    DMA1_Channel5->MADDR = (uint32_t)ws2812_bitstream;

    DMA_Cmd(DMA1_Channel5, ENABLE);

    /* Reset TIM2 counter to start PWM cycle from clean rising edge.
     * DMA transfers begin on each TIM2 CH1 compare match (CCR update). */
    TIM_SetCounter(TIM2, 0);
    TIM_Cmd(TIM2, ENABLE);
}

/*********************************************************************
 * @fn      ws2812_init
 *
 * @brief   Initialize WS2812 LED driver hardware.
 *
 *          Clock enable → PA0 AF push-pull → TIM2 800kHz PWM mode 1 →
 *          DMA1 Channel 5 memory-to-peripheral (CCR streaming) →
 *          NVIC DMA TC interrupt at priority 0x03 →
 *          Transmit initial green (IDLE) color.
 *
 * @return  none
 */
void ws2812_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    TIM_OCInitTypeDef TIM_OCInitStructure = {0};
    DMA_InitTypeDef DMA_InitStructure = {0};

    /* ---- 1. Enable peripheral clocks ---- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    /* ---- 2. Configure PA0 as alternate-function push-pull for TIM2 CH1 ---- */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* ---- 3. Configure TIM2 time base for 800kHz PWM ---- */
    /* TIM2_CLK = APB1 = 72MHz (PCLK1, when APB1 prescaler ≠ 1 → ×2).
     * PSC=0 → TIM2 counter at 72MHz.
     * ARR=89 → Period = 90 cycles → 72MHz/90 = 800kHz PWM frequency. */
    TIM_TimeBaseStructure.TIM_Period = 89;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /* ---- 4. Configure TIM2 CH1 as PWM mode 1 output ---- */
    /* PWM mode 1: output active high when CNT < CCR.
     * Initial CCR=0 → output stays low (WS2812 RESET condition >50μs). */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);

    /* ---- 5. Enable TIM2 DMA request on CH1 compare match ---- */
    TIM_DMACmd(TIM2, TIM_DMA_CC1, ENABLE);

    /* ---- 6. Configure DMA1 Channel 5: Memory→Peripheral, byte-sized ---- */
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&TIM2->CH1CVR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ws2812_bitstream;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = 0;               /* Set per-transfer via CNTR */
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;       /* Single-shot, not circular */
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);

    /* ---- 7. NVIC: DMA1 Channel 5 TC interrupt ---- */
    /* Priority 0x03: lower than EXTI4 (0x01) and USART3 (0x02). */
    NVIC_SetPriority(DMA1_Channel5_IRQn, 0x03);
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);

    /* ---- 8. Set initial state to green (IDLE) and transmit ---- */
    ws2812_set_color(0, 32, 0);

    printf("WS2812 init: TIM2 CH1 PA0 800kHz DMA1_CH5, 2 LEDs, mode=green\r\n");
}
