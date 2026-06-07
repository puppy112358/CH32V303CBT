/********************************** (C) COPYRIGHT  *******************************
* File Name          : fault.c
* Author             : GSD Phase 02
* Version            : V1.0.0
* Date               : 2026/06/07
* Description        : Fault state machine implementation.
*                      Unified handler for EXTI overcurrent and software OPP.
*                      Auto-retry with 3s cooldown, 5-retry permanent latch,
*                      and 30s fault-free retry counter reset.
*******************************************************************************/
#include "../Drivers/fault.h"
#include "debug.h"

/* --------------------------------------------------------------------------
 * Static Module State
 * -------------------------------------------------------------------------- */
static uint8_t  cooldown_counter;
static uint32_t fault_free_ms;

/* Fault register — declared extern in fault.h for main.c access */
FaultRegister fault_reg;

/* --------------------------------------------------------------------------
 * External References
 * -------------------------------------------------------------------------- */
extern INA226_Dev   devs[5];
extern SystemMode   system_mode;
extern volatile uint8_t  fault_triggered;
extern volatile uint16_t fault_source_mask;

/*********************************************************************
 * @fn      fault_init
 *
 * @brief   初始化故障处理器: 清零故障寄存器，设置 auto_recovery=1，
 *          清零冷却计数器和无故障计时器。
 *
 * @return  none
 */
void fault_init(void)
{
    fault_reg.raw = 0;
    fault_reg.bits.auto_recovery = 1;
    cooldown_counter = 0;
    fault_free_ms = 0;
}

/*********************************************************************
 * @fn      fault_handler_hw
 *
 * @brief   硬件过流故障处理 — 由主循环在检测到 fault_triggered 时调用。
 *
 *          记录 overcurrent_mask、递增 retry_count、设置 latch 标志。
 *          在永久锁存时禁用 auto_recovery。
 *
 * @param   alert_mask - 来自 ISR 的 per-channel 报警掩码
 *
 * @return  none
 */
void fault_handler_hw(uint16_t alert_mask)
{
    /* Record per-channel overcurrent mask (bits 0-3 only) */
    fault_reg.bits.overcurrent_mask = (uint8_t)(alert_mask & 0x0F);

    /* Set fault type: OC only, or BOTH if OPP was already set */
    if (fault_reg.bits.fault_type == FAULT_TYPE_OPP)
    {
        fault_reg.bits.fault_type = FAULT_TYPE_BOTH;
    }
    else
    {
        fault_reg.bits.fault_type = FAULT_TYPE_OC;
    }

    /* Increment retry count, clamp at MAX_RETRY_COUNT */
    if (fault_reg.bits.retry_count < MAX_RETRY_COUNT)
    {
        fault_reg.bits.retry_count++;
    }

    /* Permanent latch on 5th consecutive fault */
    if (fault_reg.bits.retry_count >= MAX_RETRY_COUNT)
    {
        fault_reg.bits.latched = 1;
        fault_reg.bits.auto_recovery = 0;
    }

    /* Clear ISR flag for next detection cycle */
    fault_triggered = 0;
}

/*********************************************************************
 * @fn      fault_handler_opp
 *
 * @brief   软件过功率故障处理 — 在主循环中检测到 summary power
 *          超过 RATED_WATTAGE 时调用。
 *
 *          与硬件故障共享相同的 retry/latch 逻辑。
 *
 * @return  none
 */
void fault_handler_opp(void)
{
    /* Set fault type: OPP only, or BOTH if OC was already set */
    if (fault_reg.bits.fault_type == FAULT_TYPE_OC)
    {
        fault_reg.bits.fault_type = FAULT_TYPE_BOTH;
    }
    else
    {
        fault_reg.bits.fault_type = FAULT_TYPE_OPP;
    }

    /* Increment retry count, clamp at MAX_RETRY_COUNT */
    if (fault_reg.bits.retry_count < MAX_RETRY_COUNT)
    {
        fault_reg.bits.retry_count++;
    }

    /* Permanent latch on 5th consecutive fault */
    if (fault_reg.bits.retry_count >= MAX_RETRY_COUNT)
    {
        fault_reg.bits.latched = 1;
        fault_reg.bits.auto_recovery = 0;
    }

    fault_triggered = 0;
}

