
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "os_heap.h"
#include "os_file.h"

typedef struct {
    const char* pHandleType;

    int         FileHandle;

    char*       pFilePath;
    size_t      Flags;

} InOsFile_t;

static const char HandleType[] = {"LINUX_FILE"};

OsFile_t* os_file_open(const OsFileCfg_t* pConfig)
{
    InOsFile_t* pInOsFile    = NULL;
    size_t      MallocLength = 0;

    ssize_t     RetVal       = 0;
    int         FileHandle   = -1;
    char*       pFilePath    = 0;
    size_t      PathLength   = 0;
    int         OpenFlags    = 0;
    int         ModeFlags    = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (pConfig == NULL) {
        return NULL;
    }

    /* FileHandle */
    if ((pConfig->Flags & OS_FILE_FLAG_CREAT) != 0) {
        OpenFlags = (OpenFlags | O_CREAT);
    }
    if ((pConfig->Flags & OS_FILE_FLAG_RDONLY) != 0) {
        OpenFlags = (OpenFlags | O_RDONLY);
    }
    if ((pConfig->Flags & OS_FILE_FLAG_WRONLY) != 0) {
        OpenFlags = (OpenFlags | O_WRONLY);
    }
    if ((pConfig->Flags & OS_FILE_FLAG_RDWR) != 0) {
        OpenFlags = (OpenFlags | O_RDWR);
    }
    if ((pConfig->Flags & OS_FILE_FLAG_APPEND) != 0) {
        OpenFlags = (OpenFlags | O_APPEND);
    }
    if ((pConfig->Flags & OS_FILE_FLAG_TRUNC) != 0) {
        OpenFlags = (OpenFlags | O_TRUNC);
    }
    if ((pConfig->Flags & OS_FILE_FLAG_EXCL) != 0) {
        OpenFlags = (OpenFlags | O_EXCL);
    }
    if ((pConfig->Flags & OS_FILE_FLAG_SYNC) != 0) {
        OpenFlags = (OpenFlags | O_SYNC);
    }
    if ((pConfig->Flags & OS_FILE_FLAG_NONBLOCK) != 0) {
        OpenFlags = (OpenFlags | O_NONBLOCK);
    }

    if ((pConfig->Flags & OS_FILE_FLAG_CREAT) != 0) {
        FileHandle = open(pConfig->pFilePath, OpenFlags, ModeFlags);
        if (FileHandle < 0) {
            goto __error;
        }
    } else {
        FileHandle = open(pConfig->pFilePath, OpenFlags);
        if (FileHandle < 0) {
            goto __error;
        }
    }

    /* pFilePath */
    RetVal = strlen(pConfig->pFilePath);
    if (RetVal < 0) {
        goto __error;
    }
    PathLength = RetVal;

    pFilePath = os_malloc(PathLength + 1);
    if (pFilePath == NULL) {
        goto __error;
    }
    memcpy(pFilePath, pConfig->pFilePath, PathLength);
    pFilePath[PathLength] = '\0';

    /* pInOsFile */
    MallocLength = sizeof(InOsFile_t);
    pInOsFile    = os_malloc(MallocLength);
    if (pInOsFile == NULL) {
        goto __error;
    }
    memset(pInOsFile, 0, MallocLength);

    pInOsFile->FileHandle  = FileHandle;
    pInOsFile->pFilePath   = pFilePath;
    pInOsFile->Flags       = pConfig->Flags;
    pInOsFile->pHandleType = HandleType;
    return pInOsFile;

__error:
    if (FileHandle >= 0) {
        close(FileHandle);
    }

    if (pFilePath != NULL) {
        os_free(pFilePath);
    }

    if (pInOsFile != NULL) {
        os_free(pInOsFile);
    }
    return NULL;
}

ssize_t os_file_close(OsFile_t* pFile)
{
    InOsFile_t* pInOsFile = pFile;

    if (pFile == NULL) {
        return 0;
    }

    if (pInOsFile->pHandleType != HandleType) {
        return -1;
    }

    if (pInOsFile->FileHandle >= 0) {
        close(pInOsFile->FileHandle);
        pInOsFile->FileHandle = -1;
    }

    if (pInOsFile->pFilePath != NULL) {
        os_free(pInOsFile->pFilePath);
        pInOsFile->pFilePath = NULL;
    }

    os_free(pInOsFile);
    return 0;
}

ssize_t os_file_read(OsFile_t* pFile,
                     size_t    Offset,
                     uint8_t*  pData,
                     size_t    DataLength)
{
    InOsFile_t* pInOsFile = pFile;

    if (pFile == NULL) {
        return -1;
    }

    if (pInOsFile->pHandleType != HandleType) {
        return -2;
    }

    return pread(pInOsFile->FileHandle, pData, DataLength, Offset);
}

ssize_t os_file_write(OsFile_t*      pFile,
                      size_t         Offset,
                      const uint8_t* pData,
                      size_t         DataLength)
{
    InOsFile_t* pInOsFile = pFile;

    if (pFile == NULL) {
        return -1;
    }

    if (pInOsFile->pHandleType != HandleType) {
        return -2;
    }

    return pwrite(pInOsFile->FileHandle, pData, DataLength, Offset);
}

ssize_t os_file_size(OsFile_t* pFile)
{
    struct stat FileStat;
    ssize_t     RetVal = 0;

    InOsFile_t* pInOsFile = pFile;

    if (pFile == NULL) {
        return -1;
    }

    if (pInOsFile->pHandleType != HandleType) {
        return -2;
    }

    RetVal = stat(pInOsFile->pFilePath, &FileStat);
    if (RetVal < 0) {
        return -3;
    }

    return FileStat.st_size;
}

ssize_t   os_file_clear(OsFile_t* pFile)
{
    InOsFile_t* pInOsFile = pFile;

    if (pFile == NULL) {
        return -1;
    }

    if (pInOsFile->pHandleType != HandleType) {
        return -2;
    }

    return ftruncate(pInOsFile->FileHandle, 0);
}

ssize_t os_file_exist(const char* pPath)
{
    if (pPath == NULL) {
        return -1;
    }
    return  access(pPath,  F_OK);
}

ssize_t os_file_remove(const char* pPath)
{
    if (pPath == NULL) {
        return -1;
    }
    return  unlink(pPath);
}

ssize_t os_file_rename(const char* pPathOld,
                       const char* pPathNew)
{
    if ((pPathOld == NULL) || (pPathNew == NULL)) {
        return -1;
    }
    return  rename(pPathOld,  pPathNew);
}
