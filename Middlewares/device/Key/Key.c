
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
#ifndef CONFIG_KEY_CYCLE_TIMER_MS
#define CONFIG_KEY_CYCLE_TIMER_MS 30
#endif
/* ===================================================================================== */

static void __KeyCreateTaskHandle(void);
static void __KeyRcvHandle(void *msg);
static void __KeyCycHandle(void);
static void __KeyInitHandle(void *msg);
static void __KeyRegister(TypdefKeyBSPCfg *cfg);
static void __KeyCtrl(TypdefKeyBSPCfg *cfg, uint8_t en);
static void __KeyScan(void);
static void __KeyShortPressCallback(TypdefKeyStatus *key_status);
static void __KeyLongPressCallback(TypdefKeyStatus *key_status);
static void __KeyToggleCallback(TypdefKeyStatus *key_status);

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
    // .uxPriority = 10,
    // // Task parameters
    .queue_num  = 8,                                           // Number of queues to subscribe
    .event_list = KeyEventList,                                // Event list to subscribe
    .event_num  = sizeof(KeyEventList) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                           // Events to wait for at startup
    .startup_wait_event_num  = 0,  // Number of startup events to wait for
    .xTicksToWait            = pdMS_TO_TICKS(CONFIG_KEY_CYCLE_TIMER_MS),  // Wait indefinitely
    .init_msg_cb             = __KeyInitHandle,  // Callback for initialization messages
    .rcv_msg_cb              = __KeyRcvHandle,   // Callback for received messages
    .rcv_timeout_cb          = __KeyCycHandle,   // Callback for timeout
};

/* ===================================================================================== */
#if 0
void KeyDeviceInit(void) {
    elog_i(TAG, "KeyDeviceInit");

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
        memset(&KeyStatus[i], 0, sizeof(TypdefKeyStatus));
        TypdefKeyStatus *KeyStatusHandle = &KeyStatus[i];
        KeyStatusHandle->last_pin_value  = 0xFF;  // Initialize last pin value to 0xFF
    }
    xTaskCreate(VFBTaskFrame, "VFBTaskKey", configMINIMAL_STACK_SIZE * 2, (void *)&Key_task_cfg,
                KeyPriority, NULL);
}
SYSTEM_REGISTER_INIT(ServerInitStage, KeyPriority, __KeyCreateTaskHandle,
                     __KeyCreateTaskHandle init);

static void __KeyInitHandle(void *msg) {
    elog_i(TAG, "__KeyInitHandle");
    elog_set_filter_tag_lvl(TAG, KeyLogLvl);
}
// 接收消息的回调函数
static void __KeyRcvHandle(void *msg) {
    TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
    // TypdefKeyStatus *KeyStatusTmp = &KeyStatus[0];
    char *taskName        = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case KeyStart: {
            __KeyCtrl((TypdefKeyBSPCfg *)MSG_GET_PAYLOAD(tmp_msg), 1);
        } break;
        case KeyStop: {
            __KeyCtrl((TypdefKeyBSPCfg *)MSG_GET_PAYLOAD(tmp_msg), 0);
        } break;

        case KeyRegister: {
            __KeyRegister((TypdefKeyBSPCfg *)MSG_GET_PAYLOAD(tmp_msg));
        } break;

        default:
            printf("TASK %s RCV: unknown event: %d", taskName, tmp_msg->frame->head.event);
            break;
    }
}

