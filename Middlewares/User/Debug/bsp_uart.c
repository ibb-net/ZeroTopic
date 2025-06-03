#include "dev_uart/bsp_uart.h"
#include "config.h"
#include "string.h"
/* server */
void dev_pin_init(dev_gpio_handle_t gpio_handle);              // TODO 需要放在GPIO的文件中
void dev_dma_request(void *dev_handel, EnumDevType dev_type);  // TODO 需要放在DMA的文件中
/*!
    \brief      initialize the USART configuration of the com
    \param[in]  none
    \param[out] none
    \retval     none
*/

DevUARTHandleStruct device_uart_handles[] = {
    {
        /* Uart */
        .device_name  = "debug_uart",
        .uart_base    = USART0,
        .baudrate     = 115200,
        .idle_timeout = 3,
        .buffer_size  = UART_DEF_BUFFER_SIZE,
        .buffer_num   = UART_DEF_BUF_NUM,
        /* GPIO */
        .tx_gpio_handle = {
            .device_name = "debug_uart_tx_gpio",
            .port        = EVAL_COM_GPIO_PORT,
            .pin         = EVAL_COM_TX_PIN,
            .af          = EVAL_COM_AF,
        },
        .rx_gpio_handle = {
            .device_name = "debug_uart_rx_gpio",
            .port        = EVAL_COM_GPIO_PORT,
            .pin         = EVAL_COM_RX_PIN,
            .af          = EVAL_COM_AF,
        },
        /* DMA */
        .tx_dma_handle = {
            .device_name = "debug_uart_tx_dma",
            .base_addr   = DMA1,
            .channel     = DMA_CH7,
            .request     = DMA_REQUEST_USART0_TX,
        },
        .rx_dma_handle = {
            .device_name = "debug_uart_rx_dma",
            .base_addr   = DMA0,
            .channel     = DMA_CH1,
            .request     = DMA_REQUEST_USART0_RX,
        },

    },
};
static void __dev_uart_mcu_init(bsp_uart_handle_t uart_handle);
static void __uart_tx_dma_buffer(bsp_uart_handle_t uart_handle, uint8_t *txbuffer, size_t txbuffer_size);
static void __dev_uart_source_init(bsp_uart_handle_t uart_handle);

void bsp_uart_tx_thread(void *pvParameters);
void bsp_uart_rx_thread(void *pvParameters);

