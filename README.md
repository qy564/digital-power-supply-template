# 数字电源学习模板 (Digital Power Supply Template)

**面向电赛的数字电源入门项目 —— STM32F103 + Buck 变换器**

## 项目结构

`
├── Core/
│   ├── Inc/              # 头文件
│   │   ├── main.h        # 主头文件、时钟配置
│   │   ├── pwm.h         # PWM 模块 — 互补输出 + 死区插入
│   │   ├── adc.h         # ADC 模块 — DMA 连续采样 + 均值滤波
│   │   ├── pid.h         # PID 模块 — 积分分离 + 抗饱和
│   │   └── protection.h  # 保护模块 — OVP/OCP/UVP + 软启动
│   └── Src/              # 实现文件
│       ├── main.c        # 主调度 + 10kHz 控制中断
│       ├── pwm.c         # 100kHz 互补 PWM 驱动
│       ├── adc.c         # 多通道 DMA 采样 + 滑动窗口滤波
│       ├── pid.c         # PI 控制器（增量式）
│       └── protection.c  # 保护逻辑 + 锁死机制 + 软启动
├── Tools/
│   └── serial_monitor.py # 串口调参监视器（含实时波形）
└── TUTORIAL.md           # 从零到比赛的完整教程
`

## 快速开始

1. **硬件准备**: STM32F103C8T6 + Buck 功率板（MOSFET、电感、电容）
2. **软件**: 使用 CubeMX 或直接导入 Keil/IAR 工程
3. **烧录**: 编译后烧录到 MCU
4. **调参**: 连接串口，运行 Tools/serial_monitor.py

## 控制环路

`
ADC采样 → 保护检查 → PID计算 → 更新PWM
  ↑                            │
  └────────────────────────────┘
        每 100us 执行一次 (10kHz)
`

## 模块详解

| 模块 | 功能 | 关键配置 |
|------|------|----------|
| PWM | 100kHz 互补输出 + 死区 | pwm.h 中的频率/死区宏 |
| ADC | 3通道 DMA 采样 + 8次均值滤波 | dc.h 中的分压比配置 |
| PID | 增量式 PI + 积分分离抗饱和 | pid.c 中的预置参数 |
| Protection | OVP/OCP/UVP 锁死保护 | protection.h 中的阈值 |

## 许可证

MIT License