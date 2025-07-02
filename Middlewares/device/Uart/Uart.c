
#include "UartCfg.h"

#ifndef CONFIG_UART_Uart_EN
#define CONFIG_UART_Uart_EN 1
#endif
#if CONFIG_UART_Uart_EN
#include <stdio.h>

#include "gd32h7xx.h"
#include "string.h"
/* Device */
#include "Device.h"
#include "dev_basic.h"
#include "dev_pin.h"
#include "dev_uart.h"
#include "os_server.h"
#include "stream_buffer.h"

#define TAG          "Uart"
#define UartLogLvl   ELOG_LVL_INFO
#define UartPriority PriorityCommunitGroup3

#ifndef UartChannelMax
#define UartChannelMax 1
#endif
#ifndef CONFIG_Uart_CYCLE_TIMER_MS
#define CONFIG_Uart_CYCLE_TIMER_MS 100
#endif
/* ===================================================================================== */
typedef struct {
    /* TX GPIO*/
    DevPinHandleStruct tx_gpio_cfg;
    DevPinHandleStruct rx_gpio_cfg;
    DevUartHandleStruct uart_cfg;
    /* Buffer */
    uint32_t buffer_size;

} TypdefUartBSPCfg;

static void UartCreateTaskHandle(void);
static void UartRcvHandle(void *msg);
static void UartCycHandle(void);
static void UartInitHandle(void *msg);
static void __UartRXISRHandle(void *arg);
static void __UartTXDMAISRHandle(void *arg);
void UartStreamRcvTask(void *arg);

const TypdefUartBSPCfg UartBspCfg[UartChannelMax] = {
    {

        .tx_gpio_cfg =
            {
                .device_name = "Uart_TX",
                .base        = Uart_TX_GPIO_PORT,
                .af          = Uart_TX_GPIO_AF,
                .pin         = Uart_TX_GPIO_PIN,
            },
        .rx_gpio_cfg =
            {
                .device_name = "Uart_RX",
                .base        = Uart_RX_GPIO_PORT,
                .af          = Uart_RX_GPIO_AF,
                .pin         = Uart_RX_GPIO_PIN,
            },
        .uart_cfg =
            {
                .id           = 0,  // ID
                .device_name  = "Uart_UART",
                .base         = Uart_UART_BASE,
                .baudrate     = Uart_UART_BAUDRATE,
                .idle_timeout = Uart_UART_IDLE_TIMEOUT,
                .rx_isr_cb    = __UartRXISRHandle,  // RX ISR callback function
                .tx_isr_cb    = NULL,               // TX ISR callback function
                .error_isr_cb = NULL,               // Error ISR callback function
                /* DMA */
                .tx_dma_rcu       = Uart_TX_DMA_BASE_ADDR,
                .tx_dma_base_addr = Uart_TX_DMA_BASE_ADDR,
                .tx_dma_channel   = Uart_TX_DMA_CHANNEL,
                .tx_dma_request   = Uart_TX_DMA_REQUEST,
                .tx_dma_isr_cb    = __UartTXDMAISRHandle,  // TX DMA ISR callback function
                .rx_dma_rcu       = Uart_RX_DMA_BASE_ADDR,
                .rx_dma_base_addr = Uart_RX_DMA_BASE_ADDR,
                .rx_dma_channel   = Uart_RX_DMA_CHANNEL,
                .rx_dma_request   = Uart_RX_DMA_REQUEST,
                .rx_dma_isr_cb    = __UartRXISRHandle,  // RX DMA ISR callback function
            },
        .buffer_size = Uart_UART_BUFFER_SIZE,
    },
};

typedef struct {
    uint32_t uart_base;
    char device_name[DEVICE_NAME_MAX];
    uint32_t id;  // ID
    /* Lock */
    SemaphoreHandle_t lock;
    SemaphoreHandle_t lock_tx;
    /* Queue */
    SemaphoreHandle_t rx_queue;
    StreamBufferHandle_t rx_stream_buffer;  // Stream buffer for RX
    SemaphoreHandle_t tx_queue;
    /* Thread */
    TaskHandle_t rx_task_handle;
    TaskHandle_t tx_task_handle;
    /* Buffer */
    size_t buffer_size;
    uint8_t *rx_buffer;          // Pointer to the buffer
    uint8_t *rx_buffer_for_vfb;  // Pointer to the buffer
    /* lcfg */
    const TypdefUartBSPCfg *UartBspCfg;  // BSP configuration

} TypdefUartStatus;
TypdefUartStatus UartStatus[UartChannelMax] = {0};

/* ===================================================================================== */

static const vfb_event_t UartEventList[] = {
    UartStart, UartStop, UartSet, UartGet, UartSend, UartRcv,

};

