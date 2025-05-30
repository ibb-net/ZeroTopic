
#include "timer_decoder_cfg.h"

#if TIMER_DECODER_EN
#include "gd32h7xx.h"
#include <stdio.h>
#include "timer_decoder.h"

#include "os_server.h"
#include "elog.h"
#define TAG "ENCODER_TIMER"
typedef struct
{
	uint8_t channel_ctr; // 0 stop 1:start
	uint64_t last_timer; // 上次计数值
} TypdefEncoderStruct;
TypdefEncoderStruct encoder_struct[2] = {0};
void timer_encoder_init_config(void)
{
	printf("timer_encoder_init_config\r\n");
	timer_parameter_struct timer_initpara;

	rcu_periph_clock_enable(ENCODER_TIMER_RCU);

	rcu_periph_clock_enable(TIMER_A_CH_RCU);
	rcu_periph_clock_enable(TIMER_B_CH_RCU);

	gpio_af_set(TIMER_A_CH_PORT, GPIO_AF_13, TIMER_A_CH_PIN);
	gpio_af_set(TIMER_B_CH_PORT, GPIO_AF_13, TIMER_B_CH_PIN);

	gpio_mode_set(TIMER_A_CH_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, TIMER_A_CH_PIN);
	gpio_mode_set(TIMER_B_CH_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, TIMER_B_CH_PIN);

	gpio_output_options_set(TIMER_A_CH_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, TIMER_A_CH_PIN);
	gpio_output_options_set(TIMER_B_CH_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, TIMER_B_CH_PIN);

	timer_deinit(ENCODER_TIMER);

	/* TIMER2 configuration */
	timer_initpara.prescaler = 0;
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = 65535;
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(ENCODER_TIMER, &timer_initpara);

	// TIMER_IC_POLARITY_RISING：信号不反相
	// TIMER_IC_POLARITY_FALLING:信号反相
	// timer_quadrature_decoder_mode_config函数的参数ic0polarity对应CI0FE0，ic1polarity对应CI1FE1
	timer_quadrature_decoder_mode_config(ENCODER_TIMER, TIMER_QUAD_DECODER_MODE2, TIMER_IC_POLARITY_RISING, TIMER_IC_POLARITY_RISING);

	/* auto-reload preload enable */
	timer_auto_reload_shadow_enable(ENCODER_TIMER);

	// 设置初始计数值为0x50，用于判断计数个数和计数方向
	timer_counter_value_config(ENCODER_TIMER, DEFAULT_TIMER_VALUE);

	/* TIMER2 counter enable */
	timer_enable(ENCODER_TIMER);
}
SYSTEM_REGISTER_INIT(BoardInitStage, MCUPre_TIMER_ENCODER_INIT, timer_encoder_init_config, timer_encoder_init_config);

#define ENCODER_TIMER_TASK_PRIO (tskIDLE_PRIORITY + 1)

// 函数声明
static void ENCODER_TIMER_RCV_HANDLE(void *msg);
static void ENCODER_TIMER_CYC_HANDLE(void);
static void ENCODER_TIMER_INIT_HANDLE(void *msg);

// 定义事件列表
static const vfb_event_t ENCODER_TIMER_event_list[] = {
	ENCODER_TIMER_CH0_START,
	ENCODER_TIMER_CH0_STOP,
	ENCODER_TIMER_CH1_START,
	ENCODER_TIMER_CH1_STOP,
};

// 定义任务配置结构体
static const VFBTaskStruct ENCODER_TIMER_task_cfg = {
	.name = "VFBTaskENCODER_TIMER", // Task name
	.pvParameters = NULL,
	.uxPriority = 10,													 // Task parameters
	.queue_num = 8,														 // Number of queues to subscribe
	.event_list = ENCODER_TIMER_event_list,								 // Event list to subscribe
	.event_num = sizeof(ENCODER_TIMER_event_list) / sizeof(vfb_event_t), // Number of events to subscribe
	.startup_wait_event_list = NULL,									 // Events to wait for at startup
	.startup_wait_event_num = 0,										 // Number of startup events to wait for
	.xTicksToWait = pdMS_TO_TICKS(100),									 // Wait indefinitely
	.init_msg_cb = ENCODER_TIMER_INIT_HANDLE,							 // Callback for initialization messages
	.rcv_msg_cb = ENCODER_TIMER_RCV_HANDLE,								 // Callback for received messages
	.rcv_timeout_cb = ENCODER_TIMER_CYC_HANDLE,							 // Callback for timeout
};

