/***************************************************************************
 *
 * Copyright (c) 2024 OpenIBBOs. All Rights Reserved
 *
 **************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "os_tick.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 毫秒转换为系统节拍
 */
uint32_t os_ms_to_ticks(uint32_t ms)
{
    return (uint32_t)pdMS_TO_TICKS(ms);
}

/**
 * @brief 系统节拍转换为毫秒
 */
uint32_t os_ticks_to_ms(uint32_t ticks)
{
    return (uint32_t)(ticks * portTICK_PERIOD_MS);
}

/**
 * @brief 获取当前系统节拍
 */
uint32_t os_tick_get(void)
{
    return (uint32_t)xTaskGetTickCount();
}

/**
 * @brief 获取系统节拍频率
 */
uint32_t os_tick_get_frequency(void)
{
    return (uint32_t)configTICK_RATE_HZ;
}

#ifdef __cplusplus
}
#endif
