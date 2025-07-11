# 自适应卡尔曼滤波器

## 概述

这是一个用于MCU项目的自适应卡尔曼滤波器实现，特别适用于ADC数据采集和信号处理。该滤波器能够根据输入信号的特性自动调整其参数，提供更好的滤波效果。

## 主要特性

### 1. 自适应机制
- **创新序列分析**：基于预测误差自动调整测量噪声参数
- **统计特性分析**：根据信号的统计特性调整过程噪声参数
- **信号状态识别**：自动识别信号状态（稳定、变化、噪声、饱和）

### 2. 双精度支持
- 所有计算使用双精度浮点数，确保高精度应用的数值稳定性
- 适用于高精度ADC采样和温度测量

### 3. 标定集成
- 支持简单的偏移和比例标定
- 支持多点线性插值标定
- 可在滤波前或滤波后进行标定

### 4. 参数边界控制
- 可设置Q和R参数的最大最小值
- 防止自适应过程中参数偏离合理范围

## 文件结构

```
kalman/
├── kalman_filter.h              # 头文件
├── kalman_filter.c              # 实现文件
├── adaptive_kalman_example.c    # 使用示例
└── README.md                   # 本文档
```

## 核心数据结构

### AdaptiveKalmanFilter_t
```c
typedef struct {
    // 基本卡尔曼参数
    double x;     // 状态估计值
    double P;     // 估计误差协方差
    double Q;     // 过程噪声协方差
    double R;     // 测量噪声协方差
    double K;     // 卡尔曼增益
    
    // 自适应参数
    double Q_base, R_base;           // 基础参数
    double Q_min, Q_max;             // 参数边界
    double R_min, R_max;
    
    // 历史数据
    double innovation_history[20];   // 创新序列历史
    double measurement_history[10];  // 测量值历史
    
    // 自适应控制
    int enable_adaptation;           // 是否启用自适应
    double adaptation_threshold;     // 自适应触发阈值
    double forgetting_factor;        // 遗忘因子
    
    // 性能监控
    SignalState_t signal_state;      // 信号状态
    double tracking_error;           // 跟踪误差
    double stability_metric;         // 稳定性指标
} AdaptiveKalmanFilter_t;
```

### 信号状态枚举
```c
typedef enum {
    SIGNAL_STABLE,      // 信号稳定
    SIGNAL_CHANGING,    // 信号变化中
    SIGNAL_NOISY,       // 噪声较大
    SIGNAL_SATURATED    // 信号饱和
} SignalState_t;
```

## 主要函数

### 初始化函数
```c
void adaptive_kalman_init(AdaptiveKalmanFilter_t *akf, 
                         double initial_value, 
                         double process_noise, 
                         double measurement_noise, 
                         double initial_covariance);
```

### 滤波更新函数
```c
double adaptive_kalman_update(AdaptiveKalmanFilter_t *akf, double measurement);
```

### 带标定的滤波更新
```c
double adaptive_kalman_update_with_calibration(AdaptiveKalmanFilter_t *akf, 
                                              double raw_measurement, 
                                              double offset, 
                                              double scale);
```

### 参数控制函数
```c
void adaptive_kalman_set_bounds(AdaptiveKalmanFilter_t *akf, 
                               double Q_min, double Q_max, 
                               double R_min, double R_max);
void adaptive_kalman_enable_adaptation(AdaptiveKalmanFilter_t *akf, int enable);
void adaptive_kalman_set_adaptation_rate(AdaptiveKalmanFilter_t *akf, double rate);
```

## 使用示例

### 1. 基本使用
```c
#include "kalman_filter.h"

AdaptiveKalmanFilter_t filter;

// 初始化
adaptive_kalman_init(&filter, 0.0, 0.01, 0.1, 1.0);

// 处理数据
double raw_adc = read_adc();
double filtered_value = adaptive_kalman_update(&filter, raw_adc);
```

### 2. 带标定的使用
```c
// 温度传感器标定点
const double temp_cal_points[][2] = {
    {0.0, -40.0},    // 0V对应-40°C
    {1.0, 0.0},      // 1V对应0°C
    {2.0, 40.0},     // 2V对应40°C
    {3.0, 80.0},     // 3V对应80°C
    {4.0, 120.0}     // 4V对应120°C
};

// 使用插值标定
double temperature = adaptive_kalman_update_with_interpolation(&filter, 
                                                             raw_adc, 
                                                             temp_cal_points, 
                                                             5);
```

### 3. 参数调整
```c
// 设置参数边界
adaptive_kalman_set_bounds(&filter, 0.001, 0.1, 0.01, 1.0);

// 设置自适应速率
adaptive_kalman_set_adaptation_rate(&filter, 0.1);

// 监控滤波器状态
SignalState_t state = adaptive_kalman_get_signal_state(&filter);
double stability = adaptive_kalman_get_stability_metric(&filter);
```

## 参数调整指南

### 1. 初始参数选择
- **过程噪声Q**：反映系统动态特性
  - 温度传感器：0.001-0.01（变化慢）
  - 压力传感器：0.01-0.1（变化中等）
  - 快速信号：0.1-1.0（变化快）

- **测量噪声R**：反映传感器精度
  - 高精度ADC：0.01-0.1
  - 普通ADC：0.1-1.0
  - 噪声环境：1.0-10.0

### 2. 自适应参数
- **自适应速率**：0.05-0.3
  - 安静环境：0.05
  - 正常环境：0.1
  - 噪声环境：0.2-0.3

- **参数边界**：
  - Q_min = Q_base × 0.1
  - Q_max = Q_base × 10.0
  - R_min = R_base × 0.1
  - R_max = R_base × 10.0

## 性能优化建议

### 1. 内存优化
- 对于RAM受限的MCU，可以减少历史数据窗口大小
- 可以选择性地禁用某些自适应功能

### 2. 计算优化
- 可以降低统计分析的频率（如每10次更新进行一次）
- 对于稳定信号，可以临时禁用自适应

### 3. 实时性优化
- 将复杂的统计分析放在低优先级任务中
- 使用中断驱动的数据处理

## 注意事项

1. **初始化**：确保在使用前正确初始化滤波器
2. **参数边界**：合理设置参数边界，避免过度自适应
3. **稳定性监控**：定期检查稳定性指标，必要时重置滤波器
4. **标定顺序**：通常先进行标定，再进行滤波
5. **数值精度**：对于高精度应用，注意数值精度问题

## 调试和监控

### 状态监控
```c
// 打印滤波器状态
printf("Q: %.6f, R: %.6f, K: %.4f\n", filter.Q, filter.R, filter.K);
printf("State: %d, Stability: %.4f\n", filter.signal_state, filter.stability_metric);
```

### 性能评估
```c
// 检查是否需要重置
if (filter.tracking_error > 10.0 * filter.R_base || 
    filter.stability_metric < 0.1) {
    adaptive_kalman_reset(&filter, current_measurement);
}
```

## 版本历史

- v1.0: 基本卡尔曼滤波器实现
- v2.0: 添加自适应功能
- v2.1: 添加标定集成和参数边界控制
- v2.2: 优化自适应算法，添加性能监控

## 许可证

此代码库遵循MIT许可证。
