
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "os_interrupt.h"
#include "os_timestamp.h"

static uint64_t MonotonicTimeStamp = 0;
static uint64_t AdjustMonotonic    = 0;
static uint64_t AdjustRealTime     = 0;

static const uint32_t Month_Days_Accu_C[13] =
{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

static const uint32_t Month_Days_Accu_L[13] =
{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};

void vApplicationTickHook(void)
{
    MonotonicTimeStamp = MonotonicTimeStamp + portTICK_RATE_MS;
}

uint64_t os_monotonic_time_get_millisecond(void)
{
    uint64_t Ms = 0;

    os_critical_enter();
    Ms = MonotonicTimeStamp;
    os_critical_exit();

    return Ms;
}

uint64_t os_monotonic_time_get_microsecond(void)
{
    uint64_t Ms = 0;

    os_critical_enter();
    Ms = MonotonicTimeStamp;
    os_critical_exit();

    Ms = Ms * 1000;
    return Ms;
}

uint64_t os_real_timestamp_adj_millisecond1(uint64_t RealTimeStamp,
                                            uint64_t AdjustMin)
{
    uint64_t Monotonic   = 0;
    uint64_t TimeDiff    = 0;
    uint64_t RealTime    = 0;
    uint64_t ExitCode    = 0;

    os_critical_enter();

    Monotonic = os_monotonic_time_get_millisecond();
    TimeDiff  = Monotonic - AdjustMonotonic;
    RealTime  = AdjustRealTime + TimeDiff;
    if ((RealTime < RealTimeStamp - AdjustMin) ||
        (RealTime > RealTimeStamp + AdjustMin)) {
        AdjustMonotonic = Monotonic;
        AdjustRealTime  = RealTimeStamp;
        ExitCode        = AdjustRealTime + TimeDiff;
    } else {
        ExitCode = 0;
    }

    os_critical_exit();

    return ExitCode;
}

uint64_t os_real_timestamp_adj_millisecond2(int64_t Adjust)
{
    uint64_t Monotonic   = 0;
    uint64_t TimeDiff    = 0;
    uint64_t RealTime    = 0;

    os_critical_enter();

    Monotonic = os_monotonic_time_get_millisecond();
    TimeDiff  = Monotonic - AdjustMonotonic;
    RealTime  = AdjustRealTime + TimeDiff;
    RealTime  = RealTime + Adjust;

    AdjustMonotonic = Monotonic;
    AdjustRealTime  = RealTime;

    os_critical_exit();

    return RealTime;
}

uint64_t os_real_timestamp_get_second(void)
{
    uint64_t Monotonic = 0;
    uint64_t TimeDiff  = 0;
    uint64_t RealTime  = 0;

    os_critical_enter();
    Monotonic = os_monotonic_time_get_millisecond();
    TimeDiff  = Monotonic - AdjustMonotonic;
    RealTime  = (AdjustRealTime + TimeDiff) / 1000;
    os_critical_exit();

    return RealTime;
}

uint64_t os_real_timestamp_get_millisecond(void)
{
    uint64_t Monotonic = 0;
    uint64_t TimeDiff  = 0;
    uint64_t RealTime  = 0;

    os_critical_enter();
    Monotonic = os_monotonic_time_get_millisecond();
    TimeDiff  = Monotonic - AdjustMonotonic;
    RealTime  = AdjustRealTime + TimeDiff;
    os_critical_exit();

    return RealTime;
}

uint64_t os_real_timestamp_get_microsecond(void)
{
    uint64_t Monotonic = 0;
    uint64_t TimeDiff  = 0;
    uint64_t RealTime  = 0;

    os_critical_enter();
    Monotonic = os_monotonic_time_get_millisecond();
    TimeDiff  = Monotonic - AdjustMonotonic;
    RealTime  = (AdjustRealTime + TimeDiff) * 1000;
    os_critical_exit();

    return RealTime;
}

ssize_t os_timestamp_second_to_datetime(uint64_t    TimeStamp,
                                        DateTime_t* pDateTime)
{
    uint8_t  i;
    uint32_t Year  = 1970;
    uint16_t Month = 0;
    uint16_t Day   = 0;
    uint32_t YearSecond = 0;
    uint32_t IsLeapYear = 0;
    uint32_t DayCounter = 0;

    if (pDateTime == NULL) {
        return -1;
    }

    do {
        if (((Year % 4) == 0) && ((Year % 100) != 0)) {
            IsLeapYear = 1;
        } else if ((Year % 400) == 0) {
            IsLeapYear = 1;
        } else {
            IsLeapYear = 0;
        }

        if (IsLeapYear == 1) {
            YearSecond = 366UL * 24 * 60 * 60;
        } else {
            YearSecond = 365UL * 24 * 60 * 60;
        }

        if (TimeStamp >= YearSecond) {
            TimeStamp = TimeStamp - YearSecond;
            Year++;
        } else {
            break;
        }

    } while (1);

    if (((Year % 4) == 0) && ((Year % 100) != 0)) {
        IsLeapYear = 1;
    } else if ((Year % 400) == 0) {
        IsLeapYear = 1;
    } else {
        IsLeapYear = 0;
    }

    DayCounter = TimeStamp / (60 * 60 * 24UL) + 1;

    for (i = 1; i < 13; ++i) {
        if (IsLeapYear == 1) {
            if (DayCounter <= Month_Days_Accu_L[i]) {
                Month = i;
                Day = DayCounter - Month_Days_Accu_L[i - 1];
                break;
            }
        } else {
            if (DayCounter <= Month_Days_Accu_C[i]) {
                Month = i;
                Day = DayCounter - Month_Days_Accu_C[i - 1];
                break;
            }
        }
    }

    pDateTime->Year   = Year;
    pDateTime->Month  = Month;
    pDateTime->Day    = Day;
    pDateTime->Hour   = (TimeStamp / 3600) % 24;
    pDateTime->Minute = (TimeStamp / 60) % 60;
    pDateTime->Second = TimeStamp % 60;

    return 0;
}

ssize_t os_datetime_get(TimeZone_e TimeZone, DateTime_t* pDateTime)
{
    uint64_t TimeStamp = os_real_timestamp_get_second();

    if ((TimeZone >= OS_TIME_ZONE_WEST_1) &&
        (TimeZone <= OS_TIME_ZONE_WEST_11)) {
        TimeStamp = TimeStamp - (TimeZone & 0xF) * 60 * 60;

    } else if ((TimeZone >= OS_TIME_ZONE_EAST_1) &&
               (TimeZone <= OS_TIME_ZONE_EAST_B)) {
        TimeStamp = TimeStamp + (TimeZone & 0xF) * 60 * 60;
    }

    return os_timestamp_second_to_datetime(TimeStamp, pDateTime);
}
