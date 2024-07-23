#include "set_alarm.h"

int set_alarm(void (*irq_callback)(void))
{
    datetime_t currentTime;
    
    if (!rtc_running())
    {
        debug_printf("RTC is not initialized\n");
        return 1;
    }

    // Get current time
    rtc_get_datetime(&currentTime);
    
    // If it's in between open time and close time, set the alarm for close time
    if (currentTime.hour >= open_time.hour && currentTime.hour < close_time.hour)
    {
        debug_printf("Setting alarm for %d:%02d\n", close_time.hour, close_time.min);
        rtc_set_alarm(&close_time, irq_callback);
    }
    else // Otherwise, set the alarm for open time
    {
        debug_printf("Setting alarm for %d:%02d\n", open_time.hour, open_time.min);
        rtc_set_alarm(&open_time, irq_callback);
    }

    // Blink LED at for half a second to indicate the alarm is set
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(500);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    return 0;
}