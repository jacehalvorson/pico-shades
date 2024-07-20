#ifndef SET_ALARM_H
#define SET_ALARM_H

#include "hardware/rtc.h"
#include "pico/util/datetime.h"

#include "utils.h"

// Open shades at 7 AM
static datetime_t open_time = {
    .year  = -1,
    .month = -1,
    .day   = -1,
    .dotw  = -1,
    .hour  = 7,
    .min   = 00,
    .sec   = 0
};

// Close shades at 7 PM
static datetime_t close_time = {
    .year  = -1,
    .month = -1,
    .day   = -1,
    .dotw  = -1,
    .hour  = 19,
    .min   = 00,
    .sec   = 0
};

int set_alarm(void (*irq_callback)(void));

#endif
