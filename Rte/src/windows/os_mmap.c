

#include <windows.h>

#include "os_heap.h"
#include "os_mmap.h"

#define container_of(ptr, type, member) ({               \
    const typeof( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type,member));    \
})

typedef struct {
    const char* pHandleType;
    HANDLE FileHandle;
    OsMMap_t MMap;
} InMMap_t;

static const char HandleType[] = {"WIN_MMAP"};

OsMMap_t* os_mmap_create(const char* pName, size_t Length)
{
    int    RetVal = 0;
    HANDLE FileHandle = NULL;
    void*  pBuffer = NULL;
    InMMap_t* pFileMap = NULL;

    FileHandle = CreateFileMapping(
                     INVALID_HANDLE_VALUE,
                     NULL,
                     PAGE_READWRITE,
                     0,
                     Length,
                     pName);
    if (FileHandle == NULL) {
        goto __error;
    }

    pBuffer = (LPTSTR) MapViewOfFile(FileHandle,   // handle to map object
                                     FILE_MAP_ALL_ACCESS, // read/write permission
                                     0,
                                     0,
                                     Length);
    if (FileHandle == NULL) {
        goto __error;
    }

    pFileMap = os_malloc(sizeof(InMMap_t));
    if (pFileMap == NULL) {
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

    if (pBuffer != NULL) {
        UnmapViewOfFile(pBuffer);
    }

    if (FileHandle != 0) {
        CloseHandle(FileHandle);
    }

    return NULL;

}

ssize_t os_mmap_destroy(OsMMap_t* pMMap)
{
    return os_mmap_close(pMMap);
}

OsMMap_t* os_mmap_open(const char* pName, size_t Length)
{
    int    RetVal = 0;
    HANDLE FileHandle = NULL;
    void*  pBuffer = NULL;
    InMMap_t* pFileMap = NULL;

    FileHandle = OpenFileMapping(
                     FILE_MAP_ALL_ACCESS,
                     FALSE,
                     pName);
    if (FileHandle == NULL) {
        goto __error;
    }

    pBuffer = (LPTSTR) MapViewOfFile(FileHandle,   // handle to map object
                                     FILE_MAP_ALL_ACCESS, // read/write permission
                                     0,
                                     0,
                                     Length);
    if (FileHandle == NULL) {
        goto __error;
    }

    pFileMap = os_malloc(sizeof(InMMap_t));
    if (pFileMap == NULL) {
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

    if (pBuffer != NULL) {
        UnmapViewOfFile(pBuffer);
    }

    if (FileHandle != 0) {
        CloseHandle(FileHandle);
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
        UnmapViewOfFile(pFileMap->MMap.pBuffer);
    }

    if (pFileMap->FileHandle >= 0) {
        CloseHandle(pFileMap->FileHandle);
    }

    if (pFileMap != NULL) {
        os_free(pFileMap);
    }

    return 0;
}