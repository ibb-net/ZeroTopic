
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#ifndef _OS_TIME_H_
#define _OS_TIME_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OS_TIME_ZONE_WEST_11 = 0x0B,
    OS_TIME_ZONE_WEST_10 = 0x0A,
    OS_TIME_ZONE_WEST_9  = 0x09,
    OS_TIME_ZONE_WEST_8  = 0x08,
    OS_TIME_ZONE_WEST_7  = 0x07,
    OS_TIME_ZONE_WEST_6  = 0x06,
    OS_TIME_ZONE_WEST_5  = 0x05,
    OS_TIME_ZONE_WEST_4  = 0x04,
    OS_TIME_ZONE_WEST_3  = 0x03,
    OS_TIME_ZONE_WEST_2  = 0x02,
    OS_TIME_ZONE_WEST_1  = 0x01,
    OS_TIME_ZONE_UTC     = 0,
    OS_TIME_ZONE_EAST_1  = 0x11,
    OS_TIME_ZONE_EAST_2  = 0x12,
    OS_TIME_ZONE_EAST_3  = 0x13,
    OS_TIME_ZONE_EAST_4  = 0x14,
    OS_TIME_ZONE_EAST_5  = 0x15,
    OS_TIME_ZONE_EAST_6  = 0x16,
    OS_TIME_ZONE_EAST_7  = 0x17,
    OS_TIME_ZONE_EAST_8  = 0x18,
    OS_TIME_ZONE_EAST_9  = 0x19,
    OS_TIME_ZONE_EAST_A  = 0x1A,
    OS_TIME_ZONE_EAST_B  = 0x1B,

} TimeZone_e;

typedef struct {
    uint16_t Year;
    uint8_t  Month;
    uint8_t  Day;
    uint8_t  Hour;
    uint8_t  Minute;
    uint8_t  Second;

} DateTime_t;

extern uint64_t os_monotonic_time_get_millisecond(void);
extern uint64_t os_monotonic_time_get_microsecond(void);

extern uint64_t os_real_timestamp_adj_millisecond1(uint64_t RealTimeStamp,
                                                   uint64_t AdjustMin);
extern uint64_t os_real_timestamp_adj_millisecond2(int64_t  Adjust);

extern uint64_t os_real_timestamp_get_second(void);
extern uint64_t os_real_timestamp_get_millisecond(void);
extern uint64_t os_real_timestamp_get_microsecond(void);

extern ssize_t os_timestamp_second_to_datetime(uint64_t    TimeStamp,
                                               DateTime_t* pDateTime);

extern ssize_t os_datetime_get(TimeZone_e  TimeZone,
                               DateTime_t* pDateTime);

#ifdef __cplusplus
}
#endif

#endif
