
#define CONFIG_DEV_ONEWRIE_EN 1
#if CONFIG_DEV_ONEWRIE_EN
#include "dev_basic.h"
#include "dev_delay.h"
#include "dev_onewire.h"
#include "dev_pin.h"
#include "dev_uart.h"
#include "elog.h"
#include "gd32h7xx.h"
#include "os_server.h"
#include "stdio.h"
#include "string.h"
/* Device */
#include "Device.h"
#define TAG            "OW_Uart"
#define RESET_BAUDRATE 9600
#define DATA_BAUDRATE  115200

#define MAX_ONEWIRE_DEVICES 8
#define ONEWIRE_RESET_DATA  0xF0

#define ONE_WIRE_BUFFER_SIZE 128
#define ONE_WIRE_HIGH_BIT     0xFF
#define ONE_WIRE_LOW_BIT     0x00  // 0x00
#define ONE_WIRE_STOP_BIT    USART_STB_1BIT
// DMA缓冲区

__attribute__((aligned(32))) uint8_t ow_tx_buffer[ONE_WIRE_BUFFER_SIZE];
__attribute__((aligned(32))) uint8_t ow_rx_buffer[ONE_WIRE_BUFFER_SIZE] = {0};
static volatile uint8_t tx_complete;
static volatile uint8_t rx_complete;
static volatile uint8_t transfer_error;
__IO FlagStatus g_transfer_complete = RESET;

/*!
    \brief      this function handles DMA_Channel0_IRQHandler exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DMA0_Channel3_IRQHandler(void) {
    if (RESET != dma_interrupt_flag_get(DMA0, DMA_CH3, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(DMA0, DMA_CH3, DMA_INT_FLAG_FTF);
        g_transfer_complete = SET;
        printf("\r\n[INFO]OneWire DMA RX complete, data received: %02X\r\n", ow_rx_buffer[0]);
    }
}

/*!
    \brief      this function handles DMA_Channel1_IRQHandler exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DMA1_Channel3_IRQHandler(void) {
    if (RESET != dma_interrupt_flag_get(DMA1, DMA_CH3, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(DMA1, DMA_CH3, DMA_INT_FLAG_FTF);
        printf("\r\n[INFO]OneWire DMA TX complete, data sent: %02X\r\n", ow_tx_buffer[0]);
        g_transfer_complete = SET;
    }
}
/*!
    \brief      initialize the USART configuration of the com
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ow_usart_init(void) {
    /* 使能GPIOA和UART3时钟 */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_UART3);

    /* 配置PA0为UART3_TX/RX (单线模式) */
    gpio_af_set(GPIOA, GPIO_AF_8, GPIO_PIN_0);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_100_220MHZ, GPIO_PIN_0);
    /* USART3初始化 */
    usart_deinit(UART3);
    usart_word_length_set(UART3, USART_WL_8BIT);
    usart_stop_bit_set(UART3, ONE_WIRE_STOP_BIT);
    usart_parity_config(UART3, USART_PM_NONE);
    usart_hardware_flow_rts_config(UART3, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(UART3, USART_CTS_DISABLE);
    usart_baudrate_set(UART3, RESET_BAUDRATE);

    /* 配置为单线半双工模式 */
    usart_halfduplex_enable(UART3);
    usart_transmit_config(UART3, USART_TRANSMIT_ENABLE);
    usart_receive_config(UART3, USART_RECEIVE_ENABLE);

    /* 使能USART3 */
    usart_enable(UART3);
}
void onewire_usart_init(void) {
    ow_usart_init();
    // 10. 清除缓冲区和标志
    memset(ow_tx_buffer, 0, sizeof(ow_tx_buffer));
    memset(ow_rx_buffer, 0, sizeof(ow_rx_buffer));
    tx_complete    = 0;
    rx_complete    = 0;
    transfer_error = 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), onewire_init,
                 onewire_usart_init, onewire init);
