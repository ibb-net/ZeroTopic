
#include "DmaCfg.h"
#define CONFIG_DMA_EN 1
#if CONFIG_DMA_EN
#include "gd32h7xx.h"
#include <stdio.h>
// #include "DMA.h"

#include "os_server.h"
#include "list/ex_list_dma.h"
#include "elog.h"
#define TAG "DMA"
#define DMALogLvl ELOG_LVL_INFO

#ifndef DMAChannelMax
#define DMAChannelMax 1
#endif
#ifndef CONFIG_DMA_CYCLE_TIMER_MS
#define CONFIG_DMA_CYCLE_TIMER_MS 100
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
	/* Buffer */
	uint32_t buffer_size;
	uint32_t buffer_num;

} TypdefDMABSPCfg;

#if 1
const TypdefDMABSPCfg DMABspCfg[DMAChannelMax] = {
	{
		.tx_gpio_rcu = DMA_TX_GPIO_RCU,
		.tx_gpio_port = DMA_TX_GPIO_PORT,
		.tx_gpio_af = DMA_TX_GPIO_AF,
		.tx_gpio_pin = DMA_TX_GPIO_PIN,
		.rx_gpio_rcu = DMA_RX_GPIO_RCU,
		.rx_gpio_port = DMA_RX_GPIO_PORT,
		.rx_gpio_af = DMA_RX_GPIO_AF,
		.rx_gpio_pin = DMA_RX_GPIO_PIN,
		.uart_rcu = DMA_UART_RCU,
		.uart_base = DMA_UART_BASE,
		.baudrate = DMA_UART_BAUDRATE,
		.idle_timeout = DMA_UART_IDLE_TIMEOUT,
		.rx_dma_rcu = DMA_RX_DMA_BASE_ADDR,
		.rx_dma_base_addr = DMA_RX_DMA_BASE_ADDR,
		.rx_dma_channel = DMA_RX_DMA_CHANNEL,
		.rx_dma_request = DMA_RX_DMA_REQUEST,
		.rx_dma_request= DMA_RX_DMA_REQUEST,
		.tx_dma_rcu = DMA_TX_DMA_BASE_ADDR,
		.tx_dma_base_addr = DMA_TX_DMA_BASE_ADDR,
		.tx_dma_channel = DMA_TX_DMA_CHANNEL,
		.tx_dma_request = DMA_TX_DMA_REQUEST,
		.tx_dma_request= DMA_TX_DMA_REQUEST,
		.buffer_size = DMA_UART_BUFFER_SIZE,
		.buffer_num = DMA_UART_BUFFER_NUM,
	},
};
#endif
typedef struct
{
	uint32_t uart_base;
	char device_name[16];
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

} TypdefDMAStatus;
TypdefDMAStatus DMAStatus[DMAChannelMax] = {0};

/* ===================================================================================== */

static const vfb_event_t DMAEventList[] = {
	DMAStart,
	DMAStop,
	DMASet,
	DMAGet

};
static void DMACreateTaskHandle(void);
static void DMARcvHandle(void *msg);
static void DMACycHandle(void);
static void DMAInitHandle(void *msg);

static const VFBTaskStruct DMA_task_cfg = {
	.name = "VFBTaskDMA", // Task name
	.pvParameters = NULL,
	// .uxPriority = 10,											  // Task parameters
	.queue_num = 32,											// Number of queues to subscribe
	.event_list = DMAEventList,								// Event list to subscribe
	.event_num = sizeof(DMAEventList) / sizeof(vfb_event_t),	// Number of events to subscribe
	.startup_wait_event_list = NULL,							// Events to wait for at startup
	.startup_wait_event_num = 0,								// Number of startup events to wait for
	.xTicksToWait = pdMS_TO_TICKS(CONFIG_DMA_CYCLE_TIMER_MS), // Wait indefinitely
	.init_msg_cb = DMAInitHandle,								// Callback for initialization messages
	.rcv_msg_cb = DMARcvHandle,								// Callback for received messages
	.rcv_timeout_cb = DMACycHandle,							// Callback for timeout
};

