#include "set_alarm.h"

void set_alarm(void (*irq_callback)(void))
{
    datetime_t current_time;

    // Set to parameter if not NULL, otherwise keep the previous callback
    void (*alarm_callback)(void) = (irq_callback != NULL)
                                    ? irq_callback
                                    : previous_alarm_callback;

    if (alarm_callback == NULL)
    {
        debug_printf("Invalid alarm callback\n");
        return;
    }
    
    if (!rtc_running())
    {
        debug_printf("RTC is not initialized\n");
        return;
    }

    // Get current time
    rtc_get_datetime(&current_time);
    
    // If it's in between open time and close time, set the alarm for close time
    if (current_time.hour >= open_time.hour && current_time.hour < close_time.hour)
    {
        debug_printf("Setting alarm for %02d:%02d:%02d\n", close_time.hour, close_time.min, close_time.sec);
        rtc_set_alarm(&close_time, (rtc_callback_t)alarm_callback);
        set_next_action(CLOSE_SHADES);
    }
    else // Otherwise, set the alarm for open time
    {
        debug_printf("Setting alarm for %02d:%02d:%02d\n", open_time.hour, open_time.min, open_time.sec);
        rtc_set_alarm(&open_time, (rtc_callback_t)alarm_callback);
        set_next_action(OPEN_SHADES);
    }

    previous_alarm_callback = alarm_callback;
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
    // The NULL option keeps the previous alarm callback
    set_alarm(NULL);
}