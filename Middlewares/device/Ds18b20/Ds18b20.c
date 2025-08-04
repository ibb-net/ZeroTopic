
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
typedef struct {
    DevOneWireHandleStruct one_wire;  // OneWire handle

} TypdefDs18b20BSPCfg;

static void __Ds18b20CreateTaskHandle(void);
static void __Ds18b20RcvHandle(void *msg);
static void __Ds18b20CycHandle(void);
static void __Ds18b20InitHandle(void *msg);
static void CmdDs18b20Reset(void);
static double CmdDs18b20Read(uint8_t state);
static void CmdDs18b20Covert(uint8_t state);
uint8_t crc8_maxim(const uint8_t *data, uint8_t len);
const TypdefDs18b20BSPCfg Ds18b20BspCfg[Ds18b20ChannelMax] = {
    {
        .one_wire =
            {
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

typedef struct {
    char device_name[DEVICE_NAME_MAX];
    uint8_t rom[8];
    uint32_t id;         // ID
    uint8_t status;      // Status of the sensor
    uint8_t step;        // Step in the conversion process
    double temperature;  // Temperature in Celsius

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
    // .uxPriority = 10,
    // // Task parameters
    .queue_num  = 8,                                               // Number of queues to subscribe
    .event_list = Ds18b20EventList,                                // Event list to subscribe
    .event_num  = sizeof(Ds18b20EventList) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                               // Events to wait for at startup
    .startup_wait_event_num  = 0,  // Number of startup events to wait for
    .xTicksToWait            = pdMS_TO_TICKS(CONFIG_DS18B20_CYCLE_TIMER_MS),  // Wait indefinitely
    .init_msg_cb             = __Ds18b20InitHandle,  // Callback for initialization messages
    .rcv_msg_cb              = __Ds18b20RcvHandle,   // Callback for received messages
    .rcv_timeout_cb          = __Ds18b20CycHandle,   // Callback for timeout
};

/* ===================================================================================== */

void Ds18b20DeviceInit(void) {
    elog_i(TAG, "Ds18b20DeviceInit");

    for (size_t i = 0; i < Ds18b20ChannelMax; i++) {
        TypdefDs18b20Status *Ds18b20StatusHandle = &Ds18b20Status[i];

        Ds18b20StatusHandle->id = i;
        memset(Ds18b20StatusHandle->device_name, 0, sizeof(Ds18b20StatusHandle->device_name));
        snprintf(Ds18b20StatusHandle->device_name, sizeof(Ds18b20StatusHandle->device_name),
                 "Ds18b20%d", i);
        // DevOneWireInit((DevOneWireHandleStruct *)&(Ds18b20BspCfg[i].one_wire));

        // elog_i(TAG, "Ds18b20 Channel %d initialized with pin %s %s", i,
        //        Ds18b20BspCfg[i].one_wire.device_name,
        //        DevPinMap[Ds18b20BspCfg[i].one_wire.dev_pin_id].pin_name);
    }
    onewire_usart_init();
}
SYSTEM_REGISTER_INIT(MCUInitStage, Ds18b20Priority, Ds18b20DeviceInit, Ds18b20DeviceInit);

static void __Ds18b20CreateTaskHandle(void) {
    for (size_t i = 0; i < Ds18b20ChannelMax; i++) {
    }
    xTaskCreate(VFBTaskFrame, "VFBTaskDs18b20", configMINIMAL_STACK_SIZE * 2,
                (void *)&Ds18b20_task_cfg, Ds18b20Priority, NULL);
}
SYSTEM_REGISTER_INIT(BoardInitStage, Ds18b20Priority, __Ds18b20CreateTaskHandle,
                     __Ds18b20CreateTaskHandle init);

static void __Ds18b20InitHandle(void *msg) {
    elog_i(TAG, "__Ds18b20InitHandle");
    // elog_set_filter_tag_lvl(TAG, Ds18b20LogLvl);
    Ds18b20Status[0].status      = 0;       // Initialize status
    Ds18b20Status[0].step        = 0;       // Initialize step
    Ds18b20Status[0].temperature = -100.0;  // Initialize temperature

    vTaskDelay(pdMS_TO_TICKS(
        500));  // Delay to ensure all devices are initialized before starting the task
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
                double temp_reading           = CmdDs18b20Read(0);  // Read temperature
                Ds18b20StatusTmp->temperature = temp_reading;
                elog_d(TAG, "Ds18b20 temperature: %.2f", Ds18b20StatusTmp->temperature);
                // Send temperature to HMI
                vfb_send(Ds18b20GetTemperature, 0, &Ds18b20StatusTmp->temperature,
                         sizeof(Ds18b20StatusTmp->temperature));
                elog_d(TAG, "Send Ds18b20GetTemperature done %u ", conter);
                Ds18b20StatusTmp->step = 0;
            }

        } else {
            Ds18b20StatusTmp->step = 0;
        }
    }
    conter++;
}

