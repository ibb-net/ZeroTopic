#ifndef KALMAN_FILTER_H
#define KALMAN_FILTER_H

#include <stdint.h>

// 自适应卡尔曼滤波器配置参数
#define ADAPTIVE_WINDOW_SIZE 10
#define INNOVATION_WINDOW_SIZE 20
#define MAX_ADAPTATION_RATIO 10.0    // 最大自适应调整比例
#define MIN_ADAPTATION_RATIO 0.1     // 最小自适应调整比例
#define ADAPTATION_RATE 0.1          // 自适应调整速率
#define CONVERGENCE_THRESHOLD 0.001  // 收敛阈值

typedef enum {
    SIGNAL_STABLE,      // 信号稳定
    SIGNAL_CHANGING,    // 信号变化中
    SIGNAL_NOISY,       // 噪声较大
    SIGNAL_SATURATED    // 信号饱和
} SignalState_t;

// 原始卡尔曼滤波器结构体（保持兼容性）
typedef struct {
    double x;     // 状态估计值
    double P;     // 估计误差协方差
    double Q;     // 过程噪声协方差
    double R;     // 测量噪声协方差
    double K;     // 卡尔曼增益
} KalmanFilter_t;

// 自适应卡尔曼滤波器结构体
typedef struct {
    double x;     // 状态估计值
    double P;     // 估计误差协方差
    double Q;     // 过程噪声协方差
    double R;     // 测量噪声协方差
    double K;     // 卡尔曼增益
    
    // 自适应相关参数
    double Q_base;    // 基础过程噪声
    double R_base;    // 基础测量噪声
    double Q_min;     // 最小过程噪声
    double Q_max;     // 最大过程噪声
    double R_min;     // 最小测量噪声
    double R_max;     // 最大测量噪声
    
    // 创新序列分析
    double innovation_history[INNOVATION_WINDOW_SIZE];
    int innovation_index;
    double innovation_variance;
    double innovation_mean;
    
    // 统计分析
    double measurement_history[ADAPTIVE_WINDOW_SIZE];
    int measurement_index;
    double measurement_variance;
    double measurement_mean;
    
    // 信号状态
    SignalState_t signal_state;
    SignalState_t previous_signal_state;
    int adaptation_counter;
    int state_change_counter;
    
    // 精度控制参数
    int stability_counter;          // 稳定状态计数器
    int high_precision_mode;        // 高精度模式标志
    double precision_factor;        // 当前精度因子
    
    // 性能监控
    double tracking_error;
    double convergence_metric;
    double adaptation_gain;          // 自适应增益
    double stability_metric;         // 稳定性指标
    
    // 自适应控制
    int enable_adaptation;           // 是否启用自适应
    double adaptation_threshold;     // 自适应触发阈值
    double forgetting_factor;        // 遗忘因子
} AdaptiveKalmanFilter_t;

// 自适应卡尔曼滤波器状态结构体
typedef struct {
    double current_value;
    double kalman_gain;
    double process_noise;
    double measurement_noise;
    double covariance;
    
    double adaptation_gain;
    double stability_metric;
    double tracking_error;
    double convergence_metric;
    
    SignalState_t signal_state;
    int adaptation_counter;
    int state_change_counter;
    
    double innovation_variance;
    double measurement_variance;
    
    int is_adaptation_enabled;
    double forgetting_factor;
} AdaptiveKalmanStatus_t;

// 自适应卡尔曼滤波器性能统计结构体
typedef struct {
    // 创新序列统计
    double innovation_mean;
    double innovation_variance;
    double innovation_std;
    
    // 测量序列统计
    double measurement_mean;
    double measurement_variance;
    double measurement_std;
    
    // 性能指标
    double adaptation_effectiveness;
    double filter_stability;
    double tracking_accuracy;
    double convergence_speed;
    
    // 自适应状态
    int total_adaptations;
    int state_changes;
    SignalState_t current_signal_state;
    
    // 参数漂移
    double Q_drift;
    double R_drift;
} AdaptiveKalmanPerformance_t;

// 原始卡尔曼滤波器函数（保持兼容性）
void kalman_init(KalmanFilter_t *kf, double initial_value, double process_noise, double measurement_noise, double initial_covariance);
double kalman_update(KalmanFilter_t *kf, double measurement);
void kalman_set_measurement_noise(KalmanFilter_t *kf, double new_R);

// 自适应卡尔曼滤波器函数
void adaptive_kalman_init(AdaptiveKalmanFilter_t *akf, double initial_value, 
                         double process_noise, double measurement_noise, double initial_covariance);
double adaptive_kalman_update(AdaptiveKalmanFilter_t *akf, double measurement);
SignalState_t analyze_signal_state(AdaptiveKalmanFilter_t *akf);
void innovation_based_adaptation(AdaptiveKalmanFilter_t *akf, double innovation);
void statistics_based_adaptation(AdaptiveKalmanFilter_t *akf);
void adaptive_kalman_reset(AdaptiveKalmanFilter_t *akf, double new_value);

// 自适应卡尔曼滤波器扩展函数
void adaptive_kalman_set_bounds(AdaptiveKalmanFilter_t *akf, double Q_min, double Q_max, double R_min, double R_max);
void adaptive_kalman_enable_adaptation(AdaptiveKalmanFilter_t *akf, int enable);
void adaptive_kalman_set_adaptation_rate(AdaptiveKalmanFilter_t *akf, double rate);
double adaptive_kalman_get_adaptation_gain(AdaptiveKalmanFilter_t *akf);
double adaptive_kalman_get_stability_metric(AdaptiveKalmanFilter_t *akf);
SignalState_t adaptive_kalman_get_signal_state(AdaptiveKalmanFilter_t *akf);
void adaptive_kalman_set_forgetting_factor(AdaptiveKalmanFilter_t *akf, double factor);

