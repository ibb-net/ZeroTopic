#ifndef __TIMER_DECODER_H__
#define __TIMER_DECODER_H__

#include "timer_decoder_cfg.h"
typedef enum
{
    DECODER_DIRECTION_FORWARD = 0, // 正向旋转
    DECODER_DIRECTION_BACKWARD = 1 // 反向旋转

} DecoderDirectionEnum;

typedef enum
{
    DECODER_MODE_CONTINUOUS = 0, // 连续模式
    DECODER_MODE_STEP = 1,       // 步进模式

} DecoderModeEnum;


typedef struct
{
    uint8_t channel; // 通道号 0:CH0 1:CH1
    uint32_t phy_value; // 物理值
} VFBMsgDecoderStruct;



#endif // __TIMER_DECODER_H__