static const VFBTaskStruct Uart_task_cfg = {
    .name         = "VFBTaskUart",  // Task name
    .pvParameters = NULL,
    // .uxPriority = 10,
    // // Task parameters
    .queue_num  = 32,                                           // Number of queues to subscribe
    .event_list = UartEventList,                                // Event list to subscribe
    .event_num  = sizeof(UartEventList) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                            // Events to wait for at startup
    .startup_wait_event_num  = 0,  // Number of startup events to wait for
    .xTicksToWait            = pdMS_TO_TICKS(CONFIG_Uart_CYCLE_TIMER_MS),  // Wait indefinitely
    .init_msg_cb             = UartInitHandle,  // Callback for initialization messages
    .rcv_msg_cb              = UartRcvHandle,   // Callback for received messages
    .rcv_timeout_cb          = UartCycHandle,   // Callback for timeout
};

/* ===================================================================================== */
void UartComPreInit(void) {
#if 0
    DevPinInit(&(UartBspCfg[0].tx_gpio_cfg));
    DevPinInit(&(UartBspCfg[0].rx_gpio_cfg));
    DevUartRegister(UartBspCfg[0].uart_cfg.base, (void *)&UartStatus[0]);
    DevUartPreInit(&(UartBspCfg[0].uart_cfg));
    printf("UartComPreInit Done\r\n");
#endif
}
/* ===================================================================================== */

void UartDeviceInit(void) {
    elog_i(TAG, "UartDeviceInit\r\n");

    for (size_t i = 0; i < UartChannelMax; i++) {
        TypdefUartStatus *uart_handle = &UartStatus[i];
        if (UartBspCfg[i].uart_cfg.base == 0) {
            elog_w(TAG, "UartBspCfg[%d] uart base is 0, skip init!\r\n", i);
            continue;
        }
        uart_handle->id         = i;
        uart_handle->UartBspCfg = &UartBspCfg[i];
        DevPinInit(&(UartBspCfg[i].tx_gpio_cfg));
        DevPinInit(&(UartBspCfg[i].rx_gpio_cfg));

        DevUartRegister(UartBspCfg[i].uart_cfg.base, (void *)uart_handle);
        DevUartInit(&(UartBspCfg[i].uart_cfg));

        uart_handle->uart_base        = UartBspCfg[i].uart_cfg.base;
        uart_handle->buffer_size      = UartBspCfg[i].buffer_size;
        uart_handle->rx_stream_buffer = xStreamBufferCreate(UartBspCfg[i].buffer_size, 1);
        uart_handle->rx_buffer        = pvPortMalloc(UartBspCfg[i].buffer_size);
        if (uart_handle->rx_buffer == NULL) {
            elog_e(TAG, "[Fault]malloc %s buffer failed!\r\n", UartBspCfg[i].uart_cfg.device_name);
            while (1);
            return;
        }
        uart_handle->rx_buffer_for_vfb = pvPortMalloc(UartBspCfg[i].buffer_size);
        if (uart_handle->rx_buffer_for_vfb == NULL) {
            elog_e(TAG, "[Fault]malloc %s buffer for vfb failed!\r\n",
                   UartBspCfg[i].uart_cfg.device_name);
            vPortFree(uart_handle->rx_buffer);
            while (1);
            return;
        }

        memset(uart_handle->device_name, 0, sizeof(uart_handle->device_name));
        snprintf(uart_handle->device_name, sizeof(uart_handle->device_name), "Uart%d", i);
        uart_handle->lock_tx = xSemaphoreCreateBinary();
        uart_handle->lock    = xSemaphoreCreateBinary();
        if (uart_handle->lock == NULL) {
            elog_e(TAG, "create %s mutex failed!\r\n", uart_handle->device_name);
            return;
        }
        DevUartDMARecive(&(UartBspCfg[i].uart_cfg), uart_handle->rx_buffer,
                         UartBspCfg[i].buffer_size);
    }
}
// SYSTEM_REGISTER_INIT(MCUInitStage, UartPriority, UartDeviceInit, UartDeviceInit);

static void UartCreateTaskHandle(void) {
    UartDeviceInit();

    xTaskCreate(VFBTaskFrame, "VFBTaskUart", configMINIMAL_STACK_SIZE * 2, (void *)&Uart_task_cfg,
                UartPriority, NULL);
    xTaskCreate(UartStreamRcvTask, "UartRx", configMINIMAL_STACK_SIZE, (void *)&Uart_task_cfg,
                UartPriority - 1, NULL);
}
SYSTEM_REGISTER_INIT(ServerInitStage, UartPriority, UartCreateTaskHandle,
                     UartCreateTaskHandle init);

static void UartInitHandle(void *msg) {
    elog_i(TAG, "UartInitHandle\r\n");
    elog_set_filter_tag_lvl(TAG, UartLogLvl);
    vfb_send(UartStart, 0, NULL, 0);
}
// 5A A5 05 82 00A0 007D

