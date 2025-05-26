/**
 * @file shell.c
 * @author Letter (NevermindZZT@gmail.com)
 * @version 3.0.0
 * @date 2019-12-30
 *
 * @copyright (c) 2020 Letter
 *
 */

#include "SystemRegister/SystemRegister.h"

#include "stdio.h"
#include "string.h"

#if defined(__CC_ARM) || (defined(__ARMCC_VERSION) && __ARMCC_VERSION >= 6000000)
extern const unsigned int SystemRegisterMap$$Base;
extern const unsigned int SystemRegisterMap$$Limit;
#elif defined(__ICCARM__) || defined(__ICCRX__)
#pragma section = "SystemRegisterMap"
#elif defined(__GNUC__)
extern const unsigned int _SystemRegisterMapStart;
extern const unsigned int _SystemRegisterMapEnd;
#endif

SysregisterStruct __SystemRegisterMap;

/**
 * @brief  Get the count of items in a specific startup stage.
 *
 * This function iterates through the system register map and counts the number
 * of items that belong to the specified startup stage. The startup stage is
 * determined by the `stage` field in each `StarupItemStruct`.
 *
 * @param  stage The startup stage to count items for (of type `STARTUP_STAGE_ENUM`).
 * @return The number of items in the specified startup stage.
 */
int __sGetStageCounter(STARTUP_STAGE_ENUM stage) {
    int count = 0;  // Initialize the counter to 0

    // Cast the base address of the system register map to a pointer to StarupItemStruct
    StarupItemStruct *SysStartupBase = (StarupItemStruct *)__SystemRegisterMap.base;

    // Iterate through all items in the system register map
    for (size_t i = 0; i < __SystemRegisterMap.count; i++) {
        // Check if the item's stage matches the specified stage
        if (SysStartupBase[i].stage == stage) {
            count++;  // Increment the counter if the stage matches
        }
    }

    // Return the total count of items in the specified stage
    return count;
}

