/**
 * @file adaptive_kalman_integration.c
 * @brief 自适应卡尔曼滤波器集成示例 - 适用于DP2501项目
 * @description 展示如何在实际的ADC数据采集系统中集成自适应卡尔曼滤波器
 */

#include "kalman_filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 模拟DP2501项目的ADC通道配置
typedef enum {
    ADC_CHANNEL_TEMP_1 = 0,      // 温度传感器1
    ADC_CHANNEL_TEMP_2,          // 温度传感器2
    ADC_CHANNEL_PRESSURE,        // 压力传感器
    ADC_CHANNEL_VOLTAGE,         // 电压监测
    ADC_CHANNEL_COUNT
} ADCChannel_t;

// ADC通道配置结构体
typedef struct {
    AdaptiveKalmanFilter_t filter;
    double calibration_offset;
    double calibration_scale;
    double cal_points[5][2];     // 最多5个标定点
    int cal_points_count;
    double range_min;
    double range_max;
    int enable_range_check;
    char name[16];
} ADCChannelConfig_t;

// 全局ADC滤波器配置
static ADCChannelConfig_t adc_channels[ADC_CHANNEL_COUNT];
static int system_initialized = 0;

/**
 * @brief 初始化ADC滤波器系统
 */
void adc_filter_system_init(void) {
    if (system_initialized) return;
    
    // 温度传感器1配置
    strcpy(adc_channels[ADC_CHANNEL_TEMP_1].name, "TEMP1");
    adaptive_kalman_init(&adc_channels[ADC_CHANNEL_TEMP_1].filter, 
                        25.0,     // 初始值：25°C
                        0.001,    // 过程噪声：温度变化慢
                        0.01,     // 测量噪声
                        1.0);     // 初始协方差
    
    // 设置温度传感器1的标定点
    adc_channels[ADC_CHANNEL_TEMP_1].cal_points[0][0] = 0.0;   // 0V
    adc_channels[ADC_CHANNEL_TEMP_1].cal_points[0][1] = -40.0; // -40°C
    adc_channels[ADC_CHANNEL_TEMP_1].cal_points[1][0] = 1.0;   // 1V
    adc_channels[ADC_CHANNEL_TEMP_1].cal_points[1][1] = 0.0;   // 0°C
    adc_channels[ADC_CHANNEL_TEMP_1].cal_points[2][0] = 2.0;   // 2V
    adc_channels[ADC_CHANNEL_TEMP_1].cal_points[2][1] = 40.0;  // 40°C
    adc_channels[ADC_CHANNEL_TEMP_1].cal_points[3][0] = 3.0;   // 3V
    adc_channels[ADC_CHANNEL_TEMP_1].cal_points[3][1] = 80.0;  // 80°C
    adc_channels[ADC_CHANNEL_TEMP_1].cal_points[4][0] = 4.0;   // 4V
    adc_channels[ADC_CHANNEL_TEMP_1].cal_points[4][1] = 120.0; // 120°C
    adc_channels[ADC_CHANNEL_TEMP_1].cal_points_count = 5;
    adc_channels[ADC_CHANNEL_TEMP_1].range_min = -50.0;
    adc_channels[ADC_CHANNEL_TEMP_1].range_max = 150.0;
    adc_channels[ADC_CHANNEL_TEMP_1].enable_range_check = 1;
    
    // 设置自适应参数
    adaptive_kalman_set_bounds(&adc_channels[ADC_CHANNEL_TEMP_1].filter, 
                              0.0001, 0.01, 0.001, 0.1);
    
    // 温度传感器2配置（类似温度传感器1）
    strcpy(adc_channels[ADC_CHANNEL_TEMP_2].name, "TEMP2");
    adaptive_kalman_init(&adc_channels[ADC_CHANNEL_TEMP_2].filter, 
                        25.0, 0.001, 0.01, 1.0);
    memcpy(adc_channels[ADC_CHANNEL_TEMP_2].cal_points, 
           adc_channels[ADC_CHANNEL_TEMP_1].cal_points, 
           sizeof(adc_channels[ADC_CHANNEL_TEMP_1].cal_points));
    adc_channels[ADC_CHANNEL_TEMP_2].cal_points_count = 5;
    adc_channels[ADC_CHANNEL_TEMP_2].range_min = -50.0;
    adc_channels[ADC_CHANNEL_TEMP_2].range_max = 150.0;
    adc_channels[ADC_CHANNEL_TEMP_2].enable_range_check = 1;
    adaptive_kalman_set_bounds(&adc_channels[ADC_CHANNEL_TEMP_2].filter, 
                              0.0001, 0.01, 0.001, 0.1);
    
    // 压力传感器配置
    strcpy(adc_channels[ADC_CHANNEL_PRESSURE].name, "PRESS");
    adaptive_kalman_init(&adc_channels[ADC_CHANNEL_PRESSURE].filter, 
                        0.0,      // 初始值：0 kPa
                        0.01,     // 过程噪声：压力变化中等
                        0.05,     // 测量噪声
                        1.0);     // 初始协方差
    
    // 简单的线性标定
    adc_channels[ADC_CHANNEL_PRESSURE].calibration_offset = 0.5;  // 0.5V偏移
    adc_channels[ADC_CHANNEL_PRESSURE].calibration_scale = 100.0; // 100kPa/V
    adc_channels[ADC_CHANNEL_PRESSURE].cal_points_count = 0;      // 不使用插值标定
    adc_channels[ADC_CHANNEL_PRESSURE].range_min = 0.0;
    adc_channels[ADC_CHANNEL_PRESSURE].range_max = 500.0;
    adc_channels[ADC_CHANNEL_PRESSURE].enable_range_check = 1;
    adaptive_kalman_set_bounds(&adc_channels[ADC_CHANNEL_PRESSURE].filter, 
                              0.001, 0.1, 0.01, 0.5);
    
    // 电压监测配置
    strcpy(adc_channels[ADC_CHANNEL_VOLTAGE].name, "VOLT");
    adaptive_kalman_init(&adc_channels[ADC_CHANNEL_VOLTAGE].filter, 
                        3.3,      // 初始值：3.3V
                        0.1,      // 过程噪声：电压变化快
                        0.02,     // 测量噪声
                        1.0);     // 初始协方差
    
    // 简单的比例标定
    adc_channels[ADC_CHANNEL_VOLTAGE].calibration_offset = 0.0;
    adc_channels[ADC_CHANNEL_VOLTAGE].calibration_scale = 1.0;    // 1:1
    adc_channels[ADC_CHANNEL_VOLTAGE].cal_points_count = 0;
    adc_channels[ADC_CHANNEL_VOLTAGE].range_min = 0.0;
    adc_channels[ADC_CHANNEL_VOLTAGE].range_max = 5.0;
    adc_channels[ADC_CHANNEL_VOLTAGE].enable_range_check = 1;
    adaptive_kalman_set_bounds(&adc_channels[ADC_CHANNEL_VOLTAGE].filter, 
                              0.01, 1.0, 0.005, 0.2);
    
    system_initialized = 1;
    printf("ADC Filter System Initialized\n");
}