/*********************************************************************
 * @fn      fault_state_machine
 *
 * @brief   故障状态机 — 每个控制周期在 MODE_FAULT 状态下调用。
 *
 *          管理冷却计时器、自动重试、永久锁存:
 *          - 若 latched: 永久等待显式清除（通过 fault_clear）
 *          - 若 auto_recovery: 递增冷却计数器，30 个周期后
 *            跳转 MODE_FAULT → MODE_IDLE
 *
 * @return  none
 */
void fault_state_machine(void)
{
    /* Only acts when system is in FAULT mode */
    if (system_mode != MODE_FAULT)
    {
        return;
    }

    /* Permanent latch: wait for explicit fault_clear() */
    if (fault_reg.bits.latched)
    {
        return;
    }

    /* Auto-retry: count cooldown cycles */
    if (fault_reg.bits.auto_recovery)
    {
        cooldown_counter++;

        if (cooldown_counter >= COOLDOWN_CYCLES)
        {
            system_mode = MODE_IDLE;
            cooldown_counter = 0;
            printf("[FAULT] Auto-retry %d — transitioning FAULT->IDLE\r\n",
                   fault_reg.bits.retry_count);
        }
    }
}

/*********************************************************************
 * @fn      fault_clear
 *
 * @brief   显式清除故障: 复位故障寄存器，跳转 FAULT→IDLE。
 *
 *          TODO: Phase 3 — 通过 cJSON 命令调用此函数。
 *
 * @return  none
 */
void fault_clear(void)
{
    /* Reset fault register, preserving auto_recovery default */
    fault_reg.raw = 0;
    fault_reg.bits.auto_recovery = 1;

    /* Transition to IDLE */
    if (system_mode == MODE_FAULT)
    {
        system_mode = MODE_IDLE;
    }

    /* Reset cooldown state */
    cooldown_counter = 0;

    printf("[FAULT] Cleared\r\n");
}

/*********************************************************************
 * @fn      fault_print_snapshot
 *
 * @brief   打印完整诊断快照 (D-02):
 *          所有 4 路 MOS 通道 + 汇总 INA226 数据，
 *          活动模式、DAC 值、故障类型、重试次数。
 *
 * @param   fr        - 指向当前 FaultRegister 的指针
 * @param   dac_value - 故障时的 DAC 值
 * @param   mode      - 故障前的工作模式
 *
 * @return  none
 */
void fault_print_snapshot(FaultRegister *fr, uint16_t dac_value, SystemMode mode)
{
    uint8_t ch;
    float bus_v;
    float shunt_mv;
    float current_a;
    float power_w;
    const char *fault_type_str;

    /* Resolve fault type string */
    switch (fr->bits.fault_type)
    {
        case FAULT_TYPE_OC:   fault_type_str = "OC";   break;
        case FAULT_TYPE_OPP:  fault_type_str = "OPP";  break;
        case FAULT_TYPE_BOTH: fault_type_str = "BOTH";  break;
        default:              fault_type_str = "?";     break;
    }

    printf("[FAULT SNAPSHOT] type=%s mask=0x%X retry=%d mode=%d DAC=%u\r\n",
           fault_type_str, fr->bits.overcurrent_mask,
           fr->bits.retry_count, mode, dac_value);

    /* Per-channel diagnostics for all 4 MOS channels */
    for (ch = 0; ch < 4; ch++)
    {
        bus_v     = 0.0f;
        shunt_mv  = 0.0f;
        current_a = 0.0f;
        power_w   = 0.0f;

        ina226_get_bus_voltage(&devs[ch], &bus_v);
        ina226_get_shunt_voltage(&devs[ch], &shunt_mv);
        ina226_get_current(&devs[ch], &current_a);
        ina226_get_power(&devs[ch], &power_w);

        printf("  CH%d(0x%02X): Bus=%.3fV Shunt=%.3fmV Cur=%.3fA Pwr=%.3fW\r\n",
               devs[ch].channel, devs[ch].address,
               bus_v, shunt_mv, current_a, power_w);
    }

    /* Summary channel */
    bus_v     = 0.0f;
    current_a = 0.0f;
    power_w   = 0.0f;
    ina226_get_bus_voltage(&devs[4], &bus_v);
    ina226_get_current(&devs[4], &current_a);
    ina226_get_power(&devs[4], &power_w);

    printf("  Summary(0x%02X): Bus=%.3fV Cur=%.3fA Pwr=%.3fW\r\n",
           devs[4].address, bus_v, current_a, power_w);
}