static void __KeyCycHandle(void) {
    // TypdefKeyStatus *KeyStatusHandle = &KeyStatus[0];
    // if (KeyStatusHandle == NULL) {
    //     elog_e(TAG, "[ERROR]KeyStatusHandle NULL");
    //     return;
    // }
    static uint32_t last_scan_time = 0;
    last_scan_time++;
    if (last_scan_time % (1000 / pdMS_TO_TICKS(CONFIG_KEY_CYCLE_TIMER_MS)) == 0) {
        // elog_i(TAG, "__KeyCycHandle");
    }
    __KeyScan();
}
static void __KeyShortPressCallback(TypdefKeyStatus *key_status) {
    if (key_status->cfg->short_press_event == 0) {
        // elog_e(TAG, "Short press event is not configured for key %s",
        // key_status->cfg->device_name);
        return;
    }
    elog_i(TAG, "Key %s short pressed", key_status->cfg->device_name);
    vfb_send(key_status->cfg->short_press_event, 0, NULL, 0);
}
static void __KeyLongPressCallback(TypdefKeyStatus *key_status) {
    if (key_status->cfg->long_press_event == 0) {
        // elog_e(TAG, "Long press event is not configured for key %s",
        // key_status->cfg->device_name);
        return;
    }
    elog_i(TAG, "Key %s long pressed", key_status->cfg->device_name);
    vfb_send(key_status->cfg->long_press_event, 0, NULL, 0);
}
static void __KeyToggleCallback(TypdefKeyStatus *key_status) {
    if (key_status == NULL || key_status->cfg == NULL) {
        elog_e(TAG, "Key status or configuration is NULL");
        return;
    }
    if (key_status->cfg->toggle_event == 0) {
        return;
    }
    elog_i(TAG, "Key %s toggled, state: %d", key_status->cfg->device_name, key_status->state);
    vfb_send(key_status->cfg->toggle_event, key_status->state, NULL, 0);
}
// 注册按键,遍历KeyStatus查找cfg为空,并新增
static void __KeyRegister(TypdefKeyBSPCfg *cfg) {
    if (cfg == NULL) {
        elog_e(TAG, "Key configuration is NULL, please check the configuration.");
        return;
    }
    if (cfg->pin.base == 0 || cfg->pin.pin == 0) {
        elog_e(TAG, "Key pin configuration is invalid, please check the configuration.");
        return;
    }
    if (cfg->pin.pin_mode != DevPinModeInput) {
        elog_e(TAG, "Key pin mode is not input, please check the configuration.");
        return;
    }
    for (size_t i = 0; i < KeyChannelMax; i++) {
        if (KeyStatus[i].cfg == NULL) {
            KeyStatus[i].cfg = pvPortMalloc(sizeof(TypdefKeyBSPCfg));
            if (KeyStatus[i].cfg == NULL) {
                elog_e(TAG, "Failed to allocate memory for key configuration.");
                return;
            }
            memcpy(KeyStatus[i].cfg, cfg, sizeof(TypdefKeyBSPCfg));
            KeyStatus[i].enable               = 0;
            KeyStatus[i].state                = 0;
            KeyStatus[i].press_time           = 0;
            KeyStatus[i].long_press_triggered = 0;
            KeyStatus[i].last_pin_value       = 0xFF;  // Initialize last pin value to 0xFF

            /* InitGPIO */
            DevPinInit(&cfg->pin);
            elog_i(TAG, "Key %s registered successfully on slot %d", cfg->device_name, i);
            return;
        }
    }
    elog_e(TAG, "No available key slots to register");
}
static void __KeyCtrl(TypdefKeyBSPCfg *cfg, uint8_t en) {
    // device_name
    if (cfg == NULL || cfg->device_name == NULL) {
        elog_e(TAG, "KeyCtrl: cfg or device_name is NULL");
        return;
    }
    for (size_t i = 0; i < KeyChannelMax; i++) {
        if (KeyStatus[i].cfg != NULL &&
            strcmp(KeyStatus[i].cfg->device_name, cfg->device_name) == 0) {
            if (KeyStatus[i].enable == en) {
                elog_i(TAG, "Key %s already in desired state (slot %d)", cfg->device_name, i);
                return;  // Already in the desired state
            }
            KeyStatus[i].enable               = en;
            KeyStatus[i].state                = 0;
            KeyStatus[i].press_time           = 0;
            KeyStatus[i].long_press_triggered = 0;
            KeyStatus[i].last_pin_value       = 0xFF;  // Initialize last pin value to 0xFF

            elog_i(TAG, "Key %s started (slot %d)", cfg->device_name, i);
            return;
        }
    }
    elog_e(TAG, "KeyStart: No matching key found for %s", cfg->device_name);
}