// 接收消息的回调函数
static void UartRcvHandle(void *msg) {
    TaskHandle_t curTaskHandle    = xTaskGetCurrentTaskHandle();
    TypdefUartStatus *uart_handle = &UartStatus[0];
    char *taskName                = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg         = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case UartStart: {
            elog_i(TAG, "UartStart %d", tmp_msg->frame->head.data);
            DevUartStart(&(UartBspCfg[0].uart_cfg));

        } break;
        case UartStop: {
        } break;
        case UartSet: {
        } break;
        case UartGet: {
        } break;
        case UartSend: {
            if (tmp_msg->frame->head.length == 0 || tmp_msg->frame->head.payload_offset == NULL) {
                elog_e(TAG, "[ERROR]UartSend: payload is NULL or length is 0\r\n");
                return;
            }
            elog_d(TAG, "UartSend: length: %d", MSG_GET_LENGTH(tmp_msg));
            // elog_hexdump(TAG, 16, MSG_GET_PAYLOAD(tmp_msg), MSG_GET_LENGTH(tmp_msg));
            DevUartDMASend(&UartBspCfg[0].uart_cfg, (const uint8_t *)MSG_GET_PAYLOAD(tmp_msg),
                           MSG_GET_LENGTH(tmp_msg));
            // wait for TX DMA complete lock_tx
            if (uart_handle->lock_tx != NULL) {
                if (xSemaphoreTake(uart_handle->lock_tx, pdMS_TO_TICKS(300)) == pdFALSE) {
                    elog_e(TAG, "[ERROR]UartSend: lock_tx timeout\r\n");
                }
            } else {
                elog_e(TAG, "[ERROR]UartSend: lock_tx is NULL\r\n");
            }
        } break;
        case UartRcv: {
            elog_d(TAG, "Uart Com Recive :%s", (char *)MSG_GET_PAYLOAD(tmp_msg));
            // elog_hexdump(TAG, 8, MSG_GET_PAYLOAD(tmp_msg), MSG_GET_LENGTH(tmp_msg));

        } break;
        default:
            elog_e(TAG, "TASK %s RCV: unknown event: %d\r\n", taskName, tmp_msg->frame->head.event);
            break;
    }
}

// 超时处理的回调函数
static void UartCycHandle(void) {}
// UartStreamRcvTask
void UartStreamRcvTask(void *arg) {
    TypdefUartStatus *uart_handle = &UartStatus[0];
    int rcv_count                 = 0;
    memset(uart_handle->rx_buffer_for_vfb, 0, uart_handle->buffer_size);
    while (1) {
        rcv_count =
            xStreamBufferReceive(uart_handle->rx_stream_buffer, uart_handle->rx_buffer_for_vfb,
                                 uart_handle->buffer_size, portMAX_DELAY);
        if (rcv_count > 0) {
            vfb_send(UartRcv, 0, uart_handle->rx_buffer_for_vfb, rcv_count);
            memset(uart_handle->rx_buffer_for_vfb, 0, rcv_count);
        }
    }
}
static void __UartRXISRHandle(void *arg) {
    TypdefUartStatus *uart_handle = (TypdefUartStatus *)arg;
    SCB_CleanDCache_by_Addr((uint32_t *)uart_handle, sizeof(TypdefUartStatus));
    uint32_t dma_periph;
    uint32_t channelx;
    dma_periph                   = uart_handle->UartBspCfg->uart_cfg.rx_dma_base_addr;
    channelx                     = uart_handle->UartBspCfg->uart_cfg.rx_dma_channel;
    uint32_t dma_transfer_number = dma_transfer_number_get(dma_periph, channelx);
    if (dma_transfer_number == 0) {
        printf("\r\n[ERROR]DMA transfer number is zero, no data received.\r\n");
    }
    int rx_count = uart_handle->buffer_size - dma_transfer_number;
    // printf("RX ISR: rx_count = %d, buffer_size = %d ,dma_transfer_number = %d\r\n", rx_count,
    // uart_handle->buffer_size, dma_transfer_number);
    if (rx_count <= 0) {
        printf(
            "\r\n[ERROR]Uart RX count is zero or negative, buffer_size = %d, dma_transfer_number = "
            "%d\r\n",
            uart_handle->buffer_size, dma_transfer_number);
    } else if (rx_count > uart_handle->buffer_size) {
        printf(
            "\r\n[ERROR]Uart RX count is greater than buffer_size, buffer_size = %d, rx_count = "
            "%d\r\n",
            uart_handle->buffer_size, rx_count);
    } else {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (xStreamBufferSendFromISR(uart_handle->rx_stream_buffer, uart_handle->rx_buffer,
                                     rx_count, &xHigherPriorityTaskWoken) == 0) {
            printf("\r\n[ERROR]Failed to send data to stream buffer from ISR\r\n");
        }
        DevUartDMARecive(&(uart_handle->UartBspCfg->uart_cfg), uart_handle->rx_buffer,
                         uart_handle->buffer_size);
    }
}
static void __UartTXDMAISRHandle(void *arg) {
    TypdefUartStatus *uart_handle = (TypdefUartStatus *)arg;
    xSemaphoreGiveFromISR(uart_handle->lock_tx, NULL);
}

#endif