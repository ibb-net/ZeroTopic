/***************************************************************************
 *
 * Copyright (c) 2024 OpenIBBOs. All Rights Reserved
 *
 **************************************************************************/

#ifndef _OS_TICK_H_
#define _OS_TICK_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 毫秒转换为系统节拍
 * 
 * @param ms 毫秒数
 * @return uint32_t 系统节拍数
 */
uint32_t os_ms_to_ticks(uint32_t ms);

/**
 * @brief 系统节拍转换为毫秒
 * 
 * @param ticks 系统节拍数
 * @return uint32_t 毫秒数
 */
uint32_t os_ticks_to_ms(uint32_t ticks);

/**
 * @brief 获取当前系统节拍
 * 
 * @return uint32_t 当前系统节拍
 */
uint32_t os_tick_get(void);

/**
 * @brief 获取系统节拍频率(每秒节拍数)
 * 
 * @return uint32_t 系统节拍频率
 */
uint32_t os_tick_get_frequency(void);

#ifdef __cplusplus
}
#endif

#endif /* _OS_TICK_H_ */
