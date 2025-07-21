/**
 * @file test_precision_enhancement.c
 * @brief 测试稳定后精度增强功能
 */

#include "kalman_filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// 简化的日志宏定义
#define elog_w(tag, fmt, ...) printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#define elog_i(tag, fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)

/**
 * @brief 生成带噪声的稳定信号
 */
double generate_stable_signal(double base_value, double noise_level, int sample_count) {
    // 生成高斯噪声的简化版本
    static double prev = 0;
    double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 * noise_level;
    
    // 简单的低通滤波使噪声更现实
    noise = (noise + prev) * 0.5;
    prev = noise;
    
    return base_value + noise;
}

/**
 * @brief 测试精度增强功能
 */
void test_precision_enhancement(void) {
    AdaptiveKalmanFilter_t akf;
    
    printf("\n=== 稳定状态精度增强测试 ===\n");
    
    // 初始化滤波器
    adaptive_kalman_init(&akf, 1.8, 0.01, 0.05, 1.0);
    
    // 启用高精度模式
    KALMAN_ENABLE_PRECISION(&akf);
    
    printf("初始化完成，基准电压: 1.8V，高精度模式已启用\n\n");
    
    // 阶段1: 100mV突变后的稳定过程
    printf("阶段1: 100mV突变 (1.8V -> 1.9V)\n");
    double step_values[] = {1.900, 1.901, 1.899, 1.900, 1.901};
    for (int i = 0; i < 5; i++) {
        double filtered = adaptive_kalman_update(&akf, step_values[i]);
        printf("  输入: %.4fV, 滤波后: %.6fV, 状态: %d, 精度: %.3f\n", 
               step_values[i], filtered, akf.signal_state, KALMAN_GET_PRECISION(&akf));
    }
    
    printf("\n阶段2: 长期稳定 - 观察精度提升过程\n");
    
    double base_voltage = 1.9;
    double noise_levels[] = {0.003, 0.002, 0.001, 0.0005, 0.0002};  // 逐渐减小的噪声
    
    for (int phase = 0; phase < 5; phase++) {
        printf("\n  子阶段 %d: 噪声水平 %.1fmV\n", phase + 1, noise_levels[phase] * 1000);
        
        for (int i = 0; i < 20; i++) {
            double input = generate_stable_signal(base_voltage, noise_levels[phase], i);
            double filtered = adaptive_kalman_update(&akf, input);
            
            if (i % 5 == 0) {  // 每5个样本输出一次
                double range = adaptive_kalman_get_voltage_range(&akf);
                printf("    [%02d] 输入: %.6fV, 滤波: %.6fV, 范围: %.6fV(%.2fmV), 精度: %.3f, 稳定计数: %d\n",
                       i, input, filtered, range, range*1000, KALMAN_GET_PRECISION(&akf), akf.stability_counter);
                
                // 检查是否进入高精度模式
                if (KALMAN_IS_HIGH_PRECISION(&akf)) {
                    printf("    *** 高精度模式激活! Q=%.8f, R=%.6f ***\n", akf.Q, akf.R);
                }
            }
        }
    }
    
    printf("\n阶段3: 超稳定状态测试 - 微小噪声\n");
    
    // 生成极小的波动 (< 1mV)
    for (int i = 0; i < 30; i++) {
        double micro_noise = ((double)rand() / RAND_MAX - 0.5) * 0.0004;  // ±0.2mV
        double input = base_voltage + micro_noise;
        double filtered = adaptive_kalman_update(&akf, input);
        
        if (i % 3 == 0) {
            double range = adaptive_kalman_get_voltage_range(&akf);
            printf("  [%02d] 输入: %.8fV, 滤波: %.8fV, 范围: %.8fV(%.3fμV), 精度: %.3f\n",
                   i, input, filtered, range, range*1000000, KALMAN_GET_PRECISION(&akf));
        }
    }
    
    // 最终统计
    printf("\n=== 最终精度统计 ===\n");
    double final_range = adaptive_kalman_get_voltage_range(&akf);
    printf("最终电压波动范围: %.8fV (%.3fμV)\n", final_range, final_range * 1000000);
    printf("当前精度因子: %.3f\n", KALMAN_GET_PRECISION(&akf));
    printf("稳定计数器: %d\n", akf.stability_counter);
    printf("高精度模式: %s\n", KALMAN_IS_HIGH_PRECISION(&akf) ? "活跃" : "非活跃");
    printf("滤波器参数: Q=%.8f, R=%.6f\n", akf.Q, akf.R);
    
    // 微小波动检测
    if (adaptive_kalman_is_micro_fluctuation(&akf)) {
        printf("✓ 成功达到微小波动状态 (< 21.3μV)\n");
    } else {
        printf("- 尚未达到微小波动状态\n");
    }
    
    printf("\n=== 精度增强测试完成 ===\n");
}

/**
 * @brief 对比测试：开启vs关闭精度增强
 */
void compare_precision_modes(void) {
    printf("\n=== 精度模式对比测试 ===\n");
    
    AdaptiveKalmanFilter_t akf_normal, akf_precision;
    
    // 初始化两个相同的滤波器
    adaptive_kalman_init(&akf_normal, 2.0, 0.01, 0.05, 1.0);
    adaptive_kalman_init(&akf_precision, 2.0, 0.01, 0.05, 1.0);
    
    // 一个启用精度增强，一个不启用
    KALMAN_DISABLE_PRECISION(&akf_normal);
    KALMAN_ENABLE_PRECISION(&akf_precision);
    
    printf("对比测试：相同输入信号，不同精度设置\n\n");
    
    // 使用相同的输入序列
    double test_signal = 2.0;
    for (int i = 0; i < 50; i++) {
        double noise = ((double)rand() / RAND_MAX - 0.5) * 0.002;  // ±1mV噪声
        double input = test_signal + noise;
        
        double output_normal = adaptive_kalman_update(&akf_normal, input);
        double output_precision = adaptive_kalman_update(&akf_precision, input);
        
        if (i % 10 == 0) {
            double range_normal = adaptive_kalman_get_voltage_range(&akf_normal);
            double range_precision = adaptive_kalman_get_voltage_range(&akf_precision);
            
            printf("[%02d] 输入: %.6fV\n", i, input);
            printf("     普通模式: %.6fV, 范围: %.6fV (%.2fmV)\n", 
                   output_normal, range_normal, range_normal*1000);
            printf("     精度模式: %.6fV, 范围: %.6fV (%.2fmV), 精度: %.3f\n", 
                   output_precision, range_precision, range_precision*1000, 
                   KALMAN_GET_PRECISION(&akf_precision));
            printf("\n");
        }
    }
    
    // 最终对比
    double final_range_normal = adaptive_kalman_get_voltage_range(&akf_normal);
    double final_range_precision = adaptive_kalman_get_voltage_range(&akf_precision);
    
    printf("=== 最终对比结果 ===\n");
    printf("普通模式波动范围: %.6fV (%.2fmV)\n", final_range_normal, final_range_normal*1000);
    printf("精度模式波动范围: %.6fV (%.2fmV)\n", final_range_precision, final_range_precision*1000);
    
    double improvement = (final_range_normal - final_range_precision) / final_range_normal * 100;
    printf("精度提升: %.1f%%\n", improvement);
    
    printf("\n=== 对比测试完成 ===\n");
}

int main(void) {
    printf("卡尔曼滤波器精度增强功能测试\n");
    printf("================================\n");
    
    // 设置随机种子
    srand(12345);
    
    test_precision_enhancement();
    compare_precision_modes();
    
    return 0;
}