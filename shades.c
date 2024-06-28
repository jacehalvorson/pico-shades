#include "shades.h"

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
    if (connect_to_wifi(WIFI_SSID, WIFI_PASSWORD))
        return 1;

    // Set the RTC to the current time
    set_rtc_time();

    // TEMP - Disconnect from Wi-fi (which also disables LED)
    cyw43_arch_deinit();

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
        while (!shades_toggle_queued)
        {
            __wfi();
        }

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
        set_alarm(irq_callback);

        // Turn LED off
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

        shades_toggle_queued = false;
    }

    cyw43_arch_deinit();
    return 0;
}

// -------------------Callbacks---------------------

// IRQ callback (alarm or GPIO)
void irq_callback(void)
{
    // Turn LED on
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    shades_toggle_queued = true;
}

// Button callback points to the same function as the alarm callback
void gpio_callback(uint gpio, uint32_t events)
{
    irq_callback();
}
