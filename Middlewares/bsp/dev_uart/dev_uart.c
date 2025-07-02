#include "dev_uart/dev_uart.h"

#include <stdio.h>

#include "dev_basic.h"
#include "dev_pin.h"
#include "gd32h7xx.h"

typedef struct {
    uint32_t base;
    uint8_t is_initialized;
    uint8_t is_opened;
    uint8_t is_started;
    const DevUartHandleStruct *dev_cfg;  // Pointer to the UART handle
    void *handle;
} DevUartStatusStruct;
uint8_t pre_init = 0;
static DevUartStatusStruct *__DevUartGetStatus(uint32_t base);
DevUartStatusStruct dev_uart_status[] = {{
                                             .base           = USART0,
                                             .is_initialized = 0,
                                             .is_opened      = 0,
                                             .is_started     = 0,
                                             .handle         = NULL,
                                         },
                                         {
                                             .base           = USART1,
                                             .is_initialized = 0,
                                             .is_opened      = 0,
                                             .is_started     = 0,
                                             .handle         = NULL,

                                         },
                                         {
                                             .base           = USART2,
                                             .is_initialized = 0,
                                             .is_opened      = 0,
                                             .is_started     = 0,
                                             .handle         = NULL,
                                         },
                                         {
                                             .base           = UART3,
                                             .is_initialized = 0,
                                             .is_opened      = 0,
                                             .is_started     = 0,
                                             .handle         = NULL,
                                         },
                                         {
                                             .base           = UART4,
                                             .is_initialized = 0,
                                             .is_opened      = 0,
                                             .is_started     = 0,
                                             .handle         = NULL,
                                         },
                                         {
                                             .base           = USART5,
                                             .is_initialized = 0,
                                             .is_opened      = 0,
                                             .is_started     = 0,
                                             .handle         = NULL,
                                         },
                                         {
                                             .base           = UART6,
                                             .is_initialized = 0,
                                             .is_opened      = 0,
                                             .is_started     = 0,
                                             .handle         = NULL,
                                         },
                                         {
                                             .base           = UART7,
                                             .is_initialized = 0,
                                             .is_opened      = 0,
                                             .is_started     = 0,
                                             .handle         = NULL,
                                         }

};

/* retarget the C library printf function to the
 * USART */

