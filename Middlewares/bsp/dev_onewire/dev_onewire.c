
#define CONFIG_DEV_ONEWRIE_EN 1
#if CONFIG_DEV_ONEWRIE_EN
#include "dev_onewire.h"

#include "dev_basic.h"
#include "dev_delay.h"
#define ONEWIRE_ROM_SIZE              8
#define ONEWIRE_MAX_DEVICES           1
#define ONEWIRE_RESET_TIME            480 /*!< us */
#define ONEWIRE_RESET_START_WAIT_TIME 15  /*!< us */
#define ONEWIRE_RESET_WAIT_STEP       15  /*!< us */
#define ONEWIRE_RESET_DURATION_START  60  /*!< us */
#define ONEWIRE_RESET_DURATION_END    240 /*!< us */
#define ONEWIRE_RESET_WAIT_STEP       15  /*!< us */

#define ONEWIRE_READ_ROM_CMD 0x33
int DevOneWireReleaseBus(const DevOneWireHandleStruct *handle);
void DevOneWireInit(DevOneWireHandleStruct *handle) {
    TypedefDevPinMap *ptrDevPinMap = (TypedefDevPinMap *)&DevPinMap[handle->dev_pin_id];
    rcu_periph_clock_enable(ptrDevPinMap->rcu_clock);
    gpio_mode_set(ptrDevPinMap->base, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, ptrDevPinMap->pin);
    gpio_output_options_set(ptrDevPinMap->base, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, ptrDevPinMap->pin);
    gpio_bit_set(ptrDevPinMap->base, ptrDevPinMap->pin);
    DevOneWireReleaseBus(handle);
}
void __DevOneWireInputMode(const DevOneWireHandleStruct *handle) {
    TypedefDevPinMap *ptrDevPinMap = (TypedefDevPinMap *)&DevPinMap[handle->dev_pin_id];
    gpio_mode_set(ptrDevPinMap->base, GPIO_MODE_INPUT, GPIO_PUPD_NONE, ptrDevPinMap->pin);
    // gpio_output_options_set(ptrDevPinMap->base, GPIO_OTYPE_OD, GPIO_OSPEED_100_220MHZ, ptrDevPinMap->pin);
}
void __DevOneWireOutputMode(const DevOneWireHandleStruct *handle) {
    TypedefDevPinMap *ptrDevPinMap = (TypedefDevPinMap *)&DevPinMap[handle->dev_pin_id];
    gpio_mode_set(ptrDevPinMap->base, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, ptrDevPinMap->pin);
    gpio_output_options_set(ptrDevPinMap->base, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, ptrDevPinMap->pin);
}
void __DevOneWireDelayUs(uint32_t ntime) {
    DevDelayUs(ntime);
}
void DevOneWirePinWrite(const DevOneWireHandleStruct *handle, uint8_t bit_value) {
    TypedefDevPinMap *ptrDevPinMap = (TypedefDevPinMap *)&DevPinMap[handle->dev_pin_id];
    __DevOneWireOutputMode(handle);
    if (bit_value) {
        gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, 1);
    } else {
        gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, 0);
    }
}

int DevOneWireReset(const DevOneWireHandleStruct *handle) {
    // uint8_t ack                    = 1;
    uint8_t ack_reset              = 0;
    TypedefDevPinMap *ptrDevPinMap = (TypedefDevPinMap *)&DevPinMap[handle->dev_pin_id];
    __DevOneWireOutputMode(handle);
    gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, RESET);
    __DevOneWireDelayUs(ONEWIRE_RESET_TIME);
    // gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, SET);
    // __DevOneWireInputMode(handle);
    DevOneWireReleaseBus(handle);
    // __DevOneWireDelayUs(ONEWIRE_RESET_START_WAIT_TIME);

    for (uint32_t i = 0; i < (ONEWIRE_RESET_DURATION_END); i = i + ONEWIRE_RESET_WAIT_STEP) {
        if (gpio_input_bit_get(ptrDevPinMap->base, ptrDevPinMap->pin) == RESET) {
            ack_reset = 1;
        }
        __DevOneWireDelayUs(ONEWIRE_RESET_WAIT_STEP);
    }
    __DevOneWireDelayUs(ONEWIRE_RESET_WAIT_STEP);
    if (DevOneWireReleaseBus(handle) < 0) {
        return -2;  // Reset failed
    }
    if (ack_reset) {
        return 0;  // Reset successful
    } else {
        return -1;  // Reset failed
    }
}
int DevOneWireReleaseBus(const DevOneWireHandleStruct *handle) {
    int ack                        = 0;
    TypedefDevPinMap *ptrDevPinMap = (TypedefDevPinMap*)&DevPinMap[handle->dev_pin_id];
    __DevOneWireOutputMode(handle);
    gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, SET);  // Release bus
    while (gpio_input_bit_get(ptrDevPinMap->base, ptrDevPinMap->pin) == RESET) {
        ack++;
        if (ack > 1000) {  // Timeout to prevent infinite loop
            printf("DevOneWireReleaseBus: Timeout waiting for bus release\r\n");
            return -1;  // Release bus failed
        }
        __DevOneWireDelayUs(1);  // Wait for bus to be released
    }
    __DevOneWireInputMode(handle);
    return 0;  // Release bus successful
}
int DevOneWireStop(const DevOneWireHandleStruct *handle) {
    // TypedefDevPinMap *ptrDevPinMap =(TypedefDevPinMap*) &DevPinMap[handle->dev_pin_id];
    __DevOneWireInputMode(handle);
    return 0;  // Stop successful
}

