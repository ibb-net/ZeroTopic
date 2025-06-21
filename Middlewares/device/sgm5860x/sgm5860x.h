#ifndef __DACSgm3533_CFG_H__
#define __DACSgm3533_CFG_H__

// define commands 
#define SGM58601_CMD_WAKEUP   0x00 
#define SGM58601_CMD_RDATA    0x01 
#define SGM58601_CMD_RDATAC   0x03 
#define SGM58601_CMD_SDATAC   0x0f 
#define SGM58601_CMD_RREG     0x10 
#define SGM58601_CMD_WREG     0x50 
#define SGM58601_CMD_SELFCAL  0xf0 
#define SGM58601_CMD_SELFOCAL 0xf1 
#define SGM58601_CMD_SELFGCAL 0xf2 
#define SGM58601_CMD_SYSOCAL  0xf3 
#define SGM58601_CMD_SYSGCAL  0xf4 
#define SGM58601_CMD_SYNC     0xfc 
#define SGM58601_CMD_STANDBY  0xfd 
#define SGM58601_CMD_REST    0xfe 
 
//define the SGM58601 register values 
#define SGM58601_STATUS       0x00   
#define SGM58601_MUX          0x01   
#define SGM58601_ADCON        0x02   
#define SGM58601_DRATE        0x03   
#define SGM58601_IO           0x04   
#define SGM58601_OFC0         0x05   
#define SGM58601_OFC1         0x06   
#define SGM58601_OFC2         0x07   
#define SGM58601_FSC0         0x08   
#define SGM58601_FSC1         0x09   
#define SGM58601_FSC2         0x0A 
 
 
//define multiplexer codes 
#define SGM58601_MUXP_AIN0   0x00 
#define SGM58601_MUXP_AIN1   0x10 
#define SGM58601_MUXP_AIN2   0x20 
#define SGM58601_MUXP_AIN3   0x30 
#define SGM58601_MUXP_AIN4   0x40 
#define SGM58601_MUXP_AIN5   0x50 
#define SGM58601_MUXP_AIN6   0x60 
#define SGM58601_MUXP_AIN7   0x70 
#define SGM58601_MUXP_AINCOM 0x80 
 
#define SGM58601_MUXN_AIN0   0x00 
#define SGM58601_MUXN_AIN1   0x01 
#define SGM58601_MUXN_AIN2   0x02 
#define SGM58601_MUXN_AIN3   0x03 
#define SGM58601_MUXN_AIN4   0x04 
#define SGM58601_MUXN_AIN5   0x05 
#define SGM58601_MUXN_AIN6   0x06 
#define SGM58601_MUXN_AIN7   0x07 
#define SGM58601_MUXN_AINCOM 0x08   
 
 
//define gain codes 
#define SGM58601_GAIN_1      0x00 
#define SGM58601_GAIN_2      0x01 
#define SGM58601_GAIN_4      0x02 
#define SGM58601_GAIN_8      0x03 
#define SGM58601_GAIN_16     0x04 
#define SGM58601_GAIN_32     0x05 
#define SGM58601_GAIN_64     0x06 
//#define SGM58601_GAIN_64     0x07 
 
//define drate codes 
#define SGM58601_DRATE_30000SPS   0xF0 
#define SGM58601_DRATE_15000SPS   0xE0 
#define SGM58601_DRATE_7500SPS   0xD0 
#define SGM58601_DRATE_3750SPS   0xC0 
#define SGM58601_DRATE_2000SPS   0xB0 
#define SGM58601_DRATE_1000SPS   0xA1 
#define SGM58601_DRATE_500SPS    0x92 
#define SGM58601_DRATE_100SPS    0x82 
#define SGM58601_DRATE_60SPS     0x72 
#define SGM58601_DRATE_50SPS     0x63 
#define SGM58601_DRATE_30SPS     0x53 
#define SGM58601_DRATE_25SPS     0x43 
#define SGM58601_DRATE_15SPS     0x33 
#define SGM58601_DRATE_10SPS     0x23 
#define SGM58601_DRATE_5SPS      0x13 
#define SGM58601_DRATE_2_5SPS    0x03

void SGM58601_Gpio_Init(void);												//SGM58601 GPIO初始化

void sgm5860xWriteReg(unsigned char regaddr,unsigned char databyte);			//SGM58601A 写寄存器
void SGM58601BWREG(unsigned char regaddr,unsigned char databyte);			//SGM58601B 写寄存器

// signed int sgm5860xReadReg(unsigned char channel);						//SGM58601 读数据
signed int SGM58601BReadData(unsigned char channel);						//SGM58601 读数据

void sgm5860xInit(void);													//SGM58601A 单端采集初始化
void SGM58601B_Init(void);													//SGM58601B	差分采集初始化

int  sgm5860xReadSingleData(unsigned char channel);						//SGM58601A 单端ADC读取
int  sgm5860xReadDiffData(void);											//SGM58601B 差分ADC读取

#endif  // __DACSgm3533_CFG_H__