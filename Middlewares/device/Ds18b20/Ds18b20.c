
#include "Ds18b20Cfg.h"
#define CONFIG_Ds18b20_EN 1
#if CONFIG_Ds18b20_EN
#include <stdio.h>
#include <stdlib.h>

#include "gd32h7xx.h"
#include "string.h"

/* Device */
#include "Device.h"
#include "dev_basic.h"
#include "dev_delay.h"
#include "dev_onewire.h"
#include "dev_pin.h"
#include "dev_uart.h"
#include "elog.h"
#include "os_server.h"

// #include "app_event.h"

#define TAG           "Ds18b20"
#define Ds18b20LogLvl ELOG_LVL_INFO

#define Ds18b20Priority PrioritySpecialGroup0
#ifndef Ds18b20ChannelMax
#define Ds18b20ChannelMax 1
#endif
#ifndef CONFIG_DS18B20_CYCLE_TIMER_MS
#define CONFIG_DS18B20_CYCLE_TIMER_MS 100
#endif

/* ===================================================================================== */
typedef struct
{
    DevOneWireHandleStruct one_wire;  // OneWire handle

} TypdefDs18b20BSPCfg;

static void __Ds18b20CreateTaskHandle(void);
static void __Ds18b20RcvHandle(void *msg);
static void __Ds18b20CycHandle(void);
static void __Ds18b20InitHandle(void *msg);
static void CmdDs18b20Reset(void);
static float CmdDs18b20Read(uint8_t state);
static void CmdDs18b20Covert(uint8_t state);
const TypdefDs18b20BSPCfg Ds18b20BspCfg[Ds18b20ChannelMax] = {
    {
        .one_wire = {
            .device_name = "DS18B20",
            .dev_pin_id  = DEV_PIN_PA1,  // Example pin, change as needed
        },
    },
    // {
    //     .one_wire = {
    //         .device_name = "NOT_USED",
    //         .dev_pin_id  = DEV_PIN_PA0,  // Example pin, change as needed
    //     },
    // },
};

typedef struct
{
    char device_name[DEVICE_NAME_MAX];
    uint8_t rom[8];
    uint32_t id;        // ID
    uint8_t status;     // Status of the sensor
    uint8_t step;       // Step in the conversion process
    float temperature;  // Temperature in Celsius

} TypdefDs18b20Status;
TypdefDs18b20Status Ds18b20Status[Ds18b20ChannelMax] = {0};

/* ===================================================================================== */

static const vfb_event_t Ds18b20EventList[] = {
    Ds18b20Start,
    // Ds18b20Stop,
    // Ds18b20Covert,
    // Ds18b20GetTemperature,
};

static const VFBTaskStruct Ds18b20_task_cfg = {
    .name         = "VFBTaskDs18b20",  // Task name
    .pvParameters = NULL,
    // .uxPriority = 10,											  // Task parameters
    .queue_num               = 8,                                               // Number of queues to subscribe
    .event_list              = Ds18b20EventList,                                // Event list to subscribe
    .event_num               = sizeof(Ds18b20EventList) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                                            // Events to wait for at startup
    .startup_wait_event_num  = 0,                                               // Number of startup events to wait for
    .xTicksToWait            = pdMS_TO_TICKS(CONFIG_DS18B20_CYCLE_TIMER_MS),    // Wait indefinitely
    .init_msg_cb             = __Ds18b20InitHandle,                             // Callback for initialization messages
    .rcv_msg_cb              = __Ds18b20RcvHandle,                              // Callback for received messages
    .rcv_timeout_cb          = __Ds18b20CycHandle,                              // Callback for timeout
};

/* ===================================================================================== */

