# 自适应卡尔曼滤波器 - 完整实现

## 项目概述

本项目实现了一个完整的自适应卡尔曼滤波器库，专为MCU项目（如DP2501）的ADC数据采集和信号处理设计。该滤波器能够根据输入信号的特性自动调整其参数，提供优异的滤波性能。

## 🎯 主要特性

### 1. 自适应算法
- **创新序列分析**：实时分析预测误差，动态调整测量噪声参数
- **统计特性分析**：基于信号统计特性自动调整过程噪声参数
- **信号状态识别**：自动识别信号状态（稳定、变化、噪声、饱和）
- **渐进式参数调整**：使用遗忘因子实现平滑的参数转换

### 2. 高精度支持
- **双精度计算**：所有计算使用双精度浮点数
- **数值稳定性**：防止参数发散，确保长期稳定运行
- **边界控制**：可设置参数的最大最小值

### 3. 标定集成
- **简单标定**：支持偏移和比例标定
- **多点插值**：支持多点线性插值标定
- **动态标定**：支持运行时标定模式切换

### 4. 性能监控
- **实时诊断**：提供跟踪误差、稳定性指标等性能指标
- **自检功能**：自动检测参数异常和数值不稳定
- **统计分析**：提供详细的性能统计信息

## 📁 文件结构

```
kalman/
├── kalman_filter.h                    # 头文件
├── kalman_filter.c                    # 核心实现
├── adaptive_kalman_example.c          # 基础使用示例
├── adaptive_kalman_integration.c      # 集成示例（DP2501风格）
├── test_adaptive_kalman.c             # 综合测试程序
├── README.md                          # 详细说明文档
├── Makefile                           # 编译脚本
└── CMakeLists.txt                     # CMake构建文件
```

## 🚀 核心算法

### 自适应机制原理

1. **创新序列分析**
   ```c
   innovation = measurement - prediction
   innovation_variance = calculate_variance(innovation_history)
   theoretical_variance = P + R
   adaptation_gain = innovation_variance / theoretical_variance
   ```

2. **参数自适应调整**
   ```c
   if (adaptation_gain > 2.0) {
       R = R * (1.0 + adaptation_rate)  // 增加测量噪声
   } else if (adaptation_gain < 0.5) {
       R = R * (1.0 - adaptation_rate)  // 减少测量噪声
   }
   ```

3. **信号状态识别**
   ```c
   if (variance < R_base * 0.01) {
       signal_state = SIGNAL_STABLE
   } else if (variance > R_base * 100) {
       signal_state = SIGNAL_NOISY
   } else if (max_diff > mean * 0.1) {
       signal_state = SIGNAL_CHANGING
   }
   ```

## 💡 使用示例

### 基本使用
```c
#include "kalman_filter.h"

AdaptiveKalmanFilter_t filter;

// 初始化
adaptive_kalman_init(&filter, 0.0, 0.01, 0.1, 1.0);

// 设置参数边界
adaptive_kalman_set_bounds(&filter, 0.001, 0.1, 0.01, 1.0);

// 处理数据
double raw_adc = read_adc();
double filtered_value = adaptive_kalman_update(&filter, raw_adc);
```

### 带标定的使用
```c
// 温度传感器标定点
const double temp_cal_points[][2] = {
    {0.0, -40.0},    // 0V -> -40°C
    {1.0, 0.0},      // 1V -> 0°C
    {2.0, 40.0},     // 2V -> 40°C
    {3.0, 80.0},     // 3V -> 80°C
    {4.0, 120.0}     // 4V -> 120°C
};

// 使用插值标定
double temperature = adaptive_kalman_update_with_interpolation(&filter, 
                                                             raw_adc, 
                                                             temp_cal_points, 
                                                             5);
```

### 性能监控
```c
// 获取滤波器状态
AdaptiveKalmanStatus_t status;
adaptive_kalman_get_status(&filter, &status);

// 自检
int check_result = adaptive_kalman_self_check(&filter);
if (check_result != KALMAN_CHECK_OK) {
    printf("Filter error: 0x%02X\n", check_result);
}

// 性能统计
AdaptiveKalmanPerformance_t perf;
adaptive_kalman_get_performance_stats(&filter, &perf);
printf("Tracking accuracy: %.4f\n", perf.tracking_accuracy);
```

## 📊 参数调整指南

### 初始参数选择

