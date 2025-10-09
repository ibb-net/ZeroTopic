
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#include "os_interrupt.h"

static os_critical_t CriticalEnter  = NULL;
static os_critical_t CriticalExit = NULL;

ssize_t os_critical_configure(os_critical_t Enter,
                              os_critical_t Exit)
{
    if ((Enter == NULL) || (Exit  == NULL)) {
        return -1;
    }

    CriticalEnter = Enter;
    CriticalExit  = Exit;
    return 0;
}

void os_critical_enter(void)
{
    if (CriticalEnter  != NULL) {
        (*CriticalEnter)();
    }
}

void os_critical_exit(void)
{
    if (CriticalExit  != NULL) {
        (*CriticalExit)();
    }
}