int fputc(int ch, FILE *f) {
    static DevUartStatusStruct *status;
    if (status == NULL) {
        status = __DevUartGetStatus(USART0);
    }
    if (!status || !status->is_initialized || !status->is_opened || !status->is_started) {
        return ch;  // Error handling
    }
    usart_data_transmit(USART0, (uint8_t)ch);
    while (RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    return ch;
}
int __io_putchar(int ch) {
    static DevUartStatusStruct *status;
    if (status == NULL) {
        status = __DevUartGetStatus(USART0);
    }
    if (!status || !status->is_initialized) {
        return ch;  // Error handling
    }
    // usart_data_transmit(USART0, (uint8_t)ch);
    usart_data_transmit(status->base, (uint8_t)ch);  // 这里不能用dma
    while (RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    return ch;
}
uint8_t isReady(void) {
    return (dev_uart_status[0].is_initialized && dev_uart_status[0].is_opened &&
            dev_uart_status[0].is_started);
}
int debug_putbuffer(const char *buffer, size_t len) {
    for (size_t i = 0; i < len; i++) {
        fputc(buffer[i], NULL);
    }
    return len;
}
DevUartStatusStruct *__DevUartGetStatus(uint32_t base) {
    for (size_t i = 0; i < sizeof(dev_uart_status) / sizeof(dev_uart_status[0]); i++) {
        if (dev_uart_status[i].base == base) {
            return &dev_uart_status[i];
        }
    }
    return NULL;  // Not found
}
void DevUartRegister(uint32_t base, void *handle) {
    DevUartStatusStruct *status = __DevUartGetStatus(base);
    if (status) {
        status->handle = handle;  // Store the handle
    } else {
        printf("Failed to find UART status for base %x\r\n", base);
    }
}
void DevUartPreInit(const DevUartHandleStruct *ptrDevUartHandle) {
    static const struct {
        uint32_t device;
        rcu_periph_enum rcu_clock;
    } dev_clock_map[] = {
        {USART0, RCU_USART0}, {USART1, RCU_USART1}, {USART2, RCU_USART2}, {UART3, RCU_UART3},
        {UART4, RCU_UART4},   {USART5, RCU_USART5}, {UART6, RCU_UART6},   {UART7, RCU_UART7},

    };
    uint8_t is_found = 0;
    for (size_t i = 0; i < sizeof(dev_clock_map) / sizeof(dev_clock_map[0]); i++) {
        if (dev_clock_map[i].device == ptrDevUartHandle->base) {
            rcu_periph_clock_enable(dev_clock_map[i].rcu_clock);
            is_found = 1;
            break;
        }
    }
    if (!is_found) {
        while (1);
    }

    usart_deinit(ptrDevUartHandle->base);
    usart_word_length_set(ptrDevUartHandle->base, USART_WL_8BIT);
    usart_stop_bit_set(ptrDevUartHandle->base, USART_STB_1BIT);
    usart_parity_config(ptrDevUartHandle->base, USART_PM_NONE);
    usart_baudrate_set(ptrDevUartHandle->base, ptrDevUartHandle->baudrate);
    usart_transmit_config(ptrDevUartHandle->base, USART_TRANSMIT_ENABLE);
    usart_enable(ptrDevUartHandle->base);
    DevUartStatusStruct *status = __DevUartGetStatus(ptrDevUartHandle->base);
    if (status) {
        status->is_initialized = 1;
        status->dev_cfg        = ptrDevUartHandle;
    }
}
void DevUartInit(const DevUartHandleStruct *ptrDevUartHandle) {
    static const struct {
        uint32_t device;
        rcu_periph_enum rcu_clock;
    } dev_clock_map[] = {
        {USART0, RCU_USART0}, {USART1, RCU_USART1}, {USART2, RCU_USART2}, {UART3, RCU_UART3},
        {UART4, RCU_UART4},   {USART5, RCU_USART5}, {UART6, RCU_UART6},   {UART7, RCU_UART7},

    };
    static const struct {
        uint32_t device;
        rcu_periph_enum rcu_clock;
        IRQn_Type irqn;

    } dev_irqn_map[] = {
        {USART0, RCU_USART0, USART0_IRQn}, {USART1, RCU_USART1, USART1_IRQn},
        {USART2, RCU_USART2, USART2_IRQn}, {UART3, RCU_UART3, UART3_IRQn},
        {UART4, RCU_UART4, UART4_IRQn},    {USART5, RCU_USART5, USART5_IRQn},
        {UART6, RCU_UART6, UART6_IRQn},    {UART7, RCU_UART7, UART7_IRQn},
    };

    uint8_t is_found = 0;
    for (size_t i = 0; i < sizeof(dev_clock_map) / sizeof(dev_clock_map[0]); i++) {
        if (dev_clock_map[i].device == ptrDevUartHandle->base) {
            rcu_periph_clock_enable(dev_clock_map[i].rcu_clock);
            nvic_irq_enable(dev_irqn_map[i].irqn, 2, i);
            is_found = 1;
            break;
        }
    }

    if (!is_found) {
        printf("uart base %x error!\r\n", ptrDevUartHandle->base);
        return;
    }

    usart_deinit(ptrDevUartHandle->base);
    usart_word_length_set(ptrDevUartHandle->base, USART_WL_8BIT);
    usart_stop_bit_set(ptrDevUartHandle->base, USART_STB_1BIT);
    usart_parity_config(ptrDevUartHandle->base, USART_PM_NONE);
    usart_baudrate_set(ptrDevUartHandle->base, ptrDevUartHandle->baudrate);
    // usart_receive_config(ptrDevUartHandle->base, USART_RECEIVE_ENABLE);
    // usart_enable(ptrDevUartHandle->base);
    printf("UART %s (%x) initialized successfully.\r\n", ptrDevUartHandle->device_name,
           ptrDevUartHandle->base);
    DevUartStatusStruct *status = __DevUartGetStatus(ptrDevUartHandle->base);
    if (status) {
        status->is_initialized = 1;                 // Mark as initialized
        status->dev_cfg        = ptrDevUartHandle;  // Store the handle
        if (status->handle == NULL) {
            printf(
                "[FAULT]DevUartInit: handle %s (%x) is NULL.Should call DevUartRegister() before "
                "using UART.\r\n",
                ptrDevUartHandle->device_name, ptrDevUartHandle->base);
            while (1);
        }
    } else {
        printf(
            "Failed to find UART status for base "
            "%x\r\n",
            ptrDevUartHandle->base);
    }
}

void DevUartDeinit(const DevUartHandleStruct *ptrDevUartHandle) {
    usart_deinit(ptrDevUartHandle->base);
}

void DevUartStart(const DevUartHandleStruct *ptrDevUartHandle) {
    DevUartStatusStruct *status = __DevUartGetStatus(ptrDevUartHandle->base);
    if (status == NULL) {
        printf("Failed to find UART status for base %x\r\n", ptrDevUartHandle->base);
        return;
    }
    if (!status->is_initialized) {
        printf("UART %s (%x) is not initialized. Call DevUartInit() first.\r\n",
               ptrDevUartHandle->device_name, ptrDevUartHandle->base);
        return;
    }
    if (status->handle == NULL) {
        printf(
            "[FAULT]DevUartStart: handle %s (%x) is NULL. Should call DevUartRegister() before "
            "using UART.\r\n",
            ptrDevUartHandle->device_name, ptrDevUartHandle->base);
        while (1);
    }

    if (status) {
        status->is_opened  = 1;  // Mark as opened
        status->is_started = 1;  // Mark as started
        usart_transmit_config(ptrDevUartHandle->base, USART_TRANSMIT_ENABLE);
        usart_receiver_timeout_enable(ptrDevUartHandle->base);
        usart_interrupt_enable(ptrDevUartHandle->base, USART_INT_RT);
        if (ptrDevUartHandle->base == USART0 || ptrDevUartHandle->base == USART1 ||
            ptrDevUartHandle->base == USART2 || ptrDevUartHandle->base == USART5) {
            usart_receiver_timeout_threshold_config(ptrDevUartHandle->base,
                                                    ptrDevUartHandle->idle_timeout);
        } else {
            usart_interrupt_enable(ptrDevUartHandle->base,
                                   USART_INT_IDLE);  // Enable error interrupt for other UARTs
        }
        usart_receive_config(ptrDevUartHandle->base, USART_RECEIVE_ENABLE);
        usart_transmit_config(ptrDevUartHandle->base, USART_TRANSMIT_ENABLE);
        usart_enable(ptrDevUartHandle->base);
    }
}
void DevUartDMARecive(const DevUartHandleStruct *ptrDevUartHandle, const uint8_t *data,
                      size_t len) {
    if (ptrDevUartHandle == NULL || data == NULL || len == 0) {
        printf(
            "Invalid parameters for "
            "DevUartDMARecive\r\n");
        while (1);
    }
    dma_deinit(ptrDevUartHandle->rx_dma_base_addr, ptrDevUartHandle->rx_dma_channel);
    dma_single_data_parameter_struct dma_init_struct;
    SCB_CleanDCache_by_Addr((uint32_t *)data, len);
    dma_init_struct.request             = ptrDevUartHandle->rx_dma_request;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr        = (uint32_t)data;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number              = len;
    dma_init_struct.periph_addr         = (uint32_t)(&USART_RDATA(ptrDevUartHandle->base));
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority            = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(ptrDevUartHandle->rx_dma_base_addr, ptrDevUartHandle->rx_dma_channel,
                              &dma_init_struct);

    /* configure DMA mode */
    dma_circulation_disable(ptrDevUartHandle->rx_dma_base_addr, ptrDevUartHandle->rx_dma_channel);
    dma_interrupt_enable(ptrDevUartHandle->rx_dma_base_addr, ptrDevUartHandle->rx_dma_channel,
                         DMA_CHXCTL_FTFIE);
    dma_channel_enable(ptrDevUartHandle->rx_dma_base_addr, ptrDevUartHandle->rx_dma_channel);
    /* USART DMA enable for reception */
    usart_dma_receive_config(ptrDevUartHandle->base, USART_RECEIVE_DMA_ENABLE);
    SCB_InvalidateDCache_by_Addr((uint32_t *)data, len);
}
void DevUartDMASend(const DevUartHandleStruct *ptrDevUartHandle, const uint8_t *data, size_t len) {
    if (ptrDevUartHandle == NULL || data == NULL || len == 0) {
        printf(
            "Invalid parameters for "
            "DevUartDMASend\r\n");
        return;
    }
    DevUartStatusStruct *status = __DevUartGetStatus(ptrDevUartHandle->base);
    dma_single_data_parameter_struct dma_init_struct;
    SCB_CleanDCache_by_Addr((uint32_t *)data, len);
    dma_deinit(ptrDevUartHandle->tx_dma_base_addr, ptrDevUartHandle->tx_dma_channel);
    dma_init_struct.request             = ptrDevUartHandle->tx_dma_request;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr        = (uint32_t)data;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number              = len;
    dma_init_struct.periph_addr         = (uint32_t)(&USART_TDATA(ptrDevUartHandle->base));
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority            = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(ptrDevUartHandle->tx_dma_base_addr, ptrDevUartHandle->tx_dma_channel,
                              &dma_init_struct);
    dma_circulation_disable(ptrDevUartHandle->tx_dma_base_addr, ptrDevUartHandle->tx_dma_channel);
    dma_interrupt_enable(ptrDevUartHandle->tx_dma_base_addr, ptrDevUartHandle->tx_dma_channel,
                         DMA_CHXCTL_FTFIE);
    dma_channel_enable(ptrDevUartHandle->tx_dma_base_addr, ptrDevUartHandle->tx_dma_channel);
    usart_dma_transmit_config(ptrDevUartHandle->base, USART_TRANSMIT_DMA_ENABLE);
    if (status && status->dev_cfg->tx_isr_cb) {
        // Call the TX ISR callback function
        status->dev_cfg->tx_isr_cb((void *)status->handle);
    }
}

void USART0_IRQHandler(void) {
    static DevUartStatusStruct *status;
    if (status == NULL) {
        status = __DevUartGetStatus(USART0);
    }
    if (RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_RT)) {
        usart_interrupt_flag_clear(USART0, USART_INT_FLAG_RT);
        if (status && status->dev_cfg->rx_isr_cb) {
            status->dev_cfg->rx_isr_cb((void *)status->handle);
        }
    } else {
        DevErrorLED(1);
    }
}

void DMA1_Channel0_IRQHandler(void) {
    static DevUartStatusStruct *status;
    if (status == NULL) {
        status = __DevUartGetStatus(USART0);
    }
    if (RESET != dma_interrupt_flag_get(DMA1, DMA_CH0, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(DMA1, DMA_CH0, DMA_INT_FLAG_FTF);
        if (status && status->dev_cfg->tx_dma_isr_cb) {
            status->dev_cfg->tx_dma_isr_cb((void *)status->handle);
        }
    } else {
        // DevErrorLED(1);
    }
}
/*!
    \brief      this function handles DMA_Channel0_IRQHandler exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
/* DMA_INT_FLAG_FEE: FIFO error and exception flag
DMA_INT_FLAG_SDE: single data mode exception flag
DMA_INT_FLAG_TAE: transfer access error flag
DMA_INT_FLAG_HTF: half transfer finish flag
DMA_INT_FLAG_FTF: full transfer finish flag */
void DMA0_Channel0_IRQHandler(void) {
    static DevUartStatusStruct *status;
    if (status == NULL) {
        status = __DevUartGetStatus(USART0);
    }
    if (RESET != dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_FTF);
        if (status && status->dev_cfg->rx_dma_isr_cb) {
            // Call the RX DMA ISR callback function
            status->dev_cfg->rx_dma_isr_cb((void *)status->handle);
        }
    } else if (RESET != dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_HTF)) {
        dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_HTF);

    } else if (RESET != dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_TAE)) {
        dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_TAE);
        DevErrorLED(1);
    } else if (RESET != dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_SDE)) {
        dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_SDE);
        DevErrorLED(1);
    } else if (RESET != dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_FEE)) {
        dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_FEE);
        DevErrorLED(1);
    }

    else {
        DevErrorLED(1);
    }
}

