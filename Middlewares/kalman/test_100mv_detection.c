/**
 * @file test_100mv_detection.c
 * @brief 测试100mV突变检测功能
 */

#include "kalman_filter.h"
#include <stdio.h>
#include <stdlib.h>

// 简化的日志宏定义
#define elog_w(tag, fmt, ...) printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#define elog_i(tag, fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)

/**
 * @brief 模拟100mV电压突变测试
 */
void test_100mv_detection(void) {
    AdaptiveKalmanFilter_t akf;
    
    printf("\n=== 100mV突变检测测试 ===\n");
    
    // 初始化滤波器
    adaptive_kalman_init(&akf, 1.5, 0.01, 0.05, 1.0);
    
    printf("初始化完成，基准电压: 1.5V\n\n");
    
    // 阶段1: 稳定状态 (1.5V ± 10mV)
    printf("阶段1: 稳定状态 (1.5V ± 10mV)\n");
    double stable_values[] = {1.501, 1.499, 1.502, 1.498, 1.500, 1.501, 1.499, 1.502};
    for (int i = 0; i < 8; i++) {
        double filtered = adaptive_kalman_update(&akf, stable_values[i]);
        printf("  输入: %.4fV, 滤波后: %.4fV, 状态: %d\n", 
               stable_values[i], filtered, akf.signal_state);
    }
    
    printf("\n");
    
    // 阶段2: 100mV突变 (1.5V -> 1.6V)
    printf("阶段2: 100mV正向突变 (1.5V -> 1.6V)\n");
    double step_values[] = {1.600, 1.601, 1.599, 1.600, 1.601};
    for (int i = 0; i < 5; i++) {
        double filtered = adaptive_kalman_update(&akf, step_values[i]);
        printf("  输入: %.4fV, 滤波后: %.4fV, 状态: %d", 
               step_values[i], filtered, akf.signal_state);
        
        if (akf.signal_state == SIGNAL_CHANGING) {
            printf(" *** CHANGING检测到! ***");
        }
        printf("\n");
    }
    
    printf("\n");
    
    // 阶段3: 稳定在新电平
    printf("阶段3: 稳定在新电平 (1.6V ± 5mV)\n");
    double new_stable[] = {1.601, 1.599, 1.600, 1.602, 1.598, 1.600};
    for (int i = 0; i < 6; i++) {
        double filtered = adaptive_kalman_update(&akf, new_stable[i]);
        printf("  输入: %.4fV, 滤波后: %.4fV, 状态: %d\n", 
               new_stable[i], filtered, akf.signal_state);
    }
    
    printf("\n");
    
    // 阶段4: 负向100mV突变 (1.6V -> 1.5V)
    printf("阶段4: 100mV负向突变 (1.6V -> 1.5V)\n");
    double step_down[] = {1.500, 1.501, 1.499, 1.500};
    for (int i = 0; i < 4; i++) {
        double filtered = adaptive_kalman_update(&akf, step_down[i]);
        printf("  输入: %.4fV, 滤波后: %.4fV, 状态: %d", 
               step_down[i], filtered, akf.signal_state);
        
        if (akf.signal_state == SIGNAL_CHANGING) {
            printf(" *** CHANGING检测到! ***");
        }
        printf("\n");
    }
    
    // 获取电压范围
    double voltage_range = adaptive_kalman_get_voltage_range(&akf);
    printf("\n当前电压波动范围: %.6fV (%.1fmV)\n", voltage_range, voltage_range * 1000);
    
    // 检查微小波动
    if (adaptive_kalman_is_micro_fluctuation(&akf)) {
        printf("检测到微小波动状态\n");
    } else {
        printf("未检测到微小波动状态\n");
    }
    
    printf("\n=== 测试完成 ===\n\n");
}

/**
 * @brief 测试微小波动检测
 */
void test_micro_fluctuation(void) {
    AdaptiveKalmanFilter_t akf;
    
    printf("\n=== 微小波动检测测试 ===\n");
    
    // 初始化滤波器
    adaptive_kalman_init(&akf, 2.0, 0.001, 0.01, 0.1);
    
    // 模拟非常小的波动 (< 21.3μV)
    double micro_values[] = {
        2.000000000, 2.000000010, 2.000000005, 2.000000015, 
        2.000000008, 2.000000012, 2.000000003, 2.000000018,
        2.000000006, 2.000000009
    };
    
    printf("模拟微小波动 (约10-20μV范围):\n");
    for (int i = 0; i < 10; i++) {
        double filtered = adaptive_kalman_update(&akf, micro_values[i]);
        printf("  输入: %.9fV, 滤波后: %.9fV\n", micro_values[i], filtered);
    }
    
    double range = adaptive_kalman_get_voltage_range(&akf);
    printf("\n电压范围: %.9fV (%.3fμV)\n", range, range * 1000000);
    
    if (adaptive_kalman_is_micro_fluctuation(&akf)) {
        printf("✓ 成功检测到微小波动!\n");
    } else {
        printf("✗ 未检测到微小波动\n");
    }
    
    printf("\n=== 微小波动测试完成 ===\n");
}

int main(void) {
    printf("卡尔曼滤波器电压变化检测测试\n");
    printf("==============================\n");
    
    test_100mv_detection();
    test_micro_fluctuation();
    
    return 0;
}