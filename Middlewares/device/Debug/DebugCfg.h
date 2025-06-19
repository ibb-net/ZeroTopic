#ifndef __DEBUG_CFG_H__
#define __DEBUG_CFG_H__
#include "config.h"

// UART 配置

#define DEBUG_UART_BASE          USART0
#define DEBUG_UART_BAUDRATE      115200
#define DEBUG_UART_IDLE_TIMEOUT  100  // 单位：ms
// UART 配置
#if 0           //kaifaban
// TX GPIO 配置
#define DEBUG_TX_GPIO_PORT       GPIOB
#define DEBUG_TX_GPIO_AF         GPIO_AF_4
#define DEBUG_TX_GPIO_PIN        GPIO_PIN_14

// RX GPIO 配置
#define DEBUG_RX_GPIO_PORT       GPIOB
#define DEBUG_RX_GPIO_AF         GPIO_AF_4
#define DEBUG_RX_GPIO_PIN        GPIO_PIN_15
// TX GPIO 配置
#else
#define DEBUG_TX_GPIO_PORT       GPIOB
#define DEBUG_TX_GPIO_AF         GPIO_AF_7
#define DEBUG_TX_GPIO_PIN        GPIO_PIN_6

// RX GPIO 配置
#define DEBUG_RX_GPIO_PORT       GPIOB
#define DEBUG_RX_GPIO_AF         GPIO_AF_7
#define DEBUG_RX_GPIO_PIN        GPIO_PIN_7
#endif

// RX DMA 配置
#define DEBUG_RX_DMA_BASE_ADDR   DMA0
#define DEBUG_RX_DMA_CHANNEL     DMA_CH0  //DMA_CH0
#define DEBUG_RX_DMA_REQUEST     DMA_REQUEST_USART0_RX

// TX DMA 配置
#define DEBUG_TX_DMA_BASE_ADDR   DMA1
#define DEBUG_TX_DMA_CHANNEL     DMA_CH0
#define DEBUG_TX_DMA_REQUEST     DMA_REQUEST_USART0_TX

//Buffer
#define DEBUG_UART_BUFFER_SIZE   2048
#endif // __DEBUG_CFG_H__