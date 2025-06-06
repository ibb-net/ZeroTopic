#ifndef __DMA_H
#define __DMA_H
#include <stdio.h>
#include "ex_list_dma.h"
#include "gd32h7xx.h"
#include "semphr.h"


typedef struct {
    char device_name[DEVICE_NAME_MAX];
    uint32_t rcu;
    uint32_t base_addr;
    uint32_t channel;
    uint32_t request;
    uint32_t buffer_size;
    uint32_t buffer_num;

} TypdefDMACfg;

typedef struct {
    TypdefDMACfg *TxDMACfg;
    TypdefDMACfg *RxDMACfg;

} TypdefDMAStatus;

#endif // __DMA_H