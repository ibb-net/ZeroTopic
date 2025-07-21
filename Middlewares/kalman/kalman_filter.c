#include "kalman_filter.h"

#include <math.h>
#include <string.h>
#include <stdio.h>
#include "elog.h"
#define TAG "kalman"

// 通用宏定义
#define KALMAN_CHECK_NULL(ptr, ret_val)      \
    do {                                     \
        if ((ptr) == NULL) return (ret_val); \
    } while (0)
#define KALMAN_CHECK_NULL_VOID(ptr) \
    do {                            \
        if ((ptr) == NULL) return;  \
    } while (0)
// #define sgm5860xLogLvl                ELOG_LVL_INFO
// ==================== 原始卡尔曼滤波器实现 ====================

/**
 * @brief 初始    // 先保存当前状态作为历史状态
    SignalState_t previous_state = akf->signal_state;

    // 分析信号状态
    akf->signal_state = analyze_signal_state(akf);

    // 检测状态变化
    if (akf->signal_state != previous_state) {
        akf->state_change_counter++;
        // elog_i(TAG, "State changed from %d to %d (counter=%d)",
        //        previous_state, akf->signal_state, akf->state_change_counter);
    }

    // 更新previous_signal_state用于下次比较
    akf->previous_signal_state = previous_state;*/
void kalman_init(KalmanFilter_t *kf, double initial_value, double process_noise,
                 double measurement_noise, double initial_covariance) {
    KALMAN_CHECK_NULL_VOID(kf);

    kf->x = initial_value;       // 初始状态估计
    kf->P = initial_covariance;  // 初始误差协方差
    kf->Q = process_noise;       // 过程噪声协方差
    kf->R = measurement_noise;   // 测量噪声协方差
    kf->K = 0.0;                 // 卡尔曼增益初始化为0
}

/**
 * @brief 卡尔曼滤波更新
 */
double kalman_update(KalmanFilter_t *kf, double measurement) {
    KALMAN_CHECK_NULL(kf, measurement);

    // 预测步骤
    double x_pred = kf->x;
    double P_pred = kf->P + kf->Q;

    // 更新步骤
    kf->K = P_pred / (P_pred + kf->R);
    kf->x = x_pred + kf->K * (measurement - x_pred);
    kf->P = (1.0 - kf->K) * P_pred;

    return kf->x;
}

/**
 * @brief 动态调整测量噪声
 */
void kalman_set_measurement_noise(KalmanFilter_t *kf, double new_R) {
    KALMAN_CHECK_NULL_VOID(kf);
    if (new_R <= 0.0) return;
    kf->R = new_R;
}

// ==================== 自适应卡尔曼滤波器实现 ====================

/**
 * @brief 初始化自适应卡尔曼滤波器
 */
void adaptive_kalman_init(AdaptiveKalmanFilter_t *akf, double initial_value, double process_noise,
                          double measurement_noise, double initial_covariance) {
    KALMAN_CHECK_NULL_VOID(akf);

    // 基本卡尔曼参数
    akf->x = initial_value;
    akf->P = initial_covariance;
    akf->Q = process_noise;
    akf->R = measurement_noise;
    akf->K = 0.0;

    // 保存基础参数
    akf->Q_base = process_noise;
    akf->R_base = measurement_noise;

    // 设置默认边界
    akf->Q_min = process_noise * MIN_ADAPTATION_RATIO;
    akf->Q_max = process_noise * MAX_ADAPTATION_RATIO;
    akf->R_min = measurement_noise * MIN_ADAPTATION_RATIO;
    akf->R_max = measurement_noise * MAX_ADAPTATION_RATIO;

    // 初始化历史记录
    memset(akf->innovation_history, 0, sizeof(akf->innovation_history));
    memset(akf->measurement_history, 0, sizeof(akf->measurement_history));
    akf->innovation_index  = 0;
    akf->measurement_index = 0;

    // 初始化统计参数
    akf->innovation_variance  = 0.0;
    akf->innovation_mean      = 0.0;
    akf->measurement_variance = 0.0;
    akf->measurement_mean     = initial_value;

    // 初始化状态
    akf->signal_state          = SIGNAL_STABLE;
    akf->previous_signal_state = SIGNAL_STABLE;
    akf->adaptation_counter    = 0;
    akf->state_change_counter  = 0;

    // 初始化性能监控
    akf->tracking_error     = 0.0;
    akf->convergence_metric = 0.0;
    akf->adaptation_gain    = 1.0;
    akf->stability_metric   = 1.0;

    // 初始化自适应控制
    akf->enable_adaptation    = 1;
    akf->adaptation_threshold = CONVERGENCE_THRESHOLD;
    akf->forgetting_factor    = 0.95;
}

/**
 * @brief 统计结果结构体
 */
typedef struct {
    double mean;
    double variance;
    double std_dev;
} StatisticsResult_t;

/**
 * @brief 计算数组的完整统计信息
 */
static StatisticsResult_t calculate_statistics(const double *data, int size) {
    StatisticsResult_t result = {0.0, 0.0, 0.0};

    if (size <= 0) return result;

    // 计算均值
    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        sum += data[i];
    }
    result.mean = sum / size;

    // 计算方差
    if (size > 1) {
        double variance_sum = 0.0;
        for (int i = 0; i < size; i++) {
            double diff = data[i] - result.mean;
            variance_sum += diff * diff;
        }
        result.variance = variance_sum / (size - 1);
        result.std_dev  = sqrt(result.variance);
    }

    return result;
}

/**
 * @brief 计算数组的均值（兼容接口）
 */
static double calculate_mean(const double *data, int size) {
    return calculate_statistics(data, size).mean;
}

/**
 * @brief 计算数组的方差（兼容接口）
 */
