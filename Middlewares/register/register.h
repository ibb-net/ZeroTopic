

#ifndef __SYSTEMREGISTER_MAP__
#define __SYSTEMREGISTER_MAP__
/* RTOS */
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "stdio.h"
typedef enum {
    PreStartupInitStage,
    ServerPerInitStage,
    MCUInitStage,
    BoardInitStage,
    ServerInitStage,
    AppInitStage,
    STARUP_STAGE_MAX,

} STARTUP_STAGE_ENUM;
typedef struct
{
    void *base; /**< 命令表基址 */
    uint16_t count;
    uint16_t statge_count[STARUP_STAGE_MAX];
    List_t statge_list[STARUP_STAGE_MAX];
} SysregisterStruct;

typedef struct {
    STARTUP_STAGE_ENUM stage;  // The stage of the startup process where this item is executed.
    uint8_t priority;          // The priority of this item within the same startup stage (0 is highest,lower values indicate higher priority).
    const char *name;          // A string representing the name of the startup item.
    const char *desc;          // A string providing a description of the startup item.
    void (*func)(void);        // A pointer to a function that will be executed during the startup process.
} StarupItemStruct;

#ifndef REGISTER_SECTION
#if defined(__CC_ARM) || defined(__CLANG_ARM)
#define REGISTER_SECTION(x) __attribute__((section(x), aligned(1)))
#elif defined(__IAR_SYSTEMS_ICC__)
#define REGISTER_SECTION(x) @x
#elif defined(__GNUC__)
#define REGISTER_SECTION(x) __attribute__((section(x), aligned(1)))
#else
#define REGISTER_SECTION(x)
#endif
#endif

#ifndef SYS_REGISTER_MAP_USED
#if defined(__CC_ARM) || defined(__CLANG_ARM)
#define SYS_REGISTER_MAP_USED __attribute__((used))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYS_REGISTER_MAP_USED __root
#elif defined(__GNUC__)
#define SYS_REGISTER_MAP_USED __attribute__((used))
#else
#define SYS_REGISTER_MAP_USED
#endif
#endif

#define SYSTEM_REGISTER_INIT(_stage, pri, _name, _desc)               \
    const char Register##_name[]       = #_name;                      \
    const char Register##_name##desc[] = #_desc;                      \
    SYS_REGISTER_MAP_USED const StarupItemStruct                      \
        SysRegisterMap##_name REGISTER_SECTION("SystemRegisterMap") = \
            {                                                         \
                .stage    = _stage,                                   \
                .priority = pri,                                      \
                .name     = Register##_name,                          \
                .desc     = Register##_name##desc,                    \
                .func     = _name};

void SystemRegisterInit(void);
// SystemRegisterRun
void SystemRegisterRun(STARTUP_STAGE_ENUM stage);
#endif
