#ifndef __PID_H
#define __PID_H

#include "main.h"

/* PI 控制器结构体 ------------------------------------------------------- */
/* 电源控制通常只需要 PI(比例-积分),微分项容易引入噪声                      */
typedef struct {
    /* 用户配置参数 ------------------------------------------------------ */
    float Kp;                            /* 比例系数                        */
    float Ki;                            /* 积分系数                        */
    float Kd;                            /* 微分系数(通常置0)               */
    
    float out_min;                       /* 输出下限(通常 0.0)              */
    float out_max;                       /* 输出上限(通常 0.95)             */
    float integral_limit;                /* 积分分离阈值(抗饱和)            */
    
    /* 内部状态 ---------------------------------------------------------- */
    float integral;                      /* 积分累加值                      */
    float prev_error;                    /* 上一次误差(用于微分)            */
    float output;                        /* 当前输出值                      */
    
    /* 调试记录 ---------------------------------------------------------- */
    float setpoint;                      /* 目标值(参考电压)                */
    float feedback;                      /* 反馈值(实际电压)                */
    float error;                         /* 当前误差                        */
} PID_t;

/* 公开接口 -------------------------------------------------------------- */
void  PID_Init(PID_t *pid, float Kp, float Ki, float Kd,
               float out_min, float out_max);   /* 初始化PI参数             */
void  PID_Reset(PID_t *pid);                    /* 重置积分器                */
float PID_Update(PID_t *pid, float setpoint, 
                 float feedback);                /* 核心:一次PI运算           */
void  PID_Tune(PID_t *pid, float Kp, float Ki); /* 运行时调整参数            */
float PID_GetOutput(PID_t *pid);                 /* 获取当前输出              */

/* 预置参数模板(需根据实际电路整定) -------------------------------------- */
/* 使用示例: PID_t pid = PID_BUCK_12V_5A;                                  */
extern const PID_t PID_BUCK_12V_5A;      /* 12V/5A Buck 预置参数           */
extern const PID_t PID_BOOST_24V_3A;     /* 24V/3A Boost 预置参数          */

#endif /* __PID_H */