static double calculate_variance(const double *data, int size, double mean) {
    StatisticsResult_t stats = calculate_statistics(data, size);
    return stats.variance;
}

/**
 * @brief 计算信号变化指标
 */
static double calculate_signal_variation(AdaptiveKalmanFilter_t *akf) {
    double max_diff = 0.0;
    for (int i = 0; i < ADAPTIVE_WINDOW_SIZE - 1; i++) {
        double diff = fabs(akf->measurement_history[i] - akf->measurement_history[i + 1]);
        if (diff > max_diff) max_diff = diff;
    }
    return max_diff;
}

/**
 * @brief 计算最新测量值变化
 */
static double calculate_latest_change(AdaptiveKalmanFilter_t *akf) {
    int last_index = akf->measurement_index ? akf->measurement_index - 1 : ADAPTIVE_WINDOW_SIZE - 1;
    int prev_index = last_index ? last_index - 1 : ADAPTIVE_WINDOW_SIZE - 1;
    return fabs(akf->measurement_history[last_index] - akf->measurement_history[prev_index]);
}

/**
 * @brief 检查微小波动的专用函数
 */
static void check_micro_fluctuation(AdaptiveKalmanFilter_t *akf) {
    if (akf == NULL) return;

    static int total_micro_count = 0;
    static double last_log_time  = 0;  // 简化的时间记录

    const double micro_threshold_local = 0.0001 / 4.7;  // 0.0001/4.7V 阈值

    // 计算当前窗口的最大波动
    double max_fluctuation = 0.0;
    double min_val         = akf->measurement_history[0];
    double max_val         = akf->measurement_history[0];

    for (int i = 0; i < ADAPTIVE_WINDOW_SIZE; i++) {
        if (akf->measurement_history[i] < min_val) min_val = akf->measurement_history[i];
        if (akf->measurement_history[i] > max_val) max_val = akf->measurement_history[i];
    }
    max_fluctuation = max_val - min_val;

    // 判断是否为微小波动
    if (max_fluctuation > 0 && max_fluctuation < micro_threshold_local) {
        total_micro_count++;

        // 每检测到50次微小波动输出一次日志
        // if (total_micro_count % 50 == 0) {
        //     elog_w(TAG, "MICRO-FLUCTUATION: Count=%d, Range=%.9fV (%.3fμV), Threshold=%.9fV",
        //            total_micro_count, max_fluctuation, max_fluctuation * 1000000,
        //            micro_threshold_local);

        //     elog_i(TAG, "MICRO-STATS: Min=%.9fV, Max=%.9fV, Mean=%.9fV, Variance=%.12f", min_val,
        //            max_val, akf->measurement_mean, akf->measurement_variance);
        // }

        // 特别小的波动（小于阈值的一半）特别标记
        if (max_fluctuation < micro_threshold_local / 2) {
            // elog_w(TAG, "ULTRA-MICRO: Range=%.12fV (%.6fμV) - Extremely stable signal!",
            //        max_fluctuation, max_fluctuation * 1000000);
        }
    }
}

/**
 * @brief 调试信息输出函数
 */
static void debug_signal_analysis(AdaptiveKalmanFilter_t *akf, double latest_deviation,
                                  double latest_change, double max_variation,
                                  SignalState_t detected_state) {
    static int debug_counter             = 0;
    static int micro_fluctuation_counter = 0;
    debug_counter++;

    // 定义微小波动阈值：0.0001/4.7V ≈ 0.0000213V ≈ 0.021mV
    const double micro_threshold = 0.0001 / 4.7;  // 约 0.0000213V

    // 检测微小波动
    if (max_variation > 0 && max_variation < micro_threshold) {
        micro_fluctuation_counter++;
        // elog_i(TAG, "MICRO-FLUCTUATION[%d]: Max variation=%.9fV (%.6fmV) < threshold=%.9fV",
        //        micro_fluctuation_counter, max_variation, max_variation * 1000000,
        //        micro_threshold);

        // 每100次微小波动统计一次
        if (micro_fluctuation_counter % 100 == 0) {
            // elog_w(TAG, "MICRO-FLUCTUATION STATS: Detected %d times, Current variance=%.9f",
            //        micro_fluctuation_counter, akf->measurement_variance);
        }
    }

    // 重要事件始终输出，常规信息每20次输出一次
    int should_log = (debug_counter % 20 == 0) || (detected_state == SIGNAL_CHANGING) ||
                     (latest_deviation > 0.02) || (latest_change > 0.02) ||
                     (max_variation < micro_threshold && max_variation > 0);

    if (should_log) {
        int last_index =
            akf->measurement_index ? akf->measurement_index - 1 : ADAPTIVE_WINDOW_SIZE - 1;
        double current_value = akf->measurement_history[last_index];

        // elog_i(TAG, "[%d] Val=%.6fV, Mean=%.6fV, Dev=%.6f, Step=%.6f, Var=%.9f, State=%d",
        //        debug_counter, current_value, akf->measurement_mean, latest_deviation, latest_change,
        //        akf->measurement_variance, detected_state);

        // 如果检测到100mV以上变化，特别标记
        if (latest_deviation > 0.1 || latest_change > 0.1) {
            elog_d(TAG, "*** LARGE CHANGE DETECTED: %.1fmV deviation, %.1fmV step ***",
                   latest_deviation * 1000, latest_change * 1000);
        }

        // 显示微小波动的精确数值
        if (max_variation < micro_threshold && max_variation > 0) {
            // elog_i(TAG, "MICRO: Max_var=%.9fV (%.3fμV), Threshold=%.9fV (%.3fμV)", max_variation,
            //        max_variation * 1000000, micro_threshold, micro_threshold * 1000000);
        }
    }
}

/**
 * @brief 分析信号状态（优化版）
 */
