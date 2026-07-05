/**
 * @file    adc.c
 * @brief   ADC 采样模块 - DMA 连续采样 + 滑动均值滤波
 * @note    使用 ADC1 + DMA1 实现多通道连续采样,无需 CPU 干预
 *          采样结果通过滑动窗口均值滤波,有效抑制开关噪声
 */

#include "adc.h"

/* 私有变量 -------------------------------------------------------------- */
static ADC_HandleTypeDef hadc;           /* ADC 句柄                        */
static uint16_t adc_raw_buffer[3];       /* 当前采样值(由 DMA 更新)        */
static uint16_t adc_fifo[3][ADC_SAMPLE_BUFFER_SIZE];  /* 环形 FIFO 缓冲区   */
static uint8_t  fifo_index = 0;          /* 当前 FIFO 写入位置              */
static ADC_Data_t adc_data = {0};        /* 处理后的采样数据                */

/**
 * @brief  初始化 ADC 和 DMA
 * @note   配置为 3 通道扫描模式,DMA 循环传输
 *         通道顺序: VOUT_CH -> IOUT_CH -> VIN_CH
 */
void ADC_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    
    /* ---- 1. ADC 句柄配置 ---- */
    hadc.Instance = ADC1;
    hadc.Init.ScanConvMode = ADC_SCAN_ENABLE;           /* 扫描多个通道       */
    hadc.Init.ContinuousConvMode = ENABLE;               /* 连续转换           */
    hadc.Init.DiscontinuousConvMode = DISABLE;
    hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;     /* 软件触发           */
    hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;           /* 右对齐              */
    hadc.Init.NbrOfConversion = 3;                       /* 3 个通道            */
    HAL_ADC_Init(&hadc);
    
    /* ---- 2. 配置各采样通道 ---- */
    /* 通道 0: 输出电压 */
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;  /* 长采样时间保证精度  */
    HAL_ADC_ConfigChannel(&hadc, &sConfig);
    
    /* 通道 1: 输出电流 */
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = 2;
    HAL_ADC_ConfigChannel(&hadc, &sConfig);
    
    /* 通道 2: 输入电压 */
    sConfig.Channel = ADC_CHANNEL_2;
    sConfig.Rank = 3;
    HAL_ADC_ConfigChannel(&hadc, &sConfig);
    
    /* ---- 3. 初始化 FIFO 缓冲区 ---- */
    memset(adc_fifo, 0, sizeof(adc_fifo));
    fifo_index = 0;
}

/**
 * @brief  启动 ADC DMA 连续采样
 * @note   DMA 在后台循环传输,CPU 只需定期调用 ADC_Process() 读取结果
 */
void ADC_Start(void)
{
    HAL_ADC_Start_DMA(&hadc, (uint32_t*)adc_raw_buffer, 3);
}

/**
 * @brief  处理 ADC 采样数据
 * @note   每次 DMA 半完成/完成传输中断时调用,或由主循环定时调用
 *         执行: 写入 FIFO -> 均值滤波 -> 换算实际电压/电流
 *         
 *         重点: 均值滤波能有效抑制开关电源的开关频率纹波干扰,
 *               窗口大小建议根据控制频率调整
 */
void ADC_Process(void)
{
    uint32_t sum_vout = 0, sum_iout = 0, sum_vin = 0;
    uint8_t i;
    
    /* ---- 1. 将当前采样值写入环形 FIFO ---- */
    adc_fifo[0][fifo_index] = adc_raw_buffer[0];  /* VOUT */
    adc_fifo[1][fifo_index] = adc_raw_buffer[1];  /* IOUT */
    adc_fifo[2][fifo_index] = adc_raw_buffer[2];  /* VIN  */
    
    fifo_index = (fifo_index + 1) % ADC_SAMPLE_BUFFER_SIZE;
    
    /* ---- 2. 对 FIFO 内所有数据进行累加 ---- */
    for (i = 0; i < ADC_SAMPLE_BUFFER_SIZE; i++) {
        sum_vout += adc_fifo[0][i];
        sum_iout += adc_fifo[1][i];
        sum_vin  += adc_fifo[2][i];
    }
    
    /* ---- 3. 计算均值并换算为实际物理量 ---- */
    /* 输出电压: raw * (Vref/4096) / 分压比 */
    adc_data.vout.raw = adc_raw_buffer[0];
    adc_data.vout.filtered = (float)(sum_vout) / ADC_SAMPLE_BUFFER_SIZE;
    adc_data.vout.voltage = adc_data.vout.filtered 
                          * ADC_VREF / ADC_RESOLUTION 
                          / VOUT_DIVIDER_RATIO;
    
    /* 输出电流: raw * (Vref/4096) / (运放增益 * 采样电阻) */
    adc_data.iout.raw = adc_raw_buffer[1];
    adc_data.iout.filtered = (float)(sum_iout) / ADC_SAMPLE_BUFFER_SIZE;
    adc_data.iout.voltage = adc_data.iout.filtered 
                           * ADC_VREF / ADC_RESOLUTION 
                           / IOUT_AMPLIFIER_GAIN 
                           / IOUT_SHUNT_RESISTOR;
    
    /* 输入电压(可选) */
    adc_data.vin.raw = adc_raw_buffer[2];
    adc_data.vin.filtered = (float)(sum_vin) / ADC_SAMPLE_BUFFER_SIZE;
    adc_data.vin.voltage = adc_data.vin.filtered 
                          * ADC_VREF / ADC_RESOLUTION 
                          * 11.0f;  /* 假设 10:1 分压 */
    
    adc_data.ready = 1;
}

/* ---- 公开接口实现 ---- */

float ADC_GetVoltage(void)
{
    return adc_data.vout.voltage;
}

float ADC_GetCurrent(void)
{
    return adc_data.iout.voltage;
}

float ADC_GetInputVoltage(void)
{
    return adc_data.vin.voltage;
}

uint16_t ADC_GetRaw(uint8_t channel)
{
    if (channel < 3)
        return adc_raw_buffer[channel];
    return 0;
}
