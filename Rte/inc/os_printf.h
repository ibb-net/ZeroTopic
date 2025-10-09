
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#ifndef _OS_PRINTF_H_
#define _OS_PRINTF_H_

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OS_CONSOLE_TRUE  = 1,
    OS_CONSOLE_FALSE = 0,

} OsConsoleBool_e;

extern ssize_t os_printf(const char* __format, ...);

extern ssize_t os_console_key_get(void);
extern ssize_t os_console_echo_set(OsConsoleBool_e En);
extern ssize_t os_console_ansi_set(OsConsoleBool_e En);


#ifdef __cplusplus
}
#endif

#endif
