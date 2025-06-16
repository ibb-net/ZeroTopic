#ifndef __Uart_CFG_H__
#define __Uart_CFG_H__
#include "config.h"

// UART 配置

#define Uart_UART_BASE         UART6
#define Uart_UART_BAUDRATE     115200
#define Uart_UART_IDLE_TIMEOUT 10  // 单位：ms

// TX GPIO 配置
#define Uart_TX_GPIO_PORT GPIOE
#define Uart_TX_GPIO_AF   GPIO_AF_7
#define Uart_TX_GPIO_PIN  GPIO_PIN_8

// RX GPIO 配置
#define Uart_RX_GPIO_PORT GPIOE
#define Uart_RX_GPIO_AF   GPIO_AF_7
#define Uart_RX_GPIO_PIN  GPIO_PIN_7

// RX DMA 配置
#define Uart_RX_DMA_BASE_ADDR DMA0
#define Uart_RX_DMA_CHANNEL   DMA_CH1  
#define Uart_RX_DMA_REQUEST   DMA_REQUEST_UART6_RX

// TX DMA 配置
#define Uart_TX_DMA_BASE_ADDR DMA1
#define Uart_TX_DMA_CHANNEL   DMA_CH1
#define Uart_TX_DMA_REQUEST   DMA_REQUEST_UART6_TX

// Buffer
#define Uart_UART_BUFFER_SIZE 128
#endif  // __Uart_CFG_H__