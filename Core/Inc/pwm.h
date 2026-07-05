#ifndef __PWM_H
#define __PWM_H

#include "main.h"

/* PWM 频率和死区时间配置 ------------------------------------------------ */
#define PWM_FREQUENCY       100000UL    /* 开关频率 100kHz                 */
#define PWM_PERIOD           (72000000UL / PWM_FREQUENCY)  /* 72MHz 定时器时钟 */
#define PWM_DEADTIME_NS      100         /* 死区时间 100ns                 */

/* 死区时间对应的定时器寄存器值(不同系列计算方法不同,此处以 STM32F1 为例) */
/* STM32F1: DT = (DTG[7:5]预分频) * (DTG[4:0] + 1) * t_dts              */
/* 简化:直接赋值 DTG 寄存器,具体值需根据数据手册计算                      */
#define PWM_DEADTIME_VALUE   10          /* 需根据实际时钟调整 */

/* 占空比限制 ------------------------------------------------------------ */
#define DUTY_MIN             0.0f        /* 最小占空比                      */
#define DUTY_MAX             0.95f       /* 最大占空比(留余量防止直通)     */

/* 公开接口 -------------------------------------------------------------- */
void     PWM_Init(void);                 /* 初始化 PWM 和死区              */
void     PWM_SetDuty(float duty);        /* 设置占空比 0.0 ~ 1.0           */
void     PWM_Enable(void);               /* 使能 PWM 输出                  */
void     PWM_Disable(void);              /* 关闭 PWM 输出                  */
float    PWM_GetDuty(void);              /* 获取当前占空比                  */

#endif /* __PWM_H */
