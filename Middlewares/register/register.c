/**
 * @file shell.c
 * @author Letter (NevermindZZT@gmail.com)
 * @version 3.0.0
 * @date 2019-12-30
 *
 *
 */

#include "register.h"
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
/*
 * @brief  Initialize the system register map.
 *
 * This function initializes the system register map by setting the base address
 * and count of items in the map. It also populates the stage lists and counts
 * for each startup stage, and prints the details of each item in the map.
 */
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
    printf("    count %d\r\n", __SystemRegisterMap.count);
    /* Update Ebery Stage Counter */
    for (size_t i = 0; i < STARUP_STAGE_MAX; i++) {
        vListInitialise(&(__SystemRegisterMap.statge_list[i]));
        for (size_t j = 0; j < __SystemRegisterMap.count; j++) {
            if (SysStartupBase[j].stage == i) {
                ListItem_t *newItem = (ListItem_t *)pvPortMalloc(sizeof(ListItem_t));  // Allocate memory for the new item
                if (newItem == NULL) {
                    // Handle memory allocation failure
                    printf("Failed to allocate memory for ListItem_t\n");
                    continue;
                }
                listSET_LIST_ITEM_VALUE(newItem, (TickType_t)&SysStartupBase[j]);
                ListItem_t *first_item = listGET_HEAD_ENTRY(&(__SystemRegisterMap.statge_list[i]));

                while (first_item != listGET_END_MARKER(&(__SystemRegisterMap.statge_list[i]))) {
                    StarupItemStruct *itemStruct = (StarupItemStruct *)listGET_LIST_ITEM_VALUE(first_item);
                    if (SysStartupBase[j].priority < itemStruct->priority) {
                        vListInsertBefore((ListItem_t *const)first_item, newItem);  // Insert the new item into the list
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
void SystemRegisterRun(STARTUP_STAGE_ENUM stage) {
    List_t *stage_list = &(__SystemRegisterMap.statge_list[stage]);
    ListItem_t *item   = listGET_HEAD_ENTRY(stage_list);
    printf("SystemRegisterRun Stage %d\r\n", stage);
    for (size_t i = 0; i < __SystemRegisterMap.statge_count[stage]; i++) {
        StarupItemStruct *itemStruct = (StarupItemStruct *)listGET_LIST_ITEM_VALUE(item);
        printf("\tRun Item[%d] %s : %s\r\n", i, itemStruct->name, itemStruct->desc);
        if (itemStruct->func) {
            itemStruct->func();  // Call the function associated with the item
        } else {
            printf("\t\tItem[%d] %s : %s has no function\r\n", i, itemStruct->name, itemStruct->desc);
        }
        item = item->pxNext;
        //delay
        vTaskDelay(pdMS_TO_TICKS(10));  // Delay for 10 milliseconds
    }
}

#if (DEV_XXXXX_ENABLE == 1)

void a1_MCUInitStage10(void) {
    printf("a1_MCUInitStage10 run\r\n");
}
void a1_MCUInitStage11(void) {
    printf("a1_MCUInitStage11 run\r\n");
}
void a1_MCUInitStage12(void) {
    printf("a1_MCUInitStage12 run\r\n");
}
void a1_MCUInitStage13(void) {
    printf("a1_MCUInitStage13 run\r\n");
}
SYSTEM_REGISTER_INIT(MCUInitStage, 110, a1_MCUInitStage10, MCUInitStage10);

SYSTEM_REGISTER_INIT(MCUInitStage, 211, a1_MCUInitStage11, MCUInitStage11);
SYSTEM_REGISTER_INIT(MCUInitStage, 212, a1_MCUInitStage12, MCUInitStage12);
SYSTEM_REGISTER_INIT(MCUInitStage, 213, a1_MCUInitStage13, MCUInitStage13);

#endif