/////////////////////////////////////////////////////////////
#if 0
      \arg        USART_INT_FLAG_EB: end of block interrupt and flag
      \arg        USART_INT_FLAG_RT: receiver timeout interrupt and flag
      \arg        USART_INT_FLAG_AM0: address 0 match interrupt and flag
      \arg        USART_INT_FLAG_AM1: address 1 match interrupt and flag
      \arg        USART_INT_FLAG_PERR: parity error interrupt and flag
      \arg        USART_INT_FLAG_TBE: transmitter buffer empty interrupt and flag(when FIFO is disabled)
      \arg        USART_INT_FLAG_TFNF: transmit FIFO not full interrupt and flag(when FIFO is enabled)
      \arg        USART_INT_FLAG_TC: transmission complete interrupt and flag
      \arg        USART_INT_FLAG_RBNE: read data buffer not empty interrupt and flag(when FIFO is disabled)
      \arg        USART_INT_FLAG_RFNE: receive FIFO not empty interrupt and flag(when FIFO is enabled)
      \arg        USART_INT_FLAG_RBNE_ORERR: read data buffer not empty interrupt and overrun error flag(when FIFO is disabled)
      \arg        USART_INT_FLAG_RFNE_ORERR: receive FIFO not empty interrupt and overrun error flag(when FIFO is enabled)
      \arg        USART_INT_FLAG_IDLE: IDLE line detected interrupt and flag
      \arg        USART_INT_FLAG_LBD: LIN break detected interrupt and flag
      \arg        USART_INT_FLAG_WU: wakeup from deep-sleep mode interrupt and flag
      \arg        USART_INT_FLAG_CTS: CTS interrupt and flag
      \arg        USART_INT_FLAG_ERR_NERR: error interrupt and noise error flag
      \arg        USART_INT_FLAG_ERR_ORERR: error interrupt and overrun error
      \arg        USART_INT_FLAG_ERR_FERR: error interrupt and frame error flag
      \arg        USART_INT_FLAG_TFT: transmit FIFO threshold interrupt and flag
      \arg        USART_INT_FLAG_TFE: transmit FIFO empty interrupt and flag
      \arg        USART_INT_FLAG_RFT: receive FIFO threshold interrupt and flag
      \arg        USART_INT_FLAG_RFF: receive FIFO full interrupt and flag
