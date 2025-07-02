
#define CONFIG_DEV_PIN_EN 1
#if CONFIG_DEV_PIN_EN
#include "dev_pin.h"

#include "dev_basic.h"

void DevPinInit(const DevPinHandleStruct *ptrDevPinHandle) {
    static const struct {
        uint32_t gpio_base;
        rcu_periph_enum rcu_clock;
    } dev_clock_map[] = {
        {GPIOA, RCU_GPIOA}, {GPIOB, RCU_GPIOB}, {GPIOC, RCU_GPIOC}, {GPIOD, RCU_GPIOD},
        {GPIOE, RCU_GPIOE}, {GPIOF, RCU_GPIOF}, {GPIOG, RCU_GPIOG}, {GPIOH, RCU_GPIOH},
        {GPIOJ, RCU_GPIOJ}, {GPIOK, RCU_GPIOK},
    };

    uint8_t is_found = 0;
    for (size_t i = 0; i < sizeof(dev_clock_map) / sizeof(dev_clock_map[0]); i++) {
        if (dev_clock_map[i].gpio_base == ptrDevPinHandle->base) {
            rcu_periph_clock_enable(dev_clock_map[i].rcu_clock);
            is_found = 1;
            break;
        }
    }

    if (!is_found) {
        printf("gpio base %x error!\r\n", ptrDevPinHandle->base);
        printf("Please check the pin configuration in dev_pin.h\r\n");
        printf("Device name: %s\r\n", ptrDevPinHandle->device_name);
        printf("Pin mode: %d\r\n", ptrDevPinHandle->pin_mode);
        printf("Pin: %d\r\n", ptrDevPinHandle->pin);
        return;
    }

    /* GPIO 配置 */
    if (ptrDevPinHandle->af) {
        /* 配置为复用功能 */
        gpio_af_set(ptrDevPinHandle->base, ptrDevPinHandle->af, ptrDevPinHandle->pin);
        gpio_mode_set(ptrDevPinHandle->base, GPIO_MODE_AF, GPIO_PUPD_NONE, ptrDevPinHandle->pin);
        gpio_output_options_set(ptrDevPinHandle->base, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ,
                                ptrDevPinHandle->pin);
    } else {
        if (ptrDevPinHandle->pin_mode == DevPinModeInput) {
            /* 配置为输入 */
            gpio_mode_set(ptrDevPinHandle->base, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                          ptrDevPinHandle->pin);

        } else if (ptrDevPinHandle->pin_mode == DevPinModeOutput) {
            /* 配置为普通输出 */
            gpio_bit_write(ptrDevPinHandle->base, ptrDevPinHandle->pin,
                           ptrDevPinHandle->bit_value ? SET : RESET);
            gpio_mode_set(ptrDevPinHandle->base, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP,
                          ptrDevPinHandle->pin);
            gpio_output_options_set(ptrDevPinHandle->base, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ,
                                    ptrDevPinHandle->pin);
        } else {
            // do nothing
        }
    }
}

// DevPinWrite
void DevPinWrite(const DevPinHandleStruct *ptrDevPinHandle, uint8_t bit_value) {
    if (ptrDevPinHandle->pin_mode == DevPinModeOutput) {
        gpio_bit_write(ptrDevPinHandle->base, ptrDevPinHandle->pin, bit_value ? SET : RESET);
    } else {
        printf("Error: Pin %s is not configured as output.\r\n", ptrDevPinHandle->device_name);
    }
}

uint8_t DevPinRead(const DevPinHandleStruct *ptrDevPinHandle) {
    return (gpio_input_bit_get(ptrDevPinHandle->base, ptrDevPinHandle->pin) == SET) ? 1 : 0;
}

void DevErrorLED(uint8_t is_on) {
    rcu_periph_clock_enable(RCU_GPIOC);
    gpio_bit_write(GPIOC, GPIO_PIN_14, is_on ? SET : RESET);
    gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_14);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_14);
}
#endif