#ifndef __REGISTER_SDK_CFG_H__
#define __REGISTER_SDK_CFG_H__

typedef enum {
    MCUPreDEBUG_UART_INIT = 0,  // Debug UART initialization
    MCUPreDebugRegisterPriority,
    ServerPreShellRegisterPriority,

    MCUPreDemoRegisterPriority,

    MCU_PRE_INIT_STAGE_PROCESS_MAX,  // Maximum value for MCUPreInitStage

} MCUPreInitStageProcessEnum;

/* ServerPerInitStage Startup */
typedef enum {
    ServerPreDebugRegisterPriority,

    ServerPreDemoRegisterPriority,
    SERVER_PER_INIT_STAGE_PROCESS_MAX,  // Maximum value for ServerPerInitStage

} ServerPerInitStageProcessEnum;

/* MCUInitStage Startup */
typedef enum {
    MCU_INIT_STAGE_PROCESS_MAX,  // Maximum value for MCUInitStage
} MCUInitStageProcessEnum;

/* BoardInitStage Startup */
typedef enum {
    BoardTimerEncoderRegisterPrority,
    BoardKeyRegisterPriority,
    BoardIORegisterPriority,
    BOARD_INIT_STAGE_PROCESS_MAX,  // Maximum value for BoardInitStage
} BoardInitStageProcessEnum;

/* ServerInitStage Startup */
typedef enum {
    ServerPreKeyRegisterPriority, 
    SERVER_INIT_STAGE_PROCESS_MAX,  // Maximum value for ServerInitStage
} ServerInitStageProcessEnum;

/* AppInitStage Startup */
typedef enum {
    AppLEDRegisterPriority,
    AppDemoRegisterPriority,
    AppTimerEncoderRegisterPriority,
    AppIORegisterPriority,
    APP_INIT_STAGE_PROCESS_MAX,  // Maximum value for AppInitStage
} AppInitStageProcessEnum;

#endif  // __REGISTER_SDK_CFG_H__