void Ds18b20DeviceInit(void) {
    elog_i(TAG, "Ds18b20DeviceInit");

    for (size_t i = 0; i < Ds18b20ChannelMax; i++) {
        TypdefDs18b20Status *Ds18b20StatusHandle = &Ds18b20Status[i];

        Ds18b20StatusHandle->id = i;
        memset(Ds18b20StatusHandle->device_name, 0, sizeof(Ds18b20StatusHandle->device_name));
        snprintf(Ds18b20StatusHandle->device_name, sizeof(Ds18b20StatusHandle->device_name), "Ds18b20%d", i);
        DevOneWireInit(&(Ds18b20BspCfg[i].one_wire));
        // extern const TypedefDevPinMap DevPinMap[GD32H7XXZ_PIN_MAP_MAX];
        elog_i(TAG, "Ds18b20 Channel %d initialized with pin %s %s", i, Ds18b20BspCfg[i].one_wire.device_name, DevPinMap[Ds18b20BspCfg[i].one_wire.dev_pin_id].pin_name);
    }
    DevOneWireStop(&(Ds18b20BspCfg[1].one_wire));  // Stop the OneWire bus to ensure no interference during initialization
}
SYSTEM_REGISTER_INIT(MCUInitStage, Ds18b20Priority, Ds18b20DeviceInit, Ds18b20DeviceInit);

static void __Ds18b20CreateTaskHandle(void) {
    for (size_t i = 0; i < Ds18b20ChannelMax; i++) {
    }
    xTaskCreate(VFBTaskFrame, "VFBTaskDs18b20", configMINIMAL_STACK_SIZE * 2, (void *)&Ds18b20_task_cfg, Ds18b20Priority, NULL);
}
SYSTEM_REGISTER_INIT(BoardInitStage, Ds18b20Priority, __Ds18b20CreateTaskHandle, __Ds18b20CreateTaskHandle init);

static void __Ds18b20InitHandle(void *msg) {
    elog_i(TAG, "__Ds18b20InitHandle");
    elog_set_filter_tag_lvl(TAG, Ds18b20LogLvl);
    vTaskDelay(pdMS_TO_TICKS(500));  // Delay to ensure all devices are initialized before starting the task
    vfb_send(Ds18b20Start, 0, NULL, 0);
}
// 接收消息的回调函数
static void __Ds18b20RcvHandle(void *msg) {
    TaskHandle_t curTaskHandle            = xTaskGetCurrentTaskHandle();
    TypdefDs18b20Status *Ds18b20StatusTmp = &Ds18b20Status[0];
    char *taskName                        = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg                 = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case Ds18b20Start: {
            Ds18b20StatusTmp->status = 1;
            Ds18b20StatusTmp->step   = 0;
            elog_i(TAG, "TASK %s RCV: Ds18b20Start", taskName);
            CmdDs18b20Reset();
        } break;
        default:
            elog_e(TAG, "TASK %s RCV: unknown event: %d", taskName, tmp_msg->frame->head.event);
            break;
    }
}

static void __Ds18b20CycHandle(void) {
    TypdefDs18b20Status *Ds18b20StatusTmp = &Ds18b20Status[0];
    static uint8_t step                   = 0;
    static uint32_t conter                = 0;
    if (Ds18b20StatusTmp == NULL) {
        elog_e(TAG, "[ERROR]Ds18b20StatusHandle NULL");
        return;
    }
    if (conter % 10 == 0) {
        if (Ds18b20StatusTmp->status) {
            if (Ds18b20StatusTmp->step == 0) {
                // convert
                CmdDs18b20Covert(0);
                Ds18b20StatusTmp->step = 1;
            } else {
                Ds18b20StatusTmp->temperature =
                    CmdDs18b20Read(0);  // Read temperature
                Ds18b20StatusTmp->step = 0;
                elog_d(TAG, "Ds18b20 temperature: %.2f", Ds18b20StatusTmp->temperature);
                // Send temperature to HMI
                vfb_send(Ds18b20GetTemperature, 0, &Ds18b20StatusTmp->temperature, sizeof(Ds18b20StatusTmp->temperature));
                elog_d(TAG, "Send Ds18b20GetTemperature done %u ", conter);
            }

        } else {
            Ds18b20StatusTmp->step = 0;
        }
    }
    conter++;
}

