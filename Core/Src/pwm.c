/**
 * @file    pwm.c
 * @brief   PWM 驱动模块 - 互补输出 + 死区插入
 * @note    基于 STM32 HAL 库,硬件平台: STM32F103
 *          使用时需根据实际 MCU 系列调整定时器时钟和死区计算方法
 */

#include "pwm.h"

/* 私有变量 -------------------------------------------------------------- */
static float current_duty = 0.0f;        /* 当前占空比(0.0 ~ 1.0)           */
static TIM_HandleTypeDef htim_pwm;       /* PWM 定时器句柄                  */

/**
 * @brief  初始化 PWM 定时器
 * @note   配置为 中心对齐模式(Center-aligned),适用于同步整流
 *         通道1: CH1 -> 上管驱动 (高有效)
 *         通道1N: CH1N -> 下管驱动 (低有效,互补输出)
 *         
 *         如果使用边沿对齐模式(Edge-aligned),修改 TIM_COUNTERMODE_CENTERALIGNED1
 */
void PWM_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};
    
    /* ---- 1. 定时器时基配置 ---- */
    htim_pwm.Instance = TIM1;                         /* 高级定时器才支持死区 */
    htim_pwm.Init.Prescaler = 0;                      /* 不分频,72MHz         */
    htim_pwm.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED1;  /* 中心对齐模式 */
    htim_pwm.Init.Period = PWM_PERIOD - 1;            /* 自动重装载值          */
    htim_pwm.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;  /* tDTS = tCK_INT   */
    htim_pwm.Init.RepetitionCounter = 0;              /* 重复计数器            */
    htim_pwm.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim_pwm);
    
    /* ---- 2. 互补输出通道配置 ---- */
    /* CH1 (上管) */
    sConfigOC.OCMode = TIM_OCMODE_PWM1;               /* PWM1: CNT<CCR时输出有效电平 */
    sConfigOC.Pulse = 0;                              /* 初始占空比 0           */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;        /* 高电平有效             */
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;      /* 互补通道高有效          */
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;     /* 空闲时输出低电平        */
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    HAL_TIM_PWM_ConfigChannel(&htim_pwm, &sConfigOC, TIM_CHANNEL_1);
    
    /* ---- 3. 死区时间配置 ---- */
    /* STM32F1 死区寄存器 DTG 格式:
     * DT = DTG[7:0] * t_dts   (当 DTG[7:5] == 000)
     * 假设 t_dts = 1/72MHz ≈ 13.89ns
     * DT = 10 * 13.89ns ≈ 139ns
     */
    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_ENABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_ENABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = PWM_DEADTIME_VALUE;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_ENABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.BreakFilter = 0;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_ENABLE;
    HAL_TIMEx_ConfigBreakDeadTime(&htim_pwm, &sBreakDeadTimeConfig);
    
    /* ---- 4. 启动 PWM 生成,但先不输出 ---- */
    HAL_TIM_PWM_Start(&htim_pwm, TIM_CHANNEL_1);       /* 启动 CH1               */
    HAL_TIMEx_PWMN_Start(&htim_pwm, TIM_CHANNEL_1);    /* 启动 CH1N(互补)        */
    PWM_Disable();                                      /* 先关断输出             */
}

/**
 * @brief  设置占空比
 * @param  duty 占空比值,范围 0.0 ~ 1.0
 * @note   自动钳位到安全范围内,防止直通
 *         中心对齐模式下: CCR = Period * duty (因为 Period = 2 * ARR)
 */
void PWM_SetDuty(float duty)
{
    uint32_t period;
    uint32_t ccr_value;
    
    /* 限幅保护 */
    if (duty < DUTY_MIN) duty = DUTY_MIN;
    if (duty > DUTY_MAX) duty = DUTY_MAX;
    
    current_duty = duty;
    
    /* 计算 CCR 值 */
    period = __HAL_TIM_GET_AUTORELOAD(&htim_pwm) + 1;
    ccr_value = (uint32_t)(period * duty);
    
    __HAL_TIM_SET_COMPARE(&htim_pwm, TIM_CHANNEL_1, ccr_value);
}

/**
 * @brief  使能 PWM 输出(解除闭锁)
 */
void PWM_Enable(void)
{
    HAL_TIMEx_PWMN_Start(&htim_pwm, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim_pwm, TIM_CHANNEL_1);
    __HAL_TIM_MOE_ENABLE(&htim_pwm);      /* 主输出使能(Main Output Enable) */
}

/**
 * @brief  关闭 PWM 输出
 * @note   立即关断,将输出置于安全状态
 */
void PWM_Disable(void)
{
    __HAL_TIM_MOE_DISABLE(&htim_pwm);     /* 主输出禁止,输出进入空闲态      */
}

/**
 * @brief  获取当前占空比
 * @return float 当前占空比(0.0 ~ 1.0)
 */
float PWM_GetDuty(void)
{
    return current_duty;
}
