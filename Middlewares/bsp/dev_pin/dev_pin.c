
#define CONFIG_DEV_PIN_EN 1
#if CONFIG_DEV_PIN_EN
#include "dev_pin.h"
#include "stdio.h"
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

void DevErrorLEDToggle() {
    static uint8_t is_init = 0;
    static uint32_t cyc    = 0;
    static uint8_t is_on=0;
    if (!is_init) {
        is_init = 1;
        rcu_periph_clock_enable(RCU_GPIOC);
        gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_14);
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_14);
    }
    if (cyc % 100 == 0)
    {
        is_on = !is_on;
        gpio_bit_write(GPIOC, GPIO_PIN_14, is_on ? SET : RESET);
    }
       
    cyc++;
}

void DevErrorLED1Toggle() {
    static uint8_t is_init = 0;
    static uint32_t cyc    = 0;
    static uint8_t is_on=0;
    if (!is_init) {
        is_init = 1;
        rcu_periph_clock_enable(RCU_GPIOC);
        gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_15);
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_15);
    }
    if (cyc % 10000 == 0)
    {
        is_on = !is_on;
        gpio_bit_write(GPIOC, GPIO_PIN_15, is_on ? SET : RESET);
    }
       
    cyc++;
}
void DevPinSetIsrCallback(const DevPinHandleStruct *ptrDevPinHandle,
                                 void* callback) {
    // if (ptrDevPinHandle == NULL || callback == NULL) {
    //     printf("Error: Invalid parameters for DevPinSetIsrCallback.\r\n");
    //     return;
    // } 
    //     static const struct {
    //     uint32_t gpio_base;
    //     uint8_t exti_port;
    //     uint8_t exti_pin;
    //     exti_line_enum exti_line;
    // exti_trig_type_enum exti_trig;
    // } dev_exti_map[] = {
    //     {GPIOA, 0, 0, EXTI_0},
    //     {GPIOB, 1, 0, EXTI_1},
    //     {GPIOC, 2, 0, EXTI_2},
    //     {GPIOD, 3, 0, EXTI_3},
    //     {GPIOE, 4, 0, EXTI_4},
    //     {GPIOF, 5, 0, EXTI_5},
    //     {GPIOG, 6, 0, EXTI_6},
    //     {GPIOH, 7, 0, EXTI_7},
    //     {GPIOJ, 8, 0, EXTI_8},
    //     {GPIOK, 9, 0, EXTI_9},
    // } ;
        rcu_periph_clock_enable(RCU_SYSCFG);
    nvic_irq_enable(EXTI0_IRQn, 2U, 0U);
    nvic_irq_enable(EXTI5_9_IRQn, 2U, 0U);
    /* connect key EXTI line to key GPIO pin */
    syscfg_exti_line_config(EXTI_SOURCE_GPIOD, EXTI_SOURCE_PIN9);
    /* configure key EXTI line */
    exti_init(EXTI_9, EXTI_INTERRUPT, EXTI_TRIG_BOTH);
    exti_interrupt_flag_clear(EXTI_9);
    printf("DevPinSetIsrCallback called for device: %s\r\n", ptrDevPinHandle->device_name);

    // 这里可以添加中断配置代码
    // 例如，使用 GPIO 中断配置函数设置中断回调
    // gpio_interrupt_config(ptrDevPinHandle->base, ptrDevPinHandle->pin, callback);
}

void EXTI0_IRQHandler(void)
{
    if(RESET != exti_interrupt_flag_get(EXTI_0)) {
        printf("EXTI0_IRQHandler called\r\n");
        exti_interrupt_flag_clear(EXTI_0);
    }
}
void EXTI5_9_IRQHandler(void)
{
    if(RESET != exti_interrupt_flag_get(EXTI_9)) {
        printf("EXTI5_9_IRQHandler called\r\n");
        exti_interrupt_flag_clear(EXTI_9);
    }
}




#endif