#endif

uint8_t crc8_maxim(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x01)
                crc = (crc >> 1) ^ 0x8C;  // 0x8C = 0x31 << 1
            else
                crc >>= 1;
        }
    }
    return crc;
}

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
    onewire_reset();
}

static void CmdDs18b20ReadRom(void) {
    elog_i(TAG, "Reading DS18B20 ROM code...");
    onewire_rom(Ds18b20Status[0].rom);

    elog_i(TAG, "DS18B20 ROM code: %02X%02X%02X%02X%02X%02X%02X%02X", Ds18b20Status[0].rom[0],
           Ds18b20Status[0].rom[1], Ds18b20Status[0].rom[2], Ds18b20Status[0].rom[3],
           Ds18b20Status[0].rom[4], Ds18b20Status[0].rom[5], Ds18b20Status[0].rom[6],
           Ds18b20Status[0].rom[7]);
}
static void CmdDs18b20Covert(uint8_t state) {
    elog_d(TAG, "Starting DS18B20 conversion...");
    onewire_reset();
    uint8_t cmd[2] = {0xCC, 0x44};
    onewire_write_byte(cmd, 2);
}
static double CmdDs18b20Read(uint8_t state) {
    static uint8_t err_cnt = 0;
    onewire_reset();
    uint8_t cmd[2] = {0xCC, 0xBE};
    onewire_write_byte(cmd, 2);
    uint8_t scratchpad[9] = {0};
    onewire_read_byte(scratchpad, 9);
    elog_d(TAG, "DS18B20 Scratchpad: %02X %02X %02X %02X %02X %02X %02X %02X %02X", scratchpad[0],
           scratchpad[1], scratchpad[2], scratchpad[3], scratchpad[4], scratchpad[5], scratchpad[6],
           scratchpad[7], scratchpad[8]);

    uint16_t temp_raw   = 0;
    int16_t temp_signed = 0;
    double f_tem        = 0.0;

    // 组合两个字节
    temp_raw    = ((uint16_t)scratchpad[1] << 8) | scratchpad[0];
    temp_signed = (int16_t)temp_raw;  // 转换为有符号整数

    uint8_t crc = crc8_maxim(scratchpad, 8);  // 校验CRC
    if (crc != scratchpad[8]) {
        err_cnt++;

        if (err_cnt > 3) {
            elog_e(TAG, "DS18B20 Scratchpad: %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                   scratchpad[0], scratchpad[1], scratchpad[2], scratchpad[3], scratchpad[4],
                   scratchpad[5], scratchpad[6], scratchpad[7], scratchpad[8]);
            elog_w(TAG, "DS18B20 CRC error: expected %02X, got %02X", scratchpad[8], crc);
        }
        return F_INVAILD;
    } else {
        err_cnt = 0;
    }

    // DS18B20使用二进制补码表示负温度
    if (temp_signed < 0) {
        /* 负温度处理：DS18B20负温度已经是二进制补码形式 */
        f_tem = (double)temp_signed * 0.0625;
    } else {
        /* 正温度处理 */
        f_tem = (double)temp_signed * 0.0625;
    }

    // DS18B20的温度范围检查：-55°C 到 +125°C
    if (f_tem < -55.0 || f_tem > 125.0) {
        elog_w(TAG, "DS18B20 temperature out of range: %.2f C (raw: 0x%04X)", f_tem, temp_raw);
        // 返回一个错误值来表示读取失败
        return F_INVAILD;
    }

    elog_d(TAG, "DS18B20 Raw: 0x%04X, Signed: %d, Temperature: %.2f C", temp_raw, temp_signed,
           f_tem);
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
        double temp = CmdDs18b20Read(1);
        elog_i(TAG, "Read temperature: %.2f C", temp);
        return 0;
    } else if (strcmp(argv[1], "convert") == 0) {
        elog_i(TAG, "Converting DS18B20 temperature...");
        CmdDs18b20Covert(1);
        return 0;
    } else {
        elog_e(TAG, "Unknown command: %s", argv[1]);
        CmdDs18b20Help();
        return -1;
    }

    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), ds18b20,
                 CmdDs18b20Handle, demo command);