/**
 * @brief 处理单个ADC通道的数据
 */
double process_adc_channel(ADCChannel_t channel, double raw_adc_value) {
    if (!system_initialized || channel >= ADC_CHANNEL_COUNT) {
        return raw_adc_value;
    }
    
    ADCChannelConfig_t *config = &adc_channels[channel];
    double filtered_value;
    
    // 根据标定配置选择处理方式
    if (config->cal_points_count > 0) {
        // 使用插值标定
        filtered_value = adaptive_kalman_update_with_interpolation(
            &config->filter, 
            raw_adc_value, 
            config->cal_points, 
            config->cal_points_count);
    } else {
        // 使用简单标定
        filtered_value = adaptive_kalman_update_with_calibration(
            &config->filter, 
            raw_adc_value, 
            config->calibration_offset, 
            config->calibration_scale);
    }
    
    // 范围检查
    if (config->enable_range_check) {
        if (filtered_value < config->range_min) {
            printf("Warning: %s value %.2f below minimum %.2f\n", 
                   config->name, filtered_value, config->range_min);
        } else if (filtered_value > config->range_max) {
            printf("Warning: %s value %.2f above maximum %.2f\n", 
                   config->name, filtered_value, config->range_max);
        }
    }
    
    return filtered_value;
}

/**
 * @brief 批量处理ADC数据
 */
