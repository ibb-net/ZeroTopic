#ifndef __DEV_UART_H__
#define __DEV_UART_H__
#include "dev_basic.h"


typedef struct
{
    char device_name[DEVICE_NAME_MAX];
    uint32_t base;
    uint32_t baudrate;
    uint32_t idle_timeout;
    dev_isr_cb rx_isr_cb;
    dev_isr_cb tx_isr_cb; 
    dev_isr_cb error_isr_cb; 
} DevUartHandleStruct;



int fputc(int ch, FILE *f);
int __io_putchar(int ch) ;
int debug_putbuffer(const char *buffer, size_t len);

void DevUartInit(const DevUartHandleStruct *ptrDevUartHandle);
void DevUartDeinit(const DevUartHandleStruct *ptrDevUartHandle);
void DevUarStart(const DevUartHandleStruct *ptrDevUartHandle);

/* ISR */
void USART0_IRQHandler(void);

#endif // __DEV_UART_H__