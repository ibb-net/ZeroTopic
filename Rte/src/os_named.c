
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#include <string.h>
#include "os_heap.h"
#include "os_interrupt.h"
#include "os_named.h"

typedef struct {
    const char* pName;
    void*       pPoint;
    size_t      Length;
} InNamedPoint_t;

static size_t          gNamePointCounter = 0;
static InNamedPoint_t* gpNamedPointList  = NULL;

ssize_t os_named_point_configure(size_t MaxCounter)
{
    ssize_t         ExitCode        = 0;
    size_t          MallocLength    = 0;
    InNamedPoint_t* pNamedPointList = NULL;

    MallocLength = MaxCounter * sizeof(InNamedPoint_t);
    pNamedPointList = os_malloc(MallocLength);
    if (pNamedPointList == NULL) {
        ExitCode = -1;
        goto __error;
    }

    memset(pNamedPointList, 0, MallocLength);

    gNamePointCounter = MaxCounter;
    gpNamedPointList  = pNamedPointList;
    return ExitCode;

__error:
    if (pNamedPointList == NULL) {
        os_free(pNamedPointList);
    }

    return ExitCode;

}

ssize_t os_named_point_insert(const char* pName,
                              void*       pPoint,
                              size_t      Length)
{
    size_t  i        = 0;
    ssize_t ExitCode = 0;

    if(pName != NULL) {

        os_critical_enter();
        for (i = 0; i < gNamePointCounter; i++) {
            if (gpNamedPointList[i].pName == NULL) {
                break;
            }
        }

        if (i < gNamePointCounter) {
            gpNamedPointList[i].pName  = pName;
            gpNamedPointList[i].pPoint = pPoint;
            gpNamedPointList[i].Length = Length;
        } else {
            ExitCode = -1;
        }
        os_critical_exit();
    }

    return ExitCode;
}

void* os_named_point_search(const char* pName,
                            size_t      Length)
{
    size_t  i      = 0;
    ssize_t RetVal = 0;
    void*   pPoint = NULL;

    if(pName != NULL) {

        os_critical_enter();
        for (i = 0; i < gNamePointCounter; i++) {
            if(gpNamedPointList[i].Length != Length) {
                continue;
            }
            RetVal = strcmp(gpNamedPointList[i].pName, pName);
            if (RetVal == 0) {
                pPoint = gpNamedPointList[i].pPoint;
                break;
            }
        }
        os_critical_exit();
    }

    return pPoint;
}

ssize_t os_named_point_delete(const char* pName,
                              size_t      Length)
{
    size_t  i        = 0;
    ssize_t RetVal   = 0;
    ssize_t ExitCode = 0;

    if(pName != NULL) {

        os_critical_enter();
        for (i = 0; i < gNamePointCounter; i++) {
            if(gpNamedPointList[i].Length != Length) {
                continue;
            }
            RetVal = strcmp(gpNamedPointList[i].pName, pName);
            if (RetVal == 0) {
                break;
            }
        }

        if (i < gNamePointCounter) {
            memset(&gpNamedPointList[i], 0, sizeof(InNamedPoint_t));
        } else {
            ExitCode = -1;
        }
        os_critical_exit();
    }

    return ExitCode;
}
