/********************************** (C) COPYRIGHT  *******************************
* File Name          : fan.c
* Author             : GSD Phase 04
* Version            : V1.0.0
* Date               : 2026/06/08
* Description        : Fan control implementation.
*                      Drives 4-wire PWM cooling fan: TIM3 CH1 (PA6) at 25kHz PWM,
*                      TIM3 CH2 (PA7) input capture for tachometer RPM measurement,
*                      PID temperature control (PI-only, setpoint 50°C), and stall
*                      detection (RPM=0 with duty≥25% for 2 seconds → fan_stall=1).
*******************************************************************************/
#include "../Drivers/fan.h"
#include "../Drivers/temp_sensor.h"
#include "debug.h"

/* --------------------------------------------------------------------------
 * Module Constants
 * -------------------------------------------------------------------------- */
#define TIM3_PSC         143       /* 144MHz / 144 = 1MHz counter */
#define TIM3_ARR         39        /* 1MHz / 40 = 25kHz PWM frequency */
#define TACHO_TIMEOUT_MAX 5        /* 500ms without tacho edge → RPM=0 */
#define RPM_SCALE        30000000UL /* 60 sec/min × 1,000,000 ticks/sec / 2 pulses/rev */

/* --------------------------------------------------------------------------
 * Global Variables
 * -------------------------------------------------------------------------- */
volatile uint16_t fan_rpm     = 0;
volatile uint8_t  fan_stall   = 0;
PID_Instance      fan_pid;

/* Tachometer state (accessed by TIM3_IRQHandler in ch32v30x_it.c) */
volatile uint32_t tacho_last_capture  = 0;
volatile uint32_t tacho_period_ticks  = 0;
volatile uint8_t  tacho_valid         = 0;
volatile uint16_t tacho_timeout       = 0;

/* --------------------------------------------------------------------------
 * Module Static Variables
 * -------------------------------------------------------------------------- */
static uint16_t fan_current_duty_ccr = 0;
static uint8_t  fan_duty_pct         = 0;
static uint8_t  stall_cycle_counter  = 0;

/*********************************************************************
 * @fn      fan_get_rpm
 *
 * @brief   Compute RPM from the latest tachometer capture period.
 *          At 1MHz TIM3 counter: RPM = 30,000,000 / period_ticks.
 *          Returns 0 if no valid measurement is available.
 *
 * @return  Fan speed in RPM
 */
uint16_t fan_get_rpm(void)
{
    if (tacho_valid && tacho_period_ticks > 0)
    {
        return (uint16_t)(RPM_SCALE / tacho_period_ticks);
    }
    return 0;
}

/*********************************************************************
 * @fn      fan_get_duty_pct
 *
 * @brief   Return current PWM duty cycle percentage.
 *
 * @return  Duty cycle 0-100 percent
 */
uint8_t fan_get_duty_pct(void)
{
    return fan_duty_pct;
}

/*********************************************************************
 * @fn      fan_is_stalled
 *
 * @brief   Return current stall detection status.
 *
 * @return  1 if stalled, 0 otherwise
 */
uint8_t fan_is_stalled(void)
{
    return fan_stall;
}

/*********************************************************************
 * @fn      fan_update
 *
 * @brief   Run one iteration of fan control (called each 100ms cycle).
 *
 *          Sequence:
 *          A. Compute PID output: pid_compute(&fan_pid, 50.0f, temp, 0.1f)
 *          B. Map PID output to PWM duty: OFF if temp<50°C, 30-100% if ≥50°C
 *          C. Read RPM from tachometer capture, apply timeout
 *          D. Stall detection: RPM=0 + duty≥25% for 20 cycles → stall=1
 *
 * @param   current_temp_c  Heatsink temperature in °C
 * @return  none
 */
void fan_update(float current_temp_c)
{
    float pid_out;
    uint32_t period;

    /* ---- Step A: Compute PID output ---- */
    pid_out = pid_compute(&fan_pid, FAN_TARGET_TEMP_C, current_temp_c, 0.1f);

    /* ---- Step B: Map PID output to PWM duty ---- */
    if (current_temp_c < FAN_TARGET_TEMP_C)
    {
        /* Below setpoint: fan OFF */
        fan_duty_pct = 0;
        fan_current_duty_ccr = 0;
        TIM_SetCompare1(TIM3, 0);
    }
    else
    {
        /* At or above setpoint: map PID output [0..FAN_PID_MAX] to [30..100]% */
        uint32_t tmp;

        fan_duty_pct = FAN_DUTY_MIN_PCT +
            (uint8_t)((pid_out / FAN_PID_MAX) * (float)(FAN_DUTY_MAX_PCT - FAN_DUTY_MIN_PCT));

        if (fan_duty_pct > FAN_DUTY_MAX_PCT)
        {
            fan_duty_pct = FAN_DUTY_MAX_PCT;
        }

        /* Convert percent to CCR value: CCR = duty_pct * (ARR+1) / 100 - 1
         * At ARR=39: CCR = duty_pct * 40 / 100 - 1. Simplified: CCR = duty_pct * 39 / 100 */
        tmp = (uint32_t)fan_duty_pct * (uint32_t)(TIM3_ARR);
        fan_current_duty_ccr = (uint16_t)(tmp / 100UL);
        TIM_SetCompare1(TIM3, fan_current_duty_ccr);
    }

    /* ---- Step C: Compute RPM from latest tachometer capture ---- */
    period = tacho_period_ticks;
    if (tacho_valid && period > 0)
    {
        fan_rpm = (uint16_t)(RPM_SCALE / period);
    }
    else
    {
        fan_rpm = 0;
    }

    /* Tachometer timeout: if no edge for 500ms, invalidate measurement */
    tacho_timeout++;
    if (tacho_timeout >= TACHO_TIMEOUT_MAX)
    {
        tacho_valid = 0;
        fan_rpm = 0;
    }

    /* ---- Step D: Stall detection (RPM=0 + duty≥25% for 2s) ---- */
    if (fan_duty_pct >= FAN_STALL_DUTY_THRESHOLD_PCT && fan_rpm == 0)
    {
        stall_cycle_counter++;
        if (stall_cycle_counter >= FAN_STALL_CYCLES)
        {
            if (fan_stall == 0)
            {
                printf("[FAN] STALL DETECTED — duty=%d%% RPM=0\r\n", fan_duty_pct);
            }
            fan_stall = 1;
        }
    }
    else
    {
        if (fan_stall == 1 && stall_cycle_counter > 0)
        {
            printf("[FAN] Stall cleared — RPM=%d\r\n", fan_rpm);
        }
        stall_cycle_counter = 0;
        fan_stall = 0;
    }
}

