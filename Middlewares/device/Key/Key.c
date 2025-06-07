
#include "KeyCfg.h"
#define CONFIG_DEBUG_EN 1
#if CONFIG_DEBUG_EN
#include <stdio.h>

#include "gd32h7xx.h"
#include "string.h"

/* Device */
#include "Device.h"
#include "Key.h"
#include "dev_basic.h"
#include "dev_pin.h"
#include "dev_uart.h"
#include "elog.h"
#include "os_server.h"
// #include "app_event.h"

#define TAG         "Key"
#define KeyLogLvl   ELOG_LVL_INFO
#define KeyPriority PriorityOperationGroup0

#ifndef KeyChannelMax
#define KeyChannelMax 32
#endif
#ifndef CONFIG_DEBUG_CYCLE_TIMER_MS
#define CONFIG_DEBUG_CYCLE_TIMER_MS 30
#endif
/* ===================================================================================== */

static void __KeyCreateTaskHandle(void);
static void __KeyRcvHandle(void *msg);
static void __KeyCycHandle(void);
static void __KeyInitHandle(void *msg);
static void __KeyRegister(TypdefKeyBSPCfg *cfg);
static void __KeyScan(void);
static void key_short_press_callback(TypdefKeyStatus *key_status);
static void key_short_press_callback(TypdefKeyStatus *key_status);

// const TypdefKeyBSPCfg KeyBspCfg[KeyChannelMax] = {

// };

TypdefKeyStatus KeyStatus[KeyChannelMax] = {0};

/* ===================================================================================== */

static const vfb_event_t KeyEventList[] = {
    KeyRegister,  // Register key event
    KeyUnRister,  // Unregister key event
    KeyStart,     // Start key task
    KeyStop,      // Stop key task
};

static const VFBTaskStruct Key_task_cfg = {
    .name         = "VFBTaskKey",  // Task name
    .pvParameters = NULL,
    // .uxPriority = 10,											  // Task parameters
    .queue_num               = 8,                                           // Number of queues to subscribe
    .event_list              = KeyEventList,                                // Event list to subscribe
    .event_num               = sizeof(KeyEventList) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                                        // Events to wait for at startup
    .startup_wait_event_num  = 0,                                           // Number of startup events to wait for
    .xTicksToWait            = pdMS_TO_TICKS(CONFIG_DEBUG_CYCLE_TIMER_MS),  // Wait indefinitely
    .init_msg_cb             = __KeyInitHandle,                             // Callback for initialization messages
    .rcv_msg_cb              = __KeyRcvHandle,                              // Callback for received messages
    .rcv_timeout_cb          = __KeyCycHandle,                              // Callback for timeout
};

/* ===================================================================================== */
#if 0
void KeyDeviceInit(void) {
    elog_i(TAG, "KeyDeviceInit\r\n");

    for (size_t i = 0; i < KeyChannelMax; i++) {
        TypdefKeyStatus *KeyStatusHandle      = &KeyStatus[i];
        KeyStatusHandle->cfg                  = NULL;
        KeyStatusHandle->enable               = 0;
        KeyStatusHandle->state                = 0;
        KeyStatusHandle->press_time           = 0;
        KeyStatusHandle->long_press_triggered = 0;
    }
}
SYSTEM_REGISTER_INIT(BoardInitStage, KeyPriority, KeyDeviceInit, KeyDeviceInit);
#endif
static void __KeyCreateTaskHandle(void) {
    for (size_t i = 0; i < KeyChannelMax; i++) {
        TypdefKeyStatus *KeyStatusHandle      = &KeyStatus[i];
        KeyStatusHandle->cfg                  = NULL;
        KeyStatusHandle->enable               = 0;
        KeyStatusHandle->state                = 0;
        KeyStatusHandle->press_time           = 0;
        KeyStatusHandle->long_press_triggered = 0;
    }
    xTaskCreate(VFBTaskFrame, "VFBTaskKey", configMINIMAL_STACK_SIZE * 2, (void *)&Key_task_cfg, KeyPriority, NULL);
}
SYSTEM_REGISTER_INIT(ServerInitStage, KeyPriority, __KeyCreateTaskHandle, __KeyCreateTaskHandle init);

static void __KeyInitHandle(void *msg) {
    elog_i(TAG, "__KeyInitHandle\r\n");
    elog_set_filter_tag_lvl(TAG, KeyLogLvl);
    vfb_send(KeyStart, 0, NULL, 0);
}
// 接收消息的回调函数
static void __KeyRcvHandle(void *msg) {
    TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
    // TypdefKeyStatus *KeyStatusTmp = &KeyStatus[0];
    char *taskName        = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case KeyStart: {
            elog_i(TAG, "KeyStartTask %d", tmp_msg->frame->head.data);
        } break;

        case KeyRegister: {
            __KeyRegister((TypdefKeyBSPCfg *)MSG_GET_PAYLOAD(tmp_msg));
            elog_i(TAG, "KeyRegister: %s", ((TypdefKeyBSPCfg *)MSG_GET_PAYLOAD(tmp_msg))->device_name);
        } break;

        default:
            printf("TASK %s RCV: unknown event: %d\r\n", taskName, tmp_msg->frame->head.event);
            break;
    }
}

