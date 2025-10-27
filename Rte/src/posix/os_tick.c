/***************************************************************************
 *
 * Copyright (c) 2024 OpenIBBOs. All Rights Reserved
 *
 **************************************************************************/

#include <time.h>
#include <unistd.h>
#include "os_tick.h"

#ifdef __cplusplus
extern "C" {
#endif

// 系统节拍频率定义 (1000Hz = 1ms per tick)
#define OS_TICK_FREQUENCY 1000

/**
 * @brief 毫秒转换为系统节拍
 */
uint32_t os_ms_to_ticks(uint32_t ms)
{
    // 在POSIX实现中，直接使用毫秒作为节拍单位
    return ms;
}

/**
 * @brief 系统节拍转换为毫秒
 */
uint32_t os_ticks_to_ms(uint32_t ticks)
{
    // 在POSIX实现中，节拍单位就是毫秒
    return ticks;
}

/**
 * @brief 获取当前系统节拍
 */
uint32_t os_tick_get(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    
    // 转换为毫秒
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/**
 * @brief 获取系统节拍频率
 */
uint32_t os_tick_get_frequency(void)
{
    return OS_TICK_FREQUENCY;
}

#ifdef __cplusplus
}
#endif
