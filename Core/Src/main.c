/**
 * @file    main.c
 * @brief   数字电源主框架 - Buck 变换器控制示例
 * @note    这是整个工程的主循环,展示完整的控制流程:
 *          初始化 -> 软启动 -> PI 闭环控制 -> 保护监测
 *         
 *          *** 使用前请根据实际硬件修改引脚配置和参数 ***
 * 
 * @ref     STM32F103C8T6 "Blue Pill" + HAL 库
 *          控制频率: ~10kHz (由定时器中断触发)
 *          开关频率: 100kHz (由 PWM 模块配置)
 */

#include "main.h"
#include "pwm.h"
#include "adc.h"
#include "pid.h"
#include "protection.h"

/* ====================================================================== */
/*                        全局变量                                         */
/* ====================================================================== */

static PID_t pid;                        /* PI 控制器实例                   */
static float target_voltage = 12.0f;     /* 目标输出电压(V)                 */

/* ====================================================================== */
/*                        系统初始化                                       */
/* ====================================================================== */

/**
 * @brief  系统初始化
 * @note   初始化顺序: 时钟 -> PWM -> ADC -> PI -> 保护
 *         PWM 初始化后默认输出关闭,由软启动统一开启
 */
static void System_Init(void)
{
    HAL_Init();                          /* HAL 库初始化                   */
    SystemClock_Config();                /* 配置系统时钟 72MHz             */
    
    PWM_Init();                          /* 初始化 PWM + 死区             */
    ADC_Init();                          /* 初始化 ADC + DMA              */
    
    /* PI 参数初始化(使用 Buck 12V/5A 预置参数) */
    PID_Init(&pid, 
             PID_BUCK_12V_5A.Kp,
             PID_BUCK_12V_5A.Ki,
             PID_BUCK_12V_5A.Kd,
             PID_BUCK_12V_5A.out_min,
             PID_BUCK_12V_5A.out_max);
    
    Protection_Init();                   /* 初始化保护模块                 */
    
    ADC_Start();                         /* 启动 ADC 连续采样             */
}

/* ====================================================================== */
/*                        主控制循环(由定时器中断触发)                       */
/* ====================================================================== */

/**
 * @brief  控制中断服务函数
 * @note   由定时器中断每 100us 调用一次(即 10kHz 控制频率)
 *         
 *         控制时序:
 *         1. 读取 ADC 采样值
 *         2. 检查保护条件
 *         3. 软启动(如果未完成)
 *         4. PI 控制器计算
 *         5. 更新 PWM 占空比
 *         
 *         这就是电赛电源题最核心的控制环路代码
 */
void Control_Loop_ISR(void)
{
    float vout, iout, vin;
    float duty_ref, duty_out;
    
    /* ---- 1. 读取采样数据 ---- */
    ADC_Process();                        /* 处理 ADC 数据(含均值滤波)    */
    vout = ADC_GetVoltage();              /* 输出电压                      */
    iout = ADC_GetCurrent();              /* 输出电流                      */
    vin  = ADC_GetInputVoltage();         /* 输入电压(用于前馈,可选)      */
    
    /* ---- 2. 保护检查 ---- */
    /* 保护触发会立即关断 PWM 并锁死 */
    Protection_Check(vout, iout, vin, 25.0f);
    if (Protection_GetStatus() != PROTECT_NONE) {
        return;                           /* 保护已触发,停止控制          */
    }
    
    /* ---- 3. 软启动控制 ---- */
    /* 软启动通过限制参考电压的爬升速率来抑制启动冲击 */
    duty_ref = SoftStart_GetDuty();
    
    if (SoftStart_IsComplete()) {
        /* 正常运行: 闭环 PI 控制 */
        duty_out = PID_Update(&pid, target_voltage, vout);
    } else {
        /* 软启动阶段: 开环爬升占空比 */
        /* 也可以改为闭环 + 限幅参考: PID_Update(&pid, target_voltage * duty_ref, vout) */
        duty_out = duty_ref * 0.5f;       /* 开环简单爬升(50% 最大占空比) */
    }
    
    /* ---- 4. 更新 PWM 占空比 ---- */
    PWM_SetDuty(duty_out);
}

