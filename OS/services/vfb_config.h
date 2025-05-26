#ifndef __VBF_CONFIG_H__
#define __VBF_CONFIG_H__
#include <stdint.h>

#define USE_DBC_LIB (1)
#define USE_VBF_LIB (1)
#define USE_TPCAN_LIB (1)

#define OS_WINDOWS (1)
#define OS_LINUX (2)
#define OS_FREERTOS (3)
#define OS_ESP_RTOS (4)
#define SYSTEM_MODE_GW_USE
/* HMI */
#ifdef SYSTEM_MODE_AMT630_USE  
#define OS_USED (OS_FREERTOS)
#include "trace.h"
#define VBF_I TRACE_INFO
#define VBF_E TRACE_ERROR
#define VBF_W TRACE_WARNING
#define VBF_D TRACE_DEBUG
#define MAX_THEAD_COUNT 32
#define TPCAN_TX 0x101
#define TPCAN_RX 0x102
#define TPCAN_RX_FUNC 0x7df
#define NORMAL_PAYLOAD_MAX_SIZE 4096
#define LARGE_PAYLOAD_MAX_SIZE  4096
#elif defined SYSTEM_MODE_GW_USE  // GW
#define OS_USED (OS_FREERTOS)
#define VBF_I printf
#define VBF_E printf
#define VBF_W printf
#define VBF_D printf
#define MAX_THEAD_COUNT 8
#define TPCAN_TX 0x102  /* HMI0 */
#define TPCAN_RX 0x101  /* HMI0 */
#define TPCAN_TX_1 0x112  /* HMI1 */
#define TPCAN_RX_1 0x111  /* HMI1 */
#define NORMAL_PAYLOAD_MAX_SIZE 256
#define LARGE_PAYLOAD_MAX_SIZE  4096
#else  // PC
#define OS_USED (OS_WINDOWS)
#define VBF_I printf
#define VBF_E printf
#define VBF_W printf
#define VBF_D printf
#define MAX_THEAD_COUNT 32
#endif

#if (OS_USED == OS_FREERTOS)
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#endif

#if (USE_DBC_LIB == 1)
#define SET_BLOCKE_TIMEOUT (300)
#define TP_DEFAULT_BLOCK_SIZE (8)
#define TP_DEFAULT_ST_MIN (0)
#define TP_MAX_WFT_NUMBER (1)
#define TP_DEFAULT_RESPONSE_TIMEOUT (300)
#define TP_BUFSIZE  (4096)
#endif  /* #if (USE_DBC_LIB==1) */

#if (USE_VBF_LIB == 1)

#define VFB_MAX_EVENT_NUM (256)

#endif
#endif  // __VBF_CONFIG_H__