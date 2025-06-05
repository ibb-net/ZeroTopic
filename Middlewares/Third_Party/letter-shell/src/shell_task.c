
#include "ShellCfg.h"
#define CONFIG_Shell_EN 1
#if CONFIG_Shell_EN
#include <stdio.h>

#include "gd32h7xx.h"
#include "string.h"
/* Device */
// #include "Device.h"
// #include "elog.h"
#include "elog.h"
#include "os_server.h"
#include "shell.h"

#define TAG "Shell"
#define ShellLogLvl ELOG_LVL_INFO
#define ShellBufferSize (512)
#ifndef ShellChannelMax
#define ShellChannelMax 1
#endif
#ifndef CONFIG_Shell_CYCLE_TIMER_MS
#define CONFIG_Shell_CYCLE_TIMER_MS 100
#endif
/* ===================================================================================== */
typedef struct
{
    uint32_t id;

} TypdefShellBSPCfg;

static void __ShellCreateTaskHandle(void);
static void __ShellRcvHandle(void *msg);
static void __ShellCycHandle(void);
static void __ShellInitHandle(void *msg);

const TypdefShellBSPCfg ShellBspCfg[ShellChannelMax] = {

};

typedef struct
{
    uint8_t is_init;  // Is initialized
    uint32_t id;      // ID
    Shell shell;
    uint16_t size;
    char shellBuffer[ShellBufferSize];

} TypdefShellStatus;
TypdefShellStatus ShellStatus[ShellChannelMax] = {0};

/* ===================================================================================== */

static const vfb_event_t ShellEventList[] = {
    DebugRcv,
    DebugStartReady,

};

static const VFBTaskStruct Shell_task_cfg = {
    .name         = "VFBTaskShell",  // Task name
    .pvParameters = NULL,
    // .uxPriority = 10,											  // Task parameters
    .queue_num               = 8,                                             // Number of queues to subscribe
    .event_list              = ShellEventList,                                // Event list to subscribe
    .event_num               = sizeof(ShellEventList) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                                          // Events to wait for at startup
    .startup_wait_event_num  = 0,                                             // Number of startup events to wait for
    .xTicksToWait            = pdMS_TO_TICKS(CONFIG_Shell_CYCLE_TIMER_MS),    // Wait indefinitely
    .init_msg_cb             = __ShellInitHandle,                             // Callback for initialization messages
    .rcv_msg_cb              = __ShellRcvHandle,                              // Callback for received messages
    .rcv_timeout_cb          = __ShellCycHandle,                              // Callback for timeout
};

/* ===================================================================================== */
signed short userShellWrite(char *data, unsigned short len) {
    vfb_send(DebugPrint, 0, data, len);
    return len;
}

static void __ShellCreateTaskHandle(void) {
    for (size_t i = 0; i < ShellChannelMax; i++) {
        memset(&ShellStatus[i], 0, sizeof(TypdefShellStatus));
        ShellStatus[i].is_init     = 0;  // Mark as initialized
        ShellStatus[i].id          = i;
        ShellStatus[i].size        = ShellBufferSize;
        ShellStatus[i].shell.read  = NULL;            // Assign your read function here
        ShellStatus[i].shell.write = userShellWrite;  // Assign your write function here
    }
    xTaskCreate(VFBTaskFrame, "VFBTaskShell", configMINIMAL_STACK_SIZE*4, (void *)&Shell_task_cfg, BspShellTaskPriority, NULL);
}
SYSTEM_REGISTER_INIT(ServerPerInitStage, ServerPreShellRegisterPriority, __ShellCreateTaskHandle, __ShellCreateTaskHandle init);

static void __ShellInitHandle(void *msg) {
    // printf("__ShellInitHandle\r\n");

    shellInit(&ShellStatus[0].shell, ShellStatus[0].shellBuffer, ShellStatus[0].size);
    ShellStatus[0].is_init = 1;  // Mark as initialized
}
// 接收消息的回调函数
static void __ShellRcvHandle(void *msg) {
    TaskHandle_t curTaskHandle     = xTaskGetCurrentTaskHandle();
    TypdefShellStatus *uart_handle = &ShellStatus[0];
    char *taskName                 = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg          = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case DebugStartReady: {
        } break;
        case DebugRcv: {
            if (ShellStatus[0].is_init == 0)
                break;
            char *buffer = (char *)MSG_GET_PAYLOAD(tmp_msg);
            size_t size  = MSG_GET_LENGTH(tmp_msg);
            if (size > 0 && buffer == NULL) {
                elog_i(TAG, "Received empty or invalid data\r\n");
            }
            printf("[Shell] RCV: %s\r\n", buffer);
            printf("[Shell] RCV size: %d\r\n", size);
            for (size_t i = 0; i < size-1; i++) {
                shellHandler(&(uart_handle->shell), buffer[i]);
            }
            printf("[Shell] RCV end\r\n");
        } break;
        default:
            elog_i(TAG, "TASK %s RCV: unknown event: %d\r\n", taskName, tmp_msg->frame->head.event);
            break;
    }
}

static void __ShellCycHandle(void) {
    TypdefShellStatus *ShellStatusHandle = &ShellStatus[0];
    if (ShellStatusHandle == NULL) {
        printf("[ERROR]ShellStatusHandle NULL\r\n");
        return;
    }
}

#endif