int process_adc_batch(ADCChannel_t channel, const double *raw_data, 
                     double *filtered_data, int data_length) {
    if (!system_initialized || channel >= ADC_CHANNEL_COUNT) {
        return -1;
    }
    
    ADCChannelConfig_t *config = &adc_channels[channel];
    
    // 使用批量处理功能
    if (config->cal_points_count == 0) {
        // 对于简单标定，先进行标定再批量处理
        double *calibrated_data = malloc(data_length * sizeof(double));
        if (calibrated_data == NULL) return -1;
        
        for (int i = 0; i < data_length; i++) {
            calibrated_data[i] = (raw_data[i] - config->calibration_offset) * config->calibration_scale;
        }
        
        int result = adaptive_kalman_batch_process(&config->filter, 
                                                  calibrated_data, 
                                                  filtered_data, 
                                                  data_length);
        free(calibrated_data);
        return result;
    } else {
        // 对于插值标定，逐个处理
        for (int i = 0; i < data_length; i++) {
            filtered_data[i] = process_adc_channel(channel, raw_data[i]);
        }
        return data_length;
    }
}

/**
 * @brief 获取ADC通道的滤波器状态
 */
void get_adc_channel_status(ADCChannel_t channel, AdaptiveKalmanStatus_t *status) {
    if (!system_initialized || channel >= ADC_CHANNEL_COUNT || status == NULL) {
        return;
    }
    
    adaptive_kalman_get_status(&adc_channels[channel].filter, status);
}

/**
 * @brief 系统自检
 */
int adc_filter_system_self_check(void) {
    if (!system_initialized) return -1;
    
    int total_errors = 0;
    
    printf("\n=== ADC Filter System Self-Check ===\n");
    
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
        int check_result = adaptive_kalman_self_check(&adc_channels[i].filter);
        
        printf("Channel %s: ", adc_channels[i].name);
        if (check_result == KALMAN_CHECK_OK) {
            printf("OK\n");
        } else {
            printf("ERROR 0x%02X", check_result);
            if (check_result & KALMAN_CHECK_PARAM_ERROR) printf(" PARAM");
            if (check_result & KALMAN_CHECK_BOUNDS_ERROR) printf(" BOUNDS");
            if (check_result & KALMAN_CHECK_UNSTABLE) printf(" UNSTABLE");
            if (check_result & KALMAN_CHECK_POOR_TRACKING) printf(" TRACKING");
            if (check_result & KALMAN_CHECK_POOR_STABILITY) printf(" STABILITY");
            printf("\n");
            total_errors++;
        }
    }
    
    printf("Total errors: %d\n", total_errors);
    return total_errors;
}

/**
 * @brief 打印系统状态
 */
void print_adc_system_status(void) {
    if (!system_initialized) return;
    
    printf("\n=== ADC Filter System Status ===\n");
    
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
        AdaptiveKalmanStatus_t status;
        get_adc_channel_status(i, &status);
        
        printf("\nChannel %s:\n", adc_channels[i].name);
        printf("  Current Value: %.4f\n", status.current_value);
        printf("  Kalman Gain: %.4f\n", status.kalman_gain);
        printf("  Q: %.6f, R: %.6f\n", status.process_noise, status.measurement_noise);
        printf("  Stability: %.4f\n", status.stability_metric);
        printf("  Tracking Error: %.4f\n", status.tracking_error);
        printf("  Signal State: %d\n", status.signal_state);
        printf("  Adaptations: %d\n", status.adaptation_counter);
    }
}

