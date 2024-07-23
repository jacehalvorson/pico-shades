#include "shades.h"

int main()
{
#ifdef DEBUG
    stdio_init_all();
#endif

    debug_printf("Initializing pins");
    for (int i = 0; i < NUM_PINS; i++)
    {
        gpio_init(pin_definitions[i].number);
        gpio_set_dir(pin_definitions[i].number, pin_definitions[i].direction);
        if (pin_definitions[i].direction == GPIO_OUT)
            gpio_put(pin_definitions[i].number, pin_definitions[i].default_value);
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
    if (connect_to_wifi(WIFI_SSID, WIFI_PASSWORD))
        return 1;

    // Set the RTC to the current time
    set_rtc_time();

    debug_printf("Setting IRQs for the button and alarm\n");
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    if (set_alarm(irq_callback))
    {
        debug_printf("Failed to set alarm\n");
        return 1;
    }

    // Main loop
    while (1)
    {
        debug_printf("Waiting for button press or alarm...\n");
        while (!interrupt_fired)
        {
            __wfi();
        }

        if (important_mode)
            important_mode_callback();
        else if (shades_closed)
            open_shades();
        else
            close_shades();

        // Set the next alarm
        set_alarm(irq_callback);

        // This iteration is done, reset the flag and turn off LED
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        interrupt_fired = false;
    }

    cyw43_arch_deinit();
    return 0;
}

void open_shades(void)
{
    debug_printf("Shades opening\n");
    gpio_put(CLOCKWISE_PIN, 1);
    sleep_ms(MOTOR_DURATION_MS);
    gpio_put(CLOCKWISE_PIN, 0);
    shades_closed = false;
}

void close_shades(void)
{
    debug_printf("Shades closing\n");
    gpio_put(COUNTER_CLOCKWISE_PIN, 1);
    sleep_ms(MOTOR_DURATION_MS);
    gpio_put(COUNTER_CLOCKWISE_PIN, 0);
    shades_closed = true;
}

// -------------------Callbacks---------------------

void important_mode_callback(void)
{
    unsigned cycle_count = 0;

    // Blink an LED at 6 hz
    start_blinking_led(6);

    // Keep spinning the motor back and forth until the button is pressed (or time expires)
    interrupt_fired = false;
    while (!interrupt_fired && cycle_count < (MAX_IMPORTANT_MODE_DURATION_SEC / 2))
    {
        gpio_put(CLOCKWISE_PIN, 1);
        sleep_ms(MOTOR_DURATION_MS);
        gpio_put(CLOCKWISE_PIN, 0);
        sleep_ms(1000);
        gpio_put(COUNTER_CLOCKWISE_PIN, 1);
        sleep_ms(MOTOR_DURATION_MS);
        gpio_put(COUNTER_CLOCKWISE_PIN, 0);
        sleep_ms(1000);   
        cycle_count++;
    }

    stop_blinking_led();
}

// IRQ callback (alarm or GPIO)
void irq_callback(void)
{
    interrupt_fired = true;
}

// Button callback points to the same function as the alarm callback
void gpio_callback(uint gpio, uint32_t events)
{
    irq_callback();
}