SignalState_t analyze_signal_state(AdaptiveKalmanFilter_t *akf) {
    if (akf == NULL) return SIGNAL_STABLE;

    // 使用结构体内部的状态计数器，避免静态变量问题
    static uint8_t state_counter_map[256] = {0};  // 简单的实例映射
    uint8_t instance_id                   = ((uintptr_t)akf >> 4) & 0xFF;
    uint8_t *state_counter                = &state_counter_map[instance_id];

    double variance         = akf->measurement_variance;
    double mean             = akf->measurement_mean;
    double signal_magnitude = fmax(fabs(mean), 0.1);

    // 计算各种变化指标
    double max_variation = calculate_signal_variation(akf);
    double latest_change = calculate_latest_change(akf);
    int last_index = akf->measurement_index ? akf->measurement_index - 1 : ADAPTIVE_WINDOW_SIZE - 1;
    double current_value    = akf->measurement_history[last_index];
    double latest_deviation = fabs(current_value - mean);

    // 增加立即变化检测：最新值与前一个值的直接比较
    double immediate_change = latest_change;  // 已经是相邻两个样本的差异

    // 定义清晰的阈值（针对-1V到2.5V范围，100mV变化优化）
    const double STABLE_VARIANCE_RATIO        = 0.01;
    const double STABLE_VARIATION_RATIO       = 0.1;
    const double NOISY_VARIANCE_RATIO         = 100.0;
    const double CHANGING_DEVIATION_THRESHOLD = 0.02;   // 20mV绝对阈值，保证100mV可检测
    const double CHANGING_STEP_THRESHOLD      = 0.025;  // 25mV步进阈值
    const double CHANGING_RELATIVE_THRESHOLD  = 0.015;  // 1.5%相对阈值（对3.5V范围）
    const double SATURATION_RATIO             = 0.95;
    const double VOLTAGE_MIN                  = -1.0;  // 最小电压
    const double VOLTAGE_MAX                  = 2.5;   // 最大电压
    // const double FULL_SCALE_VOLTAGE = 3.5;             // 满量程电压范围 (暂时未使用)

    // 信号状态判断逻辑
    SignalState_t detected_state = SIGNAL_STABLE;

    // 1. 检测饱和状态（优先级最高）
    if (mean > VOLTAGE_MAX * SATURATION_RATIO || mean < VOLTAGE_MIN * SATURATION_RATIO) {
        detected_state = SIGNAL_SATURATED;
    }
    // 2. 检测显著变化（第二优先级）- 增强100mV检测
    else if (immediate_change > CHANGING_STEP_THRESHOLD ||       // 直接步进变化
             latest_deviation > CHANGING_DEVIATION_THRESHOLD ||  // 与均值的偏差
             immediate_change > 0.08 ||                          // 80mV立即变化（针对100mV）
             latest_deviation > signal_magnitude * CHANGING_RELATIVE_THRESHOLD) {
        detected_state = SIGNAL_CHANGING;
    }
    // 3. 检测高噪声状态
    else if (variance > akf->R_base * NOISY_VARIANCE_RATIO) {
        detected_state = SIGNAL_NOISY;
    }
    // 4. 检测稳定状态（增强精度检测）
    else if (variance < akf->R_base * STABLE_VARIANCE_RATIO &&
             max_variation < akf->R_base * STABLE_VARIATION_RATIO) {
        
        // 额外的稳定性检查：连续样本的变化都很小
        double consecutive_stability = 0.0;
        for (int i = 1; i < ADAPTIVE_WINDOW_SIZE; i++) {
            double step_change = fabs(akf->measurement_history[i] - akf->measurement_history[i-1]);
            consecutive_stability += step_change;
        }
        consecutive_stability /= (ADAPTIVE_WINDOW_SIZE - 1);
        
        // 只有连续变化也很小时才认为真正稳定
        if (consecutive_stability < akf->R_base * STABLE_VARIATION_RATIO * 0.5) {
            detected_state = SIGNAL_STABLE;
        }
    }

    // 调试信息输出（增强版）
    debug_signal_analysis(akf, latest_deviation, immediate_change, max_variation, detected_state);

    // 特别监控100mV级别的变化
    if (immediate_change > 0.08 || latest_deviation > 0.08) {  // 80mV以上
        elog_d(TAG, "DETECTION: immediate=%.1fmV, deviation=%.1fmV, state=%d->%d, counter=%d",
               immediate_change * 1000, latest_deviation * 1000, akf->previous_signal_state,
               detected_state, *state_counter);
    }

    // 状态持续性检查和快速收敛优化
    if (detected_state == akf->previous_signal_state) {
        (*state_counter)++;
    } else {
        *state_counter = 0;
    }

    // CHANGING状态的快速收敛检查（更严格）
    if (akf->previous_signal_state == SIGNAL_CHANGING && detected_state == SIGNAL_CHANGING &&
        akf->tracking_error < 0.001 &&  // 更严格的阈值
        immediate_change < 0.01 &&      // 确保没有新的变化
        latest_deviation < 0.01) {      // 和均值偏差很小
        detected_state = SIGNAL_STABLE;
        *state_counter = 3;  // 强制通过持续性检查
        elog_i(TAG, "Fast convergence: err=%.6f, change=%.6f, dev=%.6f", akf->tracking_error,
               immediate_change, latest_deviation);
    }

    // 状态切换策略（针对100mV突变优化）
    if (detected_state == SIGNAL_CHANGING) {
        // 100mV以上的突变立即响应（无需确认）
        if (immediate_change > 0.1 || latest_deviation > 0.1) {
            elog_d(TAG, "*** 100mV+ CHANGE: immediate=%.1fmV, dev=%.1fmV ***",
                   immediate_change * 1000, latest_deviation * 1000);
            return detected_state;
        }
        // 50-100mV变化立即响应
        if (immediate_change > 0.05 || latest_deviation > 0.05) {
            elog_d(TAG, "Major CHANGING: immediate=%.1fmV, dev=%.1fmV", immediate_change * 1000,
                   latest_deviation * 1000);
            return detected_state;
        }
        // 25-50mV中等变化（包括临界100mV）需要简单确认
        if (immediate_change > 0.025 || latest_deviation > 0.025) {
            if (*state_counter >= 1) {
                elog_d(TAG, "Medium CHANGING confirmed: immediate=%.1fmV, dev=%.1fmV",
                       immediate_change * 1000, latest_deviation * 1000);
                return detected_state;
            }
        }
        // 小于25mV的微小变化需要更多确认
        if (*state_counter >= 2) {
            elog_i(TAG, "Minor CHANGING confirmed: immediate=%.1fmV, dev=%.1fmV",
                   immediate_change * 1000, latest_deviation * 1000);
            return detected_state;
        }
    } else if (*state_counter >= 2) {
        return detected_state;  // 其他状态需要持续确认
    }

    return akf->previous_signal_state;  // 保持当前状态
}

