/**
 * @file    protection.c
 * @brief   保护逻辑 + 软启动模块
 * @note    过压/过流/欠压保护,锁死功能,上电软启动
 *          "炸管"多半是因为保护没写好,这是电源最关键的防线
 */

#include "protection.h"

/* 私有变量 -------------------------------------------------------------- */
static uint8_t  protection_status = PROTECT_NONE;  /* 保护状态位掩码      */
static System_State_t system_state = STATE_INIT;   /* 系统运行状态        */

/* 软启动状态 ------------------------------------------------------------ */
static uint16_t soft_start_counter = 0;            /* 软启动计数器        */
static float    soft_start_duty = 0.0f;            /* 软启动当前占空比     */

/**
 * @brief  初始化保护模块
 */
void Protection_Init(void)
{
    protection_status = PROTECT_NONE;
    system_state = STATE_INIT;
}

/**
 * @brief  周期性检查保护条件
 * @param  vout  输出电压(V)
 * @param  iout  输出电流(A)
 * @param  vin   输入电压(V)
 * @param  temp  温度(摄氏度)
 * 
 * @note   应在主循环或控制中断中定时调用(每 1ms 至少一次)
 *         一旦触发保护,会立即关断 PWM
 *         过压/过流触发后进入锁死状态(需手动复位或重新上电)
 */
void Protection_Check(float vout, float iout, float vin, float temp)
{
    /* ---- 1. 过压保护 OVP (最高优先级) ---- */
    /* 输出过压通常意味着反馈环路失控,必须立即关断 */
    if (vout > OVP_THRESHOLD) {
        protection_status |= PROTECT_OVP;
        Protection_Latch();
        return;
    }
    
    /* ---- 2. 过流保护 OCP ---- */
    /* 短路或负载过重,持续超过阈值则关断 */
    if (iout > OCP_THRESHOLD) {
        protection_status |= PROTECT_OCP;
        Protection_Latch();
        return;
    }
    
    /* ---- 3. 欠压保护 UVP ---- */
    /* 输入电压过低时,占空比会过大,可能导致电感饱和 */
    if (vin < UVP_THRESHOLD) {
        protection_status |= PROTECT_UVP;
        Protection_Latch();
        return;
    }
    
    /* ---- 4. 过温保护 OTP(可选,需要 NTC 温度传感器) ---- */
    if (temp > OTP_THRESHOLD) {
        protection_status |= PROTECT_OTP;
        Protection_Latch();
        return;
    }
    
    /* 全部正常 */
    protection_status = PROTECT_NONE;
}

/**
 * @brief  获取当前保护状态
 * @return uint8_t 保护状态位掩码
 *         PROTECT_NONE = 正常
 *         PROTECT_OVP  = 过压
 *         PROTECT_OCP  = 过流
 */
uint8_t Protection_GetStatus(void)
{
    return protection_status;
}

/**
 * @brief  清除保护状态
 * @note   用于外部按钮复位或串口指令复位
 *         锁死状态需要先确认故障已排除
 */
void Protection_Clear(void)
{
    protection_status = PROTECT_NONE;
    system_state = STATE_INIT;
}

/**
 * @brief  锁死保护
 * @note   调用后立即关断 PWM,且只有重新上电或手动复位才能恢复
 *         这是为了防止反复重启导致的二次损坏
 *         
 *         电赛教训: 很多人锁死后没做手动复位机制,
 *         需要拔电源才能复位,非常被动
 */
void Protection_Latch(void)
{
    protection_status |= PROTECT_LATCH;
    system_state = STATE_FAULT;
    PWM_Disable();                       /* 立即关断 PWM                  */
    
    /* 可以在这里点亮故障指示灯 */
}

/* ====================================================================== */
/*                       软启动模块实现                                     */
/* ====================================================================== */

/**
 * @brief  初始化软启动
 * @note   软启动从占空比 0 开始,在指定时间内逐步增加到目标值
 *         目的是限制启动冲击电流,防止输出电容充电瞬间触发过流保护
 */
void SoftStart_Init(void)
{
    soft_start_counter = 0;
    soft_start_duty = 0.0f;
    system_state = STATE_SOFT_START;
    PWM_Enable();                        /* 使能 PWM 输出                 */
    PWM_SetDuty(0.0f);                   /* 从 0 开始                     */
}

/**
 * @brief  获取软启动当前占空比
 * @return float 当前允许的占空比(0.0 ~ 目标值)
 * 
 * @note   主循环中调用:
 *         float duty_ref = SoftStart_GetDuty();
 *         float duty_out = PID_Update(&pid, Vref * duty_ref, Vout);
 *         PWM_SetDuty(duty_out);
 */
float SoftStart_GetDuty(void)
{
    if (system_state != STATE_SOFT_START) {
        return 1.0f;                     /* 软启动已完成,返回满值         */
    }
    
    soft_start_counter++;
    
    /* 线性爬升 */
    /* e.g. SOFT_START_TIME_MS = 50ms, 控制频率 10kHz -> 500 步         */
    soft_start_duty = (float)soft_start_counter / SOFT_START_STEPS;
    
    if (soft_start_duty >= 1.0f) {
        soft_start_duty = 1.0f;
        system_state = STATE_RUNNING;    /* 进入正常运行状态              */
    }
    
    return soft_start_duty;
}

/**
 * @brief  检查软启动是否完成
 * @return uint8_t 1:完成, 0:未完成
 */
uint8_t SoftStart_IsComplete(void)
{
    return (system_state >= STATE_RUNNING) ? 1 : 0;
}
