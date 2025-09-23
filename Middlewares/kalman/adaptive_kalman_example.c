/**
 * @file adaptive_kalman_example.c
 * @brief 自适应卡尔曼滤波器使用示例
 * @description 展示如何在ADC数据采集中使用自适应卡尔曼滤波器
 */

#include "kalman_filter.h"
#include <stdio.h>

// 全局自适应卡尔曼滤波器实例
static AdaptiveKalmanFilter_t adc_filter[4];  // 假设有4个ADC通道

/**
 * @brief 初始化ADC滤波器
 */
void adc_adaptive_filter_init(void) {
    // 通道0：高精度温度传感器
    adaptive_kalman_init(&adc_filter[0], 
                        0.0,        // 初始值
                        0.001,      // 过程噪声（较小，温度变化慢）
                        0.01,       // 测量噪声
                        1.0);       // 初始协方差
    
    // 设置参数边界
    adaptive_kalman_set_bounds(&adc_filter[0], 0.0001, 0.1, 0.001, 1.0);
    
    // 通道1：压力传感器
    adaptive_kalman_init(&adc_filter[1], 
                        0.0,        // 初始值
                        0.01,       // 过程噪声（中等，压力变化中等）
                        0.05,       // 测量噪声
                        1.0);       // 初始协方差
    
    adaptive_kalman_set_bounds(&adc_filter[1], 0.001, 0.5, 0.01, 2.0);
    
    // 通道2：快速变化的信号
    adaptive_kalman_init(&adc_filter[2], 
                        0.0,        // 初始值
                        0.1,        // 过程噪声（较大，信号变化快）
                        0.02,       // 测量噪声
                        1.0);       // 初始协方差
    
    adaptive_kalman_set_bounds(&adc_filter[2], 0.01, 2.0, 0.005, 0.5);
    
    // 通道3：高噪声环境
    adaptive_kalman_init(&adc_filter[3], 
                        0.0,        // 初始值
                        0.05,       // 过程噪声
                        0.2,        // 测量噪声（较大）
                        1.0);       // 初始协方差
    
    adaptive_kalman_set_bounds(&adc_filter[3], 0.005, 1.0, 0.05, 5.0);
}

/**
 * @brief 处理ADC数据
 */
double process_adc_data(int channel, double raw_adc_value) {
    if (channel < 0 || channel >= 4) return raw_adc_value;
    
    // 标定点表（示例）
    const double temperature_cal_points[][2] = {
        {0.0, -40.0},    // 0V对应-40°C
        {1.0, 0.0},      // 1V对应0°C
        {2.0, 40.0},     // 2V对应40°C
        {3.0, 80.0},     // 3V对应80°C
        {4.0, 120.0}     // 4V对应120°C
    };
    
    double filtered_value;
    
    switch (channel) {
        case 0:  // 温度传感器（带插值标定）
            filtered_value = adaptive_kalman_update_with_interpolation(
                &adc_filter[0], 
                raw_adc_value, 
                temperature_cal_points, 
                5);
            break;
            
        case 1:  // 压力传感器（简单标定）
            filtered_value = adaptive_kalman_update_with_calibration(
                &adc_filter[1], 
                raw_adc_value, 
                0.1,    // 偏移量
                2.0);   // 比例系数
            break;
            
        default:  // 其他通道（无标定）
            filtered_value = adaptive_kalman_update(&adc_filter[channel], raw_adc_value);
            break;
    }
    
    return filtered_value;
}

/**
 * @brief 获取滤波器状态信息
 */
void print_filter_status(int channel) {
    if (channel < 0 || channel >= 4) return;
    
    AdaptiveKalmanFilter_t *akf = &adc_filter[channel];
    
    printf("Channel %d Filter Status:\n", channel);
    printf("  Current Value: %.4f\n", akf->x);
    printf("  Kalman Gain: %.4f\n", akf->K);
    printf("  Process Noise Q: %.6f\n", akf->Q);
    printf("  Measurement Noise R: %.6f\n", akf->R);
    printf("  Adaptation Gain: %.4f\n", adaptive_kalman_get_adaptation_gain(akf));
    printf("  Stability Metric: %.4f\n", adaptive_kalman_get_stability_metric(akf));
    printf("  Signal State: %d\n", adaptive_kalman_get_signal_state(akf));
    printf("  Tracking Error: %.4f\n", akf->tracking_error);
    printf("\n");
}

/**
 * @brief 动态调整滤波器参数
 */
void adjust_filter_for_environment(int channel, int environment_type) {
    if (channel < 0 || channel >= 4) return;
    
    AdaptiveKalmanFilter_t *akf = &adc_filter[channel];
    
    switch (environment_type) {
        case 0:  // 安静环境
            adaptive_kalman_set_adaptation_rate(akf, 0.05);  // 慢速自适应
            break;
            
        case 1:  // 正常环境
            adaptive_kalman_set_adaptation_rate(akf, 0.1);   // 中速自适应
            break;
            
        case 2:  // 噪声环境
            adaptive_kalman_set_adaptation_rate(akf, 0.2);   // 快速自适应
            break;
            
        case 3:  // 极度噪声环境
            adaptive_kalman_set_adaptation_rate(akf, 0.3);   // 极快自适应
            break;
    }
}

/**
 * @brief 检测滤波器是否需要重置
 */
int check_filter_reset_needed(int channel) {
    if (channel < 0 || channel >= 4) return 0;
    
    AdaptiveKalmanFilter_t *akf = &adc_filter[channel];
    
    // 检查是否需要重置的条件：
    // 1. 跟踪误差过大
    // 2. 稳定性指标过低
    // 3. 状态变化过于频繁
    
    if (akf->tracking_error > 10.0 * akf->R_base ||
        akf->stability_metric < 0.1 ||
        akf->state_change_counter > 50) {
        return 1;  // 需要重置
    }
    
    return 0;  // 不需要重置
}

/**
 * @brief 重置滤波器
 */
void reset_filter(int channel, double new_value) {
    if (channel < 0 || channel >= 4) return;
    
    adaptive_kalman_reset(&adc_filter[channel], new_value);
    printf("Filter %d reset to value: %.4f\n", channel, new_value);
}

/**
 * @brief 主要的ADC数据处理循环示例
 */
void adc_processing_loop_example(void) {
    // 模拟ADC数据
    double adc_samples[4] = {1.234, 2.567, 0.890, 3.456};
    
    // 初始化滤波器
    adc_adaptive_filter_init();
    
    // 模拟数据处理
    for (int i = 0; i < 1000; i++) {
        // 模拟噪声
        double noise = ((double)rand() / RAND_MAX - 0.5) * 0.1;
        
        for (int channel = 0; channel < 4; channel++) {
            // 添加噪声到ADC采样
            double noisy_sample = adc_samples[channel] + noise;
            
            // 处理数据
            double filtered_value = process_adc_data(channel, noisy_sample);
            
            // 检查是否需要重置
            if (check_filter_reset_needed(channel)) {
                reset_filter(channel, filtered_value);
            }
            
            // 每100次迭代打印状态
            if (i % 100 == 0) {
                printf("Iteration %d, Channel %d: Raw=%.4f, Filtered=%.4f\n", 
                       i, channel, noisy_sample, filtered_value);
            }
        }
        
        // 每200次迭代打印详细状态
        if (i % 200 == 0) {
            print_filter_status(0);  // 打印通道0的状态
        }
    }
}
