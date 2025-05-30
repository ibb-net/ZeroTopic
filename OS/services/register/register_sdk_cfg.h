#ifndef __REGISTER_SDK_CFG_H__
#define __REGISTER_SDK_CFG_H__

/*
    MCUPreInitStage,
    ServerPerInitStage,
    MCUInitStage,
    BoardInitStage,
    ServerInitStage,
    AppInitStage,
    STARUP_STAGE_PROCESS_MAX,
*/
/* MCUPreInitStage Startup  */
typedef enum{
    MCUPreDEBUG_UART_INIT = 0,     // Debug UART initialization
    MCUPre_TIMER_ENCODER_INIT,      // Timer encoder initialization
    MCU_PRE_INIT_STAGE_PROCESS_MAX,  // Maximum value for MCUPreInitStage

} MCUPreInitStageProcessEnum;

/* ServerPerInitStage Startup */
typedef enum{
    SERVER_PER_INIT_STAGE_PROCESS_MAX , // Maximum value for ServerPerInitStage
} ServerPerInitStageProcessEnum;

/* MCUInitStage Startup */
typedef enum{
    MCU_INIT_STAGE_PROCESS_MAX , // Maximum value for MCUInitStage
} MCUInitStageProcessEnum;

/* BoardInitStage Startup */
typedef enum{
    BOARD_INIT_STAGE_PROCESS_MAX , // Maximum value for BoardInitStage
} BoardInitStageProcessEnum;

/* ServerInitStage Startup */
typedef enum{
    SERVER_INIT_STAGE_PROCESS_MAX , // Maximum value for ServerInitStage
} ServerInitStageProcessEnum;

/* AppInitStage Startup */
typedef enum{
    APP_INIT_STAGE_PROCESS_MAX , // Maximum value for AppInitStage
} AppInitStageProcessEnum;





#endif  // __REGISTER_SDK_CFG_H__