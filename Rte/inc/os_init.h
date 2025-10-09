
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#ifndef _OS_INIT_H_
#define _OS_INIT_H_

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ssize_t os_init(void);
extern ssize_t os_exit(void);
extern ssize_t os_start(void);

#ifdef __cplusplus
}
#endif

#endif
