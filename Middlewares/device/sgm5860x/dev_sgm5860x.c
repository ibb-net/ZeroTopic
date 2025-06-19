#define CONFIG_sgm5860x_EN 1
#if CONFIG_sgm5860x_EN
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "gd32h7xx.h"
#include "string.h"

/* Device */
#include "Device.h"
#include "dev_basic.h"
#include "dev_pin.h"
#include "dev_sgm5860x.h"
#include "dev_spi.h"
#include "elog.h"
#include "os_server.h"
// #include "app_event.h"

#define TAG            "sgm5860x"
#define sgm5860xLogLvl ELOG_LVL_INFO

#define sgm5860xPriority PriorityNormalEventGroup0
#ifndef sgm5860xChannelMax
#define sgm5860xChannelMax 4
#endif
#endif

/* =====================================================================================
 */
#define SET_RESET1_HIGH
#define SET_RESET1_LOW

#define SET_CS1_HIGH
#define SET_CS1_LOW

ai_contorl ai_Ctr;  // ADC控制结构体
/*SGM58601初始化*/
uint8_t dev_sgm5860x_init(void) {
    DevPinInit(&sgm5860xBspCfg.drdy);
    DevPinInit(&sgm5860xBspCfg.nest);
    DevPinInit(&sgm5860xBspCfg.sync);
    DevSpiInit(&sgm5860xBspCfg.spicfg);

    SET_RESET1_HIGH;
    SET_CS1_HIGH;

    /*SGM58601初始化配置*/
    dev_sgm5860x_init_config();
    return 1;
}
/*SGM58601 initialization*/
uint8_t dev_sgm5860x_init_config(void) {
    uint16_t i = 0;  // as temporary delay count

    memset(&reg_value_cache1, 0x00, sizeof(reg_value_cache1));
    memset(&reg_value_cache2, 0x00, sizeof(reg_value_cache2));

    /*calc default calibration coefficient*/
    default_cali_calc();

    // reset SGM58601
    SET_RESET1_LOW;
    for (i = 0; i < 200; i++) { /*none*/
    };  // delay 7us
    SET_RESET1_HIGH;
    DRDY1_ready_flag = 0;
    while (DRDY1_ready_flag == 0) { /*none*/
    };

    /*configuration SGM58601 status register*/
    ai_Ctr.status_reg[0] = STATUS_REG_SET_VALUE;  //   |
                                                  //   ANALOG_INPUT_BUFFER_ENABLE;
    ai_Ctr.tran_commond[0] = (ai_Ctr.status_reg[0] << 16) | ((uint32_t)OPREATION_REG_NUM_1 << 8) |
                             ((uint32_t)STATUS_REG | (uint32_t)WREG);

    spi0_transmit_data(3, (uint8_t *)&ai_Ctr.tran_commond[0]);

    /*configuration SGM58601 I/O contorl register*/
    ai_Ctr.gpio_conReg[0]  = IO_CONTORL_REG_SET_VALUE;
    ai_Ctr.tran_commond[0] = (ai_Ctr.gpio_conReg[0] << 16) | ((uint32_t)OPREATION_REG_NUM_1 << 8) |
                             ((uint32_t)IO_REG | (uint32_t)WREG);

    spi0_transmit_data(3, (uint8_t *)&ai_Ctr.tran_commond[0]);

    /*configuration SGM58601 adc contorl register*/
    ai_Ctr.ad_conReg[0]    = SET_PGA_1;
    ai_Ctr.tran_commond[0] = (ai_Ctr.ad_conReg[0] << 16) | ((uint32_t)OPREATION_REG_NUM_1 << 8) |
                             ((uint32_t)ADCON_REG | (uint32_t)WREG);

    spi0_transmit_data(3, (uint8_t *)&ai_Ctr.tran_commond[0]);

    /*configuration SGM58601 adc data rate register*/
    ai_Ctr.dataRate_reg[0] = SET_DATA_RATE_50SPS;
    ai_Ctr.tran_commond[0] = (ai_Ctr.dataRate_reg[0] << 16) | ((uint32_t)OPREATION_REG_NUM_1 << 8) |
                             ((uint32_t)DRATE_REG | (uint32_t)WREG);

    spi0_transmit_data(3, (uint8_t *)&ai_Ctr.tran_commond[0]);

    DRDY1_ready_flag = 0;
    while (DRDY1_ready_flag == 0) { /*none*/
    };

    return 1;
}


/*SGM58601 Scan*/
uint8_t sgm58601_scan(uint16_t scan_count) {
    uint32_t cache1[3] = {0, 0, 0};

    uint16_t samp_count = 0;  // Sampling count

    /*Scan channel_1*/
    for (ai_Ctr.sample_count = 0, ai_Ctr.sample_sum[0] = 0, ai_Ctr.sample_count < scan_count;
         /*count in */) {
        if (samp_count == 0) {
            /*configuration SGM58601 MUX register*/
            ai_Ctr.mux_conReg[0]   = MUX_CHN1_SET_VALUE;  // enable AD channel_1
            ai_Ctr.tran_commond[0] = (ai_Ctr.mux_conReg[0] << 16) |
                                     ((uint32_t)OPREATION_REG_NUM_1 << 8) |
                                     ((uint32_t)MUX_REG | (uint32_t)WREG);

            spi0_transmit_data(3, (uint8_t *)&ai_Ctr.tran_commond[0]);
            samp_count = 1;
        }

        /*read back data register*/
        ai_Ctr.tran_commond[0] = (uint32_t)RDATA;  // read AD channel_1

        DRDY1_ready_flag = 0;
        while (DRDY1_ready_flag == 0) { /*none*/
        };

        spi0_transmit_data(4, (uint8_t *)&ai_Ctr.tran_commond[0]);
        read_adc_data(0, &spi0_ctr.rec_data[1]);  // fill AD channel_1

        cache1[ai_Ctr.sample_count - 1] = ai_Ctr.adc_data[0];

        if (ai_Ctr.sample_count == (scan_count - 1)) {
            ai_Ctr.sample_ave[0] = medianFilter(cache1[0], cache1[1], cache1[2]);
        }
        ai_Ctr.sample_count++;
    }
}
