
#include "demo_cfg.h"
#define CONFIG_TIMER_DECODER_EN 1
#if CONFIG_TIMER_DECODER_EN
#include "gd32h7xx.h"
#include <stdio.h>
#include "timer_decoder.h"

#include "os_server.h"
#include "elog.h"
#define TAG "ENCODER"

#ifndef DemoChannelMax
#define DemoChannelMax 1
#endif
/* ===================================================================================== */

typedef struct
{
	uint32_t gpio_rcu;
	uint32_t gpio_port;
	uint32_t gpio_af;
	uint32_t gpio_pin;
} TypdefDemoBSPCfg;
const TypdefDemoBSPCfg DemoBspCfg[DemoChannelMax] = {

	{
		.gpio_rcu = ENCODER_CH0_GPIO_RCU,
		.gpio_port = ENCODER_CH0_GPIO_PORT,
		.gpio_af = ENCODER_CH0_GPIO_AF,
		.gpio_pin_b = ENCODER_CH0_GPIO_PIN_B,
	},
#if (DemoChannelMax > 1)
	{
		.gpio_rcu = ENCODER_CH0_GPIO_RCU,
		.gpio_port = ENCODER_CH0_GPIO_PORT,
		.gpio_af = ENCODER_CH0_GPIO_AF,
		.gpio_pin_b = ENCODER_CH0_GPIO_PIN_B,
	},
#endif
};

typedef struct
{
	uint8_t channel; // 通道号 0:CH0 1:CH1

} TypdefDemoStatus;
TypdefDemoStatus DemoStatus[DemoChannelMax] = {0};

/* ===================================================================================== */

static const vfb_event_t DemoEventList[] = {
	DemoStart,
	DemoStop,
	DemoSet,
	DemoGet

};
static void DemoCreateTaskHandle(void);
static void DemoRcvHandle(void *msg);
static void DemoCycHandle(void);
static void DemoInitHandle(void *msg);

static const VFBTaskStruct Demo_task_cfg = {
	.name = "VFBTaskDemo", // Task name
	.pvParameters = NULL,
	.uxPriority = 10,											  // Task parameters
	.queue_num = 8,												  // Number of queues to subscribe
	.event_list = DemoEventList,								  // Event list to subscribe
	.event_num = sizeof(DemoEventList) / sizeof(vfb_event_t),	  // Number of events to subscribe
	.startup_wait_event_list = NULL,							  // Events to wait for at startup
	.startup_wait_event_num = 0,								  // Number of startup events to wait for
	.xTicksToWait = pdMS_TO_TICKS(CONFIG_ENCODER_CYCLE_TIMER_MS), // Wait indefinitely
	.init_msg_cb = DemoInitHandle,								  // Callback for initialization messages
	.rcv_msg_cb = DemoRcvHandle,								  // Callback for received messages
	.rcv_timeout_cb = DemoCycHandle,							  // Callback for timeout
};

/* ===================================================================================== */

void DemoDeviceInit(void)
{
	printf("DemoDeviceInit\r\n");
	// TODO
}
SYSTEM_REGISTER_INIT(BoardInitStage, MCUPreDemoRegisterPriority, DemoDeviceInit, DemoDeviceInit);

static void DemoCreateTaskHandle(void)
{
	for (size_t i = 0; i < DemoChannelMax; i++)
	{
	}

	xTaskCreate(VFBTaskFrame, "VFBTaskENCODE", configMINIMAL_STACK_SIZE, (void *)&Demo_task_cfg, BSP_DEMO_TASK_PRIORITY, NULL);
}
SYSTEM_REGISTER_INIT(AppInitStage, ServerPreDemoRegisterPriority, DemoCreateTaskHandle, DemoCreateTaskHandle init);

static void DemoInitHandle(void *msg)
{
	elog_i(TAG, "DemoInitHandle\r\n");

	vfb_publish(DemoStartReady);					//
	vfb_send(DemoStart, DECODER_CHANNEL0, 0, NULL); // 启动计时器

	VFBMsgDecoderStruct msg_decoder = {
		.channel = DECODER_CHANNEL0,
		.phy_value = 0,
		.mode = DECODER_MODE_STEP,
	};
	vfb_send(DemoSet, 0, sizeof(VFBMsgDecoderStruct), &msg_decoder); // 设置计时器模式为步进模式
}
// 接收消息的回调函数
static void DemoRcvHandle(void *msg)
{
	TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
	char *taskName = pcTaskGetName(curTaskHandle);
	vfb_message_t tmp_msg = (vfb_message_t)msg;
	switch (tmp_msg->frame->head.event)
	{
	case DemoStart:
	{
	}
	break;
	case DemoStop:
	{
	}
	break;
	case DemoSet:
	{
	}
	break;
	default:
		printf("TASK %s RCV: unknown event: %d\r\n", taskName, tmp_msg->frame->head.event);
		break;
	}
}

// 超时处理的回调函数
static void DemoCycHandle(void)
{
	//
}

#endif