/**
 * @brief 调整系统参数
 */
void tune_adc_system(double responsiveness, double stability_preference) {
    if (!system_initialized) return;
    
    printf("Tuning ADC system: Responsiveness=%.2f, Stability=%.2f\n", 
           responsiveness, stability_preference);
    
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
        adaptive_kalman_tune_parameters(&adc_channels[i].filter, 
                                       responsiveness, 
                                       stability_preference);
    }
}

/**
 * @brief 重置指定通道的滤波器
 */
void reset_adc_channel(ADCChannel_t channel, double new_value) {
    if (!system_initialized || channel >= ADC_CHANNEL_COUNT) return;
    
    adaptive_kalman_reset(&adc_channels[channel].filter, new_value);
    printf("Reset channel %s to value %.4f\n", adc_channels[channel].name, new_value);
}

/**
 * @brief 模拟ADC数据处理主循环
 */
void simulate_adc_processing(void) {
    // 初始化系统
    adc_filter_system_init();
    
    // 模拟数据处理
    printf("\n=== Simulating ADC Data Processing ===\n");
    
    for (int cycle = 0; cycle < 100; cycle++) {
        // 模拟ADC读取
        double temp1_raw = 2.0 + 0.1 * sin(cycle * 0.1) + ((double)rand() / RAND_MAX - 0.5) * 0.05;
        double temp2_raw = 2.1 + 0.08 * sin(cycle * 0.12) + ((double)rand() / RAND_MAX - 0.5) * 0.06;
        double pressure_raw = 1.5 + 0.3 * sin(cycle * 0.05) + ((double)rand() / RAND_MAX - 0.5) * 0.1;
        double voltage_raw = 3.3 + ((double)rand() / RAND_MAX - 0.5) * 0.02;
        
        // 处理数据
        double temp1_filtered = process_adc_channel(ADC_CHANNEL_TEMP_1, temp1_raw);
        double temp2_filtered = process_adc_channel(ADC_CHANNEL_TEMP_2, temp2_raw);
        double pressure_filtered = process_adc_channel(ADC_CHANNEL_PRESSURE, pressure_raw);
        double voltage_filtered = process_adc_channel(ADC_CHANNEL_VOLTAGE, voltage_raw);
        
        // 每10个周期打印一次结果
        if (cycle % 10 == 0) {
            printf("Cycle %d: T1=%.2f°C, T2=%.2f°C, P=%.1fkPa, V=%.3fV\n", 
                   cycle, temp1_filtered, temp2_filtered, pressure_filtered, voltage_filtered);
        }
        
        // 每50个周期进行一次自检
        if (cycle % 50 == 0 && cycle > 0) {
            adc_filter_system_self_check();
        }
    }
    
    // 打印最终状态
    print_adc_system_status();
    
    // 获取性能统计
    printf("\n=== Performance Statistics ===\n");
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
        AdaptiveKalmanPerformance_t perf;
        adaptive_kalman_get_performance_stats(&adc_channels[i].filter, &perf);
        
        printf("\nChannel %s Performance:\n", adc_channels[i].name);
        printf("  Tracking Accuracy: %.4f\n", perf.tracking_accuracy);
        printf("  Filter Stability: %.4f\n", perf.filter_stability);
        printf("  Convergence Speed: %.4f\n", perf.convergence_speed);
        printf("  Q Drift: %.4f%%\n", perf.Q_drift * 100);
        printf("  R Drift: %.4f%%\n", perf.R_drift * 100);
    }
}

/**
 * @brief 主函数
 */
int main(void) {
    printf("Adaptive Kalman Filter Integration for DP2501\n");
    printf("=============================================\n");
    
    // 初始化随机数生成器
    srand(42);  // 使用固定种子以便重现结果
    
    // 运行仿真
    simulate_adc_processing();
    
    printf("\nIntegration test completed!\n");
    return 0;
}