void SystemRegisterInit(void) {
#if defined(__CC_ARM) || (defined(__ARMCC_VERSION) && __ARMCC_VERSION >= 6000000)
    __SystemRegisterMap.base  = (SysregisterStruct *)(&SystemRegisterMap$$Base);
    __SystemRegisterMap.count = ((size_t)(&SystemRegisterMap$$Limit) - (size_t)(&SystemRegisterMap$$Base)) / sizeof(StarupItemStruct);

#elif defined(__ICCARM__) || defined(__ICCRX__)
    __SystemRegisterMap.base  = (SysregisterStruct *)(__section_begin("SystemRegisterMap"));
    __SystemRegisterMap.count = ((size_t)(__section_end("SystemRegisterMap")) - (size_t)(__section_begin("SystemRegisterMap"))) / sizeof(StarupItemStruct);
#elif defined(__GNUC__)
    __SystemRegisterMap.base  = (SysregisterStruct *)(&_SystemRegisterMapStart);
    __SystemRegisterMap.count = ((size_t)(&_SystemRegisterMapEnd) - (size_t)(&_SystemRegisterMapStart)) / sizeof(StarupItemStruct);
#else
#error not supported compiler, please use command table mode
#endif
    StarupItemStruct *SysStartupBase = (StarupItemStruct *)__SystemRegisterMap.base;
    printf("SystemRegisterMap :\r\n");
    printf("    Base %p\r\n", __SystemRegisterMap.base);
    printf("    count %d\r\b", __SystemRegisterMap.count);
    /* Update Ebery Stage Counter */
    for (size_t i = 0; i < STARUP_STAGE_MAX; i++) {
        vListInitialise(&(__SystemRegisterMap.statge_list[i]));
        for (size_t j = 0; j < __SystemRegisterMap.count; j++) {
            if (SysStartupBase[j].stage == i) {
                ListItem_t *item    = listGET_HEAD_ENTRY(&(__SystemRegisterMap.statge_list[i]));
                ListItem_t *newItem = (ListItem_t *)pvPortMalloc(sizeof(ListItem_t));  // Allocate memory for the new item
                if (newItem == NULL) {
                    // Handle memory allocation failure
                    printf("Failed to allocate memory for ListItem_t\n");
                    continue;
                }

                // Initialize the new item and set its value to the current startup item
                vListInitialiseItem(newItem);
                listSET_LIST_ITEM_VALUE(newItem, (TickType_t)&SysStartupBase[j]);

                // Traverse the list to find the correct position based on priority
                /* if (listLIST_IS_EMPTY(&(__SystemRegisterMap.statge_list[i]))) {
                    vListInsert(&(__SystemRegisterMap.statge_list[i]), newItem);  // Insert the new item into the list
                } else  */{
                    ListItem_t *first_item = listGET_HEAD_ENTRY(&(__SystemRegisterMap.statge_list[i]));

                    while (first_item != listGET_END_MARKER(&(__SystemRegisterMap.statge_list[i]))) {
                        StarupItemStruct *itemStruct = (StarupItemStruct *)listGET_LIST_ITEM_VALUE(first_item);
                        if (SysStartupBase[j].priority < itemStruct->priority) {
                            vListInsert(&(__SystemRegisterMap.statge_list[i]), newItem);  // Insert the new item into the list
                            break;
                        }
                        first_item = first_item->pxNext;
                    }

                    // If the new item was not inserted, append it to the end of the list
                    if (first_item == listGET_END_MARKER(&(__SystemRegisterMap.statge_list[i]))) {
                        vListInsertEnd(&(__SystemRegisterMap.statge_list[i]), newItem);
                    }
                }
            }
        }

        // Update the count of items in the current stage
        __SystemRegisterMap.statge_count[i] = listCURRENT_LIST_LENGTH(&(__SystemRegisterMap.statge_list[i]));
        printf("Stage[%d] count %d\r\n", i, __SystemRegisterMap.statge_count[i]);

        // Print the items in the current stage
        ListItem_t *item = listGET_HEAD_ENTRY(&(__SystemRegisterMap.statge_list[i]));
        for (size_t j = 0; j < __SystemRegisterMap.statge_count[i]; j++) {
            StarupItemStruct *itemStruct = (StarupItemStruct *)listGET_LIST_ITEM_VALUE(item);
            printf("\tItem[%d] %s : %s\r\n", j, itemStruct->name, itemStruct->desc);
            item = item->pxNext;
        }
    }
}
#if (DEV_XXXXX_ENABLE==1)
SYSTEM_REGISTER_INIT(MCUInitStage, 110, a1_MCUInitStage10, MCUInitStage10);
#endif
SYSTEM_REGISTER_INIT(MCUInitStage, 211, a1_MCUInitStage11, MCUInitStage11);
SYSTEM_REGISTER_INIT(MCUInitStage, 212, a1_MCUInitStage12, MCUInitStage12);
SYSTEM_REGISTER_INIT(MCUInitStage, 213, a1_MCUInitStage13, MCUInitStage13);

SYSTEM_REGISTER_INIT(MCUPreInitStage, 0, MCUPreInitStage0, SystemRegisterInit);
SYSTEM_REGISTER_INIT(MCUPreInitStage, 1, MCUPreInitStage1, SystemRegisterInit);
SYSTEM_REGISTER_INIT(MCUPreInitStage, 2, MCUPreInitStage2, SystemRegisterInit);
SYSTEM_REGISTER_INIT(MCUPreInitStage, 3, MCUPreInitStage3, SystemRegisterInit);
SYSTEM_REGISTER_INIT(ServerPerInitStage, 0, ServerPerInitStage0, ServerPerInitStage);
SYSTEM_REGISTER_INIT(ServerPerInitStage, 0, ServerPerInitStage0_0, ServerPerInitStage);
SYSTEM_REGISTER_INIT(ServerPerInitStage, 1, ServerPerInitStage1, ServerPerInitStage);
SYSTEM_REGISTER_INIT(ServerPerInitStage, 2, ServerPerInitStage2, ServerPerInitStage);
SYSTEM_REGISTER_INIT(ServerPerInitStage, 0, ServerPerInitStage0_0_0, ServerPerInitStage);
SYSTEM_REGISTER_INIT(ServerPerInitStage, 3, ServerPerInitStage3, ServerPerInitStage);
SYSTEM_REGISTER_INIT(ServerPerInitStage, 4, ServerPerInitStage4, ServerPerInitStage);
SYSTEM_REGISTER_INIT(ServerPerInitStage, 5, ServerPerInitStage5, ServerPerInitStage);
SYSTEM_REGISTER_INIT(ServerPerInitStage, 6, ServerPerInitStage6, ServerPerInitStage);

