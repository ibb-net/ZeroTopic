
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
#define TAG                 "OW_Uart"
#define RESET_BAUDRATE      9600
#define DATA_BAUDRATE       115200
#define MAX_ONEWIRE_DEVICES 8
void onewire_dma_init(void);
static void start_dma_transfer(uint8_t tx_len, uint8_t rx_len, uint8_t simultaneous);
#define ONE_WIRE_BUFFER_SIZE 32
// DMA缓冲区

__attribute__((aligned(32))) uint8_t ow_tx_buffer[ONE_WIRE_BUFFER_SIZE];
__attribute__((aligned(32))) uint8_t ow_rx_buffer[ONE_WIRE_BUFFER_SIZE] = {0x05, 0x06, 0x07};
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
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_0);

    /* configure USART RX as alternate function push-pull */
    // gpio_af_set(GPIOA, GPIO_AF_8, GPIO_PIN_1);
    // gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_1);
    // gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_1);
    /* USART3初始化 */
    usart_deinit(UART3);
    usart_word_length_set(UART3, USART_WL_8BIT);
    usart_stop_bit_set(UART3, USART_STB_1BIT);
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

    onewire_dma_init();

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

void onewire_dma_init(void) {}
void ow_uart_dma_send(uint8_t *data, uint8_t len) {
    rcu_periph_clock_enable(RCU_DMA1);
    /* enable DMAMUX clock */
    rcu_periph_clock_enable(RCU_DMAMUX);
    dma_single_data_parameter_struct dma_init_struct;

    // 配置发送缓冲区
    // SCB_InvalidateDCache_by_Addr((uint32_t *)ow_tx_buffer, len);
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
    // dma_interrupt_enable(DMA1, DMA_CH3, DMA_CHXCTL_FTFIE);

    // USART DMA enable for transmission
    dma_channel_enable(DMA1, DMA_CH3);
    usart_dma_transmit_config(UART3, USART_TRANSMIT_DMA_ENABLE);

    // 启动DMA发送

    // 等待DMA发送完成
    while (RESET == dma_flag_get(DMA1, DMA_CH3, DMA_FLAG_FTF));

    // 清除发送完成标志
    dma_flag_clear(DMA1, DMA_CH3, DMA_FLAG_FTF);

    // invalidate d-cache by address
    // SCB_InvalidateDCache_by_Addr((uint32_t *)ow_tx_buffer, len);

    printf("\r\n[INFO]DMA Send Done. Data: ");
    for (uint8_t i = 0; i < len; i++) {
        printf("%02X ", ow_tx_buffer[i]);
    }
    printf("\r\n");
}
#endif /* onewire_dma_init */
static void switch_baudrate(uint32_t baudrate) {
    while (!usart_flag_get(UART3, USART_FLAG_TC));
    usart_disable(UART3);
    usart_baudrate_set(UART3, baudrate);
    usart_enable(UART3);
    for (volatile int i = 0; i < 100; i++);
}
uint8_t buffer_tmp[3] = {0x01, 0x02, 0x03};
uint8_t cnt           = 0;
uint8_t onewire_reset(void) {
    // switch_baudrate(RESET_BAUDRATE);
    // ow_tx_buffer[0] = 0x55;  // 0xF0;  // 发送复位脉冲
    // ow_tx_buffer[1] = 0xAA;  // 发送复位脉冲
    // printf("ow_tx_buffer[0] = %2X\r\n", ow_tx_buffer[0]);
    // ow_rx_buffer[0] = 0;
    buffer_tmp[0] = cnt++;
    buffer_tmp[1] = cnt++;
    buffer_tmp[2] = cnt++;

#if 0
    while (RESET == usart_flag_get(UART3, USART_FLAG_TBE));
    usart_data_transmit(UART3, buffer_tmp[0]);

    // while (RESET == usart_flag_get(UART3, USART_FLAG_RBNE)) {
    // }

    /* store the received byte in the receiver_buffer1 */
    // ow_rx_buffer[0] = usart_data_receive(UART3);
#else
    // start_dma_transfer(2, 0, 0);

#endif

    ow_uart_dma_send(buffer_tmp, 3);
    uint8_t presence = (ow_rx_buffer[0] != 0xF0) && (ow_rx_buffer[0] < 0xF0);
    elog_i(TAG, "onewire_reset: %s ,Rcv %2X", presence ? "Pass" : "Failed", ow_rx_buffer[0]);
    elog_i(TAG, "Snd %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X",
           ow_tx_buffer[0], ow_tx_buffer[1], ow_tx_buffer[2], ow_tx_buffer[3], ow_tx_buffer[4],
           ow_tx_buffer[5], ow_tx_buffer[6], ow_tx_buffer[7], ow_tx_buffer[8], ow_tx_buffer[9],
           ow_tx_buffer[10], ow_tx_buffer[11], ow_tx_buffer[12], ow_tx_buffer[13], ow_tx_buffer[14],
           ow_tx_buffer[15]);
    elog_i(TAG, "Rcv %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X",
           ow_rx_buffer[0], ow_rx_buffer[1], ow_rx_buffer[2], ow_rx_buffer[3], ow_rx_buffer[4],
           ow_rx_buffer[5], ow_rx_buffer[6], ow_rx_buffer[7], ow_rx_buffer[8], ow_rx_buffer[9],
           ow_rx_buffer[10], ow_rx_buffer[11], ow_rx_buffer[12], ow_rx_buffer[13], ow_rx_buffer[14],
           ow_rx_buffer[15]);
    return presence;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), onewire_reset,
                 onewire_reset, onewire reset);

#endif  // CONFIG_DEV_ONEWRIE_EN