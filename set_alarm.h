#ifndef SET_ALARM_H
#define SET_ALARM_H

#include "hardware/rtc.h"
#include "pico/util/datetime.h"

#include "utils.h"
#include "shades.h"

// Set the RTC alarm for the next alarm. The parameter can be NULL to keep the previous callback
void set_alarm();
// Set either the SHADES_OPEN (0) or (SHADES_CLOSED) 1 alarm for a certain time
void set_alarm_time(const datetime_t *alarm_time, int alarm_type);

#endif
