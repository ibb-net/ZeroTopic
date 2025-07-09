
#include "osMonitorCfg.h"
#define CONFIG_osMonitor_EN 1
#if CONFIG_osMonitor_EN
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

#define TAG        "osMonitor"
#define osMonitorLogLvl ELOG_LVL_INFO

#define osMonitorPriority PriorityIdleGroup1
#ifndef osMonitorChannelMax
#define osMonitorChannelMax 1
#endif
#ifndef CONFIG_osMonitor_CYCLE_TIMER_MS
#define CONFIG_osMonitor_CYCLE_TIMER_MS 5
#endif
/* ===================================================================================== */
typedef struct
{
    uint32_t id;
} TypdefosMonitorBSPCfg;

static void __osMonitorCreateTaskHandle(void);
static void __osMonitorRcvHandle(void *msg);
static void __osMonitorCycHandle(void);
static void __osMonitorInitHandle(void *msg);

const TypdefosMonitorBSPCfg osMonitorBspCfg[osMonitorChannelMax] = {

};

typedef struct
{
    char device_name[DEVICE_NAME_MAX];
    uint32_t id;  // ID

} TypdefosMonitorStatus;
TypdefosMonitorStatus osMonitorStatus[osMonitorChannelMax] = {0};

/* ===================================================================================== */

static const vfb_event_t osMonitorEventList[] = {
    /*
        osMonitorStart,
        osMonitorStop,
        osMonitorSet,
        osMonitorGet,
        osMonitorPrint,
        osMonitorRcv,
    */

};

static const VFBTaskStruct osMonitor_task_cfg = {
    .name         = "VFBTaskosMonitor",  // Task name
    .pvParameters = NULL,
    // .uxPriority = 10,											  // Task parameters
    .queue_num               = 8,                                            // Number of queues to subscribe
    .event_list              = osMonitorEventList,                                // Event list to subscribe
    .event_num               = sizeof(osMonitorEventList) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                                         // Events to wait for at startup
    .startup_wait_event_num  = 0,                                            // Number of startup events to wait for
    .xTicksToWait            = pdMS_TO_TICKS(CONFIG_osMonitor_CYCLE_TIMER_MS),   // Wait indefinitely
    .init_msg_cb             = __osMonitorInitHandle,                             // Callback for initialization messages
    .rcv_msg_cb              = __osMonitorRcvHandle,                              // Callback for received messages
    .rcv_timeout_cb          = __osMonitorCycHandle,                              // Callback for timeout
};

/* ===================================================================================== */

void osMonitorDeviceInit(void) {
    elog_i(TAG, "osMonitorDeviceInit");

    for (size_t i = 0; i < osMonitorChannelMax; i++) {
        TypdefosMonitorStatus *osMonitorStatusHandle = &osMonitorStatus[i];

        osMonitorStatusHandle->id = i;
        memset(osMonitorStatusHandle->device_name, 0, sizeof(osMonitorStatusHandle->device_name));
        snprintf(osMonitorStatusHandle->device_name, sizeof(osMonitorStatusHandle->device_name), "osMonitor%d", i);
    }
}
SYSTEM_REGISTER_INIT(PreStartupInitStage, osMonitorPriority, osMonitorDeviceInit, osMonitorDeviceInit);

static void __osMonitorCreateTaskHandle(void) {
    for (size_t i = 0; i < osMonitorChannelMax; i++) {
    }
    xTaskCreate(VFBTaskFrame, "VFBTaskosMonitor", configMINIMAL_STACK_SIZE, (void *)&osMonitor_task_cfg, osMonitorPriority, NULL);
}
SYSTEM_REGISTER_INIT(AppInitStage, osMonitorPriority, __osMonitorCreateTaskHandle, __osMonitorCreateTaskHandle init);

static void __osMonitorInitHandle(void *msg) {
    elog_i(TAG, "__osMonitorInitHandle");
    //elog_set_filter_tag_lvl(TAG, osMonitorLogLvl);
    vfb_send(osMonitorStart, 0, NULL, 0);
}
// 接收消息的回调函数
static void __osMonitorRcvHandle(void *msg) {
    TaskHandle_t curTaskHandle      = xTaskGetCurrentTaskHandle();
    TypdefosMonitorStatus *osMonitorStatusTmp = &osMonitorStatus[0];
    char *taskName                  = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg           = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case osMonitorStart: {
            elog_i(TAG, "osMonitorStartTask %d", tmp_msg->frame->head.data);
        } break;
        default:
            elog_e(TAG, "TASK %s RCV: unknown event: %d", taskName, tmp_msg->frame->head.event);
            break;
    }
}

static void __osMonitorCycHandle(void) {
    TypdefosMonitorStatus *osMonitorStatusHandle = &osMonitorStatus[0];
    if (osMonitorStatusHandle == NULL) {
        elog_e(TAG, "[ERROR]osMonitorStatusHandle NULL");
        return;
    }
}

#endif

static void CmdosMonitorHelp(void) {
    elog_i(TAG, "\r\nUsage: osMonitor <state>");
    elog_i(TAG, "  <state>: 0 for off, 1 for on");
    elog_i(TAG, "Example: osMonitor 1");
}
static int CmdosMonitorHandle(int argc, char *argv[]) {
    if (argc < 2) {
        CmdosMonitorHelp();
        return 0;
    }

    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), osMonitor, CmdosMonitorHandle, osMonitor command);