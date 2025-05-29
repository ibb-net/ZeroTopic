#ifndef __DEV_UART_H__
#define __DEV_UART_H__
#include <stdio.h>
#include "gd32h7xx.h"
void com_usart_init(void);
int fputc(int ch, FILE *f);
int __io_putchar(int ch) ;
int debug_putbuffer(const char *buffer, size_t len);
#endif // __DEV_UART_H__