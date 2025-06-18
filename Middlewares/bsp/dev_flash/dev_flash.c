#include "dev_flash.h"

#include <stdio.h>

#include "dev_basic.h"
#include "elog.h"
#include "shell.h"
#define TAG "dev_flash"

/*!
    \brief      clear all FMC flag status
    \param[in]  none
    \param[out] none
    \retval     none
*/
void fmc_all_flags_clear(void) {
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_RPERR);
    fmc_flag_clear(FMC_FLAG_RSERR);
    fmc_flag_clear(FMC_FLAG_ECCCOR);
    fmc_flag_clear(FMC_FLAG_ECCDET);
    fmc_flag_clear(FMC_FLAG_OBMERR);
}

void dev_flash_init(void) {
}
void dev_flash_write(uint32_t address, uint32_t *data, uint32_t size) {
    if (address < FLASH_BASE_ADDR || size > FLASH_MAX_SIZE) {
        elog_e(TAG, "Address out of range: 0x%08X", address);
        return;
    }
    uint32_t *ptr = (uint32_t *)data;

    uint32_t start = address;
    uint32_t end   = address + size;
    fmc_unlock();
    fmc_all_flags_clear();
    fmc_sector_erase(address);
    fmc_all_flags_clear();

    fmc_lock();

    fmc_unlock();
    /* program flash */
    while (start < end) {
        fmc_word_program(start, *ptr++);
        start += 4U;
        fmc_all_flags_clear();
    }
    fmc_all_flags_clear();
    fmc_lock();
    SCB_CleanInvalidateDCache();
}

void dev_flash_read(uint32_t address, uint32_t *data, uint32_t size) {
    uint32_t i;
    uint32_t *ptr = (uint32_t *)address;

    for (i = 0; i < size / 4; i++) {
        data[i] = ptr[i];
    }
}

TypedefLCFG *dev_read_cfg(void) {
    return (TypedefLCFG *)CFG_ADDR;
}
void dev_write_cfg(TypedefLCFG *cfg) {
    dev_flash_write(CFG_ADDR, (uint32_t *)cfg, sizeof(TypedefLCFG));
}
////////////////////////////////////////////////////////////////////////////////////////////////////
