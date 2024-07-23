#include "utils.h"

// -------------------Wi-Fi Functions-------------------

int connect_to_wifi(char *wifi_ssid, char *wifi_password)
{
    int connection_attempts = 0;

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA))
    {
        debug_printf("Failed to initialize Wi-Fi\n");
        return 1;
    }
    debug_printf("Wi-Fi Initialized\n");
    
    cyw43_arch_enable_sta_mode();

    // Blink an LED at 2 Hz while attempting to connect
    start_blinking_led(2);

    // Connect to the Wi-Fi network.
    // If the connection attempt returns an error code, retry up to 3 times.
    while (cyw43_arch_wifi_connect_timeout_ms(wifi_ssid, wifi_password, CYW43_AUTH_WPA2_AES_PSK, 10000))
    {
        debug_printf("Failed to connect to '%s'\n", wifi_ssid);
        if (connection_attempts > 3)
        {
            debug_printf("Exiting\n");
            return 1;
        }

        connection_attempts++;
        sleep_ms(1000);
        debug_printf("Retrying...\n");
    }

    debug_printf("Connected to '%s'\n", wifi_ssid);

    stop_blinking_led();

    return 0;
}

// -------------------Blink LED functions-----------

void start_blinking_led(const int frequency_hz)
{
    // Negative timeout means exact delay (rather than delay between callbacks)
    if (!add_repeating_timer_us((-1 * MICROSECONDS_PER_SECOND) / frequency_hz, led_blink_timer_callback, NULL, &led_blink_timer))
    {
        debug_printf("Failed to add timer\n");
        return;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}

void stop_blinking_led(void)
{
    cancel_repeating_timer(&led_blink_timer);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    blinking_led_on = 0;
}

bool led_blink_timer_callback(repeating_timer_t *rt)
{
    // Toggle LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, !blinking_led_on);
    blinking_led_on = !blinking_led_on;

    // Return true to keep repeating
    return true;
}
