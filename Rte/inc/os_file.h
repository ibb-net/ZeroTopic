
#ifndef _OS_FILE_H_
#define _OS_FILE_H_

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OS_FILE_FLAG_CREAT    = 0x0001,
    OS_FILE_FLAG_RDONLY   = 0x0002,
    OS_FILE_FLAG_WRONLY   = 0x0004,
    OS_FILE_FLAG_RDWR     = 0x0008,
    OS_FILE_FLAG_APPEND   = 0x0010,
    OS_FILE_FLAG_TRUNC    = 0x0020,
    OS_FILE_FLAG_EXCL     = 0x0040,
    OS_FILE_FLAG_SYNC     = 0x0080,
    OS_FILE_FLAG_NONBLOCK = 0x0100,

} OsFileFlag_e;

typedef struct {
    const char* pFilePath;
    size_t      Flags;

} OsFileCfg_t;

typedef void OsFile_t;

extern OsFile_t* os_file_open(const OsFileCfg_t* pConfig);
extern ssize_t   os_file_close(OsFile_t* pFile);

extern ssize_t   os_file_read(OsFile_t* pFile,
                              size_t    Offset,
                              uint8_t*  pData,
                              size_t    DataLength);

extern ssize_t   os_file_write(OsFile_t* pFile,
                               size_t    Offset,
                               const uint8_t*  pData,
                               size_t    DataLength);
extern ssize_t   os_file_size(OsFile_t* pFile);
extern ssize_t   os_file_clear(OsFile_t* pFile);

extern ssize_t   os_file_exist(const char* pPath);
extern ssize_t   os_file_remove(const char* pPath);

extern ssize_t   os_file_rename(const char* pPathOld,
                                const char* pPathNew);

#ifdef __cplusplus
}
#endif

#endif
