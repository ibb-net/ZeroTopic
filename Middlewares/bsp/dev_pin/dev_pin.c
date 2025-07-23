
#define CONFIG_DEV_PIN_EN 1
#if CONFIG_DEV_PIN_EN
#include "dev_pin.h"

#include "dev_basic.h"
#include "stdio.h"
#define DEVICE_PIN_MAX 32

#define PIN_EXIT_IRQ_PRE_PRIORITY 2U
#define PIN_EXIT_IRQ_SUB_PRIORITY 0U
typedef struct {
    uint8_t enable;
    uint8_t start;
    exti_line_enum linex;
    DevPinHandleStruct *ptrDevPinHandle;
    void (*isrcb)(void *ptrDevPinHandle);
} DevPinStatus;

DevPinStatus dev_pin_status[DEVICE_PIN_MAX] = {0};

/* GPIO 时钟使能 */

static const struct {
    uint32_t gpio_base;
    rcu_periph_enum rcu_clock;
} dev_clock_map[] = {
    {GPIOA, RCU_GPIOA}, {GPIOB, RCU_GPIOB}, {GPIOC, RCU_GPIOC}, {GPIOD, RCU_GPIOD},
    {GPIOE, RCU_GPIOE}, {GPIOF, RCU_GPIOF}, {GPIOG, RCU_GPIOG}, {GPIOH, RCU_GPIOH},
    {GPIOJ, RCU_GPIOJ}, {GPIOK, RCU_GPIOK},
};

typedef struct {
    uint32_t pin;
    IRQn_Type nvic_irq;
    uint8_t exti_pin;
    exti_line_enum exti_line;
} dev_exti_pin_map_t;

const dev_exti_pin_map_t dev_exti_pin_map[] = {
    {GPIO_PIN_0, EXTI0_IRQn, EXTI_SOURCE_PIN0, EXTI_0},
    {GPIO_PIN_1, EXTI1_IRQn, EXTI_SOURCE_PIN1, EXTI_1},
    {GPIO_PIN_2, EXTI2_IRQn, EXTI_SOURCE_PIN2, EXTI_2},
    {GPIO_PIN_3, EXTI3_IRQn, EXTI_SOURCE_PIN3, EXTI_3},
    {GPIO_PIN_4, EXTI4_IRQn, EXTI_SOURCE_PIN4, EXTI_4},
    {GPIO_PIN_5, EXTI5_9_IRQn, EXTI_SOURCE_PIN5, EXTI_5},
    {GPIO_PIN_6, EXTI5_9_IRQn, EXTI_SOURCE_PIN6, EXTI_6},
    {GPIO_PIN_7, EXTI5_9_IRQn, EXTI_SOURCE_PIN7, EXTI_7},
    {GPIO_PIN_8, EXTI5_9_IRQn, EXTI_SOURCE_PIN8, EXTI_8},
    {GPIO_PIN_9, EXTI5_9_IRQn, EXTI_SOURCE_PIN9, EXTI_9},
    {GPIO_PIN_10, EXTI10_15_IRQn, EXTI_SOURCE_PIN10, EXTI_10},
    {GPIO_PIN_11, EXTI10_15_IRQn, EXTI_SOURCE_PIN11, EXTI_11},
    {GPIO_PIN_12, EXTI10_15_IRQn, EXTI_SOURCE_PIN12, EXTI_12},
    {GPIO_PIN_13, EXTI10_15_IRQn, EXTI_SOURCE_PIN13, EXTI_13},
    {GPIO_PIN_14, EXTI10_15_IRQn, EXTI_SOURCE_PIN14, EXTI_14},
    {GPIO_PIN_15, EXTI10_15_IRQn, EXTI_SOURCE_PIN15, EXTI_15},
};
typedef struct {
    uint32_t port;
    uint8_t exti_port;
} dev_exti_port_map_t;
const dev_exti_port_map_t dev_exti_port_map[] = {
    {GPIOA, EXTI_SOURCE_GPIOA}, {GPIOB, EXTI_SOURCE_GPIOB}, {GPIOC, EXTI_SOURCE_GPIOC},
    {GPIOD, EXTI_SOURCE_GPIOD}, {GPIOE, EXTI_SOURCE_GPIOE}, {GPIOF, EXTI_SOURCE_GPIOF},
    {GPIOG, EXTI_SOURCE_GPIOG}, {GPIOH, EXTI_SOURCE_GPIOH}, {GPIOJ, EXTI_SOURCE_GPIOJ},
    {GPIOK, EXTI_SOURCE_GPIOK},
};
dev_exti_pin_map_t *DevGetPinExtiMap(const DevPinHandleStruct *ptrDevPinHandle) {
    for (int i = 0; i < sizeof(dev_exti_pin_map) / sizeof(dev_exti_pin_map[0]); i++) {
        if (dev_exti_pin_map[i].pin == ptrDevPinHandle->pin) {
            return &dev_exti_pin_map[i];
        }
    }
    return NULL;
}
dev_exti_port_map_t *DevGetPortExtiMap(const DevPinHandleStruct *ptrDevPinHandle) {
    for (int i = 0; i < sizeof(dev_exti_port_map) / sizeof(dev_exti_port_map[0]); i++) {
        if (dev_exti_port_map[i].port == ptrDevPinHandle->base) {
            return &dev_exti_port_map[i];
        }
    }
    return NULL;
}
void DevPinInit(const DevPinHandleStruct *ptrDevPinHandle) {
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
            if (ptrDevPinHandle->isrcb != NULL) {
                // rcu_periph_clock_enable(RCU_SYSCFG);
                dev_exti_pin_map_t *dev_exti_pin_map   = DevGetPinExtiMap(ptrDevPinHandle);
                dev_exti_port_map_t *dev_exti_port_map = DevGetPortExtiMap(ptrDevPinHandle);
                nvic_irq_enable(dev_exti_pin_map->nvic_irq, PIN_EXIT_IRQ_PRE_PRIORITY,
                                PIN_EXIT_IRQ_SUB_PRIORITY);
                syscfg_exti_line_config(dev_exti_port_map->exti_port, dev_exti_pin_map->exti_pin);
                exti_init(dev_exti_pin_map->exti_line, EXTI_INTERRUPT, ptrDevPinHandle->exti_trig);
                exti_interrupt_flag_clear(dev_exti_pin_map->exti_line);
                for (int j = 0; j < DEVICE_PIN_MAX; j++) {
                    if (dev_pin_status[j].enable == 0) {
                        dev_pin_status[j].enable          = 1;
                        dev_pin_status[j].start           = 0;
                        dev_pin_status[j].ptrDevPinHandle = (DevPinHandleStruct *)ptrDevPinHandle;
                        dev_pin_status[j].isrcb           = ptrDevPinHandle->isrcb;
                        dev_pin_status[j].linex           = dev_exti_pin_map->exti_line;
                        printf("DevPinInit: %s pin %d is ready for interrupt.\r\n",
                               ptrDevPinHandle->device_name, ptrDevPinHandle->pin);
                        break;
                    }
                }
            }

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

void DevErrorLEDToggle() {
    static uint8_t is_init = 0;
    static uint32_t cyc    = 0;
    static uint8_t is_on   = 0;
    if (!is_init) {
        is_init = 1;
        rcu_periph_clock_enable(RCU_GPIOC);
        gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_14);
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_14);
    }
    if (cyc % 100 == 0) {
        is_on = !is_on;
        gpio_bit_write(GPIOC, GPIO_PIN_14, is_on ? SET : RESET);
    }

    cyc++;
}

