/**
 * @file test_adaptive_kalman.c
 * @brief 自适应卡尔曼滤波器测试程序
 */

#include "kalman_filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// 测试信号生成函数
double generate_test_signal(int type, int sample_index, double noise_level) {
    double signal = 0.0;
    double t = sample_index * 0.1;  // 时间步长0.1秒
    
    switch (type) {
        case 0:  // 常数信号
            signal = 5.0;
            break;
            
        case 1:  // 正弦信号
            signal = 5.0 + 2.0 * sin(0.1 * t);
            break;
            
        case 2:  // 阶跃信号
            signal = (sample_index > 100) ? 8.0 : 2.0;
            break;
            
        case 3:  // 斜坡信号
            signal = 2.0 + 0.01 * t;
            break;
            
        case 4:  // 脉冲信号
            signal = ((sample_index % 200) < 20) ? 10.0 : 3.0;
            break;
    }
    
    // 添加噪声
    double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0 * noise_level;
    return signal + noise;
}

// 测试函数
void test_adaptive_filter(const char* test_name, int signal_type, double noise_level, int samples) {
    printf("\n=== %s ===\n", test_name);
    printf("Signal Type: %d, Noise Level: %.3f, Samples: %d\n\n", signal_type, noise_level, samples);
    
    AdaptiveKalmanFilter_t filter;
    
    // 初始化滤波器
    adaptive_kalman_init(&filter, 0.0, 0.01, 0.1, 1.0);
    
    // 设置参数边界
    adaptive_kalman_set_bounds(&filter, 0.001, 0.5, 0.01, 2.0);
    
    // 统计变量
    double sum_error = 0.0;
    double sum_squared_error = 0.0;
    double max_error = 0.0;
    
    printf("Sample\tTrue\tNoisy\tFiltered\tError\tQ\tR\tState\n");
    printf("------\t----\t-----\t--------\t-----\t-----\t-----\t-----\n");
    
    for (int i = 0; i < samples; i++) {
        // 生成真实信号和噪声信号
        double true_signal = generate_test_signal(signal_type, i, 0.0);
        double noisy_signal = generate_test_signal(signal_type, i, noise_level);
        
        // 滤波处理
        double filtered_signal = adaptive_kalman_update(&filter, noisy_signal);
        
        // 计算误差
        double error = fabs(filtered_signal - true_signal);
        sum_error += error;
        sum_squared_error += error * error;
        if (error > max_error) max_error = error;
        
        // 每10个采样打印一次
        if (i % 10 == 0) {
            printf("%d\t%.3f\t%.3f\t%.3f\t%.3f\t%.4f\t%.4f\t%d\n", 
                   i, true_signal, noisy_signal, filtered_signal, error,
                   filter.Q, filter.R, filter.signal_state);
        }
    }
    
    // 计算统计结果
    double mean_error = sum_error / samples;
    double rms_error = sqrt(sum_squared_error / samples);
    double stability = adaptive_kalman_get_stability_metric(&filter);
    
    printf("\n--- Test Results ---\n");
    printf("Mean Error: %.4f\n", mean_error);
    printf("RMS Error: %.4f\n", rms_error);
    printf("Max Error: %.4f\n", max_error);
    printf("Final Q: %.6f\n", filter.Q);
    printf("Final R: %.6f\n", filter.R);
    printf("Stability Metric: %.4f\n", stability);
    printf("Final Signal State: %d\n", filter.signal_state);
    printf("Adaptation Counter: %d\n", filter.adaptation_counter);
}

// 测试标定功能
void test_calibration_function(void) {
    printf("\n=== Calibration Test ===\n");
    
    // 测试标定点
    const double cal_points[][2] = {
        {0.0, -40.0},    // 0V -> -40°C
        {1.0, 0.0},      // 1V -> 0°C
        {2.0, 40.0},     // 2V -> 40°C
        {3.0, 80.0},     // 3V -> 80°C
        {4.0, 120.0}     // 4V -> 120°C
    };
    
    printf("Raw Value\tCalibrated Value\n");
    printf("---------\t----------------\n");
    
    for (double raw = 0.0; raw <= 4.0; raw += 0.5) {
        double calibrated = kalman_linear_interpolation_calibration(raw, cal_points, 5);
        printf("%.1f\t\t%.1f\n", raw, calibrated);
    }
}

// 性能测试
void test_performance(void) {
    printf("\n=== Performance Test ===\n");
    
    AdaptiveKalmanFilter_t filter;
    adaptive_kalman_init(&filter, 0.0, 0.01, 0.1, 1.0);
    
    const int iterations = 10000;
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        double measurement = sin(i * 0.01) + ((double)rand() / RAND_MAX - 0.5) * 0.1;
        adaptive_kalman_update(&filter, measurement);
    }
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Processed %d samples in %.4f seconds\n", iterations, time_taken);
    printf("Average time per sample: %.2f microseconds\n", (time_taken / iterations) * 1000000);
    printf("Processing rate: %.0f samples/second\n", iterations / time_taken);
}

// 主函数
int main(void) {
    printf("Adaptive Kalman Filter Test Suite\n");
    printf("==================================\n");
    
    // 初始化随机数生成器
    srand(time(NULL));
    
    // 测试1：常数信号，低噪声
    test_adaptive_filter("Test 1: Constant Signal, Low Noise", 0, 0.1, 200);
    
    // 测试2：正弦信号，中等噪声
    test_adaptive_filter("Test 2: Sine Signal, Medium Noise", 1, 0.3, 300);
    
    // 测试3：阶跃信号，高噪声
    test_adaptive_filter("Test 3: Step Signal, High Noise", 2, 0.5, 250);
    
    // 测试4：斜坡信号，低噪声
    test_adaptive_filter("Test 4: Ramp Signal, Low Noise", 3, 0.1, 200);
    
    // 测试5：脉冲信号，中等噪声
    test_adaptive_filter("Test 5: Pulse Signal, Medium Noise", 4, 0.2, 400);
    
    // 测试标定功能
    test_calibration_function();
    
    // 性能测试
    test_performance();
    
    printf("\nAll tests completed!\n");
    return 0;
}
