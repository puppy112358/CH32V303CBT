/********************************** (C) COPYRIGHT  *******************************
* File Name          : pid.c
* Author             : GSD Phase 02
* Version            : V1.0.0
* Date               : 2026/06/07
* Description        : PID controller implementation.
*                      Positional (parallel) form with anti-windup conditional
*                      integration freeze and derivative-on-measurement.
*******************************************************************************/
#include "../Drivers/pid.h"

/*********************************************************************
 * @fn      pid_init
 *
 * @brief   初始化 PID 实例，设置系数并清零所有状态。
 *
 * @param   pid - 指向 PID_Instance 结构体的指针
 * @param   kp  - 比例增益
 * @param   ki  - 积分增益
 * @param   kd  - 微分增益
 *
 * @return  none
 */
void pid_init(PID_Instance *pid, float kp, float ki, float kd)
{
    pid->Kp        = kp;
    pid->Ki        = ki;
    pid->Kd        = kd;
    pid->setpoint  = 0.0f;
    pid->integral  = 0.0f;
    pid->last_feedback = 0.0f;
    pid->output    = 0.0f;
    pid->saturated = 0;
}

/*********************************************************************
 * @fn      pid_compute
 *
 * @brief   执行一次 PID 迭代计算。
 *
 *          采用位置式（并联）形式:
 *            output = Kp*error + integral + Kd*d(error)/dt
 *
 *          抗积分饱和: 当输出饱和时条件性冻结积分累加——
 *            若 output == 65535 且 error > 0: 跳过正积分（防止继续增大）
 *            若 output == 0     且 error < 0: 跳过负积分（防止继续减小）
 *
 *          微分先行: D_term = -Kd * (feedback - last_feedback) / dt
 *          避免设定值阶跃引起的微分冲击。
 *
 * @param   pid      - 指向 PID_Instance 结构体的指针
 * @param   setpoint - 目标值（电压或电流）
 * @param   feedback - 当前测量值
 * @param   dt       - 时间步长（秒），例如 0.1f 对应 100ms
 *
 * @return  钳位后的 PID 输出值 [0, 65535]
 */
float pid_compute(PID_Instance *pid, float setpoint, float feedback, float dt)
{
    float error;
    float p_term;
    float d_term;
    float output;

    /* Compute error */
    error = setpoint - feedback;

    /* Proportional term */
    p_term = pid->Kp * error;

    /* Conditional integration (anti-windup):
     *   sat==1 (high): skip I accumulation when error > 0 (would push further up)
     *   sat==2 (low):  skip I accumulation when error < 0 (would push further down)
     *   sat==0 (normal): accumulate normally */
    if (!((pid->saturated == 1 && error > 0.0f) ||
          (pid->saturated == 2 && error < 0.0f)))
    {
        /* Accumulate integral: Ki * error * dt */
        pid->integral += pid->Ki * error * dt;

        /* Clamp integral to configured limits */
        if (pid->integral > PID_INTEGRAL_MAX)
        {
            pid->integral = PID_INTEGRAL_MAX;
        }
        else if (pid->integral < PID_INTEGRAL_MIN)
        {
            pid->integral = PID_INTEGRAL_MIN;
        }
    }

    /* Derivative-on-measurement: avoids setpoint-step kick */
    d_term = -pid->Kd * (feedback - pid->last_feedback) / dt;

    /* Compute raw output */
    output = p_term + pid->integral + d_term;

    /* Clamp output and set saturation flag */
    if (output > PID_OUTPUT_MAX)
    {
        output = PID_OUTPUT_MAX;
        pid->saturated = 1;
    }
    else if (output < PID_OUTPUT_MIN)
    {
        output = PID_OUTPUT_MIN;
        pid->saturated = 2;
    }
    else
    {
        pid->saturated = 0;
    }

    /* Update state for next iteration */
    pid->last_feedback = feedback;
    pid->output = output;
    pid->setpoint = setpoint;

    return pid->output;
}

/*********************************************************************
 * @fn      pid_reset
 *
 * @brief   重置 PID 实例: 清零积分项、上次反馈值、输出值和饱和标志。
 *          用于模式切换或故障恢复后。
 *
 * @param   pid - 指向 PID_Instance 结构体的指针
 *
 * @return  none
 */
void pid_reset(PID_Instance *pid)
{
    pid->integral      = 0.0f;
    pid->last_feedback = 0.0f;
    pid->output        = 0.0f;
    pid->saturated     = 0;
}

/*********************************************************************
 * @fn      pid_set_integral
 *
 * @brief   预加载积分项以实现无扰切换。
 *          将值钳位到 [PID_INTEGRAL_MIN, PID_INTEGRAL_MAX]。
 *          在软启动完成后调用，使 PID 从软启动终点平稳接续。
 *
 * @param   pid   - 指向 PID_Instance 结构体的指针
 * @param   value - 目标积分值
 *
 * @return  none
 */
void pid_set_integral(PID_Instance *pid, float value)
{
    if (value > PID_INTEGRAL_MAX)
    {
        pid->integral = PID_INTEGRAL_MAX;
    }
    else if (value < PID_INTEGRAL_MIN)
    {
        pid->integral = PID_INTEGRAL_MIN;
    }
    else
    {
        pid->integral = value;
    }
}