void DevErrorLED1Toggle() {
    static uint8_t is_init = 0;
    static uint32_t cyc    = 0;
    static uint8_t is_on   = 0;
    if (!is_init) {
        is_init = 1;
        rcu_periph_clock_enable(RCU_GPIOC);
        gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_15);
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_15);
    }
    if (cyc % 10000 == 0) {
        is_on = !is_on;
        gpio_bit_write(GPIOC, GPIO_PIN_15, is_on ? SET : RESET);
    }

    cyc++;
}
void DevPinGetCallback(exti_line_enum linex) {
    uint8_t is_found = 0;
    for (int i = 0; i < DEVICE_PIN_MAX; i++) {
        if (dev_pin_status[i].enable && dev_pin_status[i].ptrDevPinHandle != NULL) {
            if (dev_pin_status[i].linex == linex) {
                if (dev_pin_status[i].isrcb != NULL) {
                    if (dev_pin_status[i].start) {
                        dev_pin_status[i].isrcb((void *)dev_pin_status[i].ptrDevPinHandle);
                    }
                    is_found = 1;
                }
            }
        }
    }
    if (!is_found) {
        printf("DevPinGetCallback: No callback found for line %d\r\n", linex);
    }
}
void DevPinIsStartISR(const DevPinHandleStruct *ptrDevPinHandle, uint8_t istart) {
    if (ptrDevPinHandle == NULL) {
        printf("Error: Invalid parameters for DevPinIsStartISR.\r\n");
        return;
    }
    for (int i = 0; i < DEVICE_PIN_MAX; i++) {
        if (dev_pin_status[i].enable && dev_pin_status[i].ptrDevPinHandle != NULL) {
            if (dev_pin_status[i].ptrDevPinHandle == ptrDevPinHandle) {
                dev_pin_status[i].start = istart;
                if (istart) {
                    // Enable the EXTI line
                    exti_interrupt_flag_clear(dev_pin_status[i].linex);
                    exti_interrupt_enable(dev_pin_status[i].linex);
                    printf("DevPinIsStartISR: %s pin %d interrupt enabled.\r\n",
                           ptrDevPinHandle->device_name, ptrDevPinHandle->pin);
                } else {
                    // Disable the EXTI line
                    exti_interrupt_disable(dev_pin_status[i].linex);
                    printf("DevPinIsStartISR: %s pin %d interrupt disabled.\r\n",
                           ptrDevPinHandle->device_name, ptrDevPinHandle->pin);
                }
                return;
            }
        }
    }
}

void EXTI0_IRQHandler(void) {
    if (RESET != exti_interrupt_flag_get(EXTI_0)) {
        DevPinGetCallback(EXTI_0);
        exti_interrupt_flag_clear(EXTI_0);
    }
}
void EXTI5_9_IRQHandler(void) {
    if (RESET != exti_interrupt_flag_get(EXTI_9)) {
        DevPinGetCallback(EXTI_9);
        exti_interrupt_flag_clear(EXTI_9);
    }
}

#endif