SYSTEM_REGISTER_INIT(MCUInitStage, 4, MCUInitStage4, MCUInitStage4);
SYSTEM_REGISTER_INIT(MCUInitStage, 5, MCUInitStage5, MCUInitStage5);
SYSTEM_REGISTER_INIT(MCUInitStage, 6, MCUInitStage6, MCUInitStage6);
SYSTEM_REGISTER_INIT(MCUInitStage, 7, MCUInitStage7, MCUInitStage7);

SYSTEM_REGISTER_INIT(MCUInitStage, 0, MCUInitStage0, MCUInitStage0);
SYSTEM_REGISTER_INIT(MCUInitStage, 1, MCUInitStage1, MCUInitStage1);
SYSTEM_REGISTER_INIT(MCUInitStage, 2, MCUInitStage2, MCUInitStage2);
SYSTEM_REGISTER_INIT(MCUInitStage, 3, MCUInitStage3, MCUInitStage3);

SYSTEM_REGISTER_INIT(MCUInitStage, 8, MCUInitStage8, MCUInitStage8);
SYSTEM_REGISTER_INIT(MCUInitStage, 9, MCUInitStage9, MCUInitStage9);
SYSTEM_REGISTER_INIT(MCUInitStage, 10, MCUInitStage10, MCUInitStage10);
SYSTEM_REGISTER_INIT(MCUInitStage, 3, a1_MCUInitStage3, MCUInitStage3);
SYSTEM_REGISTER_INIT(MCUInitStage, 4, a1_MCUInitStage4, MCUInitStage4);
SYSTEM_REGISTER_INIT(MCUInitStage, 5, a1_MCUInitStage5, MCUInitStage5);
SYSTEM_REGISTER_INIT(MCUInitStage, 6, a1_MCUInitStage6, MCUInitStage6);
SYSTEM_REGISTER_INIT(MCUInitStage, 0, a1_MCUInitStage0, MCUInitStage0);
SYSTEM_REGISTER_INIT(MCUInitStage, 1, a1_MCUInitStage1, MCUInitStage1);
SYSTEM_REGISTER_INIT(MCUInitStage, 2, a1_MCUInitStage2, MCUInitStage2);

SYSTEM_REGISTER_INIT(MCUInitStage, 7, a1_MCUInitStage7, MCUInitStage7);
SYSTEM_REGISTER_INIT(MCUInitStage, 8, a1_MCUInitStage8, MCUInitStage8);
SYSTEM_REGISTER_INIT(MCUInitStage, 9, a1_MCUInitStage9, MCUInitStage9);

SYSTEM_REGISTER_INIT(BoardInitStage, 0, BoardInitStage0, dec.......);
SYSTEM_REGISTER_INIT(BoardInitStage, 1, BoardInitStage1, dec.......);
SYSTEM_REGISTER_INIT(BoardInitStage, 2, BoardInitStage2, dec.......);
SYSTEM_REGISTER_INIT(BoardInitStage, 3, BoardInitStage3, dec.......);

SYSTEM_REGISTER_INIT(ServerInitStage, 1, ServerInitStage1, dec.......);
SYSTEM_REGISTER_INIT(ServerInitStage, 2, ServerInitStage2, dec.......);
SYSTEM_REGISTER_INIT(ServerInitStage, 0, ServerInitStage0, dec.......);

SYSTEM_REGISTER_INIT(AppInitStage, 0, AppInitStage0, dec.......);
SYSTEM_REGISTER_INIT(AppInitStage, 1, AppInitStage01, dec.......);
SYSTEM_REGISTER_INIT(AppInitStage, 2, AppInitStage02, dec.......);
