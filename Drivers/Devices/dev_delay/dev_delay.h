#ifndef __DEV_DELAY_H
#define __DEV_DELAY_H
// #include "semphr.h"
#include "dev_basic.h"
void DevDelayInit(void);
void TIMER3_IRQHandler(void);
void DevDelayUs(uint32_t ntime);
void DevDelayMs(uint32_t ntime);
void TIMER50_IRQHandler(void);
#endif  // __DEV_DELAY_H