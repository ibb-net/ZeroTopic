#include "kalman_filter.h"
#include <math.h>
#include <string.h>

// ==================== 原始卡尔曼滤波器实现 ====================

/**
 * @brief 初始化卡尔曼滤波器
 */
void kalman_init(KalmanFilter_t *kf, double initial_value, double process_noise, double measurement_noise, double initial_covariance) {
    if (kf == NULL) return;
    
    kf->x = initial_value;          // 初始状态估计
    kf->P = initial_covariance;     // 初始误差协方差
    kf->Q = process_noise;          // 过程噪声协方差
    kf->R = measurement_noise;      // 测量噪声协方差
    kf->K = 0.0;                    // 卡尔曼增益初始化为0
}

/**
 * @brief 卡尔曼滤波更新
 */
double kalman_update(KalmanFilter_t *kf, double measurement) {
    if (kf == NULL) return measurement;
    
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
    if (kf == NULL || new_R <= 0.0) return;
    kf->R = new_R;
}

// ==================== 自适应卡尔曼滤波器实现 ====================

/**
 * @brief 初始化自适应卡尔曼滤波器
 */
void adaptive_kalman_init(AdaptiveKalmanFilter_t *akf, double initial_value, 
                         double process_noise, double measurement_noise, double initial_covariance) {
    if (akf == NULL) return;
    
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
    akf->innovation_index = 0;
    akf->measurement_index = 0;
    
    // 初始化统计参数
    akf->innovation_variance = 0.0;
    akf->innovation_mean = 0.0;
    akf->measurement_variance = 0.0;
    akf->measurement_mean = initial_value;
    
    // 初始化状态
    akf->signal_state = SIGNAL_STABLE;
    akf->previous_signal_state = SIGNAL_STABLE;
    akf->adaptation_counter = 0;
    akf->state_change_counter = 0;
    
    // 初始化性能监控
    akf->tracking_error = 0.0;
    akf->convergence_metric = 0.0;
    akf->adaptation_gain = 1.0;
    akf->stability_metric = 1.0;
    
    // 初始化自适应控制
    akf->enable_adaptation = 1;
    akf->adaptation_threshold = CONVERGENCE_THRESHOLD;
    akf->forgetting_factor = 0.95;
}

/**
 * @brief 计算数组的方差
 */
static double calculate_variance(const double *data, int size, double mean) {
    if (size <= 1) return 0.0;
    
    double variance = 0.0;
    for (int i = 0; i < size; i++) {
        double diff = data[i] - mean;
        variance += diff * diff;
    }
    return variance / (size - 1);
}

/**
 * @brief 计算数组的均值
 */
static double calculate_mean(const double *data, int size) {
    if (size <= 0) return 0.0;
    
    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        sum += data[i];
    }
    return sum / size;
}

/**
 * @brief 分析信号状态
 */
SignalState_t analyze_signal_state(AdaptiveKalmanFilter_t *akf) {
    if (akf == NULL) return SIGNAL_STABLE;
    
    // 计算测量值的统计特性
    double variance = akf->measurement_variance;
    double mean = akf->measurement_mean;
    
    // 计算最大差值
    double max_diff = 0.0;
    for (int i = 0; i < ADAPTIVE_WINDOW_SIZE - 1; i++) {
        double diff = fabs(akf->measurement_history[i] - akf->measurement_history[i + 1]);
        if (diff > max_diff) max_diff = diff;
    }
    
    // 判断信号状态
    if (variance < akf->R_base * 0.01 && max_diff < akf->R_base * 0.1) {
        return SIGNAL_STABLE;
    } else if (variance > akf->R_base * 100) {
        return SIGNAL_NOISY;
    } else if (max_diff > fabs(mean) * 0.1 && max_diff > akf->R_base * 10) {
        return SIGNAL_CHANGING;
    } else if (fabs(mean) > 0.95 * 10.0) {  // 假设满量程为10V
        return SIGNAL_SATURATED;
    }
    
    return SIGNAL_STABLE;
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
    akf->innovation_variance = calculate_variance(akf->innovation_history, INNOVATION_WINDOW_SIZE, akf->innovation_mean);
    
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
        akf->R = fmin(new_R, akf->R_max);
    } else if (akf->adaptation_gain < 0.5) {
        // 实际噪声比预期小，减少测量噪声估计
        double new_R = akf->R * (1.0 - adaptation_rate);
        akf->R = fmax(new_R, akf->R_min);
    }
    
    // 计算稳定性指标
    akf->stability_metric = 1.0 / (1.0 + fabs(akf->adaptation_gain - 1.0));
}

