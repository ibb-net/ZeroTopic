#include <stdio.h>

// #include "dev/device_driver.h"
#include "ex_list_dma.h"
// #include "gd32h759i_eval.h"
#include "gd32h7xx.h"
#include "semphr.h"

#define DEVICE_NAME_MAX 32
#define DEVICE_UART_MAX 8
#define UART_DEF_BUF_NUM 4
#define UART_DEF_BUFFER_SIZE 128
// TODO 需要放在dev.h中
typedef enum {
    DEV_TYPE_GPIO,
    DEV_TYPE_DMA,
    DEV_TYPE_UART,
    DEV_TYPE_SPI,
    DEV_TYPE_I2C,
    DEV_TYPE_ADC,
    DEV_TYPE_CAN,
    DEV_TYPE_I2S,
    DEV_TYPE_PWM,
    DEV_TYPE_RTC,
    DEV_TYPE_SAI,
    DEV_TYPE_USB,
    DEV_TYPE_SDIO,
    DEV_TYPE_ETH,
    DEV_TYPE_FLASH,
    DEV_TYPE_SRAM,
    DEV_TYPE_NOR,
    DEV_TYPE_NAND,
    DEV_TYPE_QSPI,
    DEV_TYPE_UNKNOWN,
    DEV_TYPE_MAX,
} EnumDevType;
typedef steam_info_struct *steam_buffer_handle_t;
typedef struct {
    char device_name[DEVICE_NAME_MAX];
    uint32_t base_addr;
    uint32_t channel;
    uint32_t request;

} DevDMAHandleStruct;
typedef struct {
    char device_name[DEVICE_NAME_MAX];
    uint32_t port;
    uint32_t pin;
    uint32_t af;

} DevGPIOHandleStruct;
typedef DevGPIOHandleStruct *dev_gpio_handle_t;
typedef struct bsp_uart {
    /* basic */
    char device_name[DEVICE_NAME_MAX];
    /* GPIO */
    DevGPIOHandleStruct tx_gpio_handle;
    DevGPIOHandleStruct rx_gpio_handle;
    /* Uart */
    uint32_t uart_base;
    uint32_t baudrate;
    uint32_t idle_timeout;
    /* DMA Config */
    DevDMAHandleStruct tx_dma_handle;
    DevDMAHandleStruct rx_dma_handle;
    /* Lock */
    SemaphoreHandle_t lock;
    SemaphoreHandle_t lock_tx;
    /* Queue */
    SemaphoreHandle_t rx_queue;
    SemaphoreHandle_t tx_queue;
    /* Thread */
    TaskHandle_t rx_task_handle;
    TaskHandle_t tx_task_handle;
    /* Buffer */
    size_t buffer_size;
    size_t buffer_num;
    /* DMA List */
    list_dma_buffer_t rx_list_handle;
    list_dma_buffer_t tx_list_handle;

} DevUARTHandleStruct;
typedef DevUARTHandleStruct *bsp_uart_handle_t;

void dev_uart_init(void);
void uart_nvic_config(void);
void com_uart_rx_isr(void);
void uart_tx_dma_isr(void);
void bsp_uart_init(void *handle);
