#ifndef __ADC_H
#define __ADC_H

#include "main.h"

/* ADC 采样配置 ---------------------------------------------------------- */
#define ADC_SAMPLE_BUFFER_SIZE   8       /* 采样缓冲区大小(用于均值滤波)    */
#define ADC_VREF                  3.3f   /* 参考电压                        */
#define ADC_RESOLUTION          4096.0f  /* 12位 ADC: 2^12                 */

/* 电压电流通道映射(根据实际硬件修改) ------------------------------------ */
#define ADC_CH_VOUT              0       /* 输出电压采样通道                */
#define ADC_CH_IOUT              1       /* 输出电流采样通道                */
#define ADC_CH_VIN               2       /* 输入电压采样通道(可选)          */

/* 分压电阻比例(根据实际电路修改) ---------------------------------------- */
#define VOUT_DIVIDER_RATIO       0.1f    /* Vout分压比,如 10k/100k 分压    */
#define IOUT_AMPLIFIER_GAIN      1.0f    /* 电流采样运放增益                */
#define IOUT_SHUNT_RESISTOR      0.01f   /* 电流采样电阻(10mOhm)            */

/* 结构体:单通道采样结果 ------------------------------------------------- */
typedef struct {
    uint16_t raw;                        /* ADC原始值 0~4095                */
    float    voltage;                    /* 转换后的电压值(V)               */
    float    filtered;                   /* 滤波后的值                      */
} ADC_Channel_t;

/* 结构体:所有采样通道汇总 ----------------------------------------------- */
typedef struct {
    ADC_Channel_t vout;                  /* 输出电压                        */
    ADC_Channel_t iout;                  /* 输出电流                        */
    ADC_Channel_t vin;                   /* 输入电压                        */
    uint8_t       ready;                 /* 数据就绪标志                    */
} ADC_Data_t;

/* 公开接口 -------------------------------------------------------------- */
void     ADC_Init(void);                 /* 初始化 ADC + DMA               */
void     ADC_Start(void);                /* 开始连续采样                    */
void     ADC_Process(void);              /* 处理采样数据(滤波+换算)        */
float    ADC_GetVoltage(void);           /* 获取输出电压(V)                 */
float    ADC_GetCurrent(void);           /* 获取输出电流(A)                 */
float    ADC_GetInputVoltage(void);      /* 获取输入电压(V)                 */
uint16_t ADC_GetRaw(uint8_t channel);    /* 获取原始值(调试用)              */

#endif /* __ADC_H */