/**
 * @brief 基于创新序列的自适应调整
 */
void innovation_based_adaptation(AdaptiveKalmanFilter_t *akf, double innovation) {
    if (akf == NULL || !akf->enable_adaptation) return;

    // 更新创新序列历史
    akf->innovation_history[akf->innovation_index] = innovation;
    akf->innovation_index = (akf->innovation_index + 1) % INNOVATION_WINDOW_SIZE;

    // 计算创新序列统计量
    akf->innovation_mean = calculate_mean(akf->innovation_history, INNOVATION_WINDOW_SIZE);
    akf->innovation_variance =
        calculate_variance(akf->innovation_history, INNOVATION_WINDOW_SIZE, akf->innovation_mean);

    // 理论创新方差 = P + R
    double theoretical_variance = akf->P + akf->R;

    // 计算自适应增益
    if (theoretical_variance > 0) {
        akf->adaptation_gain = akf->innovation_variance / theoretical_variance;
    } else {
        akf->adaptation_gain = 1.0;
    }

    // 自适应调整 R（使用遗忘因子）
    double adaptation_rate = ADAPTATION_RATE * akf->forgetting_factor;

    if (akf->adaptation_gain > 2.0) {
        // 实际噪声比预期大，增加测量噪声估计
        double new_R = akf->R * (1.0 + adaptation_rate);
        akf->R       = fmin(new_R, akf->R_max);
    } else if (akf->adaptation_gain < 0.5) {
        // 实际噪声比预期小，减少测量噪声估计
        double new_R = akf->R * (1.0 - adaptation_rate);
        akf->R       = fmax(new_R, akf->R_min);
    }

    // 计算稳定性指标
    akf->stability_metric = 1.0 / (1.0 + fabs(akf->adaptation_gain - 1.0));
}

/**
 * @brief 参数边界检查和约束函数
 */
static void constrain_parameter_bounds(AdaptiveKalmanFilter_t *akf) {
    if (akf == NULL) return;

    akf->Q = fmax(fmin(akf->Q, akf->Q_max), akf->Q_min);
    akf->R = fmax(fmin(akf->R, akf->R_max), akf->R_min);
}

/**
 * @brief 验证参数范围合理性
 */
static int validate_parameter_range(double min_val, double max_val) {
    return (min_val > 0 && max_val > min_val);
}

/**
 * @brief 参数调整通用函数
 */
static void adjust_parameters_gradual(AdaptiveKalmanFilter_t *akf, double target_Q_ratio,
                                      double target_R_ratio, double rate) {
    akf->Q = akf->Q * (1.0 - rate) + akf->Q_base * target_Q_ratio * rate;
    akf->R = akf->R * (1.0 - rate) + akf->R_base * target_R_ratio * rate;
    constrain_parameter_bounds(akf);
}

/**
 * @brief 参数调整直接函数
 */
static void adjust_parameters_direct(AdaptiveKalmanFilter_t *akf, double Q_ratio, double R_ratio) {
    akf->Q = akf->Q_base * Q_ratio;
    akf->R = akf->R_base * R_ratio;
    constrain_parameter_bounds(akf);
}

/**
 * @brief 快速收敛检测
 */
static int check_fast_convergence(AdaptiveKalmanFilter_t *akf) {
    static int convergence_counters[256] = {0};  // 简单的实例映射
    uint8_t instance_id                  = ((uintptr_t)akf >> 4) & 0xFF;
    int *counter                         = &convergence_counters[instance_id];

    // 更严格的快速收敛条件，避免过早切换
    if (akf->tracking_error < 0.001) {  // 降低到1mV，更严格
        (*counter)++;
        if (*counter >= 3) {  // 需要连续3次才确认收敛
            *counter = 0;
            return 1;  // 快速收敛
        }
    } else {
        *counter = 0;
    }
    return 0;
}

/**
 * @brief 计算稳定性系数（用于精度提升）
 */
static double calculate_stability_factor(AdaptiveKalmanFilter_t *akf) {
    // 使用结构体内部的稳定性计数器
    if (akf->signal_state == SIGNAL_STABLE) {
        akf->stability_counter++;
    } else {
        akf->stability_counter = 0;
    }
    
    // 稳定性系数：0.1(刚稳定) 到1.0(长期稳定)
    double base_stability = fmin(1.0, 0.1 + (akf->stability_counter * 0.008));
    
    // 额外考虑测量方差：方差越小，稳定性越高
    double variance_factor = 1.0;
    if (akf->measurement_variance > 0) {
        variance_factor = 1.0 / (1.0 + akf->measurement_variance * 500);
    }
    
    // 考虑跟踪误差：误差越小，稳定性越高
    double error_factor = 1.0;
    if (akf->tracking_error > 0) {
        error_factor = 1.0 / (1.0 + akf->tracking_error * 100);
    }
    
    double final_stability = base_stability * variance_factor * error_factor;
    
    // 更新精度因子
    akf->precision_factor = final_stability;
    
    return final_stability;
}

