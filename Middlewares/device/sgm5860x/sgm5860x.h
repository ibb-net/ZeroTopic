#ifndef __DACSgm3533_CFG_H__
#define __DACSgm3533_CFG_H__

#define KALMAN_FILTER_MODE (1)  // 0: No filter, 1: Kalman filter
#define AVG_FILTER_MODE    (0)  // 0: No filter, 1: Average filter
#define FILTER_MODE        (KALMAN_FILTER_MODE)
#if FILTER_MODE == KALMAN_FILTER_MODE
#include "kalman_filter.h"
#define AVG_MAX_CNT (3)
#elif FILTER_MODE == AVG_FILTER_MODE
#define AVG_MAX_CNT (10)
#else
#error \
    "Invalid filter mode selected. Please set FILTER_MODE to either KALMAN_FILTER_MODE or AVG_FILTER_MODE"
#endif

typedef enum {
    SGM5860xScanModeAll = 0,     // 扫描所有通道
    SGM5860xScanModeSingle40mV,  // 扫描单个通道
    SGM5860xScanModeSingle9V1,   // 扫描单个通道
    SGM5860xScanModeSingle9V2,
    SGM5860xScanModeSingle9V3,
    SGM5860xScanModeAll9V,
}SGM5860xScanMode_t;
#endif  // __DACSgm3533_CFG_H__