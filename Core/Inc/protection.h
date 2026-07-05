#ifndef __PROTECTION_H
#define __PROTECTION_H

#include "main.h"

/* 保护阈值配置(根据实际需求修改) ---------------------------------------- */
#define OVP_THRESHOLD          13.0f     /* 过压保护阈值(V)                 */
#define OCP_THRESHOLD           5.5f     /* 过流保护阈值(A)                 */
#define UVP_THRESHOLD           8.0f     /* 欠压保护阈值(V)                 */
#define OTP_THRESHOLD          85.0f     /* 过温保护阈值(摄氏度)            */

#define SOFT_START_STEPS       100       /* 软启动步数                      */
#define SOFT_START_TIME_MS     50        /* 软启动持续时间(ms)              */

/* 保护状态枚举 ---------------------------------------------------------- */
typedef enum {
    PROTECT_NONE      = 0x00,            /* 正常                            */
    PROTECT_OVP       = 0x01,            /* 过压                            */
    PROTECT_OCP       = 0x02,            /* 过流                            */
    PROTECT_UVP       = 0x04,            /* 欠压                            */
    PROTECT_OTP       = 0x08,            /* 过温                            */
    PROTECT_LATCH     = 0x80             /* 锁死状态(需手动复位)            */
} Protection_Status_t;

/* 系统运行状态枚举 ------------------------------------------------------ */
typedef enum {
    STATE_INIT,                          /* 初始化中                        */
    STATE_SOFT_START,                    /* 软启动                          */
    STATE_RUNNING,                       /* 正常运行                        */
    STATE_FAULT,                         /* 故障(保护触发)                  */
    STATE_SHUTDOWN                       /* 安全停机                        */
} System_State_t;

/* 公开接口 -------------------------------------------------------------- */
void     Protection_Init(void);                           /* 初始化保护模块  */
void     Protection_Check(float vout, float iout, 
                          float vin, float temp);         /* 周期性检查保护条件 */
uint8_t  Protection_GetStatus(void);                      /* 获取保护状态      */
void     Protection_Clear(void);                          /* 清除保护状态      */
void     Protection_Latch(void);                          /* 锁死(需人工复位)   */

/* 软启动接口 ------------------------------------------------------------- */
void     SoftStart_Init(void);                            /* 初始化软启动      */
float    SoftStart_GetDuty(void);                         /* 获取当前软启动占空比 */
uint8_t  SoftStart_IsComplete(void);                      /* 软启动是否完成    */

#endif /* __PROTECTION_H */