// 任务初始化函数
void ENCODER_TIMER_CREATE_HANDLE(void)
{
	encoder_struct[0].channel_ctr = 0;					// 初始化通道0状态
	encoder_struct[1].channel_ctr = 0;					// 初始化通道1状态
	encoder_struct[0].last_timer = DEFAULT_TIMER_VALUE; // 初始化通道0上次计数值
	encoder_struct[1].last_timer = DEFAULT_TIMER_VALUE; // 初始化通道1上次计数值
	xTaskCreate(VFBTaskFrame, "VFBTaskENCODE", configMINIMAL_STACK_SIZE, (void *)&ENCODER_TIMER_task_cfg, ENCODER_TIMER_TASK_PRIO, NULL);
}
SYSTEM_REGISTER_INIT(AppInitStage, 01, ENCODER_TIMER_CREATE_HANDLE, ENCODER_TIMER_CREATE_HANDLE init);

static void ENCODER_TIMER_INIT_HANDLE(void *msg)
{
	elog_i(TAG, "ENCODER_TIMER_INIT_HANDLE\r\n");
	// 发布事件，表示计时器已准备就绪
	vfb_publish(ENCODER_TIMER_READY);
	vfb_publish(ENCODER_TIMER_CH0_START);
}
// 接收消息的回调函数
static void ENCODER_TIMER_RCV_HANDLE(void *msg)
{
	TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
	char *taskName = pcTaskGetName(curTaskHandle);
	vfb_message_t tmp_msg = (vfb_message_t)msg;
	switch (tmp_msg->frame->head.event)
	{
	case ENCODER_TIMER_CH0_START:
		elog_i(TAG, "ENCODER_TIMER_CH0_START");
		printf("ENCODER_TIMER_CH0_START\r\n");
		encoder_struct[0].channel_ctr = 1;
		encoder_struct[0].last_timer = timer_counter_read(ENCODER_TIMER);
		break;
	case ENCODER_TIMER_CH0_STOP:
		elog_i(TAG, "ENCODER_TIMER_CH0_STOP");
		encoder_struct[0].channel_ctr = 0;
		encoder_struct[0].last_timer = timer_counter_read(ENCODER_TIMER);
		break;
	case ENCODER_TIMER_CH1_START:
		elog_i(TAG, "ENCODER_TIMER_CH1_START");
		encoder_struct[1].channel_ctr = 1;
		encoder_struct[1].last_timer = timer_counter_read(ENCODER_TIMER);
		break;
	case ENCODER_TIMER_CH1_STOP:
		elog_i(TAG, "ENCODER_TIMER_CH1_STOP");
		encoder_struct[1].channel_ctr = 0;
		encoder_struct[1].last_timer = timer_counter_read(ENCODER_TIMER);
		break;

	default:
		printf("TASK %s RCV: unknown event: %d\r\n", taskName, tmp_msg->frame->head.event);
		break;
	}
}

#define ENCODER_TIMER TIMER22

// 超时处理的回调函数
static void ENCODER_TIMER_CYC_HANDLE(void)
{

	// elog_i(TAG,"ENCODER_TIMER_CYC_HANDLE\r\n");
	if (encoder_struct[0].channel_ctr)
	{
		uint64_t current_timer = timer_counter_read(ENCODER_TIMER);
		int64_t diff = current_timer - encoder_struct[0].last_timer;
		if (current_timer != encoder_struct[0].last_timer)
		{
			encoder_struct[0].last_timer = current_timer;
			elog_i(TAG, "CH0 diff: %ld", diff);
			elog_i(TAG, "CH0 Direction: %s", (diff > 0) ? "Forward" : "Backward");
			vfb_publish(ENCODER_TIMER_GET_CNT_CH0);
		}
	}
	if (encoder_struct[1].channel_ctr)
	{
		uint64_t current_timer = timer_counter_read(ENCODER_TIMER);
		int64_t diff = current_timer - encoder_struct[1].last_timer;
		if (current_timer != encoder_struct[1].last_timer)
		{
			encoder_struct[1].last_timer = current_timer;
			elog_i(TAG, "CH1 diff: %ld", diff);
			elog_i(TAG, "CH1 Direction: %s", (diff > 0) ? "Forward" : "Backward");
			vfb_publish(ENCODER_TIMER_GET_CNT_CH1);
		}
	}
}

#endif // TIMER_DECODER_EN