void dev_uart_init(void) {
    int device_num;
    device_num = sizeof(device_uart_handles) / sizeof(DevUARTHandleStruct);
    for (int i = 0; i < device_num; i++) {
        /* Software Source */
        __dev_uart_source_init(&device_uart_handles[i]);
        /* GPIO */
        dev_pin_init(&(device_uart_handles[i].tx_gpio_handle));
        dev_pin_init(&(device_uart_handles[i].rx_gpio_handle));
        /* UART */
        __dev_uart_mcu_init(&device_uart_handles[i]);
        /* DMA */
        dev_dma_request(&device_uart_handles[i], DEV_TYPE_UART);  // TODO 需要放在DMA的文件中
        // __uart_tx_dma_buffer(&device_uart_handles[i]);
        printf("UART %s: baudRate=%u\r\n", device_uart_handles[0].device_name, device_uart_handles[0].baudrate);
    }
    printf("device uart init done ,Total uart %d\r\n", device_num);
}
void __dev_uart_source_init(bsp_uart_handle_t uart_handle) {
    size_t one_item_size        = sizeof(uint8_t) * uart_handle->buffer_size;  // todo 增加xCreateListDMA
    uart_handle->rx_list_handle = xCreateListDMA(uart_handle->buffer_num, one_item_size);
    uart_handle->tx_list_handle = xCreateListDMA(uart_handle->buffer_num, one_item_size);
    uart_handle->lock_tx        = xSemaphoreCreateBinary();
    uart_handle->lock           = xSemaphoreCreateBinary();
    if (uart_handle->rx_list_handle == NULL || uart_handle->tx_list_handle == NULL) {
        TRACE_ERROR("create %s list failed!\r\n", uart_handle->device_name);
        return;
    }
    uart_handle->lock = xSemaphoreCreateMutex();
    if (uart_handle->lock == NULL) {
        TRACE_ERROR("create %s mutex failed!\r\n", uart_handle->device_name);
        return;
    }
    uart_handle->rx_queue = xQueueCreate(uart_handle->buffer_num, sizeof(steam_info_struct));
    uart_handle->tx_queue = xQueueCreate(uart_handle->buffer_num, sizeof(steam_info_struct));
    if (uart_handle->rx_queue == NULL || uart_handle->tx_queue == NULL) {
        TRACE_ERROR("create %s steambuf failed!\r\n", uart_handle->device_name);
        return;
    }
    /* Create task */
    BaseType_t ret;
    ret = xTaskCreate(bsp_uart_rx_thread, "uart_rx_task", configMINIMAL_STACK_SIZE * 2, (void *const)uart_handle, 2, NULL);
    if (ret != pdPASS) {
        TRACE_ERROR("create rx task failed!\r\n");
        return;
    }

    ret = xTaskCreate(bsp_uart_tx_thread, "uart_tx_task", configMINIMAL_STACK_SIZE * 2, (void *const)uart_handle, 2, NULL);
    if (ret != pdPASS) {
        vTaskDelete(uart_handle->rx_task_handle);
        uart_handle->rx_task_handle = NULL;
        TRACE_ERROR("create tx task failed!\r\n");
        return;
    }
}
void dev_pin_init(dev_gpio_handle_t gpio_handle) {
    static const struct {
        uint32_t gpio_port;
        rcu_periph_enum rcu_clock;
    } gpio_clock_map[] = {
        {GPIOA, RCU_GPIOA},
        {GPIOB, RCU_GPIOB},
        {GPIOC, RCU_GPIOC},
        {GPIOD, RCU_GPIOD},
        {GPIOE, RCU_GPIOE},
        {GPIOF, RCU_GPIOF},
        {GPIOG, RCU_GPIOG},
        {GPIOH, RCU_GPIOH},
        {GPIOJ, RCU_GPIOJ},
        {GPIOK, RCU_GPIOK},
    };

    uint8_t is_found = 0;
    for (size_t i = 0; i < sizeof(gpio_clock_map) / sizeof(gpio_clock_map[0]); i++) {
        if (gpio_clock_map[i].gpio_port == gpio_handle->port) {
            // rcu_periph_clock_enable(gpio_clock_map[i].rcu_clock);
            is_found = 1;
            break;
        }
    }

    if (!is_found) {
        TRACE_ERROR("gpio base %x error!\r\n", gpio_handle->port);
        return;
    }

    /* GPIO 配置 */
    if (gpio_handle->af) {
        /* 配置为复用功能 */
        gpio_af_set(gpio_handle->port, gpio_handle->af, gpio_handle->pin);
        gpio_mode_set(gpio_handle->port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, gpio_handle->pin);
        gpio_output_options_set(gpio_handle->port, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, gpio_handle->pin);
    } else {
        /* 配置为普通输出 */
        gpio_mode_set(gpio_handle->port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, gpio_handle->pin);
        gpio_output_options_set(gpio_handle->port, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, gpio_handle->pin);
    }
}
/*!
    \brief      configure USART and GPIO
    \param[in]  none
    \param[out] none
    \retval     none
*/

static void __dev_uart_mcu_init(bsp_uart_handle_t uart_handle) {
    /*  USART clock */
    static const struct {
        uint32_t uart_base;
        rcu_periph_enum rcu_clock;
    } uart_clock_map[] = {
        {USART0, RCU_USART0},
        {USART1, RCU_USART1},
        {USART2, RCU_USART2},
        {UART3, RCU_UART3},
        {UART4, RCU_UART4},
        {USART5, RCU_USART5},
        {UART6, RCU_UART6},
        {UART7, RCU_UART7},
    };
    uint8_t is_found = 0;
    for (size_t i = 0; i < sizeof(uart_clock_map) / sizeof(uart_clock_map[0]); i++) {
        if (uart_clock_map[i].uart_base == uart_handle->uart_base) {
            rcu_periph_clock_enable(uart_clock_map[i].rcu_clock);
            is_found = 1;
            break;
        }
    }
    if (!is_found) {
        TRACE_ERROR("uart base %x error!\r\n", uart_handle->uart_base);
    }
    /* USART configure */
    usart_deinit(uart_handle->uart_base);
    usart_word_length_set(uart_handle->uart_base, USART_WL_8BIT);
    usart_stop_bit_set(uart_handle->uart_base, USART_STB_1BIT);
    usart_parity_config(uart_handle->uart_base, USART_PM_NONE);
    usart_baudrate_set(uart_handle->uart_base, uart_handle->baudrate);
    usart_receive_config(uart_handle->uart_base, USART_RECEIVE_ENABLE);
    usart_transmit_config(uart_handle->uart_base, USART_TRANSMIT_ENABLE);

    /* enable the USART receive timeout and configure the time of timeout */
    usart_receiver_timeout_enable(uart_handle->uart_base);
    usart_interrupt_enable(uart_handle->uart_base, USART_INT_RT);
    usart_receiver_timeout_threshold_config(uart_handle->uart_base, uart_handle->idle_timeout);
    usart_enable(uart_handle->uart_base);
}