// 带标定的自适应卡尔曼滤波器
double adaptive_kalman_update_with_calibration(AdaptiveKalmanFilter_t *akf, double raw_measurement, double offset, double scale);
double adaptive_kalman_update_with_interpolation(AdaptiveKalmanFilter_t *akf, double raw_measurement, const double cal_points[][2], int num_points);

/**
 * @brief 获取当前滤波器状态
 * @param kf 卡尔曼滤波器结构体指针
 * @return 当前状态估计值
 */
double kalman_get_value(KalmanFilter_t *kf);

/**
 * @brief 获取当前卡尔曼增益
 * @param kf 卡尔曼滤波器结构体指针
 * @return 当前卡尔曼增益
 */
double kalman_get_gain(KalmanFilter_t *kf);

/**
 * @brief 重置卡尔曼滤波器
 * @param kf 卡尔曼滤波器结构体指针
 * @param new_value 新的状态值
 */
void kalman_reset(KalmanFilter_t *kf, double new_value);

/**
 * @brief 带标定的卡尔曼滤波更新
 * @param kf 卡尔曼滤波器结构体指针
 * @param raw_measurement 原始测量值
 * @param offset 偏移校正值
 * @param scale 比例校正值
 * @return 标定并滤波后的估计值
 */
double kalman_update_with_calibration(KalmanFilter_t *kf, double raw_measurement, double offset, double scale);

/**
 * @brief 多点线性插值标定
 * @param raw_value 原始值
 * @param cal_points 标定点数组 [原始值, 标准值]
 * @param num_points 标定点数量
 * @return 标定后的值
 */
double kalman_linear_interpolation_calibration(double raw_value, const double cal_points[][2], int num_points);

/**
 * @brief 标定状态管理
 * @param kf 卡尔曼滤波器结构体指针
 * @param is_calibrating 是否正在标定
 */
void kalman_set_calibration_mode(KalmanFilter_t *kf, int is_calibrating);

// 自适应卡尔曼滤波器高级功能函数
void adaptive_kalman_get_status(AdaptiveKalmanFilter_t *akf, AdaptiveKalmanStatus_t *status);
int adaptive_kalman_self_check(AdaptiveKalmanFilter_t *akf);
void adaptive_kalman_tune_parameters(AdaptiveKalmanFilter_t *akf, double responsiveness, double stability_preference);
int adaptive_kalman_batch_process(AdaptiveKalmanFilter_t *akf, const double *input_data, double *output_data, int data_length);
void adaptive_kalman_get_performance_stats(AdaptiveKalmanFilter_t *akf, AdaptiveKalmanPerformance_t *stats);
SignalState_t adaptive_kalman_query_signal_state(AdaptiveKalmanFilter_t *akf);

// 微小波动检测函数
double adaptive_kalman_get_voltage_range(AdaptiveKalmanFilter_t *akf);
int adaptive_kalman_is_micro_fluctuation(AdaptiveKalmanFilter_t *akf);

// 精度控制函数
void adaptive_kalman_enable_high_precision(AdaptiveKalmanFilter_t *akf, int enable);
int adaptive_kalman_is_high_precision_mode(AdaptiveKalmanFilter_t *akf);
double adaptive_kalman_get_precision_factor(AdaptiveKalmanFilter_t *akf);
// 自检错误码定义
#define KALMAN_CHECK_OK                0x00    // 正常
#define KALMAN_CHECK_PARAM_ERROR       0x01    // 参数错误
#define KALMAN_CHECK_BOUNDS_ERROR      0x02    // 参数超出边界
#define KALMAN_CHECK_UNSTABLE          0x04    // 数值不稳定
#define KALMAN_CHECK_POOR_TRACKING     0x08    // 跟踪性能差
#define KALMAN_CHECK_POOR_STABILITY    0x10    // 稳定性差

// 工具宏定义
#define KALMAN_IS_STABLE(akf)          ((akf)->signal_state == SIGNAL_STABLE)
#define KALMAN_IS_CHANGING(akf)        ((akf)->signal_state == SIGNAL_CHANGING)
#define KALMAN_IS_NOISY(akf)           ((akf)->signal_state == SIGNAL_NOISY)
#define KALMAN_IS_SATURATED(akf)       ((akf)->signal_state == SIGNAL_SATURATED)
#define KALMAN_GET_ACCURACY(akf)       (1.0 / (1.0 + (akf)->tracking_error))
#define KALMAN_GET_RESPONSIVENESS(akf) ((akf)->K)
#define KALMAN_IS_CONVERGED(akf)       ((akf)->convergence_metric < CONVERGENCE_THRESHOLD)

// 微小波动检测宏
#define MICRO_FLUCTUATION_THRESHOLD    (0.0001 / 4.7)  // 0.0001/4.7V ≈ 21.3μV
#define KALMAN_IS_MICRO_FLUCTUATION(range) ((range) > 0 && (range) < MICRO_FLUCTUATION_THRESHOLD)

// 精度控制宏
#define KALMAN_IS_HIGH_PRECISION(akf)   adaptive_kalman_is_high_precision_mode(akf)
#define KALMAN_GET_PRECISION(akf)       adaptive_kalman_get_precision_factor(akf)
#define KALMAN_ENABLE_PRECISION(akf)    adaptive_kalman_enable_high_precision(akf, 1)
#define KALMAN_DISABLE_PRECISION(akf)   adaptive_kalman_enable_high_precision(akf, 0)

#endif // KALMAN_FILTER_H
