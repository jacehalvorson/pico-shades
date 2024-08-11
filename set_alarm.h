#ifndef SET_ALARM_H
#define SET_ALARM_H

#include "hardware/rtc.h"
#include "pico/util/datetime.h"

#include "utils.h"
#include "shades.h"

// Until the user requests a specific time,
// open shades at 7 AM by default
static datetime_t open_time = {
    .year  = -1,
    .month = -1,
    .day   = -1,
    .dotw  = -1,
    .hour  = 7,
    .min   = 0,
    .sec   = 0
};

// Until the user requests a specific time,
// close shades at 7 PM by default
static datetime_t close_time = {
    .year  = -1,
    .month = -1,
    .day   = -1,
    .dotw  = -1,
    .hour  = 19,
    .min   = 0,
    .sec   = 0
};

static void (*previous_alarm_callback)(void);

// Set the RTC alarm for the next alarm. The parameter can be NULL to keep the previous callback
void set_alarm(void (*alarm_callback)(void));
// Set either the SHADES_OPEN (0) or (SHADES_CLOSED) 1 alarm for a certain time
void set_alarm_time(const datetime_t *alarm_time, int alarm_type);

#endif