/**
 * @brief 增强稳定状态的精度调整
 */
static void enhanced_stable_adjustment(AdaptiveKalmanFilter_t *akf, double adaptation_rate) {
    double stability_factor = calculate_stability_factor(akf);
    
    // 基础精度参数
    double base_Q_ratio = 0.3;  // 从0.5降低到0.3，提高稳定性
    double base_R_ratio = 1.2;  // 从0.8增加到1.2，降低测量噪声权重
    
    // 根据稳定性动态调整
    double precision_Q_ratio = base_Q_ratio * (1.0 - stability_factor * 0.7);  // 最低可降到70%
    double precision_R_ratio = base_R_ratio * (1.0 + stability_factor * 0.5);  // 最高可增加50%
    
    // 特别针对高精度状态的优化
    if (akf->high_precision_mode && stability_factor > 0.6) {
        // 长期稳定：最大化精度
        double ultra_precision_Q = akf->Q_base * 0.02;  // 极小的过程噪声（2%）
        double ultra_precision_R = akf->R_base * 3.0;   // 增加测量噪声，强化滤波
        
        // 直接设置，不使用渐进式
        akf->Q = fmax(ultra_precision_Q, akf->Q_min);
        akf->R = fmin(ultra_precision_R, akf->R_max);
        
        // 高精度模式日志
        if (akf->stability_counter % 50 == 0) {
            elog_d(TAG, "ULTRA-PRECISION: Q=%.8f, R=%.6f, stability=%.3f, counter=%d", 
                   akf->Q, akf->R, stability_factor, akf->stability_counter);
        }
    } else if (stability_factor > 0.4) {
        // 中等稳定：增强精度
        double enhanced_Q = akf->Q_base * 0.1;  // 10%过程噪声
        double enhanced_R = akf->R_base * 2.0;  // 2倍测量噪声
        
        adjust_parameters_gradual(akf, enhanced_Q / akf->Q_base, enhanced_R / akf->R_base, adaptation_rate * 2);
    } else {
        // 正常稳定：渐进式调整
        adjust_parameters_gradual(akf, precision_Q_ratio, precision_R_ratio, adaptation_rate);
    }
    
    constrain_parameter_bounds(akf);
}

/**
 * @brief 基于统计特性的自适应调整（增强精度版）
 */
void statistics_based_adaptation(AdaptiveKalmanFilter_t *akf) {
    if (akf == NULL || !akf->enable_adaptation) return;

    // 计算测量统计特性
    akf->measurement_mean = calculate_mean(akf->measurement_history, ADAPTIVE_WINDOW_SIZE);
    akf->measurement_variance =
        calculate_variance(akf->measurement_history, ADAPTIVE_WINDOW_SIZE, akf->measurement_mean);

    // 分析信号状态
    akf->signal_state = analyze_signal_state(akf);

    double adaptation_rate = ADAPTATION_RATE * akf->forgetting_factor;

    // 状态切换日志（增强版）
    if (akf->signal_state != akf->previous_signal_state) {
        const char *state_names[] = {"STABLE", "CHANGING", "NOISY", "SATURATED"};
        elog_d(TAG, "State: %s -> %s, mean=%.4f, var=%.6f, err=%.4f",
               state_names[akf->previous_signal_state], state_names[akf->signal_state],
               akf->measurement_mean, akf->measurement_variance, akf->tracking_error);

        if (akf->previous_signal_state == SIGNAL_CHANGING && akf->signal_state == SIGNAL_STABLE) {
            elog_d(TAG, "Signal stabilized from CHANGING to STABLE - Precision enhancement starting");
            // 重置精度计数器，开始新的稳定周期
            akf->stability_counter = 0;
            akf->precision_factor = 0.1;
        }
        akf->previous_signal_state = akf->signal_state;
        akf->state_change_counter++;
    }

    // 根据信号状态调整参数（增强精度版）
    switch (akf->signal_state) {
        case SIGNAL_STABLE:
            enhanced_stable_adjustment(akf, adaptation_rate);
            break;

        case SIGNAL_CHANGING:
            // 检查快速收敛
            if (check_fast_convergence(akf)) {
                adjust_parameters_direct(akf, 0.5, 1.5);
                return;
            }
            // 正常CHANGING状态调整
            adjust_parameters_direct(akf, 100.0, 0.1);
            break;

        case SIGNAL_NOISY:
            adjust_parameters_gradual(akf, 1.0, 3.0, adaptation_rate);
            break;

        case SIGNAL_SATURATED:
            adjust_parameters_gradual(akf, 5.0, 2.0, adaptation_rate);
            break;
    }

    // 检测微小波动的独立检查
    check_micro_fluctuation(akf);

    akf->adaptation_counter++;
}
/**
 * @brief 查询当前自适应卡尔曼滤波器的信号状态
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @return 当前信号状态枚举值
 */
SignalState_t adaptive_kalman_query_signal_state(AdaptiveKalmanFilter_t *akf) {
    if (akf == NULL) return SIGNAL_STABLE;
    return akf->signal_state;
}
/**
 * @brief 自适应卡尔曼滤波更新
 */