void bsp_uart_dma_rx_cfg(bsp_uart_handle_t uart_handle, uint8_t *rxbuffer) {
    dma_deinit(uart_handle->rx_dma_handle.base_addr, uart_handle->rx_dma_handle.channel);  // TODO
    dma_single_data_parameter_struct dma_init_struct;
    dma_single_data_para_struct_init(&dma_init_struct);
    dma_init_struct.request             = uart_handle->rx_dma_handle.request;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr        = (uint32_t)rxbuffer;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number              = uart_handle->buffer_size;
    dma_init_struct.periph_addr         = (uint32_t)&USART_RDATA(uart_handle->uart_base);
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority            = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(uart_handle->rx_dma_handle.base_addr, uart_handle->rx_dma_handle.channel, &dma_init_struct);

    /* configure DMA mode */
    dma_circulation_disable(uart_handle->rx_dma_handle.base_addr, uart_handle->rx_dma_handle.channel);
    dma_interrupt_enable(uart_handle->rx_dma_handle.base_addr, uart_handle->rx_dma_handle.channel, DMA_CHXCTL_FTFIE);
    dma_channel_enable(uart_handle->rx_dma_handle.base_addr, uart_handle->rx_dma_handle.channel);
    /* USART DMA enable for reception */
    usart_dma_receive_config(uart_handle->uart_base, USART_RECEIVE_DMA_ENABLE);
    SCB_InvalidateDCache_by_Addr((uint32_t *)rxbuffer, uart_handle->buffer_size);
}

// UART 打开
static DeviceResult_t UART_Open(void *device, uint32_t flags) {
    printf("UART Open\n");

    return DEVICE_OK;
}

/*!
    \brief      request DMA for UART
    \param[in]  dev_handel: device handle
    \param[in]  dev_type: device type
    \param[out] none
    \retval     none
*/
// TODO 需要放在DMA的文件中
// TODO 需要增加返回结果
void dev_dma_request(void *dev_handel, EnumDevType dev_type) {
    switch (dev_type) {
        case DEV_TYPE_UART: {
            bsp_uart_handle_t uart_handle = (bsp_uart_handle_t)dev_handel;
            // rcu_periph_clock_enable(RCU_DMA0);//TODO 需要放在DMA的文件中,增加判断
            steam_info_struct item_steam_info;
            void *rxbuffer = NULL;
            vReqestListDMABuffer(uart_handle->rx_list_handle, &item_steam_info);
            rxbuffer = item_steam_info.buffer;
            bsp_uart_dma_rx_cfg(uart_handle, rxbuffer);
        } break;

        default: {
            TRACE_ERROR("dev type %d not support!\r\n", dev_type);
        } break;
    }
}
/*!
    \brief      request DMA for UART from ISR
    \param[in]  dev_handel: device handle
    \param[in]  dev_type: device type
    \param[out] none
    \retval     none
*/
static void uart_rx_req_dma_fromISR(bsp_uart_handle_t uart_handle) {
    // rcu_periph_clock_enable(RCU_DMA0);//TODO 需要放在DMA的文件中,增加判断
    steam_info_struct item_steam_info;
    void *rxbuffer = NULL;
    // TODO DMA的初始化需要放在此处,但是使能需要放在Uart的Start中
    vReqestListDMABufferFromISR(uart_handle->rx_list_handle, &item_steam_info);  //
    rxbuffer = item_steam_info.buffer;
    bsp_uart_dma_rx_cfg(uart_handle, rxbuffer);
}
/*!
    \brief      configure DMA
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void __uart_tx_dma_buffer(bsp_uart_handle_t uart_handle, uint8_t *tx_data, size_t txbuffer_size) {
    // rcu_periph_clock_enable(RCU_DMA1); //TODO 需要放在DMA的文件中,增加判断
    static int counter = 0;
    steam_info_struct item_steam_info;
    void *txbuffer = NULL;
    vReqestListDMABuffer(uart_handle->tx_list_handle, &item_steam_info);
    txbuffer = item_steam_info.buffer;
    // printf("\r\n tx_data %p size  %d txbuffer = %p \r\n", tx_data, txbuffer_size, txbuffer);
    if (txbuffer == NULL) {
        TRACE_ERROR("%s tx buffer is NULL!\r\n", uart_handle->device_name);
        return;
    }
    memcpy((void *)txbuffer, (void *)tx_data, txbuffer_size);

    dma_single_data_parameter_struct dma_init_struct;
    /* clean the cache lines */
    SCB_CleanDCache_by_Addr((uint32_t *)txbuffer, uart_handle->buffer_size);
    /* deinitialize uart_handle->tx_dma_handle.base_addr channel7(uart_handle->uart_base tx) */
    dma_deinit(uart_handle->tx_dma_handle.base_addr, uart_handle->tx_dma_handle.channel);
    dma_single_data_para_struct_init(&dma_init_struct);

    dma_init_struct.request             = uart_handle->tx_dma_handle.request;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr        = (uint32_t)txbuffer;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number              = uart_handle->buffer_size < txbuffer_size ? uart_handle->buffer_size : txbuffer_size;
    dma_init_struct.periph_addr         = (uint32_t)(&USART_TDATA(uart_handle->uart_base));
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority            = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(uart_handle->tx_dma_handle.base_addr, uart_handle->tx_dma_handle.channel, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(uart_handle->tx_dma_handle.base_addr, uart_handle->tx_dma_handle.channel);
    dma_interrupt_enable(uart_handle->tx_dma_handle.base_addr, uart_handle->tx_dma_handle.channel, DMA_CHXCTL_FTFIE);
    dma_channel_enable(uart_handle->tx_dma_handle.base_addr, uart_handle->tx_dma_handle.channel);
    usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_ENABLE);
}
void uart_tx_dma_isr(void) {
    bsp_uart_handle_t uart_handle = &device_uart_handles[0];
    xSemaphoreGiveFromISR(uart_handle->lock_tx, NULL);
}
void com_uart_rx_isr(void) {
    bsp_uart_handle_t uart_handle = &device_uart_handles[0];
    steam_info_struct rx_steam_sruct;
    // SCB_CleanDCache_by_Addr((uint32_t *)uart_handle, sizeof(DevUARTHandleStruct));//NOTICE 需要清除缓存,否则会导致数据错误,否则会导致数据错误
    int rx_count        = uart_handle->buffer_size - (dma_transfer_number_get(uart_handle->rx_dma_handle.base_addr, uart_handle->rx_dma_handle.channel));
    rx_steam_sruct.size = rx_count;
    vActiveListDMABufferFromISR(uart_handle->rx_list_handle, &rx_steam_sruct);

    if (rx_steam_sruct.item == NULL) {
        TRACE_ERROR("rx buffer is NULL!\r\n");
    }
    // Give Rx semaphore
    uart_rx_req_dma_fromISR(uart_handle);

    xQueueSendFromISR(uart_handle->rx_queue, &rx_steam_sruct, NULL);
}

