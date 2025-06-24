#ifndef __DEV_FLASH_H__
#define __DEV_FLASH_H__
#include "dev_basic.h"
#define FLASH_BASE_ADDR    ((uint32_t)0x08000000U)
#define FLASH_MAX_SIZE     ((uint32_t)(1024 * 1024 * 3))
#define FLASH_SECTOR_SIZE  ((uint32_t)0x00001000U)               // 4K
#define FLASH_SECTOR_COUNT (FLASH_MAX_SIZE / FLASH_SECTOR_SIZE)  // 768 sectors
#define FLASH_END_ADDR     ((uint32_t)(0x08000000U + 0x300000U))

void dev_flash_init(void);
void dev_flash_write(uint32_t address, uint32_t *data, uint32_t size);
void dev_flash_read(uint32_t address, uint32_t *data, uint32_t size);

#endif  // __DEV_FLASH_H__