static void __KeyScan(void) {
    for (int i = 0; i < KeyChannelMax; i++) {
        TypdefKeyStatus *key_states = &KeyStatus[i];
        if (key_states->cfg == NULL || !key_states->enable) {
            continue;  // Skip if key is not registered or disabled
        }
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        uint8_t pin_value     = DevPinRead(&key_states->cfg->pin);
        uint8_t active_level  = key_states->cfg->active_level;
        uint8_t triger_mode   = key_states->cfg->trigger_mode;
        uint8_t key_value     = KEY_INACTIVE;
        // Adjust key value based on active level

        switch (triger_mode) {
            case KEY_TRIGGER_MODE_PRESS:  // 按键模式
            {
                key_value = (pin_value == active_level) ? KEY_ACTIVE
                                                        : KEY_INACTIVE;  // Active level is pressed
                if (key_value == KEY_ACTIVE && key_states->state == 0) {
                    // Key pressed
                    key_states->state                = 1;
                    key_states->press_time           = current_time;
                    key_states->long_press_triggered = 0;
                } else if (key_value == KEY_ACTIVE && key_states->state == 1) {
                    // Key is still pressed
                    uint32_t press_duration = current_time - key_states->press_time;
                    if (press_duration >= LONG_PRESS_THRESHOLD &&
                        !key_states->long_press_triggered) {
                        __KeyLongPressCallback(key_states);
                        key_states->long_press_triggered = 1;
                    }
                } else if (key_value == KEY_INACTIVE && key_states->state == 1) {
                    // Key released
                    uint32_t press_duration = current_time - key_states->press_time;
                    if (press_duration < LONG_PRESS_THRESHOLD &&
                        press_duration >= SHORT_PRESS_THRESHOLD) {
                        __KeyShortPressCallback(key_states);
                    }
                    key_states->state = 0;
                }
            } break;
            case KEY_TRIGGER_MODE_TOGGLE: {  // 自锁模式
                key_value = (pin_value == active_level) ? KEY_ACTIVE
                                                        : KEY_INACTIVE;  // Active level is pressed
                if (key_value != key_states->last_pin_value) {
                    // Key state changed
                    key_states->last_pin_value       = key_value;  // Update last pin value
                    key_states->press_time           = current_time;
                    key_states->long_press_triggered = 0;
                    elog_i(TAG, "Key %s state changed: %d", key_states->cfg->device_name,
                           key_value);
                } else if (key_value == key_states->last_pin_value &&
                           key_states->long_press_triggered == 0) {
                    uint32_t press_duration = current_time - key_states->press_time;
                    if (press_duration >= SHORT_PRESS_THRESHOLD) {
                        key_states->state = key_value;
                        __KeyToggleCallback(key_states);
                        key_states->long_press_triggered = 1;
                    } else {
                        // Key state unchanged, do nothing
                        // elog_i(TAG, "Key %s state unchanged: %d", key_states->cfg->device_name,
                        // key_value);
                    }
                } else {
                    // 按键未变化,或者按键处于消抖等待状态,不做处理
                    // elog_i(TAG, "Key %s state unchanged: %d", key_states->cfg->device_name,
                    // key_value);
                }
            } break;
            default:
                elog_e(TAG, "Unknown trigger mode: %d", triger_mode);
                continue;  // Skip this key if the trigger mode is unknown
        }
    }
}
static void CmdKeyHelp(void) {
    printf("Usage: Key <state>");
    printf("  <state>: 0 for off, 1 for on");
    printf("Example: Key 1");
}
static int CmdKeyHandle(int argc, char *argv[]) {
    if (argc != 2) {
        CmdKeyHelp();
        return 0;
    }

    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), Key, CmdKeyHandle,
                 Key command);
#endif
