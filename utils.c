#include "utils.h"

volatile int blinking_led_on = 0;
repeating_timer_t led_blink_timer;

// -------------------Wi-Fi Functions-------------------

int connect_to_wifi(char *wifi_ssid, char *wifi_password)
{
    unsigned connection_attempts = 0;

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
    while (cyw43_arch_wifi_connect_timeout_ms(wifi_ssid, wifi_password, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
#ifdef DEBUG
        switch (cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA))
        {
        case CYW43_LINK_DOWN:
            debug_printf("Link down\n");
            break;
        case CYW43_LINK_JOIN:
            debug_printf("Connected to wifi\n");
            break;
        case CYW43_LINK_NOIP:
            debug_printf("Connected to wifi, but no IP address\n");
            break;
        case CYW43_LINK_UP:
            debug_printf("Connected to wifi with an IP address\n");
            break;
        case CYW43_LINK_FAIL:
            debug_printf("Connection failed\n");
            break;
        case CYW43_LINK_NONET:
            debug_printf("No matching SSID found (could be out of range, or down)\n");
            break;
        case CYW43_LINK_BADAUTH:
            debug_printf("Authenticatation failure\n");
            break;
        default:
            break;
        }
#endif

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
