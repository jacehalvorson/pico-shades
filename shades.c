#include <stdio.h>
#include "hardware/rtc.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "hardware/sync.h"

#define LED_PIN PICO_DEFAULT_LED_PIN
#define BUTTON_PIN 3

bool is_LED_on = false;

const datetime_t alarm_time = {
    .year  = -1,
    .month = -1,
    .day   = -1,
    .dotw  = -1, // 0 is Sunday
    .hour  = -1,
    .min   = -1,
    .sec   = 2
};

void alarm_callback(void) {
    is_LED_on = !is_LED_on;
    gpio_put(LED_PIN, is_LED_on);
}

void gpio_callback(uint gpio, uint32_t events) {
    is_LED_on = !is_LED_on;
    gpio_put(LED_PIN, is_LED_on);
}

int main() {
    // Start on Friday 5th of June 2020 15:45:00
    datetime_t t = {
            .year  = 2020,
            .month = 06,
            .day   = 05,
            .dotw  = 5, // 0 is Sunday, so 5 is Friday
            .hour  = 15,
            .min   = 45,
            .sec   = 00
    };

    // Start the RTC
    // rtc_init();
    // rtc_set_datetime(&t);

    // LED pin (output)
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Button (input)
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);

    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    while (1);

    // rtc_set_alarm((datetime_t *)&alarm_time, &alarm_callback);
    // __wfi();
}