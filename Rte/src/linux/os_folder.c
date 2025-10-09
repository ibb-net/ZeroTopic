
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "os_heap.h"
#include "os_folder.h"

typedef struct {
    const char* pHandleType;

    char*       pFolderPath;
    DIR*        pDir;

    char        FilePath[OS_FOLDER_PATH_MAX];

} InOsFolder_t;

static const char HandleType[] = {"LINUX_FOLDER"};

OsFolder_t* os_folder_open(const char*  pPath)
{
    InOsFolder_t* pInOsFolder  = NULL;
    size_t        MallocLength = 0;

    ssize_t       RetVal     = 0;
    size_t        PathLength = 0;

    if (pPath == NULL) {
        goto __error;
    }

    RetVal = strlen(pPath);
    if (RetVal < 0) {
        goto __error;
    }
    PathLength = RetVal;

    /* pInOsFolder */
    MallocLength = sizeof(InOsFolder_t);
    pInOsFolder  = os_malloc(MallocLength);
    if (pInOsFolder == NULL) {
        goto __error;
    }
    memset(pInOsFolder, 0, MallocLength);

    pInOsFolder->pFolderPath = os_malloc(PathLength + 1);
    if (pInOsFolder->pFolderPath == NULL) {
        goto __error;
    }
    memcpy(pInOsFolder->pFolderPath, pPath, PathLength);
    pInOsFolder->pFolderPath[PathLength] = '\0';

    pInOsFolder->pDir = opendir(pPath);
    if (pInOsFolder->pDir == NULL) {
        goto __error;
    }

    pInOsFolder->pHandleType = HandleType;
    return pInOsFolder;

__error:
    if (pInOsFolder != NULL) {
        if (pInOsFolder->pDir == NULL) {
            closedir(pInOsFolder->pDir);
            pInOsFolder->pDir = NULL;
        }
        if (pInOsFolder->pFolderPath == NULL) {
            os_free(pInOsFolder->pFolderPath);
            pInOsFolder->pFolderPath = NULL;
        }

        os_free(pInOsFolder);
    }

    return NULL;
}

ssize_t os_folder_close(OsFolder_t* pFolder)
{
    InOsFolder_t* pInOsFolder = pFolder;

    if (pInOsFolder == NULL) {
        return 0;
    }

    if (pInOsFolder->pHandleType != HandleType) {
        return -1;
    }

    if (pInOsFolder->pDir == NULL) {
        closedir(pInOsFolder->pDir);
        pInOsFolder->pDir = NULL;
    }

    if (pInOsFolder->pFolderPath == NULL) {
        os_free(pInOsFolder->pFolderPath);
        pInOsFolder->pFolderPath = NULL;
    }

    os_free(pInOsFolder);
    return 0;
}

ssize_t os_folder_rewind(OsFolder_t* pFolder)
{
    InOsFolder_t* pInOsFolder = pFolder;

    if (pInOsFolder == NULL) {
        return -1;
    }

    if (pInOsFolder->pHandleType != HandleType) {
        return -2;
    }

    rewinddir(pInOsFolder->pDir);
    return 0;
}

ssize_t os_folder_read(OsFolder_t*    pFolder,
                       FolderItem_t*  pItem)
{

    InOsFolder_t*  pInOsFolder = pFolder;
    struct stat    FileStat;

    size_t  i        = 0;
    ssize_t RetVal   = 0;

    struct dirent* pDirent = NULL;

    if ((pInOsFolder == NULL) || (pItem == NULL)) {
        return -1;
    }

    if (pInOsFolder->pHandleType != HandleType) {
        return -2;
    }

    pDirent = readdir(pInOsFolder->pDir);
    if (pDirent == NULL) {
        pItem->Type    = FOLDER_ITEM_NONE;
        pItem->Name[0] = '\0';
        return 0;
    }

    /* Type */
    if (pDirent->d_type == DT_BLK) {
        pItem->Type = FOLDER_ITEM_DEV_BLK;
    } else if (pDirent->d_type == DT_CHR) {
        pItem->Type = FOLDER_ITEM_DEV_CHR;
    } else if (pDirent->d_type == DT_DIR) {
        pItem->Type = FOLDER_ITEM_DIR;
    } else if (pDirent->d_type == DT_REG) {
        pItem->Type = FOLDER_ITEM_FILE;
    } else {
        pItem->Type = FOLDER_ITEM_UNKOWN;
    }

    /* Name */
    for (i = 0; i < OS_FOLDER_ITEM_NAME_MAX; i++) {
        pItem->Name[i] = pDirent->d_name[i];

        if (pDirent->d_name[i] == '\0') {
            break;
        }
    }

    if (i >= OS_FOLDER_ITEM_NAME_MAX) {
        return -3;
    }

    /* Size */
    RetVal = snprintf(pInOsFolder->FilePath, OS_FOLDER_PATH_MAX,
                      "%s/%s",
                      pInOsFolder->pFolderPath,
                      pItem->Name);
    if ((RetVal < 0) || (RetVal >= OS_FOLDER_PATH_MAX)) {
        return -4;
    }
    pInOsFolder->FilePath[RetVal] = '\0';

    RetVal = stat(pInOsFolder->FilePath, &FileStat);
    if (RetVal < 0) {
        return -5;
    }

    pItem->Size = FileStat.st_size;

    return 0;
}

ssize_t os_folder_create(const char* pPath, uint16_t Mode)
{
    if (pPath == NULL) {
        return -1;
    }

    return mkdir(pPath, Mode);
}

ssize_t os_folder_delete(const char* pPath)
{
    if (pPath == NULL) {
        return -1;
    }

    return remove(pPath);
}

ssize_t os_folder_exist(const char* pPath)
{
    if (pPath == NULL) {
        return -1;
    }
    return  access(pPath,  F_OK);
}
