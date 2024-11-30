#include "set_alarm.h"

// Until the user requests a specific time,
// open shades at 7 AM by default
datetime_t open_time = {
    .year  = -1,
    .month = -1,
    .day   = -1,
    .dotw  = -1,
    .hour  = 7,
    .min   = 0,
    .sec   = 0
};

// Until the user requests a specific time,
// close shades at 4:30 PM by default
datetime_t close_time = {
    .year  = -1,
    .month = -1,
    .day   = -1,
    .dotw  = -1,
    .hour  = 16,
    .min   = 30,
    .sec   = 0
};

void set_alarm()
{
    datetime_t current_time;
    
    if (!rtc_running())
    {
        debug_printf("RTC is not initialized\n");
        return;
    }

    // Get current time
    rtc_get_datetime(&current_time);
    
    bool after_open = (current_time.hour > open_time.hour) ||
                      (current_time.hour == open_time.hour && current_time.min > open_time.min) ||
                      (current_time.hour == open_time.hour && current_time.min == open_time.min && current_time.sec > open_time.sec);

    bool after_close = (current_time.hour > close_time.hour) ||
                       (current_time.hour == close_time.hour && current_time.min > close_time.min) ||
                       (current_time.hour == close_time.hour && current_time.min == close_time.min && current_time.sec > close_time.sec);

    // If it's in between open time and close time, set the alarm for close time
    if (after_open && !after_close)
    {
        debug_printf("Setting close alarm for %02d:%02d:%02d\n", close_time.hour, close_time.min, close_time.sec);
        rtc_set_alarm(&close_time, (rtc_callback_t)queue_closed_shades);
    }
    else // Otherwise, set the alarm for open time (either before open or after close)
    {
        debug_printf("Setting open alarm for %02d:%02d:%02d\n", open_time.hour, open_time.min, open_time.sec);
        rtc_set_alarm(&open_time, (rtc_callback_t)queue_open_shades);
    }
}

void set_alarm_time(const datetime_t *alarm_time, int alarm_type)
{
    if (alarm_time == NULL)
    {
        debug_printf("Invalid alarm time\n");
        return;
    }

    if (alarm_type == OPEN_SHADES)
    {
        // Set the open time
        open_time.hour = alarm_time->hour;
        open_time.min = alarm_time->min;
        open_time.sec = alarm_time->sec;
    }
    else if (alarm_type == CLOSE_SHADES)
    {
        // Set the close time
        close_time.hour = alarm_time->hour;
        close_time.min = alarm_time->min;
        close_time.sec = alarm_time->sec;
    }
    else
    {
        debug_printf("Invalid alarm type %d\n", alarm_type);
        return;
    }

    // Replace current alarm with the new time.
    set_alarm();
}