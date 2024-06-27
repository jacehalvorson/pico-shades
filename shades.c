#include <stdio.h>
#include "hardware/rtc.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/util/datetime.h"
#include "hardware/sync.h"
#include "set_rtc_time.h"

#define MOTOR_DURATION_MS              2800
#define WIFI_CONNECTION_RETRY_DELAY_MS 1000

#define CLOCKWISE_PIN                  0
#define COUNTER_CLOCKWISE_PIN          1
#define MOTOR_ENABLE_PIN               2
#define BUTTON_PIN                     3
#define BUTTON_ENABLE_PIN              22

#define DEBUG

#ifdef DEBUG
    #define debug_printf(...) printf(__VA_ARGS__)
#else
    #define debug_printf(...)
#endif

#define NUM_PINS 5

typedef struct pin_t
{
    int number;
    int direction;
    int default_value;
} pin_t;

static const pin_t pin_definitions[NUM_PINS] =
{
    {.number = BUTTON_ENABLE_PIN,     .direction = GPIO_OUT, .default_value = 1},
    {.number = CLOCKWISE_PIN,         .direction = GPIO_OUT, .default_value = 0},
    {.number = COUNTER_CLOCKWISE_PIN, .direction = GPIO_OUT, .default_value = 0},
    {.number = MOTOR_ENABLE_PIN,      .direction = GPIO_OUT, .default_value = 1},
    {.number = BUTTON_PIN,            .direction = GPIO_IN,  .default_value = 0}
};

// Open shades at 7 AM
datetime_t open_time = {
    .year  = -1,
    .month = -1,
    .day   = -1,
    .dotw  = -1,
    .hour  = 7,
    .min   = 0,
    .sec   = 0
};

// Close shades at 7 PM
datetime_t close_time = {
    .year  = -1,
    .month = -1,
    .day   = -1,
    .dotw  = -1,
    .hour  = 19,
    .min   = 0,
    .sec   = 0
};

volatile int blinking_led_on = 0;
volatile bool shades_toggle_queued = false;

void irq_callback(void)
{
    // Turn LED on
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    shades_toggle_queued = true;
}

void gpio_callback(uint gpio, uint32_t events)
{
    irq_callback();
}

bool led_blink_timer_callback(repeating_timer_t *rt)
{
    // Toggle LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, !blinking_led_on);
    blinking_led_on = !blinking_led_on;

    // Keep repeating
    return true;
}

int blink_led(const int frequency_hz, const int duration_ms)
{
    repeating_timer_t led_blink_timer;
    if (!add_repeating_timer_us(-1000000 / frequency_hz, led_blink_timer_callback, NULL, &led_blink_timer))
    {
        debug_printf("Failed to add timer\n");
        return 1;
    }

    sleep_ms(duration_ms);
    
    cancel_repeating_timer(&led_blink_timer);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    return 0;
}

int connect_to_wifi()
{
    int connection_attempts = 0;
    int led_toggle_frequency_hz = 4;

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA))
    {
        debug_printf("Failed to initialize Wi-Fi\n");
        return 1;
    }
    debug_printf("Wi-Fi Initialized\n");
    
    cyw43_arch_enable_sta_mode();
    
    // Connect to the Wi-Fi network.
    // If the connection attempt returns an error code, retry up to 3 times.

    // Set a hardware timer to blink a light every 500 ms
    repeating_timer_t led_blink_timer;

    // negative timeout means exact delay (rather than delay between callbacks)
    if (!add_repeating_timer_us(-1000000 / led_toggle_frequency_hz, led_blink_timer_callback, NULL, &led_blink_timer)) {
        debug_printf("Failed to add timer\n");
        return 1;
    }

    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000))
    {
        debug_printf("Failed to connect to '%s'\n", WIFI_SSID);
        if (connection_attempts > 3)
        {
            debug_printf("Exiting\n");
            return 1;
        }

        connection_attempts++;
        sleep_ms(WIFI_CONNECTION_RETRY_DELAY_MS);
        debug_printf("Retrying...\n");
    }

    debug_printf("Connected to '%s'\n", WIFI_SSID);

    // Disable LED blink timer and turn off LED
    cancel_repeating_timer(&led_blink_timer);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    return 0;
}

void set_next_alarm()
{
    datetime_t currentTime;
    datetime_t *alarm_time_ptr;
    
    // Get current time
    rtc_get_datetime(&currentTime);
    
    // If it's later than 7 AM and earlier than 7 PM, set the alarm for 7 PM
    if (currentTime.hour >= open_time.hour && currentTime.hour < close_time.hour)
    {
        alarm_time_ptr = &close_time;
    }
    else // Otherwise, set the alarm for 7 AM
    {
        alarm_time_ptr = &open_time;
    }

    debug_printf("Setting alarm for %d:%2d\n", alarm_time_ptr->hour, alarm_time_ptr->min);
    rtc_set_alarm(alarm_time_ptr, &irq_callback);

    // Attempt to blink LED at 10 Hz for half a second
    (void)blink_led(10, 500);
}

int main()
{
    int selected_motor_pin;
    bool shades_closed = true;

#ifdef DEBUG
    stdio_init_all();
#endif

    debug_printf("Initializing pins");
    for (int i = 0; i < NUM_PINS; i++)
    {
        gpio_init(pin_definitions[i].number);
        gpio_set_dir(pin_definitions[i].number, pin_definitions[i].direction);
        if (pin_definitions[i].default_value == GPIO_OUT)
            gpio_put(pin_definitions[i].number, GPIO_OUT);
    }

    debug_printf("Initializing motor position");
    // Find the closed position
    gpio_put(COUNTER_CLOCKWISE_PIN, 1);
    sleep_ms(5000);
    gpio_put(COUNTER_CLOCKWISE_PIN, 0);
    // Back off from the maximum
    gpio_put(CLOCKWISE_PIN, 1);
    sleep_ms(300);
    gpio_put(CLOCKWISE_PIN, 0);
    shades_closed = true;

    debug_printf("Connecting to Wi-Fi network '%s'...\n", WIFI_SSID);
    if (connect_to_wifi())
        return 1;

    // Set the RTC to the current time
    set_rtc_time();

    // Disconnect from Wi-fi
    cyw43_arch_deinit();

    debug_printf("Setting IRQs for the button and alarm\n");
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    set_next_alarm();

    while (1)
    {
        debug_printf("Waiting for button press or alarm...\n");
        // while(!shades_toggle_queued)
            // sleep_ms(100);
        __wfi();

        // Turn LED on
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

        if (shades_closed)
            selected_motor_pin = CLOCKWISE_PIN;
        else
            selected_motor_pin = COUNTER_CLOCKWISE_PIN;

        debug_printf("Shades %s\n", (selected_motor_pin == CLOCKWISE_PIN) ? "opening" : "closing");

        // Power the motor long enough to open or close the shades
        gpio_put(selected_motor_pin, 1);
        sleep_ms(MOTOR_DURATION_MS);
        gpio_put(selected_motor_pin, 0);

        // Update the state of the shades based on motor position
        shades_closed = (selected_motor_pin == COUNTER_CLOCKWISE_PIN)
            ? true
            : false;

        // Set the next alarm, either to open or close the shades
        set_next_alarm();

        // Turn LED off
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

        shades_toggle_queued = false;
    }

    return 0;
}