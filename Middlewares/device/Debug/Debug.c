
#include "DebugCfg.h"

#ifndef CONFIG_UART_DEBUG_EN
#define CONFIG_UART_DEBUG_EN 1
#endif
#if CONFIG_UART_DEBUG_EN
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

#define TAG           "DEBUG"
#define DebugLogLvl   ELOG_LVL_INFO
#define DebugPriority PriorityCommunitGroup3

#ifndef DebugChannelMax
#define DebugChannelMax 1
#endif
#ifndef CONFIG_DEBUG_CYCLE_TIMER_MS
#define CONFIG_DEBUG_CYCLE_TIMER_MS 5
#endif
/* ===================================================================================== */
typedef struct {
    /* TX GPIO*/
    DevPinHandleStruct tx_gpio_cfg;
    DevPinHandleStruct rx_gpio_cfg;
    DevUartHandleStruct uart_cfg;
    /* Buffer */
    uint32_t buffer_size;

} TypdefDebugBSPCfg;

static void DebugCreateTaskHandle(void);
static void DebugRcvHandle(void *msg);
static void DebugCycHandle(void);
static void DebugInitHandle(void *msg);
static void __DebugRXISRHandle(void *arg);
static void __DebugTXDMAISRHandle(void *arg);
void DebugStreamRcvTask(void *arg);
__attribute__((aligned(32))) uint8_t debug_rx_buffer[DebugChannelMax][DEBUG_UART_BUFFER_SIZE];
__attribute__((aligned(32))) uint8_t debug_tx_buffer[DebugChannelMax][DEBUG_UART_BUFFER_SIZE];
const TypdefDebugBSPCfg DebugBspCfg[DebugChannelMax] = {
    {

        .tx_gpio_cfg =
            {
                .device_name = "DEBUG_TX",
                .base        = DEBUG_TX_GPIO_PORT,
                .af          = DEBUG_TX_GPIO_AF,
                .pin         = DEBUG_TX_GPIO_PIN,
            },
        .rx_gpio_cfg =
            {
                .device_name = "DEBUG_RX",
                .base        = DEBUG_RX_GPIO_PORT,
                .af          = DEBUG_RX_GPIO_AF,
                .pin         = DEBUG_RX_GPIO_PIN,
            },
        .uart_cfg =
            {
                .id           = 0,  // ID
                .device_name  = "DEBUG_UART",
                .base         = DEBUG_UART_BASE,
                .baudrate     = DEBUG_UART_BAUDRATE,
                .idle_timeout = DEBUG_UART_IDLE_TIMEOUT,
                .rx_isr_cb    = __DebugRXISRHandle,  // RX ISR callback function
                .tx_isr_cb    = NULL,                // TX ISR callback function
                .error_isr_cb = NULL,                // Error ISR callback function
                /* DMA */
                .tx_dma_rcu       = DEBUG_TX_DMA_BASE_ADDR,
                .tx_dma_base_addr = DEBUG_TX_DMA_BASE_ADDR,
                .tx_dma_channel   = DEBUG_TX_DMA_CHANNEL,
                .tx_dma_request   = DEBUG_TX_DMA_REQUEST,
                .tx_dma_isr_cb    = __DebugTXDMAISRHandle,  // TX DMA ISR callback function
                .rx_dma_rcu       = DEBUG_RX_DMA_BASE_ADDR,
                .rx_dma_base_addr = DEBUG_RX_DMA_BASE_ADDR,
                .rx_dma_channel   = DEBUG_RX_DMA_CHANNEL,
                .rx_dma_request   = DEBUG_RX_DMA_REQUEST,
                .rx_dma_isr_cb    = __DebugRXISRHandle,  // RX DMA ISR callback function
            },
        .buffer_size = DEBUG_UART_BUFFER_SIZE,
    },
};

typedef struct {
    uint32_t uart_base;
    char device_name[DEVICE_NAME_MAX];
    uint8_t ready;
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
    uint8_t *rx_buffer;  // Pointer to the buffer
    /* lcfg */
    const TypdefDebugBSPCfg *DebugBspCfg;  // BSP configuration

} TypdefDebugStatus;
TypdefDebugStatus DebugStatus[DebugChannelMax] = {0};

/* ===================================================================================== */

static const vfb_event_t DebugEventList[] = {
    DebugStart, DebugStop, DebugSet, DebugGet, DebugPrint, DebugRcv,

};

