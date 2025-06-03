
#include "DebugCfg.h"
#define CONFIG_TIMER_DECODER_EN 1
#if CONFIG_TIMER_DECODER_EN
#include "gd32h7xx.h"
#include <stdio.h>
// #include "Debug.h"

#include "os_server.h"
#include "list/ex_list_dma.h"
#include "elog.h"
#define TAG "ENCODER"
#define DebugLogLvl ELOG_LVL_INFO

#ifndef DebugChannelMax
#define DebugChannelMax 1
#endif
#ifndef CONFIG_DEBUG_CYCLE_TIMER_MS
#define CONFIG_DEBUG_CYCLE_TIMER_MS 100
#endif
/* ===================================================================================== */
typedef struct
{
	/* TX GPIO*/
	uint32_t tx_gpio_rcu;
	uint32_t tx_gpio_port;
	uint32_t tx_gpio_af;
	uint32_t tx_gpio_pin;
	/* RX GPIO*/
	uint32_t rx_gpio_rcu;
	uint32_t rx_gpio_port;
	uint32_t rx_gpio_af;
	uint32_t rx_gpio_pin;
	/* Uart */
	uint32_t uart_rcu;
	uint32_t uart_base;
	uint32_t baudrate;
	uint32_t idle_timeout;
	/* DMA */
	uint32_t rx_dma_rcu;
	uint32_t rx_dma_base_addr;
	uint32_t rx_dma_channel;
	uint32_t rx_dma_request;
	uint32_t tx_dma_rcu;
	uint32_t tx_dma_base_addr;
	uint32_t tx_dma_channel;
	uint32_t tx_dma_request;

} TypdefDebugBSPCfg;

#if 1
const TypdefDebugBSPCfg DebugBspCfg[DebugChannelMax] = {
	{
		.tx_gpio_rcu = DEBUG_TX_GPIO_RCU,
		.tx_gpio_port = DEBUG_TX_GPIO_PORT,
		.tx_gpio_af = DEBUG_TX_GPIO_AF,
		.tx_gpio_pin = DEBUG_TX_GPIO_PIN,
		.rx_gpio_rcu = DEBUG_RX_GPIO_RCU,
		.rx_gpio_port = DEBUG_RX_GPIO_PORT,
		.rx_gpio_af = DEBUG_RX_GPIO_AF,
		.rx_gpio_pin = DEBUG_RX_GPIO_PIN,
		.uart_rcu = DEBUG_UART_RCU,
		.uart_base = DEBUG_UART_BASE,
		.baudrate = DEBUG_UART_BAUDRATE,
		.idle_timeout = DEBUG_UART_IDLE_TIMEOUT,
		.rx_dma_base_addr = DEBUG_RX_DMA_BASE_ADDR,
		.rx_dma_channel = DEBUG_RX_DMA_CHANNEL,
		.rx_dma_request = DEBUG_RX_DMA_REQUEST,
		.tx_dma_base_addr = DEBUG_TX_DMA_BASE_ADDR,
		.tx_dma_channel = DEBUG_TX_DMA_CHANNEL,
		.tx_dma_request = DEBUG_TX_DMA_REQUEST,
	},
};
#endif
typedef struct
{
	uint32_t uart_base;

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
	DebugGet

};
static void DebugCreateTaskHandle(void);
static void DebugRcvHandle(void *msg);
static void DebugCycHandle(void);
static void DebugInitHandle(void *msg);

static const VFBTaskStruct Debug_task_cfg = {
	.name = "VFBTaskDebug", // Task name
	.pvParameters = NULL,
	// .uxPriority = 10,											  // Task parameters
	.queue_num = 32,											// Number of queues to subscribe
	.event_list = DebugEventList,								// Event list to subscribe
	.event_num = sizeof(DebugEventList) / sizeof(vfb_event_t),	// Number of events to subscribe
	.startup_wait_event_list = NULL,							// Events to wait for at startup
	.startup_wait_event_num = 0,								// Number of startup events to wait for
	.xTicksToWait = pdMS_TO_TICKS(CONFIG_DEBUG_CYCLE_TIMER_MS), // Wait indefinitely
	.init_msg_cb = DebugInitHandle,								// Callback for initialization messages
	.rcv_msg_cb = DebugRcvHandle,								// Callback for received messages
	.rcv_timeout_cb = DebugCycHandle,							// Callback for timeout
};

/* ===================================================================================== */