double adaptive_kalman_update(AdaptiveKalmanFilter_t *akf, double measurement) {
    KALMAN_CHECK_NULL(akf, measurement);

    // 更新测量历史
    akf->measurement_history[akf->measurement_index] = measurement;
    akf->measurement_index = (akf->measurement_index + 1) % ADAPTIVE_WINDOW_SIZE;

    // 预测步骤
    double x_pred = akf->x;
    double P_pred = akf->P + akf->Q;

    // 计算创新序列
    double innovation = measurement - x_pred;

    // 更新步骤
    akf->K = P_pred / (P_pred + akf->R);
    akf->x = x_pred + akf->K * innovation;
    akf->P = (1.0 - akf->K) * P_pred;

    // 计算跟踪误差和收敛指标
    akf->tracking_error     = fabs(innovation);
    akf->convergence_metric = akf->P;

    // 自适应调整
    if (akf->enable_adaptation) {
        innovation_based_adaptation(akf, innovation);

        // 每隔几次更新进行统计分析
        if (akf->adaptation_counter % 1 == 0) {
            statistics_based_adaptation(akf);
        }
    }

    return akf->x;
}

/**
 * @brief 重置自适应卡尔曼滤波器
 */
void adaptive_kalman_reset(AdaptiveKalmanFilter_t *akf, double new_value) {
    if (akf == NULL) return;

    akf->x = new_value;
    akf->P = 1.0;
    akf->K = 0.0;

    // 重置自适应参数
    akf->Q = akf->Q_base;
    akf->R = akf->R_base;

    // 清空历史记录
    memset(akf->innovation_history, 0, sizeof(akf->innovation_history));
    memset(akf->measurement_history, 0, sizeof(akf->measurement_history));
    akf->innovation_index  = 0;
    akf->measurement_index = 0;

    akf->signal_state       = SIGNAL_STABLE;
    akf->adaptation_counter = 0;
}

// ==================== 原始卡尔曼滤波器扩展函数 ====================

/**
 * @brief 带标定的卡尔曼滤波更新
 * @param kf 卡尔曼滤波器结构体指针
 * @param raw_measurement 原始测量值
 * @param offset 偏移校正值
 * @param scale 比例校正值
 * @return 标定并滤波后的估计值
 */
double kalman_update_with_calibration(KalmanFilter_t *kf, double raw_measurement, double offset,
                                      double scale) {
    if (kf == NULL) return raw_measurement;

    // 先进行标定：calibrated = (raw - offset) * scale
    double calibrated_measurement = (raw_measurement - offset) * scale;

    // 再进行卡尔曼滤波
    return kalman_update(kf, calibrated_measurement);
}

/**
 * @brief 获取当前滤波器状态
 * @param kf 卡尔曼滤波器结构体指针
 * @return 当前状态估计值
 */
double kalman_get_value(KalmanFilter_t *kf) {
    if (kf == NULL) return 0.0;
    return kf->x;
}

/**
 * @brief 获取当前卡尔曼增益
 * @param kf 卡尔曼滤波器结构体指针
 * @return 当前卡尔曼增益
 */
double kalman_get_gain(KalmanFilter_t *kf) {
    if (kf == NULL) return 0.0;
    return kf->K;
}

/**
 * @brief 重置卡尔曼滤波器
 * @param kf 卡尔曼滤波器结构体指针
 * @param new_value 新的状态值
 */
void kalman_reset(KalmanFilter_t *kf, double new_value) {
    if (kf == NULL) return;

    kf->x = new_value;
    kf->P = 1.0;  // 重置协方差为初始值
    kf->K = 0.0;
}

/**
 * @brief 多点线性插值标定
 * @param raw_value 原始值
 * @param cal_points 标定点数组
 * @param num_points 标定点数量
 * @return 标定后的值
 */
double kalman_linear_interpolation_calibration(double raw_value, const double cal_points[][2],
                                               int num_points) {
    if (num_points < 2) return raw_value;

    // 查找插值区间
    for (int i = 0; i < num_points - 1; i++) {
        if (raw_value >= cal_points[i][0] && raw_value <= cal_points[i + 1][0]) {
            // 线性插值
            double x1 = cal_points[i][0];
            double y1 = cal_points[i][1];
            double x2 = cal_points[i + 1][0];
            double y2 = cal_points[i + 1][1];

            return y1 + (y2 - y1) * (raw_value - x1) / (x2 - x1);
        }
    }

    // 超出范围，使用边界值
    if (raw_value < cal_points[0][0]) {
        return cal_points[0][1];
    } else {
        return cal_points[num_points - 1][1];
    }
}

/**
 * @brief 标定状态管理
 * @param kf 卡尔曼滤波器结构体指针
 * @param is_calibrating 是否正在标定
 */
void kalman_set_calibration_mode(KalmanFilter_t *kf, int is_calibrating) {
    if (kf == NULL) return;

    static double original_Q = 0.0;
    static double original_R = 0.0;

    if (is_calibrating) {
        // 保存原始参数
        original_Q = kf->Q;
        original_R = kf->R;

        // 标定模式：降低过程噪声，提高测量噪声权重
        kf->Q = kf->Q * 0.1;  // 降低过程噪声
        kf->R = kf->R * 0.5;  // 降低测量噪声，提高测量可信度
    } else {
        // 正常模式：恢复原始参数
        if (original_Q > 0 && original_R > 0) {
            kf->Q = original_Q;
            kf->R = original_R;
        }
    }
}

// ==================== 自适应卡尔曼滤波器高级功能 ====================

/**
 * @brief 获取自适应卡尔曼滤波器的详细状态信息
 */