static const VFBTaskStruct Debug_task_cfg = {
    .name         = "VFBTaskDebug",  // Task name
    .pvParameters = NULL,
    // .uxPriority = 10,
    // // Task parameters
    .queue_num  = 32,                                            // Number of queues to subscribe
    .event_list = DebugEventList,                                // Event list to subscribe
    .event_num  = sizeof(DebugEventList) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                             // Events to wait for at startup
    .startup_wait_event_num  = 0,  // Number of startup events to wait for
    .xTicksToWait            = pdMS_TO_TICKS(CONFIG_DEBUG_CYCLE_TIMER_MS),  // Wait indefinitely
    .init_msg_cb             = DebugInitHandle,  // Callback for initialization messages
    .rcv_msg_cb              = DebugRcvHandle,   // Callback for received messages
    .rcv_timeout_cb          = DebugCycHandle,   // Callback for timeout
};

/* ===================================================================================== */
void DebugComPreInit(void) {
    DevPinInit(&(DebugBspCfg[0].tx_gpio_cfg));
    DevPinInit(&(DebugBspCfg[0].rx_gpio_cfg));
    DevUartRegister(DebugBspCfg[0].uart_cfg.base, (void *)&DebugStatus[0]);
    DevUartPreInit(&(DebugBspCfg[0].uart_cfg));
    printf("DebugComPreInit Done\r\n");
}
/* ===================================================================================== */

void DebugDeviceInit(void) {
    printf("DebugDeviceInit\r\n");

    for (size_t i = 0; i < DebugChannelMax; i++) {
        TypdefDebugStatus *uart_handle = &DebugStatus[i];
        if (DebugBspCfg[i].uart_cfg.base == 0) {
            printf("DebugBspCfg[%d] uart base is 0, skip init!\r\n", i);
            continue;
        }
        uart_handle->id          = i;
        uart_handle->DebugBspCfg = &DebugBspCfg[i];
        DevPinInit(&(DebugBspCfg[i].tx_gpio_cfg));
        DevPinInit(&(DebugBspCfg[i].rx_gpio_cfg));

        DevUartRegister(DebugBspCfg[i].uart_cfg.base, (void *)uart_handle);
        DevUartInit(&(DebugBspCfg[i].uart_cfg));

        uart_handle->uart_base        = DebugBspCfg[i].uart_cfg.base;
        uart_handle->buffer_size      = DebugBspCfg[i].buffer_size;
        uart_handle->rx_stream_buffer = xStreamBufferCreate(DebugBspCfg[i].buffer_size, 1);
        // uart_handle->rx_buffer        = pvPortMalloc(DebugBspCfg[i].buffer_size);
        uart_handle->rx_buffer = debug_rx_buffer[i];  // Use the static aligned buffer
        if (uart_handle->rx_buffer == NULL) {
            printf("[Fault]malloc %s buffer failed!\r\n", DebugBspCfg[i].uart_cfg.device_name);
            while (1);
            return;
        }

        memset(uart_handle->device_name, 0, sizeof(uart_handle->device_name));
        snprintf(uart_handle->device_name, sizeof(uart_handle->device_name), "Debug%d", i);
        uart_handle->lock_tx = xSemaphoreCreateBinary();
        uart_handle->lock    = xSemaphoreCreateBinary();
        if (uart_handle->lock == NULL) {
            printf("create %s mutex failed!\r\n", uart_handle->device_name);
            return;
        }
        DevUartDMARecive(&(DebugBspCfg[i].uart_cfg), uart_handle->rx_buffer,
                         DebugBspCfg[i].buffer_size);
        // DevUartStart(&(DebugBspCfg[i].uart_cfg));
    }

    elog_start();
}
// SYSTEM_REGISTER_INIT(PreStartupInitStage, DebugPriority, DebugDeviceInit, DebugDeviceInit);

static void DebugCreateTaskHandle(void) {
    DebugDeviceInit();
    DebugStatus[0].ready = 0;
    xTaskCreate(VFBTaskFrame, "VFBTaskDebug", configMINIMAL_STACK_SIZE * 2, (void *)&Debug_task_cfg,
                DebugPriority, NULL);
    xTaskCreate(DebugStreamRcvTask, "DebugRx", configMINIMAL_STACK_SIZE, (void *)&Debug_task_cfg,
                DebugPriority - 1, NULL);
}
SYSTEM_REGISTER_INIT(PreStartupInitStage, DebugPriority, DebugCreateTaskHandle,
                     DebugCreateTaskHandle init);

