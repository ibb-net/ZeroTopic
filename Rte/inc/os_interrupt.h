
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#ifndef _OS_INTERRUPT_H_
#define _OS_INTERRUPT_H_

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*os_critical_t)(void);
extern ssize_t os_critical_configure(os_critical_t Enter,
                                     os_critical_t Exit);

extern void os_critical_enter(void);
extern void os_critical_exit(void);

#ifdef __cplusplus
}
#endif

#endif
