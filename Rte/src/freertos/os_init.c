
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "os_init.h"

ssize_t os_init(void)
{
    return 0;
}

ssize_t os_exit(void)
{
    return 0;
}

ssize_t os_start(void)
{
    vTaskStartScheduler();
    return 0;
}
