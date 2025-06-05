
#include "DebugCfg.h"
#define CONFIG_DEBUG_EN 1
#if CONFIG_DEBUG_EN
#include <stdio.h>

#include "gd32h7xx.h"
/* Device */
#include "Device.h"
#include "dev_basic.h"
#include "dev_pin.h"
#include "dev_uart.h"
// #include "Debug.h"
// #include "Dma.h"
#include "elog.h"
#include "list/ex_list_dma.h"
#include "os_server.h"

#define TAG "DEBUG"
#define DebugLogLvl ELOG_LVL_INFO

#ifndef DebugChannelMax
#define DebugChannelMax 1
#endif
#ifndef CONFIG_DEBUG_CYCLE_TIMER_MS
#define CONFIG_DEBUG_CYCLE_TIMER_MS 5
#endif
/* ===================================================================================== */
typedef struct
{
    /* TX GPIO*/
    DevPinHandleStruct tx_gpio_cfg;
    DevPinHandleStruct rx_gpio_cfg;
    DevUartHandleStruct uart_cfg;
    /* Buffer */
    uint32_t buffer_size;
    uint32_t buffer_num;

} TypdefDebugBSPCfg;

static void DebugCreateTaskHandle(void);
static void DebugRcvHandle(void *msg);
static void DebugCycHandle(void);
static void DebugInitHandle(void *msg);
static void __DebugRXISRHandle(void *arg);
static void __DebugTXDMAISRHandle(void *arg);

const TypdefDebugBSPCfg DebugBspCfg[DebugChannelMax] = {
    {

        .tx_gpio_cfg = {
            .device_name = "DEBUG_TX",
            .base        = DEBUG_TX_GPIO_PORT,
            .af          = DEBUG_TX_GPIO_AF,
            .pin         = DEBUG_TX_GPIO_PIN,
        },
        .rx_gpio_cfg = {
            .device_name = "DEBUG_RX",
            .base        = DEBUG_RX_GPIO_PORT,
            .af          = DEBUG_RX_GPIO_AF,
            .pin         = DEBUG_RX_GPIO_PIN,
        },
        .uart_cfg = {
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
            .rx_dma_isr_cb    = NULL,  // RX DMA ISR callback function
        },
        .buffer_size = DEBUG_UART_BUFFER_SIZE,
        .buffer_num  = DEBUG_UART_BUFFER_NUM,
    },
};

typedef struct
{
    uint32_t uart_base;
    char device_name[DEVICE_NAME_MAX];
    uint32_t id;  // ID
    /* Lock */
    SemaphoreHandle_t lock;
    SemaphoreHandle_t lock_tx;
    /* Queue */
    SemaphoreHandle_t rx_queue;
    SemaphoreHandle_t tx_queue;
    /* Thread */
    TaskHandle_t rx_task_handle;
    TaskHandle_t tx_task_handle;
    /* Buffer */
    size_t buffer_size;
    size_t buffer_num;
    /* DMA List */
    list_dma_buffer_t rx_list_handle;
    list_dma_buffer_t tx_list_handle;

} TypdefDebugStatus;
TypdefDebugStatus DebugStatus[DebugChannelMax] = {0};

/* ===================================================================================== */

static const vfb_event_t DebugEventList[] = {
    DebugStart,
    DebugStop,
    DebugSet,
    DebugGet,
    DebugPrint,

};

static const VFBTaskStruct Debug_task_cfg = {
    .name         = "VFBTaskDebug",  // Task name
    .pvParameters = NULL,
    // .uxPriority = 10,											  // Task parameters
    .queue_num               = 32,                                            // Number of queues to subscribe
    .event_list              = DebugEventList,                                // Event list to subscribe
    .event_num               = sizeof(DebugEventList) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                                          // Events to wait for at startup
    .startup_wait_event_num  = 0,                                             // Number of startup events to wait for
    .xTicksToWait            = pdMS_TO_TICKS(CONFIG_DEBUG_CYCLE_TIMER_MS),    // Wait indefinitely
    .init_msg_cb             = DebugInitHandle,                               // Callback for initialization messages
    .rcv_msg_cb              = DebugRcvHandle,                                // Callback for received messages
    .rcv_timeout_cb          = DebugCycHandle,                                // Callback for timeout
};

/* ===================================================================================== */

