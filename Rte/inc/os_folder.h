
#ifndef _OS_FOLDER_H_
#define _OS_FOLDER_H_

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OS_FOLDER_ITEM_NAME_MAX        256
#define OS_FOLDER_PATH_MAX             512

typedef enum {
    FOLDER_ITEM_NONE       = 0x00,
    FOLDER_ITEM_DIR        = 0x01,
    FOLDER_ITEM_FILE       = 0x02,
    FOLDER_ITEM_DEV_BLK    = 0x03,
    FOLDER_ITEM_DEV_CHR    = 0x04,
    FOLDER_ITEM_UNKOWN     = 0x05,

} FolderItem_e;

typedef struct {
    FolderItem_e Type;
    size_t       Size;
    char         Name[OS_FOLDER_ITEM_NAME_MAX];

} FolderItem_t;

typedef void OsFolder_t;

extern OsFolder_t* os_folder_open(const char*   pPath);
extern ssize_t     os_folder_close(OsFolder_t*  pFolder);

extern ssize_t     os_folder_rewind(OsFolder_t* pFolder);
extern ssize_t     os_folder_read(OsFolder_t*   pFolder,
                                  FolderItem_t* pItem);

extern ssize_t os_folder_create(const char* pPath, uint16_t Mode);
extern ssize_t os_folder_delete(const char* pPath);

extern ssize_t os_folder_exist(const char* pPath);

#ifdef __cplusplus
}
#endif

#endif