void DebugDeviceInit(void)
{
	printf("DebugDeviceInit\r\n");

	for (size_t i = 0; i < DebugChannelMax; i++)
	{
		TypdefDebugStatus * uart_handle = &DebugStatus[i];
		gpio_af_set(DebugBspCfg[i].tx_gpio_port, DebugBspCfg[i].tx_gpio_af, DebugBspCfg[i].tx_gpio_pin);
		gpio_af_set(DebugBspCfg[i].rx_gpio_port, DebugBspCfg[i].rx_gpio_af, DebugBspCfg[i].rx_gpio_pin);
		gpio_mode_set(DebugBspCfg[i].tx_gpio_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DebugBspCfg[i].tx_gpio_pin);
		gpio_mode_set(DebugBspCfg[i].rx_gpio_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DebugBspCfg[i].rx_gpio_pin);
		gpio_output_options_set(DebugBspCfg[i].tx_gpio_port, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, DebugBspCfg[i].tx_gpio_pin);
		gpio_output_options_set(DebugBspCfg[i].rx_gpio_port, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, DebugBspCfg[i].rx_gpio_pin);
		uart_handle->uart_base = DebugBspCfg[i].uart_base;
		rcu_periph_clock_enable(DebugBspCfg[i].uart_rcu);
		usart_deinit(uart_handle->uart_base);
		usart_word_length_set(uart_handle->uart_base, USART_WL_8BIT);
		usart_stop_bit_set(uart_handle->uart_base, USART_STB_1BIT);
		usart_parity_config(uart_handle->uart_base, USART_PM_NONE);
		usart_baudrate_set(uart_handle->uart_base, DebugBspCfg[i].baudrate);
		usart_receive_config(uart_handle->uart_base, USART_RECEIVE_ENABLE);
		usart_transmit_config(uart_handle->uart_base, USART_TRANSMIT_ENABLE);

		// usart_receiver_timeout_enable(uart_handle->uart_base);
		// usart_interrupt_enable(uart_handle->uart_base, USART_INT_RT);
		usart_receiver_timeout_threshold_config(uart_handle->uart_base, DebugBspCfg[i].idle_timeout);
		usart_enable(uart_handle->uart_base); // TODO

#if 0
		uart_handle->rx_list_handle = xCreateListDMA(uart_handle->buffer_num, one_item_size);
		uart_handle->tx_list_handle = xCreateListDMA(uart_handle->buffer_num, one_item_size);
		uart_handle->lock_tx = xSemaphoreCreateBinary();
		uart_handle->lock = xSemaphoreCreateBinary();
		if (uart_handle->rx_list_handle == NULL || uart_handle->tx_list_handle == NULL)
		{
			printf("create %s list failed!\r\n", uart_handle->device_name);
			return;
		}
		uart_handle->lock = xSemaphoreCreateMutex();
		if (uart_handle->lock == NULL)
		{
			printf("create %s mutex failed!\r\n", uart_handle->device_name);
			return;
		}
		uart_handle->rx_queue = xQueueCreate(uart_handle->buffer_num, sizeof(steam_info_struct));
		uart_handle->tx_queue = xQueueCreate(uart_handle->buffer_num, sizeof(steam_info_struct));
		if (uart_handle->rx_queue == NULL || uart_handle->tx_queue == NULL)
		{
			printf("create %s steambuf failed!\r\n", uart_handle->device_name);
			return;
		}
#endif
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
SYSTEM_REGISTER_INIT(BoardInitStage, MCUPreDebugRegisterPriority, DebugDeviceInit, DebugDeviceInit);

static void DebugCreateTaskHandle(void)
{
	for (size_t i = 0; i < DebugChannelMax; i++)
	{
	}

	xTaskCreate(VFBTaskFrame, "VFBTaskENCODE", configMINIMAL_STACK_SIZE, (void *)&Debug_task_cfg, BspDebugTaskPriority, NULL);
}
SYSTEM_REGISTER_INIT(AppInitStage, ServerPreDebugRegisterPriority, DebugCreateTaskHandle, DebugCreateTaskHandle init);

static void DebugInitHandle(void *msg)
{
	elog_i(TAG, "DebugInitHandle\r\n");
	elog_set_filter_tag_lvl(TAG, DebugLogLvl);

	vfb_publish(DebugStartReady);
	vfb_send(DebugStart, 0, 0, NULL);
}
// 接收消息的回调函数
static void DebugRcvHandle(void *msg)
{
	TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
	char *taskName = pcTaskGetName(curTaskHandle);
	vfb_message_t tmp_msg = (vfb_message_t)msg;
	switch (tmp_msg->frame->head.event)
	{
	case DebugStart:
	{
		elog_i(TAG, "DebugStart %d", tmp_msg->frame->head.data);
	}
	break;
	case DebugStop:
	{
	}
	break;
	case DebugSet:
	{
	}
	break;
	case DebugGet:
	{
	}
	break;
	default:
		printf("TASK %s RCV: unknown event: %d\r\n", taskName, tmp_msg->frame->head.event);
		break;
	}
}

// 超时处理的回调函数
static void DebugCycHandle(void)
{
	//
}

#endif