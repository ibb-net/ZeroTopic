#ifndef MCU_SIM_H_
#define MCU_SIM_H_

#include <stdint.h>
#include <stddef.h>

/* 包含配置头文件，获取MCU主频配置 */
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== MCU主频配置 ==================== */
/* MCU主频配置在config.h中定义，默认180MHz */

/* MCU系统配置计算 */
#define MCU_SIM_CORE_FREQ_HZ ((uint64_t)MCU_SIM_CORE_FREQ_MHZ * 1000000ULL)  /* 主频（Hz） */
#define MCU_SIM_CORE_CLOCK_PERIOD_NS (1000000000ULL / MCU_SIM_CORE_FREQ_HZ)  /* 时钟周期（纳秒） */

/* ==================== MCU模拟函数 ==================== */

/*
 * @brief 获取MCU主频（MHz）
 * @return MCU主频（MHz）
 */
static inline uint32_t mcu_sim_get_core_freq_mhz(void) {
    return MCU_SIM_CORE_FREQ_MHZ;
}

/*
 * @brief 获取MCU主频（Hz）
 * @return MCU主频（Hz）
 */
static inline uint64_t mcu_sim_get_core_freq_hz(void) {
    return MCU_SIM_CORE_FREQ_HZ;
}

/*
 * @brief 获取MCU时钟周期（纳秒）
 * @return 时钟周期（纳秒）
 */
static inline uint64_t mcu_sim_get_clock_period_ns(void) {
    return MCU_SIM_CORE_CLOCK_PERIOD_NS;
}

/*
 * @brief 获取MCU系统位数
 * @return 系统位数（32或64）
 */
static inline uint32_t mcu_sim_get_system_bits(void) {
    return (uint32_t)MCU_SIM_SYSTEM_BITS;
}

/*
 * @brief 将时钟周期数转换为纳秒
 * @param cycles 时钟周期数
 * @return 纳秒数
 */
static inline uint64_t mcu_sim_cycles_to_ns(uint64_t cycles) {
    return cycles * MCU_SIM_CORE_CLOCK_PERIOD_NS;
}

/*
 * @brief 将纳秒数转换为时钟周期数
 * @param ns 纳秒数
 * @return 时钟周期数
 */
static inline uint64_t mcu_sim_ns_to_cycles(uint64_t ns) {
    return ns / MCU_SIM_CORE_CLOCK_PERIOD_NS;
}

/*
 * @brief 将微秒数转换为时钟周期数
 * @param us 微秒数
 * @return 时钟周期数
 */
static inline uint64_t mcu_sim_us_to_cycles(uint64_t us) {
    return mcu_sim_ns_to_cycles(us * 1000ULL);
}

/*
 * @brief 将时钟周期数转换为微秒
 * @param cycles 时钟周期数
 * @return 微秒数
 */
static inline uint64_t mcu_sim_cycles_to_us(uint64_t cycles) {
    return mcu_sim_cycles_to_ns(cycles) / 1000ULL;
}

/*
 * @brief 打印MCU配置信息
 */
void mcu_sim_print_config(void);

#ifdef __cplusplus
}
#endif

#endif /* MCU_SIM_H_ */

