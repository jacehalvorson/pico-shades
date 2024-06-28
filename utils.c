#include "utils.h"

// -------------------Wi-Fi Functions-------------------

int connect_to_wifi(char *wifi_ssid, char *wifi_password)
{
    int connection_attempts = 0;
    repeating_timer_t led_blink_timer;

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA))
    {
        debug_printf("Failed to initialize Wi-Fi\n");
        return 1;
    }
    debug_printf("Wi-Fi Initialized\n");
    
    cyw43_arch_enable_sta_mode();

    // Set a hardware timer to blink a light while attempting to connect

    // negative timeout means exact delay (rather than delay between callbacks)
    if (!add_repeating_timer_us(-1000000 / 2, led_blink_timer_callback, NULL, &led_blink_timer)) {
        debug_printf("Failed to add timer\n");
        return 1;
    }

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

    // Disable LED blink timer and turn off LED
    cancel_repeating_timer(&led_blink_timer);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    return 0;
}

// -------------------Blink LED functions-----------

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