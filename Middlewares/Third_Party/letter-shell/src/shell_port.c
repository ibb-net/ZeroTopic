/**
 * @file shell_port.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2019-02-22
 * 
 * @copyright (c) 2019 Letter
 * 
 */
//#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


/* Library includes. */
#include "shell.h"
#include "os_server.h"
#include "task.h"



Shell shell;
char shellBuffer[512];

static SemaphoreHandle_t shellMutex;

/**
 * @brief 用户shell写
 * 
 * @param data 数据
 * @param len 数据长度
 * 
 * @return short 实际写入的数据长度
 */
short userShellWrite(char *data, unsigned short len)
{
    //iUartWrite(shell_uart_handle, (uint8_t *)data, len, portMAX_DELAY);
    vfb_send(DebugPrint, 0, data, len);
    return len;
}


/**
 * @brief 用户shell读
 * 
 * @param data 数据
 * @param len 数据长度
 * 
 * @return short 实际读取到
 */
short userShellRead(char *data, unsigned short len)
{
    int ret=0;
    ret =iUartRead(shell_uart_handle, (uint8_t *)data, (size_t) len, pdMS_TO_TICKS(100));
    return (short) ret;
}

/**
 * @brief 用户shell上锁
 * 
 * @param shell shell
 * 
 * @return int 0
 */
int userShellLock(Shell *shell)
{
    xSemaphoreTakeRecursive(shellMutex, portMAX_DELAY);
    return 0;
}

/**
 * @brief 用户shell解锁
 * 
 * @param shell shell
 * 
 * @return int 0
 */
int userShellUnlock(Shell *shell)
{
    xSemaphoreGiveRecursive(shellMutex);
    return 0;
}

/**
 * @brief 用户shell初始化
 * 
 */
void userShellInit(void)
{
    printf("letter shell Start");
    /* Init Uart */
    shell_uart_handle = xUartOpen(0);

	if (shell_uart_handle != NULL) {
		vUartInit(shell_uart_handle, 115200, 0);
	}
    else
    {
        TRACE_ERROR("Shell Open uart failed");

    }
    shellMutex = xSemaphoreCreateMutex();
    if(shellMutex ==NULL)
        TRACE_ERROR("Shell shellMutex create failed");
    shell.write = userShellWrite;
    shell.read = userShellRead;
    #if SHELL_USING_LOCK == 1
    shell.lock = userShellLock;
    shell.unlock = userShellUnlock;
    #endif
    TRACE_INFO("vShellTask Start");
    shellInit(&shell, shellBuffer, 512);
    if (xTaskCreate( shellTask, "vShellTask", configMINIMAL_STACK_SIZE*8, &shell, SystemPriGroup+0, NULL) != pdPASS)
    {
        TRACE_ERROR("shell task creat failed");
    }
}

