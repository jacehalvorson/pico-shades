#include "set_alarm.h"

int set_alarm(void (*irq_callback)(void))
{
    datetime_t currentTime;
    datetime_t *alarm_time_ptr;
    
    if (rtc_running() == false)
    {
        debug_printf("RTC is not initialized\n");
        return 1;
    }

    // Get current time
    rtc_get_datetime(&currentTime);
    
    // If it's in between open time and close time, set the alarm for close time
    if (currentTime.hour >= open_time.hour && currentTime.hour < close_time.hour)
    {
        alarm_time_ptr = &close_time;
    }
    else // Otherwise, set the alarm for open time
    {
        alarm_time_ptr = &open_time;
    }

    debug_printf("Setting alarm for %d:%02d\n", alarm_time_ptr->hour, alarm_time_ptr->min);
    rtc_set_alarm(alarm_time_ptr, irq_callback);

    // Blink LED at 5 Hz for half a second to indicate the alarm is set
    // Ignore return status
    (void)blink_led(5, 500);

    return 0;
}