static void __KeyCycHandle(void) {
    // TypdefKeyStatus *KeyStatusHandle = &KeyStatus[0];
    // if (KeyStatusHandle == NULL) {
    //     elog_e(TAG, "[ERROR]KeyStatusHandle NULL\r\n");
    //     return;
    // }
    static uint32_t last_scan_time = 0;
    last_scan_time++;
    if (last_scan_time % (1000 / pdMS_TO_TICKS(CONFIG_DEBUG_CYCLE_TIMER_MS)) == 0) {
        // elog_i(TAG, "__KeyCycHandle\r\n");
    }
    __KeyScan();
}
static void key_short_press_callback(TypdefKeyStatus *key_status) {
    if (key_status->cfg->short_press_event == 0) {
        // elog_e(TAG, "Short press event is not configured for key %s\r\n", key_status->cfg->device_name);
        return;
    }
    elog_i(TAG, "Key %s short pressed\r\n", key_status->cfg->device_name);
    vfb_send(key_status->cfg->short_press_event, 0, NULL, 0);
}
static void key_long_press_callback(TypdefKeyStatus *key_status) {
    if (key_status->cfg->long_press_event == 0) {
        // elog_e(TAG, "Long press event is not configured for key %s\r\n", key_status->cfg->device_name);
        return;
    }
    elog_i(TAG, "Key %s long pressed\r\n", key_status->cfg->device_name);
    vfb_send(key_status->cfg->long_press_event, 0, NULL, 0);
}
// 注册按键,遍历KeyStatus查找cfg为空,并新增
static void __KeyRegister(TypdefKeyBSPCfg *cfg) {
    if (cfg == NULL) {
        elog_e(TAG, "Key configuration is NULL, please check the configuration.\r\n");
        return;
    }
    if (cfg->pin.base == 0 || cfg->pin.pin == 0) {
        elog_e(TAG, "Key pin configuration is invalid, please check the configuration.\r\n");
        return;
    }
    if (cfg->pin.pin_mode != DevPinModeInput) {
        elog_e(TAG, "Key pin mode is not input, please check the configuration.\r\n");
        return;
    }
    for (size_t i = 0; i < KeyChannelMax; i++) {
        if (KeyStatus[i].cfg == NULL) {
            KeyStatus[i].cfg = pvPortMalloc(sizeof(TypdefKeyBSPCfg));
            if (KeyStatus[i].cfg == NULL) {
                elog_e(TAG, "Failed to allocate memory for key configuration.\r\n");
                return;
            }
            memcpy(KeyStatus[i].cfg, cfg, sizeof(TypdefKeyBSPCfg));
            KeyStatus[i].enable               = 1;
            KeyStatus[i].state                = 0;
            KeyStatus[i].press_time           = 0;
            KeyStatus[i].long_press_triggered = 0;

            /* InitGPIO */
            DevPinInit(&cfg->pin);
            elog_i(TAG, "Key %s registered successfully on slot %d\r\n", cfg->device_name, i);
            return;
        }
    }
    elog_e(TAG, "No available key slots to register\r\n");
}
static void __KeyScan(void) {
    for (int i = 0; i < KeyChannelMax; i++) {
        TypdefKeyStatus *key_states = &KeyStatus[i];
        if (key_states->cfg == NULL || !key_states->enable) {
            continue;  // Skip if key is not registered or disabled
        }
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        uint8_t key_value     = DevPinRead(&key_states->cfg->pin);

        if (key_value == 0 && key_states->state == 0) {
            // Key pressed
            key_states->state                = 1;
            key_states->press_time           = current_time;
            key_states->long_press_triggered = 0;
        } else if (key_value == 0 && key_states->state == 1) {
            // Key is still pressed
            uint32_t press_duration = current_time - key_states->press_time;
            if (press_duration >= LONG_PRESS_THRESHOLD && !key_states->long_press_triggered) {
                key_long_press_callback(key_states);
                key_states->long_press_triggered = 1;
            }
        } else if (key_value == 1 && key_states->state == 1) {
            // Key released
            uint32_t press_duration = current_time - key_states->press_time;
            if (press_duration < LONG_PRESS_THRESHOLD) {
                key_short_press_callback(key_states);
            }
            key_states->state = 0;
        }
    }
}
static void CmdKeyHelp(void) {
    printf("Usage: Key <state>\r\n");
    printf("  <state>: 0 for off, 1 for on\r\n");
    printf("Example: Key 1\r\n");
}
static int CmdKeyHandle(int argc, char *argv[]) {
    if (argc != 2) {
        CmdKeyHelp();
        return 0;
    }

    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), Key, CmdKeyHandle, Key command);
#endif
