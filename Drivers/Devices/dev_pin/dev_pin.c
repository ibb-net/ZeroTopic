
#define CONFIG_DEV_PIN_EN 1
#if CONFIG_DEV_PIN_EN
#include "dev_basic.h"
#include "dev_pin.h"


void DevPinInit(const DevPinHandleStruct *ptrDevPinHandle)
{
    static const struct
    {
        uint32_t gpio_base;
        rcu_periph_enum rcu_clock;
    } dev_clock_map[] = {
        {GPIOA, RCU_GPIOA},
        {GPIOB, RCU_GPIOB},
        {GPIOC, RCU_GPIOC},
        {GPIOD, RCU_GPIOD},
        {GPIOE, RCU_GPIOE},
        {GPIOF, RCU_GPIOF},
        {GPIOG, RCU_GPIOG},
        {GPIOH, RCU_GPIOH},
        {GPIOJ, RCU_GPIOJ},
        {GPIOK, RCU_GPIOK},
    };

    uint8_t is_found = 0;
    for (size_t i = 0; i < sizeof(dev_clock_map) / sizeof(dev_clock_map[0]); i++)
    {
        if (dev_clock_map[i].gpio_base == ptrDevPinHandle->base)
        {
            rcu_periph_clock_enable(dev_clock_map[i].rcu_clock);
            is_found = 1;
            break;
        }
    }

    if (!is_found)
    {
        printf("gpio base %x error!\r\n", ptrDevPinHandle->base);
        return;
    }

    /* GPIO 配置 */
    if (ptrDevPinHandle->af)
    {
        /* 配置为复用功能 */
        gpio_af_set(ptrDevPinHandle->base, ptrDevPinHandle->af, ptrDevPinHandle->pin);
        gpio_mode_set(ptrDevPinHandle->base, GPIO_MODE_AF, GPIO_PUPD_PULLUP, ptrDevPinHandle->pin);
        gpio_output_options_set(ptrDevPinHandle->base, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, ptrDevPinHandle->pin);
    }
    else
    {
        /* 配置为普通输出 */
        gpio_mode_set(ptrDevPinHandle->base, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, ptrDevPinHandle->pin);
        gpio_output_options_set(ptrDevPinHandle->base, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, ptrDevPinHandle->pin);
    }
}

#endif