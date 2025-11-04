#include "mcu_sim.h"
#include "../../Rte/inc/os_printf.h"

/*
 * @brief 打印MCU配置信息
 */
void mcu_sim_print_config(void) {
    os_printf("\n=== MCU模拟环境配置 ===\n");
    os_printf("主频: %u MHz (%llu Hz)\n", 
              MCU_SIM_CORE_FREQ_MHZ, (unsigned long long)MCU_SIM_CORE_FREQ_HZ);
    os_printf("系统位数: %u位\n", MCU_SIM_SYSTEM_BITS);
    os_printf("时钟周期: %llu ns\n", (unsigned long long)MCU_SIM_CORE_CLOCK_PERIOD_NS);
    os_printf("======================\n\n");
}