void uart_nvic_config(void) {
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    nvic_irq_enable(USART0_IRQn, 2, 0);
    nvic_irq_enable(DMA0_Channel1_IRQn, 2, 1);
    nvic_irq_enable(DMA1_Channel7_IRQn, 2, 0);
}
void bsp_uart_tx_thread(void *pvParameters) {
    bsp_uart_handle_t uart_handle = (bsp_uart_handle_t)pvParameters;
    steam_info_struct txbuffer;

    while (1) {
        if (pdTRUE == xSemaphoreTake(uart_handle->lock_tx, portMAX_DELAY)) {
            vActiveListDMABuffer(uart_handle->tx_list_handle, &txbuffer);
            vFreeListDMABuffer(uart_handle->tx_list_handle, &txbuffer);
        }
    }
    vTaskDelete(NULL);
}
void bsp_uart_rx_thread(void *pvParameters) {
    bsp_uart_handle_t uart_handle = (bsp_uart_handle_t)pvParameters;
    steam_info_struct rxbuffer;
    uint8_t *rxbuffer_data = NULL;

    while (1) {
        if (xQueueReceive(uart_handle->rx_queue, &rxbuffer, portMAX_DELAY) == pdTRUE) {
            if (rxbuffer.item == NULL) {
                TRACE_ERROR("%s rx buffer is NULL!\r\n", uart_handle->device_name);
                continue;
            }
            if (rxbuffer.size == 0) {
                TRACE_ERROR("%s rx buffer size is 0!\r\n", uart_handle->device_name);
                continue;
            }
            rxbuffer_data = (uint8_t *)rxbuffer.buffer;
            if (rxbuffer_data == NULL) {
                TRACE_ERROR("%s rx buffer data is NULL!\r\n", uart_handle->device_name);
                continue;
            }
            if (0xFFFFFFFF == (uint32_t)rxbuffer_data) {
                TRACE_ERROR("%s rx buffer data is 0xFFFFFFFF!\r\n", uart_handle->device_name);
                continue;
            }
            // rxbuffer_data[rxbuffer.size]     = '\r';  // add null terminator for string
            // rxbuffer_data[rxbuffer.size + 1] = '\n';  // add null terminator for string
            // rxbuffer_data[rxbuffer.size + 2] = '\0';  // add null terminator for string
            // rxbuffer.size                    = rxbuffer.size + 3;
            // printf("Recv %s\r\n", rxbuffer_data);
            __uart_tx_dma_buffer(uart_handle, rxbuffer_data, rxbuffer.size);
            // printf("vFreeListDMABuffer %s\r\n", rxbuffer_data);
            vFreeListDMABuffer(uart_handle->rx_list_handle, &rxbuffer);
        }
    }
    vTaskDelete(NULL);
}