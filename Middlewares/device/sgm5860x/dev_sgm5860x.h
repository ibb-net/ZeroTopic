/***********************************************************************************
*说 明：此C文件为圣邦威58601-AD驱动头文件
*创 建：MaJinWen
*时 间：2023-03-09
*版 本：V_1.0.0
*修 改：none
************************************************************************************/
#ifndef __SGM58601_H__
#define __SGM58601_H__


#define  SET_CS1_HIGH       do{GPIO_BOP(GPIOA) = GPIO_PIN_4;}while(0)       
#define  SET_CS1_LOW        do{GPIO_BC(GPIOA) = GPIO_PIN_4;}while(0)        
#define  SET_RESET1_HIGH    do{GPIO_BOP(GPIOB) = GPIO_PIN_1;}while(0)       //pull up RESET1
#define  SET_RESET1_LOW     do{GPIO_BC(GPIOB) = GPIO_PIN_1;}while(0)        //pull down RESET1

#define  SET_CS2_HIGH       do{GPIO_BOP(GPIOC) = GPIO_PIN_6;}while(0)     
#define  SET_CS2_LOW        do{GPIO_BC(GPIOC) = GPIO_PIN_6;}while(0)       
#define  SET_RESET2_HIGH    do{GPIO_BOP(GPIOB) = GPIO_PIN_2;}while(0)       //pull up RESET2
#define  SET_RESET2_LOW     do{GPIO_BC(GPIOB) = GPIO_PIN_2;}while(0)        //pull down RESET2

#define  READ_REG(regAddr)      ((uint32_t)0x00100000 | ((uint32_t)regAddr<<8))   //读寄存器地址
#define  WRITE_REG(regAddr)     ((uint32_t)0x00500000 | ((uint32_t)regAddr<<8))   //写寄存器地址

/*The number of operation registers*/
#define  OPREATION_REG_NUM_1            0x01        //操作一个寄存器
#define  OPREATION_REG_NUM_2            0x02        //操作两个寄存器
#define  OPREATION_REG_NUM_3            0x03        //操作三个寄存器
#define  OPREATION_REG_NUM_4            0x04        //操作四个寄存器

/*SGM58601寄存器地址*/
#define  STATUS_REG                     0x00         //状态寄存器
#define  MUX_REG                        0x01         //多路输入复用控制器寄存器
#define  ADCON_REG                      0x02         //A/D控制器寄存器
#define  DRATE_REG                      0x03         //A/D速率控制器
#define  IO_REG                         0x04         //GPIO控制寄存器
#define  OFC0_REG                       0x05         //偏移校准字节0-最小显著字节寄存器
#define  OFC1_REG                       0x06         //偏移校准字节1
#define  OFC2_REG                       0x07         //偏移校准字节2-最高显著字节寄存器
#define  FSC0_REG                       0x08         //全缩放校准字节0-最小显著字节
#define  FSC1_REG                       0x09         //全缩放校准字节1
#define  FSC2_REG                       0x0A         //全缩放校准字节2-最高显著字节
#define  STATUS1_REG                    0x0B         //状态1寄存器

/*SGM58601命令*/
#define  WAKEUP                         0x00         //完成同步模式并退出备用模式
#define  RDATA                          0x01         //read data
#define  RDATAC                         0x03         //continuous read data
#define  SDATAC                         0x0F         //stop continuous read data
#define  RREG                           0x10         //读reg寄存器
#define  WREG                           0x50         //写reg寄存器
#define  SELFCAL                        0xF0         //偏移和增益自校准
#define  SELFOCAL                       0xF1         //偏移量自校准
#define  SELFGCAL                       0xF2         //增益自校准
#define  SYSOCAL                        0xF3         //系统偏移校准
#define  SYSGCAL                        0xF4         //系统增益校准
#define  SYNC                           0xFC         //同步A/D转换
#define  STANDBY                        0xFD         //开始待机模式
#define  CHIP_RESET                     0xFE         //重置为通电值

/*SGM58601 auto-Calibration bit*/
#define AUTO_CALIBRATION_ENABLE         0x04

/*SGM58601 analog input buffer enable bit*/
#define ANALOG_INPUT_BUFFER_ENABLE      0x02
/*SGM58601 buffer control*/
#define REF_BUFP_ENABLE                 0x08        //Positive Reference Input Buffer Enable
#define REF_BUFM_ENABLE                 0x04        //Negative Reference Input Buffer Enable
#define AIN_BUFP_ENABLE                 0x02        //Positive Analog Input Buffer Enable
#define AIN_BUFM_ENABLE                 0x01        //Negative Analog Input Buffer Enable

/*SGM58601 PGA configuration*/
#define SET_PGA_1                       0x00
#define SET_PGA_2                       0x01
#define SET_PGA_4                       0x02
#define SET_PGA_8                       0x03
#define SET_PGA_16                      0x04
#define SET_PGA_32                      0x05
#define SET_PGA_64                      0x06
#define SET_PGA_128                     0x07

/*SGM58601 data rate configuration*/
#define SET_DATA_RATE_60000SPS          0xF1
#define SET_DATA_RATE_30000SPS          0xF0
#define SET_DATA_RATE_15000SPS          0xE0
#define SET_DATA_RATE_7500SPS           0xD0
#define SET_DATA_RATE_3750SPS           0xC0
#define SET_DATA_RATE_2000SPS           0xB0
#define SET_DATA_RATE_1000SPS           0xA1
#define SET_DATA_RATE_500SPS            0x92
#define SET_DATA_RATE_100SPS            0x82
#define SET_DATA_RATE_60SPS             0x72
#define SET_DATA_RATE_50SPS             0x63
#define SET_DATA_RATE_30SPS             0x53
#define SET_DATA_RATE_25SPS             0x43
#define SET_DATA_RATE_15SPS             0x33
#define SET_DATA_RATE_10SPS             0x23
#define SET_DATA_RATE_5SPS              0x13
#define SET_DATA_RATE_2_5SPS            0x03

/*SGM58601 Register configuration*/
#define  STATUS_REG_SET_VALUE           0x00    //status register
#define  ADC_CONTORL_REG_SET_VALUE      0x00    //ADC control register(PGA=1)
#define  AD_DATA_RATE_REG_SET_VALUE     0x00    //ADC data rate register(50SPS)
#define  IO_CONTORL_REG_SET_VALUE       0xF0    //I/O contorl register
#define  STATUS1_REG_SET_VALUE          0x00    //status1 register

/*SGM58601 MUX register configuration*/
#define  MUX_CHN1_SET_VALUE             0x78    //0x08 Indicates the conversion of AD channel 1
#define  MUX_CHN2_SET_VALUE             0x68    //0x18 Indicates the conversion of AD channel 2
#define  MUX_CHN3_SET_VALUE             0x58    //0x28 Indicates the conversion of AD channel 3
#define  MUX_CHN4_SET_VALUE             0x48    //0x38 Indicates the conversion of AD channel 4
#define  MUX_CHN5_SET_VALUE             0x38    //0x48 Indicates the conversion of AD channel 5
#define  MUX_CHN6_SET_VALUE             0x28    //0x58 Indicates the conversion of AD channel 6
#define  MUX_CHN7_SET_VALUE             0x18    //0x68 Indicates the conversion of AD channel 7
#define  MUX_CHN8_SET_VALUE             0x08    //0x78 Indicates the conversion of AD channel 8
#endif /*__SGM58601_H__*/