void __DevOneWireWriteBit(const DevOneWireHandleStruct *handle, uint8_t bit_value) {
    TypedefDevPinMap *ptrDevPinMap = (TypedefDevPinMap*)&DevPinMap[handle->dev_pin_id];

    gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, SET);
    __DevOneWireOutputMode(handle);
    __DevOneWireDelayUs(2);  // Write 1
    if (bit_value) {
        gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, RESET);
        __DevOneWireDelayUs(15);  // Release bus
        __DevOneWireInputMode(handle);
        __DevOneWireDelayUs(45);

    } else {
        gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, RESET);
        __DevOneWireDelayUs(60);  // Release bus
    }
    gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, SET);
    __DevOneWireOutputMode(handle);

    __DevOneWireDelayUs(1);
}
uint8_t __DevOneWireReadBit(const DevOneWireHandleStruct *handle) {
    uint8_t bit_value              = 0;
    TypedefDevPinMap *ptrDevPinMap = (TypedefDevPinMap *)&DevPinMap[handle->dev_pin_id];
    gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, RESET);
    __DevOneWireOutputMode(handle);

    __DevOneWireDelayUs(2);  // Start bit
    __DevOneWireInputMode(handle);
    __DevOneWireDelayUs(15);  // Wait for data
    if (gpio_input_bit_get(ptrDevPinMap->base, ptrDevPinMap->pin) == RESET) {
        bit_value = 0;  // Read 1
    } else {
        bit_value = 1;  // Read 0
    }
    __DevOneWireDelayUs(30);  // Release bus
    gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, SET);
    __DevOneWireOutputMode(handle);

    __DevOneWireDelayUs(1);

    return bit_value;
}

void DevOneWireWriteByte(const DevOneWireHandleStruct *handle, uint8_t byte_value) {
    for (int i = 0; i < 8; i++) {
        __DevOneWireWriteBit(handle, (byte_value >> i) & 0x01);
    }
    // unsigned char i;
    // unsigned char wr               = byte_value;
    // TypedefDevPinMap *ptrDevPinMap = &DevPinMap[handle->dev_pin_id];
    // __DevOneWireOutputMode(handle);
    // gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, SET);
    // for (i = 0; i < 8; i++) {
    //     gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, RESET);
    //     __DevOneWireDelayUs(15);
    //     if (wr & 0x01) {
    //         gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, SET);

    //     } else {
    //         gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, RESET);
    //     }
    //     __DevOneWireDelayUs(45);  // delay 45 uS //5

    //     gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, SET);
    //     __DevOneWireDelayUs(1);  // delay 5 uS
    //     wr >>= 1;
    // }
    // DevOneWireReleaseBus(handle);  // Release bus
}
uint8_t DevOneWireReadByte(const DevOneWireHandleStruct *handle) {
    uint8_t byte_value = 0;
    for (int i = 0; i < 8; i++) {
        byte_value |= (__DevOneWireReadBit(handle) << i);
    }
    // return byte_value;
    // unsigned char i, u = 0;
    // TypedefDevPinMap *ptrDevPinMap = &DevPinMap[handle->dev_pin_id];
    // __DevOneWireOutputMode(handle);
    // gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, SET);
    // for (i = 0; i < 8; i++) {
    //     __DevOneWireOutputMode(handle);
    //     gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, RESET);
    //     __DevOneWireDelayUs(1);
    //     // gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, SET);
    //     u >>= 1;
    //     // gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, SET);
    //     __DevOneWireInputMode(handle);
    //     __DevOneWireDelayUs(15);

    //     if (gpio_input_bit_get(ptrDevPinMap->base, ptrDevPinMap->pin) == 1)
    //         u |= 0x80;
    //     __DevOneWireDelayUs(45);
    //     __DevOneWireInputMode(handle);
    // }
    // DevOneWireReleaseBus(handle);  // Release bus
    return (byte_value);
}

int DevOneWireReadRom(const DevOneWireHandleStruct *handle, uint8_t *rom_code) {
    // if (DevOneWireReset(handle) < 0) {
    //     return -1;  // Reset failed
    // }
    // DevOneWireWriteByte(handle, ONEWIRE_READ_ROM_CMD);
    DevOneWireWriteByte(handle, 0xCC);
    DevOneWireWriteByte(handle, 0x44);
    DevOneWireReset(handle);  // Reset the bus before reading ROM

    // DevOneWireReadByte(handle);  // Read the first byte (family code)
    for (int i = 0; i < ONEWIRE_ROM_SIZE; i++) {
        rom_code[i] = DevOneWireReadByte(handle);
    }
    return 0;  // Read ROM successful
}
// // DevPinWrite
// void DevPinWrite(const DevOneWireHandleStruct *ptrDevPinHandle, uint8_t bit_value) {
//     TypedefDevPinMap *ptrDevPinMap = &DevPinMap[ptrDevPinHandle->pin_id];
//     if (bit_value) {
//         gpio_bit_set(ptrDevPinMap->base, ptrDevPinMap->pin);
//     } else {
//         gpio_bit_write(ptrDevPinMap->base, ptrDevPinMap->pin, RESET);
//     }

// }

// uint8_t DevPinRead(const DevOneWireHandleStruct *ptrDevPinHandle) {
//     return (gpio_input_bit_get(ptrDevPinHandle->base, ptrDevPinHandle->pin) == SET) ? 1 : 0;
// }
#endif