#if 1 /* onewire_dma_init */

void ow_uart_dma_send(uint8_t *data, uint8_t len) {
    rcu_periph_clock_enable(RCU_DMA1);
    /* enable DMAMUX clock */
    rcu_periph_clock_enable(RCU_DMAMUX);
    dma_single_data_parameter_struct dma_init_struct;
    memset(ow_tx_buffer, 0, sizeof(ow_tx_buffer));
    memcpy(ow_tx_buffer, data, len);
    SCB_CleanDCache_by_Addr((uint32_t *)ow_tx_buffer, len);

    // ================ DMA1 CH2: UART3发送通道配置 ================
    dma_deinit(DMA1, DMA_CH3);
    dma_single_data_para_struct_init(&dma_init_struct);
    dma_init_struct.request             = DMA_REQUEST_UART3_TX;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr        = (uint32_t)ow_tx_buffer;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number              = len;
    dma_init_struct.periph_addr         = (uint32_t)&USART_TDATA(UART3);
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority            = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA1, DMA_CH3, &dma_init_struct);

    dma_circulation_disable(DMA1, DMA_CH3);
    dma_channel_enable(DMA1, DMA_CH3);
    usart_dma_transmit_config(UART3, USART_TRANSMIT_DMA_ENABLE);
    while (RESET == dma_flag_get(DMA1, DMA_CH3, DMA_FLAG_FTF)) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // 清除发送完成标志
    dma_flag_clear(DMA1, DMA_CH3, DMA_FLAG_FTF);
}

void ow_uart_dma_recv(uint8_t *data, size_t len) {
    dma_deinit(DMA0, DMA_CH3);
    dma_single_data_parameter_struct dma_init_struct;
    SCB_CleanDCache_by_Addr((uint32_t *)data, len);
    dma_init_struct.request             = DMA_REQUEST_UART3_RX;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr        = (uint32_t)data;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number              = len;
    dma_init_struct.periph_addr         = (uint32_t)(&USART_RDATA(UART3));
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority            = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH3, &dma_init_struct);

    /* configure DMA mode */
    dma_circulation_disable(DMA0, DMA_CH3);
    // dma_interrupt_enable(DMA0, DMA_CH3, DMA_CHXCTL_FTFIE);
    dma_channel_enable(DMA0, DMA_CH3);
    /* USART DMA enable for reception */
    usart_dma_receive_config(UART3, USART_RECEIVE_DMA_ENABLE);
    SCB_InvalidateDCache_by_Addr((uint32_t *)data, len);
}
#endif /* onewire_dma_init */
static void switch_baudrate(uint32_t baudrate) {
    while (!usart_flag_get(UART3, USART_FLAG_TC)) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    usart_disable(UART3);
    usart_baudrate_set(UART3, baudrate);
    usart_enable(UART3);
    vTaskDelay(pdMS_TO_TICKS(2));
    // for (volatile int i = 0; i < 1000; i++);
}
uint8_t onewire_reset(void) {
    switch_baudrate(RESET_BAUDRATE);
    uint8_t buffer_tmp[1] = {ONEWIRE_RESET_DATA};
    ow_uart_dma_recv(ow_rx_buffer, 1);
    ow_uart_dma_send(buffer_tmp, 1);
    vTaskDelay(pdMS_TO_TICKS(1));

    uint8_t presence = (ow_rx_buffer[0] != 0xF0) /*  && (ow_rx_buffer[0] < ONEWIRE_RESET_DATA )*/;
    if (presence) {
        // elog_i(TAG, "onewire_reset: Pass ,Rcv %2X", ow_rx_buffer[0]);
    } else {
        elog_i(TAG, "onewire_reset: Failed ,Rcv %2X", ow_rx_buffer[0]);
    }

    return presence;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), onewire_reset,
                 onewire_reset, onewire reset);