/*********************************************************************
 * @fn      fan_init
 *
 * @brief   Initialize fan controller: TIM3 PWM + input capture + PID.
 *
 *          Hardware:
 *          - PA6: TIM3 CH1 alternate function push-pull (25kHz PWM)
 *          - PA7: TIM3 CH2 input pull-up (tachometer open-drain)
 *          - TIM3 clock: APB1 PCLK1 × 2 = 144MHz (APB1 prescaler /2)
 *          - PSC=143 → 1MHz counter, ARR=39 → 25kHz PWM
 *          - CH2 input capture: rising edge, direct TI2, no prescaler
 *          - NVIC: TIM3_IRQn at priority 0x03
 *
 *          Software:
 *          - PID: PI-only (Kp=0.20, Ki=0.02, Kd=0) with anti-windup
 *          - All tachometer and stall state zeroed
 *
 * @return  none
 */
void fan_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    TIM_OCInitTypeDef TIM_OCInitStructure = {0};
    TIM_ICInitTypeDef TIM_ICInitStructure = {0};

    /* ---- 1. Enable peripheral clocks ---- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* ---- 2. Configure PA6 as AF push-pull for TIM3 CH1 PWM output ---- */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* ---- 3. Configure PA7 as input with weak pull-up for tachometer ---- */
    /* PC fan tachometer is open-drain; internal pull-up ensures clean edges. */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* ---- 4. Configure TIM3 time base ---- */
    /* APB1 PCLK1 = 72MHz (96MHz HSE, APB1 prescaler /2).
     * TIM3 clock = PCLK1 × 2 = 144MHz (CH32V303 timer clock doubler).
     * PSC=143 → 144MHz / 144 = 1MHz counter.
     * ARR=39 → 1MHz / 40 = 25kHz PWM (Intel 4-wire fan spec). */
    TIM_TimeBaseStructure.TIM_Period = TIM3_ARR;
    TIM_TimeBaseStructure.TIM_Prescaler = TIM3_PSC;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    /* ---- 5. Configure TIM3 CH1 as PWM mode 1 output ---- */
    /* Initial duty = 0% (fan OFF). PWM mode 1: output HIGH when CNT < CCR. */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);

    /* ---- 6. Configure TIM3 CH2 as input capture (rising edge) ---- */
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStructure.TIM_ICFilter = 0x0;
    TIM_ICInit(TIM3, &TIM_ICInitStructure);

    /* ---- 7. Enable TIM3 CH2 capture interrupt ---- */
    TIM_ITConfig(TIM3, TIM_IT_CC2, ENABLE);

    /* ---- 8. Configure NVIC for TIM3 ---- */
    /* Priority 0x03: same as DMA1_CH5, lower than EXTI4 (0x01) and USART2 (0x02). */
    NVIC_SetPriority(TIM3_IRQn, 0x03);
    NVIC_EnableIRQ(TIM3_IRQn);

    /* ---- 9. Enable TIM3 counter (PWM runs immediately at 0% duty = output LOW) ---- */
    TIM_Cmd(TIM3, ENABLE);

    /* ---- 10. Initialize fan PID (PI-only) ---- */
    pid_init(&fan_pid, FAN_KP, FAN_KI, FAN_KD);
    pid_compute(&fan_pid, FAN_TARGET_TEMP_C, 25.0f, 0.1f);  /* Seed with room temp */

    /* ---- 11. Zero tachometer and stall state ---- */
    tacho_last_capture  = 0;
    tacho_period_ticks  = 0;
    tacho_valid         = 0;
    tacho_timeout       = 0;
    fan_rpm             = 0;
    fan_duty_pct        = 0;
    fan_current_duty_ccr = 0;
    stall_cycle_counter = 0;
    fan_stall           = 0;

    printf("Fan init: TIM3 PA6 PWM 25kHz + PA7 tacho IC, target=%.0f°C, Kp=%.2f Ki=%.3f\r\n",
           FAN_TARGET_TEMP_C, FAN_KP, FAN_KI);
}
