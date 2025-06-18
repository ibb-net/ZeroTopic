#ifndef __DEVICE_BASIC_H
#define __DEVICE_BASIC_H
#include <stdio.h>

#include "gd32h7xx.h"
#define GD32H7XXZ_PIN_MAP_MAX DEV_PIN_MAX
#define DEVICE_NAME_MAX       32
typedef void (*dev_isr_cb)(void *arg);
typedef struct {
    char pin_name[DEVICE_NAME_MAX];  // Device name, e.g., "GPIOA_PIN0"
    uint32_t pin_id;
    rcu_periph_enum rcu_clock;
    uint32_t base;
    uint32_t pin;
} TypedefDevPinMap;
typedef enum{
    DEV_SPI0 = 0,
    DEV_SPI1,
    DEV_SPI2,
    DEV_SPI3,
    DEV_SPI4,
    DEV_SPI5,
}EnumDevMaps;
typedef enum {
    DEV_PIN_START = 0,
    DEV_PIN_PE2   = 1,      // GPIOE_PIN2
    DEV_PIN_PE3,            // GPIOE_PIN3
    DEV_PIN_PE4,            // GPIOE_PIN4
    DEV_PIN_PE5,            // GPIOE_PIN5
    DEV_PIN_PE6,            // GPIOE_PIN6
    DEV_PIN_VBAT,           // VBAT
    DEV_PIN_PC13,           // GPIOC_PIN13
    DEV_PIN_PC14_OSC32IN,   // GPIOC_PIN14_OSC32IN
    DEV_PIN_PC15_OSC32OUT,  // GPIOC_PIN15_OSC32OUT
    DEV_PIN_PF0,            // GPIOF_PIN0
    DEV_PIN_PF1,            // GPIOF_PIN1
    DEV_PIN_PF2,            // GPIOF_PIN2
    DEV_PIN_PF3,            // GPIOF_PIN3
    DEV_PIN_PF4,            // GPIOF_PIN4
    DEV_PIN_PF5,            // GPIOF_PIN5
    DEV_PIN_VSS,            // VSS
    DEV_PIN_VDD,            // VDD
    DEV_PIN_PF6,            // GPIOF_PIN6
    DEV_PIN_PF7,            // GPIOF_PIN7
    DEV_PIN_PF8,            // GPIOF_PIN8
    DEV_PIN_PF9,            // GPIOF_PIN9
    DEV_PIN_PF10,           // GPIOF_PIN10
    DEV_PIN_PH0_OSCIN,      // GPIOH_PIN0_OSCIN
    DEV_PIN_PH1_OSCOUT,     // GPIOH_PIN1_OSCOUT
    DEV_PIN_NRST,           // NRST
    DEV_PIN_PC0,            // GPIOC_PIN0
    DEV_PIN_PC1,            // GPIOC_PIN1
    DEV_PIN_PC2_C,          // GPIOC_PIN2_C
    DEV_PIN_PC3_C,          // GPIOC_PIN3_C
    DEV_PIN_VDD_2,          // VDD (duplicate for clarity)
    DEV_PIN_VSSA,           // VSSA
    DEV_PIN_VREFP,          // VREFP
    DEV_PIN_VDDA,           // VDDA
    DEV_PIN_PA0,            // GPIOA_PIN0
    DEV_PIN_PA1,            // GPIOA_PIN1
    DEV_PIN_PA2,            // GPIOA_PIN2
    DEV_PIN_PA3,            // GPIOA_PIN3
    DEV_PIN_VSS_2,          // VSS (duplicate for clarity)
    DEV_PIN_VDD_3,          // VDD (duplicate for clarity)
    DEV_PIN_PA4,            // GPIOA_PIN4
    DEV_PIN_PA5,            // GPIOA_PIN5
    DEV_PIN_PA6,            // GPIOA_PIN6
    DEV_PIN_PA7,            // GPIOA_PIN7
    DEV_PIN_PC4,            // GPIOC_PIN4
    DEV_PIN_PC5,            // GPIOC_PIN5
    DEV_PIN_PB0,            // GPIOB_PIN0
    DEV_PIN_PB1,            // GPIOB_PIN1
    DEV_PIN_PB2,            // GPIOB_PIN2
    DEV_PIN_PF11,           // GPIOF_PIN11
    DEV_PIN_PF12,           // GPIOF_PIN12
    DEV_PIN_VSS_3,          // VSS (duplicate for clarity)
    DEV_PIN_VDD_4,          // VDD (duplicate for clarity)
    DEV_PIN_PF13,           // GPIOF_PIN13
    DEV_PIN_PF14,           // GPIOF_PIN14
    DEV_PIN_PF15,           // GPIOF_PIN15
    DEV_PIN_PG0,            // GPIOG_PIN0
    DEV_PIN_PG1,            // GPIOG_PIN1
    DEV_PIN_PE7,            // GPIOE_PIN7
    DEV_PIN_PE8,            // GPIOE_PIN8
    DEV_PIN_PE9,            // GPIOE_PIN9
    DEV_PIN_VSS_4,          // VSS (duplicate for clarity)
    DEV_PIN_VDD_5,          // VDD (duplicate for clarity)
    DEV_PIN_PE10,           // GPIOE_PIN10
    DEV_PIN_PE11,           // GPIOE_PIN11
    DEV_PIN_PE12,           // GPIOE_PIN12
    DEV_PIN_PE13,           // GPIOE_PIN13
    DEV_PIN_PE14,           // GPIOE_PIN14
    DEV_PIN_PE15,           // GPIOE_PIN15
    DEV_PIN_PB10,           // GPIOB_PIN10
    DEV_PIN_PB11,           // GPIOB_PIN11
    DEV_PIN_VCORE,          // VCORE
    DEV_PIN_VDD_6,          // VDD (duplicate for clarity)
    DEV_PIN_PB12,           // GPIOB_PIN12
    DEV_PIN_PB13,           // GPIOB_PIN13
    DEV_PIN_PB14,           // GPIOB_PIN14
    DEV_PIN_PB15,           // GPIOB_PIN15
    DEV_PIN_PD8,            // GPIOD_PIN8
    DEV_PIN_PD9,            // GPIOD_PIN9
    DEV_PIN_PD10,           // GPIOD_PIN10
    DEV_PIN_PD11,           // GPIOD_PIN11
    DEV_PIN_PD12,           // GPIOD_PIN12
    DEV_PIN_PD13,           // GPIOD_PIN13
    DEV_PIN_VSS_5,          // VSS (duplicate for clarity)
    DEV_PIN_VDD_7,          // VDD (duplicate for clarity)
    DEV_PIN_PD14,           // GPIOD_PIN14
    DEV_PIN_PD15,           // GPIOD_PIN15
    DEV_PIN_PG2,            // GPIOG_PIN2
    DEV_PIN_PG3,            // GPIOG_PIN3
    DEV_PIN_PG4,            // GPIOG_PIN4
    DEV_PIN_PG5,            // GPIOG_PIN5
    DEV_PIN_PG6,            // GPIOG_PIN6
    DEV_PIN_PG7,            // GPIOG_PIN7
    DEV_PIN_PG8,            // GPIOG_PIN8
    DEV_PIN_VSS_6,          // VSS (duplicate for clarity)
    DEV_PIN_VDD33USB,       // VDD33USB
    DEV_PIN_PC6,            // GPIOC_PIN6
    DEV_PIN_PC7,            // GPIOC_PIN7
    DEV_PIN_PC8,            // GPIOC_PIN8
    DEV_PIN_PC9,            // GPIOC_PIN9
    DEV_PIN_PA8,            // GPIOA_PIN8
    DEV_PIN_PA9,            // GPIOA_PIN9
    DEV_PIN_PA10,           // GPIOA_PIN10
    DEV_PIN_USBHS0_DM,      // USBHS0_DM
    DEV_PIN_USBHS0_DP,      // USBHS0_DP
    DEV_PIN_PA13,           // GPIOA_PIN13
    DEV_PIN_VCORE_2,        // VCORE (duplicate for clarity)
    DEV_PIN_VSS_7,          // VSS (duplicate for clarity)
    DEV_PIN_VDD_8,          // VDD (duplicate for clarity)
    DEV_PIN_PA14,           // GPIOA_PIN14
    DEV_PIN_PA15,           // GPIOA_PIN15
    DEV_PIN_PC10,           // GPIOC_PIN10
    DEV_PIN_PC11,           // GPIOC_PIN11
    DEV_PIN_PC12,           // GPIOC_PIN12
    DEV_PIN_PD0,            // GPIOD_PIN0
    DEV_PIN_PD1,            // GPIOD_PIN1
    DEV_PIN_PD2,            // GPIOD_PIN2
    DEV_PIN_PD3,            // GPIOD_PIN3
    DEV_PIN_PD4,            // GPIOD_PIN4
    DEV_PIN_PD5,            // GPIOD_PIN5
    DEV_PIN_VSS_8,          // VSS (duplicate for clarity)
    DEV_PIN_VDD_9,          // VDD (duplicate for clarity)
    DEV_PIN_PD6,            // GPIOD_PIN6
    DEV_PIN_PD7,            // GPIOD_PIN7
    DEV_PIN_PG9,            // GPIOG_PIN9
    DEV_PIN_PG10,           // GPIOG_PIN10
    DEV_PIN_PG11,           // GPIOG_PIN11
    DEV_PIN_PG12,           // GPIOG_PIN12
    DEV_PIN_PG13,           // GPIOG_PIN13
    DEV_PIN_PG14,           // GPIOG_PIN14
    DEV_PIN_VSS_9,          // VSS (duplicate for clarity)
    DEV_PIN_VDD_10,         // VDD (duplicate for clarity)
    DEV_PIN_PG15,           // GPIOG_PIN15
    DEV_PIN_PB3,            // GPIOB_PIN3
    DEV_PIN_PB4,            // GPIOB_PIN4
    DEV_PIN_PB5,            // GPIOB_PIN5
    DEV_PIN_PB6,            // GPIOB_PIN6
    DEV_PIN_PB7,            // GPIOB_PIN7
    DEV_PIN_BOOT,           // BOOT
    DEV_PIN_PB8,            // GPIOB_PIN8
    DEV_PIN_PB9,            // GPIOB_PIN9
    DEV_PIN_PE0,            // GPIOE_PIN0
    DEV_PIN_PE1,            // GPIOE_PIN1
    DEV_PIN_PDR_ON,         // PDR_ON
    DEV_PIN_VDD_11,         // VDD (duplicate for clarity)
    DEV_PIN_MAX
} EnumKeyPinMapID;
extern const TypedefDevPinMap DevPinMap[GD32H7XXZ_PIN_MAP_MAX];

#endif  // __DEVICE_BASIC_H