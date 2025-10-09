
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "os_heap.h"
#include "os_file.h"

typedef struct {
    const char* pHandleType;

    FILE*       pFileHandler;

    char*       pFilePath;
    size_t      Flags;

} InOsFile_t;

static const char HandleType[] = {"OS_FILE"};

OsFile_t* os_file_open(const OsFileCfg_t* pConfig)
{
    InOsFile_t* pInOsFile     = NULL;
    size_t      MallocLength  = 0;

    ssize_t     RetVal        = 0;
    FILE*       pFileHandler  = NULL;
    char*       pFilePath     = 0;
    size_t      PathLength    = 0;
    const char* pMode         = NULL;

    if (pConfig == NULL) {
        return NULL;
    }

    /* pFileHandler */
    if ((pConfig->Flags & OS_FILE_FLAG_CREAT) != 0) {
        pMode = "wb+";
    } else {
        pMode = "rb+";
    }

    if ((pConfig->Flags & OS_FILE_FLAG_APPEND) != 0) {
        pMode = "ab+";
    }

    pFileHandler = fopen(pConfig->pFilePath, pMode);
    if (pFileHandler == NULL) {
        goto __error;
    }

    RetVal = strlen(pConfig->pFilePath);
    if (RetVal < 0) {
        goto __error;
    }
    PathLength = RetVal;

    /* pFilePath */
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

    pInOsFile->pFileHandler = pFileHandler;
    pInOsFile->pFilePath   = pFilePath;
    pInOsFile->Flags       = pConfig->Flags;
    pInOsFile->pHandleType = HandleType;
    return pInOsFile;

__error:
    if (pFileHandler != NULL) {
        fclose(pFileHandler);
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


    if (pInOsFile->pFileHandler  != NULL) {
        fclose(pInOsFile->pFileHandler);
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
    ssize_t      RetVal    = 0;
    InOsFile_t*  pInOsFile = pFile;

    if (pFile == NULL) {
        return -1;
    }

    if (pInOsFile->pHandleType != HandleType) {
        return -2;
    }

    RetVal = fseek(pInOsFile->pFileHandler, Offset, SEEK_SET);
    if (RetVal < 0) {
        return -3;
    }

    return fread(pData, 1, DataLength, pInOsFile->pFileHandler);
}

ssize_t os_file_write(OsFile_t* pFile,
                      size_t    Offset,
                      const uint8_t*  pData,
                      size_t    DataLength)
{
    ssize_t      RetVal    = 0;
    InOsFile_t*  pInOsFile = pFile;

    if (pFile == NULL) {
        return -1;
    }

    if (pInOsFile->pHandleType != HandleType) {
        return -2;
    }

    RetVal = fseek(pInOsFile->pFileHandler, Offset, SEEK_SET);
    if (RetVal < 0) {
        return -3;
    }

    RetVal = fwrite(pData, 1, DataLength, pInOsFile->pFileHandler);
    if (RetVal < 0) {
        return -4;
    }

    if ((pInOsFile->Flags | OS_FILE_FLAG_SYNC) != 0) {
        RetVal = fflush(pInOsFile->pFileHandler);
        if (RetVal < 0) {
            return -4;
        }
    }

    return DataLength;
}

ssize_t os_file_size(OsFile_t* pFile)
{
    ssize_t      RetVal    = 0;
    InOsFile_t*  pInOsFile = pFile;

    if (pFile == NULL) {
        return -1;
    }

    if (pInOsFile->pHandleType != HandleType) {
        return -2;
    }

    RetVal = fseek(pInOsFile->pFileHandler, 0, SEEK_END);
    if (RetVal < 0) {
        return -3;
    }

    return ftell(pInOsFile->pFileHandler);
}

ssize_t os_file_exist(const char* pPath)
{
    if (pPath == NULL) {
        return -1;
    }
    return  access(pPath,  F_OK);
}
