
// #include "DebugCfg.h"
#define CONFIG_TIMER_DECODER_EN 1
#if CONFIG_TIMER_DECODER_EN
#include "gd32h7xx.h"
#include <stdio.h>
// #include "Debug.h"

#include "os_server.h"
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
	uint32_t gpio_rcu;
	uint32_t gpio_port;
	uint32_t gpio_af;
	uint32_t gpio_pin;
} TypdefDebugBSPCfg;
#if 0
const TypdefDebugBSPCfg DebugBspCfg[DebugChannelMax] = {

	{
		.gpio_rcu = ENCODER_CH0_GPIO_RCU,
		.gpio_port = ENCODER_CH0_GPIO_PORT,
		.gpio_af = ENCODER_CH0_GPIO_AF,
		.gpio_pin_b = ENCODER_CH0_GPIO_PIN_B,
	},
#if (DebugChannelMax > 1)
	{
		.gpio_rcu = ENCODER_CH0_GPIO_RCU,
		.gpio_port = ENCODER_CH0_GPIO_PORT,
		.gpio_af = ENCODER_CH0_GPIO_AF,
		.gpio_pin_b = ENCODER_CH0_GPIO_PIN_B,
	},
#endif
};
#endif
typedef struct
{
	uint8_t channel; // 通道号 0:CH0 1:CH1

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
	// TODO
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