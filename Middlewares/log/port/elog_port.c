/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2015-04-28
 */

#include <elog.h>
#include <string.h>

#include "dev_uart/dev_uart.h"
#include "os_server.h"
#include "task.h"
/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode elog_port_init(void) {
    ElogErrCode result = ELOG_NO_ERR;

    /* add your code here */

    return result;
}

/**
 * EasyLogger port deinitialize
 *
 */
void elog_port_deinit(void) {
    /* add your code here */
}

/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char *log, size_t size) {
    static uint32_t is_os_run = 0;
    if (is_os_run == 0) {
        debug_putbuffer(log, size);
        is_os_run = xTaskGetSchedulerState() == taskSCHEDULER_RUNNING ? 1 : 0;
    } else {
        vfb_send(DebugPrint, 0, log, size);
    }
}

/**
 * output lock
 */
void elog_port_output_lock(void) {
    /* add your code here */
}

/**
 * output unlock
 */
void elog_port_output_unlock(void) {
    /* add your code here */
}

/**
 * get current time interface
 *
 * @return current time
 */
char time_buffer[32];
const char *elog_port_get_time(void) {
    /* add your code here */
    uint32_t ticks   = xTaskGetTickCount();
    uint32_t seconds = ticks / configTICK_RATE_HZ;
    memset((void *)time_buffer, 0, sizeof(time_buffer));  // Clear the time_buffer
    // 时间格式为HH:MM:SS.ms
    snprintf(time_buffer, sizeof(time_buffer), "%02lu:%02lu:%02lu.%03lu",
             (seconds / 3600) % 24, (seconds / 60) % 60, seconds % 60, (ticks % configTICK_RATE_HZ) * 1000 / configTICK_RATE_HZ);
    return (const char *)time_buffer;

    // return "00:00:00";
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {
    /* add your code here */
    return "ProcessName";
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {
    /* add your code here */
    return "ThreadName";
}