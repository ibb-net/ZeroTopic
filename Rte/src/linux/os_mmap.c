

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "os_heap.h"
#include "os_mmap.h"

#define container_of(ptr, type, member) ({                   \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type,member));    \
    })

typedef struct {
    const char* pHandleType;
    char*       pName;
    int         FileHandle;
    OsMMap_t    MMap;
} InMMap_t;

static const char HandleType[] = {"LINUX_MMAP"};

OsMMap_t* os_mmap_create(const char* pName, size_t Length)
{
    int        RetVal = 0;
    int        FileHandle = -1;
    void*      pBuffer = NULL;
    char*      pNameTemp = NULL;
    InMMap_t* pFileMap = NULL;

    if (pName == NULL) {
        return NULL;
    }

    if (access(pName, F_OK) >= 0) {
        if (unlink(pName) < 0) {
            return NULL;
        }
    }

    FileHandle = open(pName, O_CREAT | O_RDWR, 00600);
    if (FileHandle < 0) {
        goto __error;
    }

    RetVal = ftruncate(FileHandle, Length);
    if (RetVal < 0) {
        goto __error;
    }

    pBuffer = mmap(NULL,
                   Length,
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED,
                   FileHandle,
                   0);
    if (pBuffer == MAP_FAILED) {
        goto __error;
    }

    pNameTemp = os_malloc(strlen(pName) + 1);
    if (pNameTemp == NULL) {
        goto __error;
    }

    pFileMap = os_malloc(sizeof(InMMap_t));
    if (pFileMap == NULL) {
        goto __error;
    }

    pFileMap->pName = strcpy(pNameTemp, pName);
    if (pFileMap->pName == NULL) {
        goto __error;
    }

    pFileMap->pHandleType = HandleType;
    pFileMap->FileHandle = FileHandle;
    pFileMap->MMap.Length = Length;
    pFileMap->MMap.pBuffer = pBuffer;
    return &(pFileMap->MMap);

__error:
    if (pFileMap != NULL) {
        os_free(pFileMap);
    }

    if (pNameTemp != NULL) {
        os_free(pNameTemp);
    }

    if (pBuffer != NULL) {
        munmap(pBuffer, Length);
    }

    if (FileHandle >= 0) {
        close(FileHandle);
    }

    return NULL;
}

ssize_t os_mmap_destroy(OsMMap_t* pMMap)
{
    InMMap_t* pFileMap = NULL;

    if (pMMap == NULL) {
        return 0;
    }

    pFileMap = container_of(pMMap, InMMap_t, MMap);
    if (pFileMap->pHandleType != HandleType) {
        return -1;
    }

    if (pFileMap->MMap.pBuffer != NULL) {
        munmap(pFileMap->MMap.pBuffer, pFileMap->MMap.Length);
    }

    if (pFileMap->FileHandle >= 0) {
        close(pFileMap->FileHandle);
    }

    if (access(pFileMap->pName, F_OK) >= 0) {
        unlink(pFileMap->pName);
    }

    if (pFileMap->pName != NULL) {
        os_free(pFileMap->pName);
    }

    if (pFileMap != NULL) {
        os_free(pFileMap);
    }

    return 0;
}

OsMMap_t* os_mmap_open(const char* pName, size_t Length)
{
    int         RetVal = 0;
    int         FileHandle = -1;
    void*       pBuffer = NULL;
    char*       pNameTemp = NULL;
    InMMap_t*   pFileMap = NULL;

    if (pName == NULL) {
        return NULL;
    }

    FileHandle = open(pName, O_RDWR, 00600);
    if (FileHandle < 0) {
        goto __error;
    }

    pBuffer = mmap(NULL,
                   Length,
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED,
                   FileHandle,
                   0);
    if (pBuffer == MAP_FAILED) {
        goto __error;
    }

    pNameTemp = os_malloc(strlen(pName) + 1);
    if (pNameTemp == NULL) {
        goto __error;
    }

    pFileMap = os_malloc(sizeof(InMMap_t));
    if (pFileMap == NULL) {
        goto __error;
    }

    pFileMap->pName = strcpy(pNameTemp, pName);
    if (pFileMap->pName == NULL) {
        goto __error;
    }
    pFileMap->pHandleType = HandleType;
    pFileMap->FileHandle = FileHandle;
    pFileMap->MMap.Length = Length;
    pFileMap->MMap.pBuffer = pBuffer;
    return &(pFileMap->MMap);

__error:
    if (pFileMap != NULL) {
        os_free(pFileMap);
    }

    if (pNameTemp != NULL) {
        os_free(pNameTemp);
    }

    if (pBuffer != NULL) {
        munmap(pBuffer, Length);
    }

    if (FileHandle >= 0) {
        close(FileHandle);
    }

    return NULL;
}

ssize_t os_mmap_close(OsMMap_t* pMMap)
{
    InMMap_t* pFileMap = NULL;

    if (pMMap == NULL) {
        return 0;
    }
    pFileMap = container_of(pMMap, InMMap_t, MMap);
    if (pFileMap->pHandleType != HandleType) {
        return -1;
    }

    if (pFileMap->MMap.pBuffer != NULL) {
        munmap(pFileMap->MMap.pBuffer, pFileMap->MMap.Length);
    }

    if (pFileMap->FileHandle >= 0) {
        close(pFileMap->FileHandle);
    }

    if (pFileMap->pName != NULL) {
        os_free(pFileMap->pName);
    }

    if (pFileMap != NULL) {
        os_free(pFileMap);
    }

    return 0;
}