static void DebugInitHandle(void *msg) {
    printf("DebugInitHandle\r\n");
    elog_set_filter_tag_lvl(TAG, DebugLogLvl);
    vfb_send(DebugStart, 0, NULL, 0);
    printf("DebugInitHandle Done\r\n");
}
// 接收消息的回调函数
static void DebugRcvHandle(void *msg) {
    TaskHandle_t curTaskHandle     = xTaskGetCurrentTaskHandle();
    TypdefDebugStatus *uart_handle = &DebugStatus[0];
    char *taskName                 = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg          = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case DebugStart: {
            DebugStatus[0].ready = 1;
            // DevUartDMARecive(&(DebugBspCfg[0].uart_cfg), uart_handle->rx_buffer,
            //                  DebugBspCfg[0].buffer_size);
            DevUartStart(&(DebugBspCfg[0].uart_cfg));
            elog_i(TAG, "DebugStart %d", tmp_msg->frame->head.data);
        } break;
        case DebugStop: {
        } break;
        case DebugSet: {
        } break;
        case DebugGet: {
        } break;
        case DebugPrint: {
            if (tmp_msg->frame->head.length == 0 || tmp_msg->frame->head.payload_offset == NULL) {
                printf("[ERROR]DebugPrint: payload is NULL or length is 0\r\n");
                return;
            }
            memcpy(debug_tx_buffer[0], MSG_GET_PAYLOAD(tmp_msg), MSG_GET_LENGTH(tmp_msg));
            DevUartDMASend(&DebugBspCfg[0].uart_cfg, (const uint8_t *)debug_tx_buffer[0], MSG_GET_LENGTH(tmp_msg));
            // wait for TX DMA complete lock_tx
            if (uart_handle->lock_tx != NULL) {
                if (xSemaphoreTake(uart_handle->lock_tx, pdMS_TO_TICKS(300)) == pdFALSE) {
                    printf("[ERROR]DebugPrint: lock_tx timeout\r\n");
                }
            } else {
                printf("[ERROR]DebugPrint: lock_tx is NULL\r\n");
            }
        } break;
        case DebugRcv: {
            // elog_i(TAG, "Debug Com Recive :%s", (char *)MSG_GET_PAYLOAD(tmp_msg));

        } break;
        default:
            printf("TASK %s RCV: unknown event: %d\r\n", taskName, tmp_msg->frame->head.event);
            break;
    }
}

// 超时处理的回调函数
uint8_t dma_buffer[DEBUG_UART_BUFFER_SIZE] = "Hello, Debug!";
static void DebugCycHandle(void) {
    // TypdefDebugStatus *uart_handle = &DebugStatus[0];
    // memset(dma_buffer, 0, sizeof(dma_buffer));
    // if (xStreamBufferReceive(uart_handle->rx_stream_buffer, dma_buffer, uart_handle->buffer_size,
    // 0) != 0) {
    //     vfb_send(DebugRcv, 0, dma_buffer, strlen((char *)dma_buffer));
    // }
}
// DebugStreamRcvTask
void DebugStreamRcvTask(void *arg) {
    TypdefDebugStatus *uart_handle = &DebugStatus[0];
    int rcv_count                  = 0;
    memset(dma_buffer, 0, sizeof(dma_buffer));
    while (1) {
        rcv_count = xStreamBufferReceive(uart_handle->rx_stream_buffer, dma_buffer,
                                         uart_handle->buffer_size, portMAX_DELAY);
        if (rcv_count > 0) {
            if (DebugStatus[0].ready) {
                vfb_send(DebugRcv, 0, dma_buffer, rcv_count);
            }
            memset(dma_buffer, 0, rcv_count);
        }
    }
}
static void __DebugRXISRHandle(void *arg) {
    TypdefDebugStatus *uart_handle = (TypdefDebugStatus *)arg;
    SCB_CleanDCache_by_Addr((uint32_t *)uart_handle, sizeof(TypdefDebugStatus));
    uint32_t dma_periph;
    uint32_t channelx;
    dma_periph                   = uart_handle->DebugBspCfg->uart_cfg.rx_dma_base_addr;
    channelx                     = uart_handle->DebugBspCfg->uart_cfg.rx_dma_channel;
    uint32_t dma_transfer_number = dma_transfer_number_get(dma_periph, channelx);
    if (dma_transfer_number == 0) {
        printf("DMA transfer number is zero, no data received.\r\n");
    }
    int rx_count = uart_handle->buffer_size - dma_transfer_number;
    // printf("RX ISR: rx_count = %d, buffer_size = %d ,dma_transfer_number = %d\r\n", rx_count,
    // uart_handle->buffer_size, dma_transfer_number);
    if (rx_count <= 0) {
        // DevErrorLED(1);  // Turn on error LED if no data received
    } else {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (xStreamBufferSendFromISR(uart_handle->rx_stream_buffer, uart_handle->rx_buffer,
                                     rx_count, &xHigherPriorityTaskWoken) == 0) {
            printf("Failed to send data to stream buffer from ISR\r\n");
        }
        DevUartDMARecive(&(uart_handle->DebugBspCfg->uart_cfg), uart_handle->rx_buffer,
                         uart_handle->buffer_size);
        DevErrorLED(0);
    }
}
static void __DebugTXDMAISRHandle(void *arg) {
    TypdefDebugStatus *uart_handle = (TypdefDebugStatus *)arg;
    xSemaphoreGiveFromISR(uart_handle->lock_tx, NULL);
}

#endif