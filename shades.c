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

typedef struct pin_t
{
    int number;
    int direction;
    int default_value;
} pin_t;

static const pin_t pin_definitions[NUM_PINS] =
{
    {.number = LED_PIN,               .direction = GPIO_OUT, .default_value = 0},
    {.number = BUTTON_ENABLE_PIN,     .direction = GPIO_OUT, .default_value = 1},
    {.number = CLOCKWISE_PIN,         .direction = GPIO_OUT, .default_value = 0},
    {.number = COUNTER_CLOCKWISE_PIN, .direction = GPIO_OUT, .default_value = 0},
    {.number = MOTOR_ENABLE_PIN,      .direction = GPIO_OUT, .default_value = 1},
    {.number = BUTTON_PIN,            .direction = GPIO_IN,  .default_value = 0}
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

void irq_callback(void)
{
    gpio_put(LED_PIN, 1);
}

void gpio_callback(uint gpio, uint32_t events)
{
    gpio_put(LED_PIN, 1);
}

int main()
{
    int selected_motor_pin;
    bool shades_closed = true;
    datetime_t start_time = {
        .year = 2024,
        .month = 6,
        .day = 10,
        .dotw = 1,
        .hour = 23,
        .min = 38,
        .sec = 0
    };

    // Start the RTC
    rtc_init();
    rtc_set_datetime(&start_time);

    // Initialize pins
    for (int i = 0; i < NUM_PINS; i++)
    {
        gpio_init(pin_definitions[i].number);
        gpio_set_dir(pin_definitions[i].number, pin_definitions[i].direction);
        if (pin_definitions[i].default_value == GPIO_OUT)
            gpio_put(pin_definitions[i].number, GPIO_OUT);
    }

    // Find the closed position
    gpio_put(COUNTER_CLOCKWISE_PIN, 1);
    sleep_ms(5000);
    gpio_put(COUNTER_CLOCKWISE_PIN, 0);
    // Back off from the maximum
    gpio_put(CLOCKWISE_PIN, 1);
    sleep_ms(300);
    gpio_put(CLOCKWISE_PIN, 0);

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