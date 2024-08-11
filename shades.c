#include "shades.h"

int main()
{
    TCP_SERVER_T *tcp_state;

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
    sleep_ms(3000);
    gpio_put(COUNTER_CLOCKWISE_PIN, 0);
    // Back off from the maximum
    gpio_put(CLOCKWISE_PIN, 1);
    sleep_ms(500);
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

    tcp_state = tcp_server_open();

    // Main loop
    while (1)
    {
        debug_printf("Waiting for button press or alarm...\n");
        while (!interrupt_fired)
        {
            __wfi();
        }

        if (shades_mode == IMPORTANT)
            important_mode_callback();
        else if (shades_closed)
            open_shades();
        else
            close_shades();

        // Set the next alarm
        set_alarm(irq_callback);

        // Make sure TCP server is running
        if (!tcp_state)
        {
            tcp_state = tcp_server_open();
            if (!tcp_state)
            {
                debug_printf("Failed to open TCP server\n");
            }
        }

        // This iteration is done, reset the flag and turn off LED
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        interrupt_fired = false;
    }

    // tcp_server_close(tcp_state);
    cyw43_arch_deinit();
    return 0;
}

// -------------------Shades operations---------------------

bool are_shades_closed(void)
{
    return shades_closed;
}

void open_shades(void)
{
    if (!shades_closed)
    {
        debug_printf("Shades already open\n");
        return;
    }

    debug_printf("Shades opening\n");
    gpio_put(CLOCKWISE_PIN, 1);

    // Set a timer to disable the motor
    
    if (add_alarm_in_ms(MOTOR_DURATION_MS, (alarm_callback_t)open_shades_finish, NULL, true) <= 0)
    {
        debug_printf("Failed to add open timer\n");
        gpio_put(CLOCKWISE_PIN, 0);
    }
}

static int64_t open_shades_finish(alarm_id_t id, void *user_data)
{
    // Disable motor and update shades state to open
    gpio_put(CLOCKWISE_PIN, 0);
    shades_closed = false;

    // Return 0 to not repeat
    return 0;
}

void close_shades(void)
{
    if (shades_closed)
    {
        debug_printf("Shades already closed\n");
        return;
    }

    debug_printf("Shades closing\n");
    gpio_put(COUNTER_CLOCKWISE_PIN, 1);

    // Set a timer to disable the motor
    if (add_alarm_in_ms(MOTOR_DURATION_MS, (alarm_callback_t)close_shades_finish, NULL, true) <= 0)
    {
        debug_printf("Failed to add close timer\n");
        gpio_put(CLOCKWISE_PIN, 0);
    }
}

static int64_t close_shades_finish(alarm_id_t id, void *user_data)
{
    // Disable motor and update shades state to closed
    gpio_put(COUNTER_CLOCKWISE_PIN, 0);
    shades_closed = true;
    
    // Return 0 to not repeat
    return 0;
}

void set_mode(shades_mode_t new_mode)
{
    shades_mode = new_mode;
    debug_printf("shades_mode set to %d\n", shades_mode);
}

shades_mode_t get_mode(void)
{
    return shades_mode;
}

// -------------------Callbacks---------------------

static void important_mode_callback(void)
{
    unsigned cycle_count = 0;

    // Blink an LED at 6 hz
    start_blinking_led(6);

    if (!shades_closed)
    {
        close_shades();
    }

    // Keep spinning the motor back and forth until the button is pressed (or time expires)
    interrupt_fired = false;
    while (!interrupt_fired && cycle_count < (MAX_IMPORTANT_MODE_DURATION_SEC / 2))
    {
        open_shades();
        sleep_ms(500);
        close_shades();
        sleep_ms(500);   
        cycle_count++;
    }

    stop_blinking_led();
}

// IRQ callback to indicate an interrupt happened from various sources (alarm, GPIO, timer)
static void irq_callback(void)
{
    interrupt_fired = true;
}

// Button callback points to the same function as the alarm callback
static void gpio_callback(uint gpio, uint32_t events)
{
    irq_callback();
}