#endif

static void CmdDs18b20Help(void) {
    elog_i(TAG, "Usage: ds18b20 <command>");
    elog_i(TAG, "Commands:");
    elog_i(TAG, "  reset       - Reset the DS18B20 sensor");
    elog_i(TAG, "  read        - Read the temperature from the DS18B20 sensor");
    elog_i(TAG, "  status      - Show DS18B20 status");
    elog_i(TAG, "  rom          - Read the ROM code of the DS18B20 sensor");
    elog_i(TAG, "  pin_high    - Set DS18B20 pin to HIGH");
    elog_i(TAG, "  pin_low     - Set DS18B20 pin to LOW");
    elog_i(TAG, "  delay_test  - Test DS18B20 delay (usage: ds18b20 delay_test <delay_us>)");
    elog_i(TAG, "  help        - Show this help message");
    elog_i(TAG, "Example: ds18b20 reset");
}
static void CmdDs18b20Reset(void) {
    elog_i(TAG, "Resetting DS18B20...");
    int status = 0;
    vTaskSuspendAll();
    status = DevOneWireReset(&(Ds18b20BspCfg[0].one_wire));
    xTaskResumeAll();
    elog_i(TAG, "Ds18b20StartTask DS18B20 status: %s", (status == 0) ? "OK" : "FAIL");
    if (status != 0) {
        elog_e(TAG, "Failed to reset DS18B20 Status %d", status);
    } else {
        // vfb_send(Ds18b20Covert, 0, NULL, 0);  // Start conversion
    }
    // Reset logic for DS18B20 can be added here
}
// DevOneWirePinWrite
static void CmdDs18b20DelayTest(int delay_us) {
    elog_i(TAG, "Testing DS18B20 delay with %d us...", delay_us);
    vTaskDelay(pdMS_TO_TICKS(100));  // Delay to ensure all devices are initialized before starting the task

    vTaskSuspendAll();
    NVIC_DisableIRQ(SysTick_IRQn);

    DevOneWirePinWrite(&(Ds18b20BspCfg[0].one_wire), 1);  // Set pin HIGH
    DevDelayUs(delay_us);                                 // Delay for 1 millisecond
    DevOneWirePinWrite(&(Ds18b20BspCfg[0].one_wire), 0);  // Set pin LOW
    DevDelayUs(delay_us);                                 // Delay for 1 millisecond
    DevOneWirePinWrite(&(Ds18b20BspCfg[0].one_wire), 1);  // Set pin HIGH again
    NVIC_EnableIRQ(SysTick_IRQn);

    xTaskResumeAll();
    elog_i(TAG, "DS18B20 delay test completed");
}
static void CmdDs18b20ReadRom(void) {
    elog_i(TAG, "Reading DS18B20 ROM code...");
    int status = 0;
    vTaskDelay(pdMS_TO_TICKS(100));
    vTaskSuspendAll();
    NVIC_DisableIRQ(SysTick_IRQn);
    status = DevOneWireReadRom(&(Ds18b20BspCfg[0].one_wire), Ds18b20Status[0].rom);
    NVIC_EnableIRQ(SysTick_IRQn);
    xTaskResumeAll();
    if (status != 0) {
        elog_e(TAG, "Failed to read ROM code from DS18B20 Status %d", status);
    } else {
        elog_i(TAG, "DS18B20 ROM code: %02X%02X%02X%02X%02X%02X%02X%02X",
               Ds18b20Status[0].rom[0], Ds18b20Status[0].rom[1], Ds18b20Status[0].rom[2],
               Ds18b20Status[0].rom[3], Ds18b20Status[0].rom[4], Ds18b20Status[0].rom[5],
               Ds18b20Status[0].rom[6], Ds18b20Status[0].rom[7]);
    }
}
static void CmdDs18b20Covert(uint8_t state) {
    elog_d(TAG, "Starting DS18B20 conversion...");

    DevOneWireHandleStruct *handle = &(Ds18b20BspCfg[0].one_wire);
    vTaskSuspendAll();
    DevOneWireReset(handle);
    DevOneWireWriteByte(handle, 0xCC);
    DevOneWireWriteByte(handle, 0x44);
    xTaskResumeAll();
    // delay 100ms
    // vTaskDelay(pdMS_TO_TICKS(1000));
}
static float CmdDs18b20Read(uint8_t state) {
    DevOneWireHandleStruct *handle = &(Ds18b20BspCfg[0].one_wire);
    elog_d(TAG, "Reading DS18B20 temperature...");
    uint8_t scratchpad[9] = {0};
    vTaskSuspendAll();
    DevOneWireReset(handle);  // Reset the bus before reading ROM
    DevOneWireWriteByte(handle, 0xCC);
    DevOneWireWriteByte(handle, 0xBE);  // Read Scratchpad command
    for (int i = 0; i < 2; i++) {
        scratchpad[i] = DevOneWireReadByte(handle);
    }
    xTaskResumeAll();
    // elog_i(TAG, "DS18B20 Scratchpad: %02X %02X %02X %02X %02X %02X %02X %02X %02X",
    //        scratchpad[0], scratchpad[1], scratchpad[2], scratchpad[3],
    //        scratchpad[4], scratchpad[5], scratchpad[6], scratchpad[7], scratchpad[8]);

    uint16_t temp = 0;
    float f_tem   = 0.0f;
    temp          = scratchpad[1] << 8;
    temp |= scratchpad[0];  // Combine the two bytes
    if (temp < 0)           /* 负温度 */
        f_tem = (~temp + 1) * 0.0625;
    else
        f_tem = temp * 0.0625;
    elog_d(TAG, "DS18B20 Temperature: %.2f C", f_tem);
    return f_tem;
}
static int CmdDs18b20Handle(int argc, char *argv[]) {
    if (argc < 2) {
        CmdDs18b20Help();
        return 0;
    } else if (strcmp(argv[1], "reset") == 0) {
        CmdDs18b20Reset();
        return 0;
    } else if (strcmp(argv[1], "help") == 0) {
        CmdDs18b20Help();
        return 0;
    } else if (strcmp(argv[1], "status") == 0) {
        elog_i(TAG, "DS18B20 Status:");
        for (size_t i = 0; i < Ds18b20ChannelMax; i++) {
            TypdefDs18b20Status *Ds18b20StatusHandle = &Ds18b20Status[i];
            elog_i(TAG, "ID: %d, Device: %s, Temperature: %.2f C", Ds18b20StatusHandle->id,
                   Ds18b20StatusHandle->device_name, Ds18b20StatusHandle->temperature);
        }
        return 0;
    } else if (strcmp(argv[1], "rom") == 0) {
        CmdDs18b20ReadRom();
        return 0;
    } else if (strcmp(argv[1], "read") == 0) {
        float temp = CmdDs18b20Read(1);
        elog_i(TAG, "Read temperature: %.2f C", temp);
        return 0;
    } else if (strcmp(argv[1], "convert") == 0) {
        elog_i(TAG, "Converting DS18B20 temperature...");
        CmdDs18b20Covert(1);
        return 0;
    }
    /* else if (strcmp(argv[1], "pin_low") == 0) {
       CmdDs18b20Pin(0);
       return 0;
   }  */
    else if (strcmp(argv[1], "delay_test") == 0) {
        if (argc != 3) {
            elog_e(TAG, "Usage: ds18b20 delay_test <delay_us>");
            return -1;
        }
        int delay_us = atoi(argv[2]);
        if (delay_us <= 0) {
            elog_e(TAG, "Invalid delay value: %d", delay_us);
            return -1;
        }
        CmdDs18b20DelayTest(delay_us);
        return 0;
    } else {
        elog_e(TAG, "Unknown command: %s", argv[1]);
        CmdDs18b20Help();
        return -1;
    }

    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), ds18b20, CmdDs18b20Handle, demo command);