void DebugDeviceInit(void) {
    printf("DebugDeviceInit\r\n");

    for (size_t i = 0; i < DebugChannelMax; i++) {
        TypdefDebugStatus *uart_handle = &DebugStatus[i];
        if (DebugBspCfg[i].uart_cfg.base == 0) {
            printf("DebugBspCfg[%d] uart base is 0, skip init!\r\n", i);
            continue;
        }
        uart_handle->id = i;
        DevPinInit(&(DebugBspCfg[i].tx_gpio_cfg));
        DevPinInit(&(DebugBspCfg[i].rx_gpio_cfg));
        DevUartRegister(DebugBspCfg[i].uart_cfg.base, (void *)uart_handle);
        DevUartInit(&(DebugBspCfg[i].uart_cfg));
        DevUarStart(&(DebugBspCfg[i].uart_cfg));  // TODO 启动需要单独剥离出来

        uart_handle->uart_base   = DebugBspCfg[i].uart_cfg.base;
        uart_handle->buffer_size = DebugBspCfg[i].buffer_size;
        uart_handle->buffer_num  = DebugBspCfg[i].buffer_num;
        memset(uart_handle->device_name, 0, sizeof(uart_handle->device_name));
        snprintf(uart_handle->device_name, sizeof(uart_handle->device_name), "Debug%d", i);
        size_t one_item_size        = sizeof(uint8_t) * uart_handle->buffer_size;
        uart_handle->rx_list_handle = xCreateListDMA(uart_handle->buffer_num, one_item_size);
        uart_handle->tx_list_handle = xCreateListDMA(uart_handle->buffer_num, one_item_size);
        uart_handle->lock_tx        = xSemaphoreCreateBinary();
        uart_handle->lock           = xSemaphoreCreateBinary();
        if (uart_handle->rx_list_handle == NULL || uart_handle->tx_list_handle == NULL) {
            printf("create %s list failed!\r\n", uart_handle->device_name);
            return;
        }
        uart_handle->lock = xSemaphoreCreateMutex();
        if (uart_handle->lock == NULL) {
            printf("create %s mutex failed!\r\n", uart_handle->device_name);
            return;
        }
        uart_handle->rx_queue = xQueueCreate(uart_handle->buffer_num, sizeof(steam_info_struct));
        uart_handle->tx_queue = xQueueCreate(uart_handle->buffer_num, sizeof(steam_info_struct));
        if (uart_handle->rx_queue == NULL || uart_handle->tx_queue == NULL) {
            printf("create %s steambuf failed!\r\n", uart_handle->device_name);
            return;
        }

        printf("create %s ok!\r\n", uart_handle->device_name);

#if 0
		/* Create task */
		BaseType_t ret;
		ret = xTaskCreate(bsp_uart_rx_thread, "uart_rx_task", configMINIMAL_STACK_SIZE * 2, (void *const)uart_handle, 2, NULL);
		if (ret != pdPASS)
		{
			printf("create rx task failed!\r\n");
			return;
		}

		ret = xTaskCreate(bsp_uart_tx_thread, "uart_tx_task", configMINIMAL_STACK_SIZE * 2, (void *const)uart_handle, 2, NULL);
		if (ret != pdPASS)
		{
			vTaskDelete(uart_handle->rx_task_handle);
			uart_handle->rx_task_handle = NULL;
			printf("create tx task failed!\r\n");
			return;
		}
#endif
    }
}
SYSTEM_REGISTER_INIT(MCUPreInitStage, MCUPreDebugRegisterPriority, DebugDeviceInit, DebugDeviceInit);

static void DebugCreateTaskHandle(void) {
    for (size_t i = 0; i < DebugChannelMax; i++) {
    }

    xTaskCreate(VFBTaskFrame, "VFBTaskDebug", configMINIMAL_STACK_SIZE, (void *)&Debug_task_cfg, BspDebugTaskPriority, NULL);
}
SYSTEM_REGISTER_INIT(AppInitStage, ServerPreDebugRegisterPriority, DebugCreateTaskHandle, DebugCreateTaskHandle init);

static void DebugInitHandle(void *msg) {
    elog_i(TAG, "DebugInitHandle\r\n");
    elog_set_filter_tag_lvl(TAG, DebugLogLvl);

    vfb_publish(DebugStartReady);
    vfb_send(DebugStart, 0, NULL, 0);
}
// 接收消息的回调函数
static void DebugRcvHandle(void *msg) {
    TaskHandle_t curTaskHandle     = xTaskGetCurrentTaskHandle();
    TypdefDebugStatus *uart_handle = &DebugStatus[0];
    char *taskName                 = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg          = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case DebugStart: {
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
            DevUartDMASend(&DebugBspCfg[0].uart_cfg, (const uint8_t *)MSG_GET_PAYLOAD(tmp_msg), MSG_GET_LENGTH(tmp_msg));
            // wait for TX DMA complete lock_tx
            if (uart_handle->lock_tx != NULL) {
                if (xSemaphoreTake(uart_handle->lock_tx, pdMS_TO_TICKS(100)) == pdFALSE) {
                    printf("[ERROR]DebugPrint: lock_tx timeout\r\n");
                }
            } else {
                printf("[ERROR]DebugPrint: lock_tx is NULL\r\n");
            }
        } break;
        default:
            printf("TASK %s RCV: unknown event: %d\r\n", taskName, tmp_msg->frame->head.event);
            break;
    }
}

// 超时处理的回调函数
uint8_t dma_buffer[128] = "Hello, Debug!";
static void DebugCycHandle(void) {
    TypdefDebugStatus *uart_handle = &DebugStatus[0];
    elog_d(TAG, "DebugCycHandle called");
    static uint32_t cycle_count = 0;
    static uint32_t counter     = 0;
    if (cycle_count++ % (1000 / CONFIG_DEBUG_CYCLE_TIMER_MS) == 0) {
        memset(dma_buffer, 0, sizeof(dma_buffer));
        sprintf((char *)dma_buffer, "DebugCycHandle cycle count: %d\r\n", counter);
        // DevUartDMASend(&DebugBspCfg[0].uart_cfg, (const uint8_t *)dma_buffer, 128);
        vfb_send(DebugPrint, 0, dma_buffer, sizeof(dma_buffer));
        elog_i(TAG, "DebugCycHandle cycle count: %d", counter);
        elog_w(TAG, "DebugCycHandle cycle count: %d", counter);
        elog_e(TAG, "DebugCycHandle cycle count: %d", counter);
        counter++;
    } else {
        // elog_d(TAG, "DebugCycHandle cycle count: %d", cycle_count);
    }
}
static void __DebugRXISRHandle(void *arg) {
    printf("RX ISR callback invoked with argument: %p\n", arg);
}
static void __DebugTXDMAISRHandle(void *arg) {
    TypdefDebugStatus *uart_handle = (TypdefDebugStatus *)arg;
    xSemaphoreGiveFromISR(uart_handle->lock_tx, NULL);
}

#endif