void adaptive_kalman_get_status(AdaptiveKalmanFilter_t *akf, AdaptiveKalmanStatus_t *status) {
    if (akf == NULL || status == NULL) return;

    status->current_value     = akf->x;
    status->kalman_gain       = akf->K;
    status->process_noise     = akf->Q;
    status->measurement_noise = akf->R;
    status->covariance        = akf->P;

    status->adaptation_gain    = akf->adaptation_gain;
    status->stability_metric   = akf->stability_metric;
    status->tracking_error     = akf->tracking_error;
    status->convergence_metric = akf->convergence_metric;

    status->signal_state         = akf->signal_state;
    status->adaptation_counter   = akf->adaptation_counter;
    status->state_change_counter = akf->state_change_counter;

    status->innovation_variance  = akf->innovation_variance;
    status->measurement_variance = akf->measurement_variance;

    status->is_adaptation_enabled = akf->enable_adaptation;
    status->forgetting_factor     = akf->forgetting_factor;
}

/**
 * @brief 自适应卡尔曼滤波器的自检功能
 */
int adaptive_kalman_self_check(AdaptiveKalmanFilter_t *akf) {
    if (akf == NULL) return -1;

    int error_code = 0;

    // 检查基本参数
    if (akf->Q <= 0 || akf->R <= 0 || akf->P < 0) {
        error_code |= 0x01;  // 参数错误
    }

    // 检查参数边界
    if (akf->Q < akf->Q_min || akf->Q > akf->Q_max || akf->R < akf->R_min || akf->R > akf->R_max) {
        error_code |= 0x02;  // 参数超出边界
    }

    // 检查数值稳定性
    if (akf->P > 1e6 || akf->K > 1.0 || akf->K < 0.0) {
        error_code |= 0x04;  // 数值不稳定
    }

    // 检查跟踪性能
    if (akf->tracking_error > 10.0 * akf->R_base) {
        error_code |= 0x08;  // 跟踪性能差
    }

    // 检查稳定性
    if (akf->stability_metric < 0.1) {
        error_code |= 0x10;  // 稳定性差
    }

    return error_code;
}

/**
 * @brief 动态调整自适应参数
 */
void adaptive_kalman_tune_parameters(AdaptiveKalmanFilter_t *akf, double responsiveness,
                                     double stability_preference) {
    if (akf == NULL) return;

    // 限制参数范围
    responsiveness       = fmax(0.1, fmin(1.0, responsiveness));
    stability_preference = fmax(0.1, fmin(1.0, stability_preference));

    // 根据响应性调整自适应速率
    double adaptation_rate = 0.05 + 0.25 * responsiveness;
    adaptive_kalman_set_adaptation_rate(akf, adaptation_rate);

    // 根据稳定性偏好调整参数边界
    double stability_factor = 1.0 + stability_preference;

    akf->Q_min = akf->Q_base * MIN_ADAPTATION_RATIO / stability_factor;
    akf->Q_max = akf->Q_base * MAX_ADAPTATION_RATIO / stability_factor;
    akf->R_min = akf->R_base * MIN_ADAPTATION_RATIO / stability_factor;
    akf->R_max = akf->R_base * MAX_ADAPTATION_RATIO / stability_factor;
}

/**
 * @brief 批量处理ADC数据
 */
int adaptive_kalman_batch_process(AdaptiveKalmanFilter_t *akf, const double *input_data,
                                  double *output_data, int data_length) {
    if (akf == NULL || input_data == NULL || output_data == NULL || data_length <= 0) {
        return -1;
    }

    int processed_count = 0;

    for (int i = 0; i < data_length; i++) {
        output_data[i] = adaptive_kalman_update(akf, input_data[i]);
        processed_count++;

        // 检查是否需要自检
        if (i % 100 == 0) {
            int check_result = adaptive_kalman_self_check(akf);
            if (check_result & 0x04) {  // 数值不稳定
                // 重置滤波器
                adaptive_kalman_reset(akf, input_data[i]);
            }
        }
    }

    return processed_count;
}

/**
 * @brief 获取自适应卡尔曼滤波器的性能统计
 */
void adaptive_kalman_get_performance_stats(AdaptiveKalmanFilter_t *akf,
                                           AdaptiveKalmanPerformance_t *stats) {
    if (akf == NULL || stats == NULL) return;

    // 计算创新序列的统计信息
    stats->innovation_mean     = akf->innovation_mean;
    stats->innovation_variance = akf->innovation_variance;
    stats->innovation_std      = sqrt(akf->innovation_variance);

    // 计算测量序列的统计信息
    stats->measurement_mean     = akf->measurement_mean;
    stats->measurement_variance = akf->measurement_variance;
    stats->measurement_std      = sqrt(akf->measurement_variance);

    // 性能指标
    stats->adaptation_effectiveness = akf->adaptation_gain;
    stats->filter_stability         = akf->stability_metric;
    stats->tracking_accuracy        = 1.0 / (1.0 + akf->tracking_error);
    stats->convergence_speed        = 1.0 / (1.0 + akf->convergence_metric);

    // 自适应状态
    stats->total_adaptations    = akf->adaptation_counter;
    stats->state_changes        = akf->state_change_counter;
    stats->current_signal_state = akf->signal_state;

    // 参数漂移
    stats->Q_drift = fabs(akf->Q - akf->Q_base) / akf->Q_base;
    stats->R_drift = fabs(akf->R - akf->R_base) / akf->R_base;
}

// ==================== 缺少的自适应卡尔曼滤波器扩展函数实现 ====================

/**
 * @brief 设置自适应卡尔曼滤波器的参数边界
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @param Q_min 最小过程噪声
 * @param Q_max 最大过程噪声
 * @param R_min 最小测量噪声
 * @param R_max 最大测量噪声
 */
void adaptive_kalman_set_bounds(AdaptiveKalmanFilter_t *akf, double Q_min, double Q_max,
                                double R_min, double R_max) {
    if (akf == NULL) return;

    // 验证并设置参数边界
    if (validate_parameter_range(Q_min, Q_max)) {
        akf->Q_min = Q_min;
        akf->Q_max = Q_max;
    }

    if (validate_parameter_range(R_min, R_max)) {
        akf->R_min = R_min;
        akf->R_max = R_max;
    }

    // 约束当前值在边界内
    constrain_parameter_bounds(akf);
}

