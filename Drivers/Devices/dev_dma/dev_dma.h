#ifndef __DEV_DMA_H
#define __DEV_DMA_H
// #include "semphr.h"
#include "dev_basic.h"

typedef struct
{
    uint32_t base;
    uint32_t channel;  // DMA channel
    uint32_t request;  // DMA request source
    dma_single_data_parameter_struct dma_cfg;
} DevDMAHandleStruct;

void DevDmaInit(const DevDMAHandleStruct * ptrDevDmaHandle);



#endif // __DEV_DMA_H