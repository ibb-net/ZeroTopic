
#include "DemoCfg.h"
#define CONFIG_Demo_EN 1
#if CONFIG_Demo_EN
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "gd32h7xx.h"
#include "string.h"

/* Device */
#include "Device.h"
#include "dev_basic.h"
#include "dev_pin.h"
#include "dev_uart.h"
#include "elog.h"
#include "os_server.h"
// #include "app_event.h"

#define TAG        "Demo"
#define DemoLogLvl ELOG_LVL_INFO

#define DemoPriority PriorityNormalEventGroup0
#ifndef DemoChannelMax
#define DemoChannelMax 1
#endif
#ifndef CONFIG_Demo_CYCLE_TIMER_MS
#define CONFIG_Demo_CYCLE_TIMER_MS 300
#endif
/* ===================================================================================== */
typedef struct
{
    uint32_t id;
} TypdefDemoBSPCfg;

static void __DemoCreateTaskHandle(void);
static void __DemoRcvHandle(void *msg);
static void __DemoCycHandle(void);
static void __DemoInitHandle(void *msg);

const TypdefDemoBSPCfg DemoBspCfg[DemoChannelMax] = {

};

typedef struct
{
    char device_name[DEVICE_NAME_MAX];
    uint32_t id;  // ID

} TypdefDemoStatus;
TypdefDemoStatus DemoStatus[DemoChannelMax] = {0};

/* ===================================================================================== */

static const vfb_event_t DemoEventList[] = {
    /*
        DemoStart,
        DemoStop,
        DemoSet,
        DemoGet,
        DemoPrint,
        DemoRcv,
    */

};

static const VFBTaskStruct Demo_task_cfg = {
    .name         = "VFBTaskDemo",  // Task name
    .pvParameters = NULL,
    // .uxPriority = 10,											  // Task parameters
    .queue_num               = 8,                                            // Number of queues to subscribe
    .event_list              = DemoEventList,                                // Event list to subscribe
    .event_num               = sizeof(DemoEventList) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                                         // Events to wait for at startup
    .startup_wait_event_num  = 0,                                            // Number of startup events to wait for
    .xTicksToWait            = pdMS_TO_TICKS(CONFIG_Demo_CYCLE_TIMER_MS),   // Wait indefinitely
    .init_msg_cb             = __DemoInitHandle,                             // Callback for initialization messages
    .rcv_msg_cb              = __DemoRcvHandle,                              // Callback for received messages
    .rcv_timeout_cb          = __DemoCycHandle,                              // Callback for timeout
};

/* ===================================================================================== */

void DemoDeviceInit(void) {
    elog_i(TAG, "DemoDeviceInit");

    for (size_t i = 0; i < DemoChannelMax; i++) {
        TypdefDemoStatus *DemoStatusHandle = &DemoStatus[i];

        DemoStatusHandle->id = i;
        memset(DemoStatusHandle->device_name, 0, sizeof(DemoStatusHandle->device_name));
        snprintf(DemoStatusHandle->device_name, sizeof(DemoStatusHandle->device_name), "Demo%d", i);
    }
}
SYSTEM_REGISTER_INIT(PreStartupInitStage, DemoPriority, DemoDeviceInit, DemoDeviceInit);

static void __DemoCreateTaskHandle(void) {
    for (size_t i = 0; i < DemoChannelMax; i++) {
    }
    xTaskCreate(VFBTaskFrame, "VFBTaskDemo", configMINIMAL_STACK_SIZE, (void *)&Demo_task_cfg, DemoPriority, NULL);
}
SYSTEM_REGISTER_INIT(AppInitStage, DemoPriority, __DemoCreateTaskHandle, __DemoCreateTaskHandle init);

static void __DemoInitHandle(void *msg) {
    elog_i(TAG, "__DemoInitHandle");
    //elog_set_filter_tag_lvl(TAG, DemoLogLvl);
    vfb_send(DemoStart, 0, NULL, 0);
}
// 接收消息的回调函数
static void __DemoRcvHandle(void *msg) {
    TaskHandle_t curTaskHandle      = xTaskGetCurrentTaskHandle();
    TypdefDemoStatus *DemoStatusTmp = &DemoStatus[0];
    char *taskName                  = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg           = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case DemoStart: {
            elog_i(TAG, "DemoStartTask %d", tmp_msg->frame->head.data);
        } break;
        default:
            elog_e(TAG, "TASK %s RCV: unknown event: %d", taskName, tmp_msg->frame->head.event);
            break;
    }
}

static void __DemoCycHandle(void) {
    TypdefDemoStatus *DemoStatusHandle = &DemoStatus[0];
    if (DemoStatusHandle == NULL) {
        elog_e(TAG, "[ERROR]DemoStatusHandle NULL");
        return;
    }
}

#endif

static void CmdDemoHelp(void) {
    elog_i(TAG, "\r\nUsage: demo <state>");
    elog_i(TAG, "  <state>: 0 for off, 1 for on");
    elog_i(TAG, "Example: demo 1");
}
static int CmdDemoHandle(int argc, char *argv[]) {
    if (argc < 2) {
        CmdDemoHelp();
        return 0;
    }

    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), Demo, CmdDemoHandle, demo command);