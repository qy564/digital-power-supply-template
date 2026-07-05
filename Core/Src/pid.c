/**
 * @file    pid.c
 * @brief   数字 PI 控制器模块 - 增量式算法 + 积分分离抗饱和
 * @note    开关电源控制环路的核心
 *         
 *         控制算法:
 *         u(k) = Kp * e(k) + Ki * T * sum(e(k))
 *         
 *         积分分离: |e(k)| > 阈值时冻结积分
 *         输出限幅: 防止占空比超出安全范围
 */

#include "pid.h"
#include <math.h>

/* 预置参数: 12V/5A Buck (需根据实际电路整定) ------------------------------ */
/* 这些参数在真实电路中需要通过 Ziegler-Nichols 或试凑法得到                    */
const PID_t PID_BUCK_12V_5A = {
    .Kp = 0.5f,
    .Ki = 0.01f,
    .Kd = 0.0f,
    .out_min = 0.0f,
    .out_max = 0.95f,
    .integral_limit = 0.5f,              /* 积分输出不超过 0.5               */
    .integral = 0.0f,
    .prev_error = 0.0f,
    .output = 0.0f,
    .setpoint = 12.0f,
    .feedback = 0.0f,
    .error = 0.0f
};

/* 预置参数: 24V/3A Boost ---------------------------------------------------- */
const PID_t PID_BOOST_24V_3A = {
    .Kp = 0.8f,
    .Ki = 0.02f,
    .Kd = 0.0f,
    .out_min = 0.1f,                     /* Boost 需要最小占空比维持输出      */
    .out_max = 0.90f,
    .integral_limit = 0.6f,
    .integral = 0.0f,
    .prev_error = 0.0f,
    .output = 0.0f,
    .setpoint = 24.0f,
    .feedback = 0.0f,
    .error = 0.0f
};

/**
 * @brief  初始化 PI 控制器
 * @param  pid     控制器结构体指针
 * @param  Kp      比例系数
 * @param  Ki      积分系数
 * @param  Kd      微分系数(电源控制通常设为 0)
 * @param  out_min 输出下限
 * @param  out_max 输出上限
 */
void PID_Init(PID_t *pid, float Kp, float Ki, float Kd,
              float out_min, float out_max)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->out_min = out_min;
    pid->out_max = out_max;
    pid->integral_limit = out_max * 0.6f;  /* 积分限幅默认 60% 最大输出 */
    
    PID_Reset(pid);
}

/**
 * @brief  重置积分器
 * @note   切换目标电压或重新启动时调用,防止积分突变
 */
void PID_Reset(PID_t *pid)
{
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->output = 0.0f;
}

/**
 * @brief  PI 控制器核心运算
 * @param  pid       控制器结构体指针
 * @param  setpoint  目标值(参考电压)
 * @param  feedback  反馈值(实际输出电压)
 * @return float     控制输出(即占空比)
 * 
 * @note   (1) 积分分离: 当误差过大(如启动瞬间)时冻结积分,防止超调
 *         (2) 积分限幅: 限制积分项大小,防止"积分饱和(windup)"
 *         (3) 输出限幅: 确保占空比在安全范围内
 *         
 *         控制频率建议: 10kHz ~ 50kHz(与开关频率匹配)
 */
float PID_Update(PID_t *pid, float setpoint, float feedback)
{
    /* 记录调试信息 */
    pid->setpoint = setpoint;
    pid->feedback = feedback;
    
    /* ---- 1. 计算误差 ---- */
    pid->error = setpoint - feedback;
    
    /* ---- 2. 比例项 ---- */
    float p_out = pid->Kp * pid->error;
    
    /* ---- 3. 积分项(带积分分离和限幅) ---- */
    float i_out = 0.0f;
    
    /* 积分分离: 误差较小时才累加积分 */
    if (fabsf(pid->error) < pid->integral_limit) {
        pid->integral += pid->Ki * pid->error;
        
        /* 积分限幅: 防止积分饱和 */
        if (pid->integral > pid->out_max) pid->integral = pid->out_max;
        if (pid->integral < pid->out_min) pid->integral = pid->out_min;
    }
    /* 误差过大时冻结积分(如启动瞬间或负载突变) */
    /* 如果积分值已经超限,也冻结 */
    i_out = pid->integral;
    
    /* ---- 4. 微分项(电源控制通常不用) ---- */
    float d_out = 0.0f;
    /* d_out = pid->Kd * (pid->error - pid->prev_error); */
    
    /* ---- 5. 合成输出 ---- */
    pid->output = p_out + i_out + d_out;
    
    /* ---- 6. 输出限幅 ---- */
    if (pid->output > pid->out_max) pid->output = pid->out_max;
    if (pid->output < pid->out_min) pid->output = pid->out_min;
    
    /* ---- 7. 保存误差用于下次微分计算 ---- */
    pid->prev_error = pid->error;
    
    return pid->output;
}

/**
 * @brief  运行时调整 PI 参数
 * @param  pid  控制器结构体指针
 * @param  Kp   新比例系数
 * @param  Ki   新积分系数
 * @note   用于上位机串口调参,或自适应控制
 */
void PID_Tune(PID_t *pid, float Kp, float Ki)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
}

/**
 * @brief  获取当前控制器输出
 * @return float 占空比指令
 */
float PID_GetOutput(PID_t *pid)
{
    return pid->output;
}