/**
 * @brief 基于统计特性的自适应调整
 */
void statistics_based_adaptation(AdaptiveKalmanFilter_t *akf) {
    if (akf == NULL || !akf->enable_adaptation) return;
    
    // 计算测量统计特性
    akf->measurement_mean = calculate_mean(akf->measurement_history, ADAPTIVE_WINDOW_SIZE);
    akf->measurement_variance = calculate_variance(akf->measurement_history, ADAPTIVE_WINDOW_SIZE, akf->measurement_mean);
    
    // 保存上一个状态
    akf->previous_signal_state = akf->signal_state;
    
    // 分析信号状态
    akf->signal_state = analyze_signal_state(akf);
    
    // 检测状态变化
    if (akf->signal_state != akf->previous_signal_state) {
        akf->state_change_counter++;
    }
    
    // 根据信号状态调整参数（使用渐进式调整）
    double adaptation_rate = ADAPTATION_RATE * akf->forgetting_factor;
    
    switch (akf->signal_state) {
        case SIGNAL_STABLE:
            // 信号稳定：减少过程噪声，增强平滑性
            akf->Q = akf->Q * (1.0 - adaptation_rate) + akf->Q_base * 0.5 * adaptation_rate;
            akf->R = akf->R * (1.0 - adaptation_rate) + akf->R_base * 0.8 * adaptation_rate;
            akf->Q = fmax(akf->Q, akf->Q_min);
            akf->R = fmax(akf->R, akf->R_min);
            break;
            
        case SIGNAL_CHANGING:
            // 信号变化：大幅增加过程噪声，极大提高响应速度，减小测量噪声权重
            akf->Q = akf->Q_base * 8.0; // 原来是2.0，改为8.0
            akf->R = akf->R_base * 0.5; // 原来是1.0，改为0.5
            akf->Q = fmin(akf->Q, akf->Q_max);
            akf->R = fmax(akf->R, akf->R_min);
            break;
            
        case SIGNAL_NOISY:
            // 噪声较大：减少测量权重，增强滤波
            akf->Q = akf->Q * (1.0 - adaptation_rate) + akf->Q_base * adaptation_rate;
            akf->R = akf->R * (1.0 - adaptation_rate) + akf->R_base * 3.0 * adaptation_rate;
            akf->R = fmin(akf->R, akf->R_max);
            break;
            
        case SIGNAL_SATURATED:
            // 信号饱和：增加过程噪声，减少测量权重
            akf->Q = akf->Q * (1.0 - adaptation_rate) + akf->Q_base * 5.0 * adaptation_rate;
            akf->R = akf->R * (1.0 - adaptation_rate) + akf->R_base * 2.0 * adaptation_rate;
            akf->Q = fmin(akf->Q, akf->Q_max);
            akf->R = fmin(akf->R, akf->R_max);
            break;
    }
    
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
    if (akf == NULL) return measurement;
    
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
    akf->tracking_error = fabs(innovation);
    akf->convergence_metric = akf->P;
    
    // 自适应调整
    if (akf->enable_adaptation) {
        innovation_based_adaptation(akf, innovation);
        
        // 每隔几次更新进行统计分析
        if (akf->adaptation_counter % 5 == 0) {
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
    akf->innovation_index = 0;
    akf->measurement_index = 0;
    
    akf->signal_state = SIGNAL_STABLE;
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
double kalman_update_with_calibration(KalmanFilter_t *kf, double raw_measurement, double offset, double scale) {
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
double kalman_linear_interpolation_calibration(double raw_value, const double cal_points[][2], int num_points) {
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
    
    status->current_value = akf->x;
    status->kalman_gain = akf->K;
    status->process_noise = akf->Q;
    status->measurement_noise = akf->R;
    status->covariance = akf->P;
    
    status->adaptation_gain = akf->adaptation_gain;
    status->stability_metric = akf->stability_metric;
    status->tracking_error = akf->tracking_error;
    status->convergence_metric = akf->convergence_metric;
    
    status->signal_state = akf->signal_state;
    status->adaptation_counter = akf->adaptation_counter;
    status->state_change_counter = akf->state_change_counter;
    
    status->innovation_variance = akf->innovation_variance;
    status->measurement_variance = akf->measurement_variance;
    
    status->is_adaptation_enabled = akf->enable_adaptation;
    status->forgetting_factor = akf->forgetting_factor;
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
    if (akf->Q < akf->Q_min || akf->Q > akf->Q_max ||
        akf->R < akf->R_min || akf->R > akf->R_max) {
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
void adaptive_kalman_tune_parameters(AdaptiveKalmanFilter_t *akf, double responsiveness, double stability_preference) {
    if (akf == NULL) return;
    
    // 限制参数范围
    responsiveness = fmax(0.1, fmin(1.0, responsiveness));
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
void adaptive_kalman_get_performance_stats(AdaptiveKalmanFilter_t *akf, AdaptiveKalmanPerformance_t *stats) {
    if (akf == NULL || stats == NULL) return;
    
    // 计算创新序列的统计信息
    stats->innovation_mean = akf->innovation_mean;
    stats->innovation_variance = akf->innovation_variance;
    stats->innovation_std = sqrt(akf->innovation_variance);
    
    // 计算测量序列的统计信息
    stats->measurement_mean = akf->measurement_mean;
    stats->measurement_variance = akf->measurement_variance;
    stats->measurement_std = sqrt(akf->measurement_variance);
    
    // 性能指标
    stats->adaptation_effectiveness = akf->adaptation_gain;
    stats->filter_stability = akf->stability_metric;
    stats->tracking_accuracy = 1.0 / (1.0 + akf->tracking_error);
    stats->convergence_speed = 1.0 / (1.0 + akf->convergence_metric);
    
    // 自适应状态
    stats->total_adaptations = akf->adaptation_counter;
    stats->state_changes = akf->state_change_counter;
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
void adaptive_kalman_set_bounds(AdaptiveKalmanFilter_t *akf, double Q_min, double Q_max, double R_min, double R_max) {
    if (akf == NULL) return;
    
    // 确保参数合理性
    if (Q_min > 0 && Q_max > Q_min) {
        akf->Q_min = Q_min;
        akf->Q_max = Q_max;
    }
    
    if (R_min > 0 && R_max > R_min) {
        akf->R_min = R_min;
        akf->R_max = R_max;
    }
    
    // 约束当前值在边界内
    if (akf->Q < akf->Q_min) akf->Q = akf->Q_min;
    if (akf->Q > akf->Q_max) akf->Q = akf->Q_max;
    if (akf->R < akf->R_min) akf->R = akf->R_min;
    if (akf->R > akf->R_max) akf->R = akf->R_max;
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
double adaptive_kalman_update_with_calibration(AdaptiveKalmanFilter_t *akf, double raw_measurement, double offset, double scale) {
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
double adaptive_kalman_update_with_interpolation(AdaptiveKalmanFilter_t *akf, double raw_measurement, const double cal_points[][2], int num_points) {
    if (akf == NULL || cal_points == NULL || num_points < 2) return raw_measurement;
    
    // 应用线性插值标定
    double calibrated_measurement = kalman_linear_interpolation_calibration(raw_measurement, cal_points, num_points);
    
    // 进行自适应滤波
    return adaptive_kalman_update(akf, calibrated_measurement);
}
