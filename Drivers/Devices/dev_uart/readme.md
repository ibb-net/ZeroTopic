1.重定向
(1)在keil中的串口重定向为int fputc(int ch, FILE *f)
(2)在使用gcc则使用int _write(int file, char *ptr, int len)进行实现,其中依赖的函数为int  __io_putchar(int ch) 