int onewire_write_byte(uint8_t *data, uint8_t len) {
    switch_baudrate(DATA_BAUDRATE);
    uint8_t tmp[ONE_WIRE_BUFFER_SIZE] = {0};
    if (data == NULL || len == 0) {
        return -1;
    }

    for (int i = 0; i < len; i++) {
        for (int j = 0; j < 8; j++) {
            tmp[i * 8 + j] = (data[i] & (1 << j)) ? ONE_WIRE_HIGH_BIT : 0x00;
        }
    }
    ow_uart_dma_send(tmp, len * 8);
    return 0;
}
int onewire_read_byte(uint8_t *data, uint8_t len) {
    switch_baudrate(DATA_BAUDRATE);
    if (data == NULL || len == 0) {
        return -1;
    }
    uint8_t tmp[256] = {0};
    uint8_t read_tmp_byte[64];
    memset(tmp, 0xFF, sizeof(tmp));
    ow_uart_dma_recv(ow_rx_buffer, len * 8);
    ow_uart_dma_send(tmp, len * 8);

    // LSB解析: ow_rx_buffer每个字节小于0xFF则为0，否则为1，按位组装成data
    for (int i = 0; i < len; i++) {
        uint8_t value = 0;
        for (int j = 0; j < 8; j++) {
            value |= (ow_rx_buffer[i * 8 + j] < ONE_WIRE_HIGH_BIT ? 0 : 1) << j;
        }
        data[i] = value;
    }
    // 打印ow_rx_buffer八个一组，并计算出每组拼包后的单总线字节
    // for (int i = 0; i < len; i++) {
    //     uint8_t value = 0;
    //     printf("[Group %d] Raw: ", i);
    //     for (int j = 0; j < 8; j++) {
    //         printf("%02X ", ow_rx_buffer[i * 8 + j]);
    //         value |= (ow_rx_buffer[i * 8 + j] < ONE_WIRE_HIGH_BIT ? 0 : 1) << j;
    //     }
    //     printf("-> OneWire Byte: %02X\r\n", value);
    // }
    // printf("\r\n");

    return 0;
}

int onewire_rom(uint8_t *rom_code) {
    uint8_t rom_tmp[ONEWIRE_ROM_SIZE] = {0};
    onewire_reset();  // Reset the bus before reading ROM
    uint8_t cmd = ONEWIRE_READ_ROM_CMD;
    onewire_write_byte(&cmd, 1);
    onewire_read_byte(rom_tmp, ONEWIRE_ROM_SIZE);  // Read ROM code
    elog_i(TAG, "OneWire ROM code: %02X %02X %02X %02X %02X %02X %02X %02X", rom_tmp[0], rom_tmp[1],
           rom_tmp[2], rom_tmp[3], rom_tmp[4], rom_tmp[5], rom_tmp[6], rom_tmp[7]);
    memcpy(rom_code, rom_tmp, ONEWIRE_ROM_SIZE);
    return 0;  // Read ROM successful
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), onewire_rom,
                 onewire_rom, onewire rom);

void onewire_convert_temperature(void) {
    onewire_reset();
    uint8_t cmd[2] = {0xCC, 0x44};
    onewire_write_byte(cmd, 2);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
                 onewire_convert_temperature, onewire_convert_temperature,
                 onewire convert temperature);

double onewire_read_temperature(void) {
    onewire_reset();
    uint8_t cmd[2] = {0xCC, 0xBE};
    onewire_write_byte(cmd, 2);
    uint8_t scratchpad[9] = {0};
    onewire_read_byte(scratchpad, 9);
    // Convert the temperature from the scratchpad
    int16_t raw_temp = (scratchpad[1] << 8) | scratchpad[0];
    double celsius   = raw_temp * 0.0625;
    elog_i(TAG, "OneWire temperature: %.2f", celsius);
    return celsius;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
                 onewire_read_temperature, onewire_read_temperature, onewire read temperature);
#endif  // CONFIG_DEV_ONEWRIE_EN