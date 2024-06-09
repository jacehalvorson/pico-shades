#include <stdio.h>
#include "hardware/rtc.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "hardware/sync.h"

#define MOTOR_DURATION_MS             2800

#define LED_PIN                       PICO_DEFAULT_LED_PIN
#define CLOCKWISE_PIN                 0
#define COUNTER_CLOCKWISE_PIN         1
#define MOTOR_ENABLE_PIN              2
#define BUTTON_PIN                    3
#define BUTTON_ENABLE_PIN             22

#define NUM_PINS 6

const int pin_definitions[NUM_PINS][3] = {
    {LED_PIN,                   GPIO_OUT, 0},
    {BUTTON_ENABLE_PIN,         GPIO_OUT, 1},
    {CLOCKWISE_PIN,             GPIO_OUT, 0},
    {COUNTER_CLOCKWISE_PIN,     GPIO_OUT, 0},
    {MOTOR_ENABLE_PIN,          GPIO_OUT, 1},
    {BUTTON_PIN,                GPIO_IN, 0},
};

datetime_t alarm_time = {
    .year  = -1,
    .month = -1,
    .day   = -1,
    .dotw  = -1, // 0 is Sunday
    .hour  = 7,
    .min   = 0,
    .sec   = 0
};

void irq_callback(void) {
    gpio_put(LED_PIN, 1);
}

void gpio_callback(uint gpio, uint32_t events) {
    gpio_put(LED_PIN, 1);
}

int main() {
    int selected_motor_pin;
    bool shades_closed = true;

    // Start the RTC
    rtc_init();

    // Initialize pins
    for (int i = 0; i < NUM_PINS; i++)
    {
        int pin = pin_definitions[i][0];
        int dir = pin_definitions[i][1];
        int val = pin_definitions[i][2];

        gpio_init(pin);
        gpio_set_dir(pin, dir);
        if (val == GPIO_OUT)
            gpio_put(pin, GPIO_OUT);
    }

    // Set IRQs for GPIO (button) and RTC (alarm)
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    rtc_set_alarm((datetime_t *)&alarm_time, &irq_callback);

    while (1)
    {
        // Wait here until IRQ fires (either button or alarm)
        __wfi();

        if (shades_closed)
            selected_motor_pin = CLOCKWISE_PIN;
        else
            selected_motor_pin = COUNTER_CLOCKWISE_PIN;

        // Power the motor long enough to open or close the shades
        gpio_put(selected_motor_pin, 1);
        sleep_ms(MOTOR_DURATION_MS);
        gpio_put(selected_motor_pin, 0);

        shades_closed = (selected_motor_pin == COUNTER_CLOCKWISE_PIN)
            ? true
            : false;

        // Set alarm for 7 AM
        rtc_set_alarm((datetime_t *)&alarm_time, &irq_callback);

        gpio_put(LED_PIN, 0);
    }
}