| 应用场景 | 过程噪声Q | 测量噪声R | 建议说明 |
|----------|-----------|-----------|----------|
| 温度传感器 | 0.001-0.01 | 0.01-0.1 | 变化慢，高精度 |
| 压力传感器 | 0.01-0.1 | 0.1-1.0 | 变化中等 |
| 快速信号 | 0.1-1.0 | 0.02-0.2 | 变化快 |
| 噪声环境 | 0.05-0.5 | 0.2-2.0 | 噪声大 |

### 自适应参数

| 参数 | 范围 | 建议值 | 说明 |
|------|------|--------|------|
| 自适应速率 | 0.05-0.3 | 0.1 | 环境变化快时增大 |
| 遗忘因子 | 0.9-0.99 | 0.95 | 历史数据权重 |
| 参数边界 | 0.1x-10x | 基值的0.1-10倍 | 防止参数发散 |

## 🛠️ 编译和测试

### 使用Makefile
```bash
# 编译所有目标
make all

# 运行测试
make test

# 运行集成示例
make integration

# 清理构建文件
make clean
```

### 使用CMake
```bash
mkdir build
cd build
cmake ..
make
```

## 📈 性能特点

### 优势
1. **自适应能力强**：能够适应不同的信号特性
2. **数值稳定**：双精度计算，参数边界控制
3. **集成度高**：标定、滤波、监控一体化
4. **易于使用**：简单的API，丰富的示例

### 适用场景
- 温度传感器数据处理
- 压力传感器信号调理
- ADC采样数据滤波
- 传感器信号标定
- 实时数据监控

### 性能指标
- **处理速度**：>10,000样本/秒（典型MCU）
- **内存占用**：~500字节/滤波器实例
- **精度**：双精度浮点（~15位有效数字）
- **稳定性**：长期运行无发散

## 🔧 高级功能

### 批量处理
```c
double input_data[1000];
double output_data[1000];
int processed = adaptive_kalman_batch_process(&filter, input_data, output_data, 1000);
```

### 参数调优
```c
// 自动调优：响应性0.8，稳定性0.6
adaptive_kalman_tune_parameters(&filter, 0.8, 0.6);
```

### 信号状态检测
```c
SignalState_t state = adaptive_kalman_get_signal_state(&filter);
switch (state) {
    case SIGNAL_STABLE:   // 信号稳定
    case SIGNAL_CHANGING: // 信号变化
    case SIGNAL_NOISY:    // 噪声较大
    case SIGNAL_SATURATED: // 信号饱和
}
```

## 🎓 理论基础

### 卡尔曼滤波器基本方程
```
预测步骤：
x_pred = x_prev
P_pred = P_prev + Q

更新步骤：
K = P_pred / (P_pred + R)
x = x_pred + K * (measurement - x_pred)
P = (1 - K) * P_pred
```

### 自适应调整策略
1. **创新序列方法**：基于预测误差的统计特性
2. **统计窗口方法**：基于测量序列的统计分析
3. **信号状态方法**：基于信号模式识别

## 📝 开发指南

### 集成步骤
1. 包含头文件：`#include "kalman_filter.h"`
2. 初始化滤波器：`adaptive_kalman_init()`
3. 设置参数边界：`adaptive_kalman_set_bounds()`
4. 处理数据：`adaptive_kalman_update()`
5. 监控性能：`adaptive_kalman_self_check()`

### 调试建议
1. 使用自检功能检测异常
2. 监控关键性能指标
3. 记录参数变化历史
4. 验证标定精度

## 🔄 版本更新

### v2.2 (当前版本)
- ✅ 完整的自适应卡尔曼滤波器实现
- ✅ 双精度浮点计算
- ✅ 多种标定方式支持
- ✅ 性能监控和自检功能
- ✅ 批量处理支持
- ✅ 完整的示例和文档

### 未来计划
- 🔄 多维卡尔曼滤波器支持
- 🔄 无损卡尔曼滤波器（UKF）
- 🔄 实时参数可视化
- 🔄 更多传感器标定模板

## 📞 支持和贡献

如需技术支持或想要贡献代码，请通过以下方式联系：

- 代码仓库：内部项目
- 技术文档：查看README.md
- 示例代码：参考example文件
- 测试用例：运行test程序

---

**自适应卡尔曼滤波器 - 让您的传感器数据更精准！** 🎯