/* ===================================================================================== */
static void __DMATxDmaInit(TypdefDMAStatus *uart_handle,uint8_t *tx_data, size_t txbuffer_size)
{
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
void DMADeviceInit(void)
{
	printf("DMADeviceInit\r\n");

	for (size_t i = 0; i < DMAChannelMax; i++)
	{
		TypdefDMAStatus *uart_handle = &DMAStatus[i];
		/* GPIO */
		gpio_af_set(DMABspCfg[i].tx_gpio_port, DMABspCfg[i].tx_gpio_af, DMABspCfg[i].tx_gpio_pin);
		gpio_af_set(DMABspCfg[i].rx_gpio_port, DMABspCfg[i].rx_gpio_af, DMABspCfg[i].rx_gpio_pin);
		gpio_mode_set(DMABspCfg[i].tx_gpio_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DMABspCfg[i].tx_gpio_pin);
		gpio_mode_set(DMABspCfg[i].rx_gpio_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DMABspCfg[i].rx_gpio_pin);
		gpio_output_options_set(DMABspCfg[i].tx_gpio_port, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, DMABspCfg[i].tx_gpio_pin);
		gpio_output_options_set(DMABspCfg[i].rx_gpio_port, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, DMABspCfg[i].rx_gpio_pin);
		/* Uart */
		rcu_periph_clock_enable(DMABspCfg[i].uart_rcu);
		usart_deinit(DMABspCfg[i].uart_base);
		usart_word_length_set(DMABspCfg[i].uart_base, USART_WL_8BIT);
		usart_stop_bit_set(DMABspCfg[i].uart_base, USART_STB_1BIT);
		usart_parity_config(DMABspCfg[i].uart_base, USART_PM_NONE);
		usart_baudrate_set(DMABspCfg[i].uart_base, DMABspCfg[i].baudrate);
		usart_receive_config(DMABspCfg[i].uart_base, USART_RECEIVE_ENABLE);
		usart_transmit_config(DMABspCfg[i].uart_base, USART_TRANSMIT_ENABLE);

		// TODO
		//  usart_receiver_timeout_enable(DMABspCfg[i].uart_base);
		//  usart_interrupt_enable(DMABspCfg[i].uart_base, USART_INT_RT);
		usart_receiver_timeout_threshold_config(DMABspCfg[i].uart_base, DMABspCfg[i].idle_timeout);
		usart_enable(DMABspCfg[i].uart_base); // TODO
		uart_handle->uart_base = DMABspCfg[i].uart_base;
		uart_handle->buffer_size = DMABspCfg[i].buffer_size;
		uart_handle->buffer_num = DMABspCfg[i].buffer_num;
		memset(uart_handle->device_name, 0, sizeof(uart_handle->device_name));
		snprintf(uart_handle->device_name, sizeof(uart_handle->device_name), "DMA%d", i);
		size_t one_item_size = sizeof(uint8_t) * uart_handle->buffer_size;
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

		/* DMA TX */
		rcu_periph_clock_enable(DMABspCfg[i].tx_dma_rcu);
		steam_info_struct item_steam_info;
		void *txbuffer = NULL;
		vReqestListDMABuffer(uart_handle->tx_list_handle, &item_steam_info);





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
SYSTEM_REGISTER_INIT(BoardInitStage, MCUPreDMARegisterPriority, DMADeviceInit, DMADeviceInit);

static void DMACreateTaskHandle(void)
{
	for (size_t i = 0; i < DMAChannelMax; i++)
	{
	}

	xTaskCreate(VFBTaskFrame, "VFBTaskENCODE", configMINIMAL_STACK_SIZE, (void *)&DMA_task_cfg, BspDMATaskPriority, NULL);
}
SYSTEM_REGISTER_INIT(AppInitStage, ServerPreDMARegisterPriority, DMACreateTaskHandle, DMACreateTaskHandle init);

static void DMAInitHandle(void *msg)
{
	elog_i(TAG, "DMAInitHandle\r\n");
	elog_set_filter_tag_lvl(TAG, DMALogLvl);

	vfb_publish(DMAStartReady);
	vfb_send(DMAStart, 0, NULL,0 );
}
// 接收消息的回调函数
static void DMARcvHandle(void *msg)
{
	TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
	char *taskName = pcTaskGetName(curTaskHandle);
	vfb_message_t tmp_msg = (vfb_message_t)msg;
	switch (tmp_msg->frame->head.event)
	{
	case DMAStart:
	{
		elog_i(TAG, "DMAStart %d", tmp_msg->frame->head.data);
	}
	break;
	case DMAStop:
	{
	}
	break;
	case DMASet:
	{
	}
	break;
	case DMAGet:
	{
	}
	break;
	default:
		printf("TASK %s RCV: unknown event: %d\r\n", taskName, tmp_msg->frame->head.event);
		break;
	}
}

// 超时处理的回调函数
static void DMACycHandle(void)
{
	//
}

#endif