#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------- */
#include "stm32f1xx_hal.h"               /* STM32F1 HAL 库                 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 全局函数声明 ----------------------------------------------------------- */
void SystemClock_Config(void);
void ControlTimer_Start(void);
void Control_Loop_ISR(void);

/* 错误处理 --------------------------------------------------------------- */
#define Error_Handler() _Error_Handler(__FILE__, __LINE__)

void _Error_Handler(char *file, int line);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