#endif
void UART6_IRQHandler(void) {
    static DevUartStatusStruct *status;
    if (status == NULL) {
        status = __DevUartGetStatus(UART6);
    }
    if (RESET != usart_interrupt_flag_get(UART6, USART_INT_FLAG_RT)) {
        usart_interrupt_flag_clear(UART6, USART_INT_FLAG_RT);
        if (status && status->dev_cfg->rx_isr_cb) {
            status->dev_cfg->rx_isr_cb((void *)status->handle);
        }
    } else if (RESET != usart_interrupt_flag_get(UART6, USART_INT_FLAG_IDLE)) {
        usart_interrupt_flag_clear(UART6, USART_INT_FLAG_IDLE);
        if (status && status->dev_cfg->rx_isr_cb) {
            status->dev_cfg->rx_isr_cb((void *)status->handle);
        }
    } else if (RESET != usart_interrupt_flag_get(UART6, USART_INT_FLAG_ERR_NERR)) {
        DevErrorLED(0);  // 1111
        printf("UART6 Error: Noise Error\r\n");
    } else if (RESET != usart_interrupt_flag_get(UART6, USART_INT_FLAG_TBE)) {
        DevErrorLED(0);
        printf("UART6 Error: Transmitter Buffer Empty\r\n");
    } else if (RESET != usart_interrupt_flag_get(UART6, USART_INT_FLAG_TFNF)) {
        DevErrorLED(0);
        printf("UART6 Error: Transmit FIFO Not Full\r\n");
    } else if (RESET != usart_interrupt_flag_get(UART6, USART_INT_FLAG_TC)) {
        DevErrorLED(0);
        printf("UART6 Error: Transmission Complete\r\n");
    }

    else if (RESET != usart_interrupt_flag_get(UART6, USART_INT_FLAG_RBNE)) {
        DevErrorLED(0);
        printf("UART6 Error: Read Data Buffer Not Empty\r\n");
    } else if (RESET != usart_interrupt_flag_get(UART6, USART_INT_FLAG_RFNE)) {
        DevErrorLED(0);
        printf("UART6 Error: Receive FIFO Not Empty\r\n");
    } else if (RESET != usart_interrupt_flag_get(UART6, USART_INT_FLAG_RBNE_ORERR)) {
        DevErrorLED(0);
        printf("UART6 Error: Read Data Buffer Not Empty or Overrun Error\r\n");
    } else if (RESET != usart_interrupt_flag_get(UART6, USART_INT_FLAG_RFNE_ORERR)) {
        printf("UART6 Error: Receive FIFO Not Empty or Overrun Error\r\n");
    } else if (RESET != usart_interrupt_flag_get(UART6, USART_INT_FLAG_IDLE)) {
        printf("UART6 Error: IDLE Line Detected\r\n");
    } else {
        DevErrorLED(0);
        printf("UART6 IRQ Error\r\n");
    }
}

void DMA1_Channel1_IRQHandler(void) {
    static DevUartStatusStruct *status;
    if (status == NULL) {
        status = __DevUartGetStatus(UART6);
    }
    if (RESET != dma_interrupt_flag_get(DMA1, DMA_CH1, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(DMA1, DMA_CH1, DMA_INT_FLAG_FTF);
        if (status && status->dev_cfg->tx_dma_isr_cb) {
            status->dev_cfg->tx_dma_isr_cb((void *)status->handle);
        }
    } else {
        // DevErrorLED(1);
    }
}
/*!
    \brief      this function handles DMA_Channel0_IRQHandler exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DMA0_Channel1_IRQHandler(void) {
    static DevUartStatusStruct *status;
    if (status == NULL) {
        status = __DevUartGetStatus(UART6);
    }
    if (RESET != dma_interrupt_flag_get(DMA0, DMA_CH1, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INT_FLAG_FTF);
        if (status && status->dev_cfg->rx_dma_isr_cb) {
            // Call the RX DMA ISR callback function
            status->dev_cfg->rx_dma_isr_cb((void *)status->handle);
        }
    } else {
        // DevErrorLED(1);
    }
}
