#ifndef __DEV_UART_H__
#define __DEV_UART_H__
#include "dev_basic.h"


typedef struct
{
    char device_name[DEVICE_NAME_MAX];
    uint32_t id;
    uint32_t base;
    uint32_t baudrate;
    uint32_t idle_timeout;
    dev_isr_cb rx_isr_cb;
    dev_isr_cb tx_isr_cb; 
    dev_isr_cb error_isr_cb; 
    /* DMA */
    uint32_t tx_dma_rcu;         
    uint32_t tx_dma_base_addr;   
    uint32_t tx_dma_channel;
    uint32_t tx_dma_request;
    dev_isr_cb tx_dma_isr_cb; // TX DMA ISR callback function
    uint32_t rx_dma_rcu;
    uint32_t rx_dma_base_addr;
    uint32_t rx_dma_channel;
    uint32_t rx_dma_request;
    dev_isr_cb rx_dma_isr_cb; // RX DMA ISR callback function
} DevUartHandleStruct;



int fputc(int ch, FILE *f);
int __io_putchar(int ch) ;
int debug_putbuffer(const char *buffer, size_t len);
void DevUartRegister(uint32_t base, void *handle);
void DevUartInit(const DevUartHandleStruct *ptrDevUartHandle);
void DevUartDeinit(const DevUartHandleStruct *ptrDevUartHandle);
void DevUarStart(const DevUartHandleStruct *ptrDevUartHandle);
void DevUartDMASend(const DevUartHandleStruct *ptrDevUartHandle, const uint8_t *data, size_t len);
void DevUartDMARecive(const DevUartHandleStruct *ptrDevUartHandle, const uint8_t *data, size_t len) ;

/* ISR */
void USART0_IRQHandler(void);

#endif // __DEV_UART_H__