/**
 * @brief 启用或禁用自适应功能
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @param enable 1启用，0禁用
 */
void adaptive_kalman_enable_adaptation(AdaptiveKalmanFilter_t *akf, int enable) {
    if (akf == NULL) return;
    akf->enable_adaptation = enable;
}

/**
 * @brief 设置自适应调整速率
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @param rate 自适应速率 (0.01-1.0)
 */
void adaptive_kalman_set_adaptation_rate(AdaptiveKalmanFilter_t *akf, double rate) {
    if (akf == NULL || rate <= 0 || rate > 1.0) return;
    akf->adaptation_threshold = rate;
}

/**
 * @brief 获取当前自适应增益
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @return 自适应增益值
 */
double adaptive_kalman_get_adaptation_gain(AdaptiveKalmanFilter_t *akf) {
    if (akf == NULL) return 0.0;
    return akf->adaptation_gain;
}

/**
 * @brief 获取稳定性指标
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @return 稳定性指标值
 */
double adaptive_kalman_get_stability_metric(AdaptiveKalmanFilter_t *akf) {
    if (akf == NULL) return 0.0;
    return akf->stability_metric;
}

/**
 * @brief 获取当前信号状态
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @return 信号状态枚举值
 */
SignalState_t adaptive_kalman_get_signal_state(AdaptiveKalmanFilter_t *akf) {
    if (akf == NULL) return SIGNAL_STABLE;
    return akf->signal_state;
}

/**
 * @brief 设置遗忘因子
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @param factor 遗忘因子 (0.9-1.0)
 */
void adaptive_kalman_set_forgetting_factor(AdaptiveKalmanFilter_t *akf, double factor) {
    if (akf == NULL || factor < 0.5 || factor > 1.0) return;
    akf->forgetting_factor = factor;
}

/**
 * @brief 带标定的自适应卡尔曼滤波更新
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @param raw_measurement 原始测量值
 * @param offset 偏移校正值
 * @param scale 比例校正值
 * @return 标定并滤波后的估计值
 */
double adaptive_kalman_update_with_calibration(AdaptiveKalmanFilter_t *akf, double raw_measurement,
                                               double offset, double scale) {
    if (akf == NULL || scale == 0.0) return raw_measurement;

    // 应用标定
    double calibrated_measurement = (raw_measurement + offset) * scale;

    // 进行自适应滤波
    return adaptive_kalman_update(akf, calibrated_measurement);
}

/**
 * @brief 带多点插值标定的自适应卡尔曼滤波更新
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @param raw_measurement 原始测量值
 * @param cal_points 标定点数组
 * @param num_points 标定点数量
 * @return 标定并滤波后的估计值
 */
double adaptive_kalman_update_with_interpolation(AdaptiveKalmanFilter_t *akf,
                                                 double raw_measurement,
                                                 const double cal_points[][2], int num_points) {
    if (akf == NULL || cal_points == NULL || num_points < 2) return raw_measurement;

    // 应用线性插倿标定
    double calibrated_measurement =
        kalman_linear_interpolation_calibration(raw_measurement, cal_points, num_points);

    // 进行自适应滤波
    return adaptive_kalman_update(akf, calibrated_measurement);
}

/**
 * @brief 获取当前电压波动范围
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @return 电压波动范围（最大值-最小值）
 */
double adaptive_kalman_get_voltage_range(AdaptiveKalmanFilter_t *akf) {
    KALMAN_CHECK_NULL(akf, 0.0);

    double min_val = akf->measurement_history[0];
    double max_val = akf->measurement_history[0];

    for (int i = 1; i < ADAPTIVE_WINDOW_SIZE; i++) {
        if (akf->measurement_history[i] < min_val) min_val = akf->measurement_history[i];
        if (akf->measurement_history[i] > max_val) max_val = akf->measurement_history[i];
    }

    return max_val - min_val;
}

/**
 * @brief 判断当前是否为微小波动状态
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @return 1-是微小波动，0-不是微小波动
 */
int adaptive_kalman_is_micro_fluctuation(AdaptiveKalmanFilter_t *akf) {
    KALMAN_CHECK_NULL(akf, 0);

    double range = adaptive_kalman_get_voltage_range(akf);
    return KALMAN_IS_MICRO_FLUCTUATION(range);
}

/**
 * @brief 启用/禁用高精度模式
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @param enable 1-启用，0-禁用
 */
void adaptive_kalman_enable_high_precision(AdaptiveKalmanFilter_t *akf, int enable) {
    KALMAN_CHECK_NULL_VOID(akf);
    akf->high_precision_mode = enable;
    if (!enable) {
        akf->stability_counter = 0;
        akf->precision_factor = 1.0;
        elog_i(TAG, "High precision mode disabled");
    } else {
        elog_i(TAG, "High precision mode enabled");
    }
}

/**
 * @brief 检查是否处于高精度模式
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @return 1-高精度模式，0-正常模式
 */
int adaptive_kalman_is_high_precision_mode(AdaptiveKalmanFilter_t *akf) {
    KALMAN_CHECK_NULL(akf, 0);
    return akf->high_precision_mode && (akf->precision_factor > 0.7);
}

/**
 * @brief 获取当前精度因子
 * @param akf 自适应卡尔曼滤波器结构体指针
 * @return 精度因子 (0.1-1.0)
 */
double adaptive_kalman_get_precision_factor(AdaptiveKalmanFilter_t *akf) {
    KALMAN_CHECK_NULL(akf, 1.0);
    return akf->precision_factor;
}