/* ====================================================================== */
/*                        main 主函数                                      */
/* ====================================================================== */

int main(void)
{
    /* ---- 1. 系统初始化 ---- */
    System_Init();
    
    /* ---- 2. 启动软启动 ---- */
    /* PWM 从关断状态 -> 0% 占空比 -> 逐步上升到目标值 */
    SoftStart_Init();
    
    /* ---- 3. 启动控制定时器 ---- */
    /* 配置 TIM3 产生 10kHz 中断,在中断中调用 Control_Loop_ISR() */
    ControlTimer_Start();                /* 具体实现见 platform 文件       */
    
    /* ---- 4. 主循环: 非实时任务 ---- */
    while (1)
    {
        /* 这里放不需要严格实时性的任务 */
        
        /* 更新显示(如 OLED/LCD 显示电压电流) */
        // Display_Update(vout, iout);
        
        /* 串口打印调试信息(调参时很有用) */
        // printf("Vout: %.2fV, Iout: %.2fA, Duty: %.3f\r\n",
        //        ADC_GetVoltage(), ADC_GetCurrent(), PWM_GetDuty());
        
        /* 检测按键(切换目标电压/设置) */
        // if (Button_Pressed()) {
        //     target_voltage = 5.0f;      /* 切换到 5V 输出               */
        //     PID_Reset(&pid);            /* 切换目标时重置积分器          */
        // }
        
        /* 延时(避免主循环空转消耗过多 CPU) */
        HAL_Delay(10);                    /* 10ms 循环周期                  */
    }
}

/* ====================================================================== */
/*                        硬件初始化辅助函数                                */
/* ====================================================================== */

/**
 * @brief  系统时钟配置 72MHz
 * @note   使用 HSE 8MHz 外部晶振 -> PLL 倍频 -> SYSCLK 72MHz
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    /* HSE 8MHz -> PLL -> 72MHz */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;  /* 8MHz * 9 = 72MHz */
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    /* SYSCLK = 72MHz, HCLK = 72MHz, APB1 = 36MHz, APB2 = 72MHz */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK 
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;   /* APB1: 36MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;   /* APB2: 72MHz */
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/**
 * @brief  控制定时器启动(10kHz)
 * @note   使用 TIM3 产生 10kHz 更新中断
 *         ARR = 72MHz / (Prescaler+1) / Freq = 72000000 / 72 / 10000 = 100
 *         即每 100us 产生一次中断
 */
void ControlTimer_Start(void)
{
    TIM_HandleTypeDef htim_control;
    
    htim_control.Instance = TIM3;
    htim_control.Init.Prescaler = 72 - 1;             /* 72MHz / 72 = 1MHz */
    htim_control.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim_control.Init.Period = 100 - 1;                /* 1MHz / 100 = 10kHz */
    htim_control.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim_control.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    
    HAL_TIM_Base_Init(&htim_control);
    HAL_TIM_Base_Start_IT(&htim_control);              /* 启用更新中断     */
}

/**
 * @brief  TIM3 更新中断回调
 * @note   HAL 库的中断回调函数,由 HAL_TIM_IRQHandler 自动调用
 *         这行代码将定时器中断连接到控制环路
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3) {
        Control_Loop_ISR();              /* 每 100us 执行一次控制         */
    }
}

/**
 * @brief  HAL 库 MSP 初始化(引脚配置)
 * @note   使用 CubeMX 生成此部分代码,或手动配置 GPIO
 *         这里仅作示意,实际使用时需根据原理图配置
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
    if (htim_base->Instance == TIM3) {
        /* 使能 TIM3 时钟 */
        __HAL_RCC_TIM3_CLK_ENABLE();
        
        /* 配置 TIM3 中断优先级(控制环路需要高优先级) */
        HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);         /* 最高优先级       */
        HAL_NVIC_EnableIRQ(TIM3_IRQn);
    }
}
