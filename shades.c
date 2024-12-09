#include "shades.h"

int main()
{
    TCP_SERVER_T *tcp_state;

    stdio_init_all();

    // Initialize the pins
    for (int i = 0; i < NUM_PINS; i++)
    {
        gpio_init(pin_definitions[i].number);
        gpio_set_dir(pin_definitions[i].number, pin_definitions[i].direction);
        if (pin_definitions[i].direction == GPIO_OUT)
        {
            gpio_put(pin_definitions[i].number, pin_definitions[i].default_value);
        }
        else if (pin_definitions[i].direction == GPIO_IN)
        {
            if (pin_definitions[i].default_value == 1)
            {
                gpio_pull_up(pin_definitions[i].number);
            }
            else if (pin_definitions[i].default_value == 0)
            {
                gpio_pull_down(pin_definitions[i].number);
            }
        }
    }

    // Find the closed position to initialize the motor
    gpio_put(COUNTER_CLOCKWISE_PIN, 1);
    add_alarm_in_ms(MOTOR_DURATION_MS, turn_off_motor_callback, NULL, false);
    shades_state = SHADES_CLOSED;

    debug_printf("Connecting to Wi-Fi network '%s'...\n", WIFI_SSID);
    if (connect_to_wifi(WIFI_SSID, WIFI_PASSWORD))
        return 1;

    // Set the RTC to the current time
    set_rtc_time();

    debug_printf("Setting IRQs for the button and alarm\n");
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    set_alarm(irq_callback);

    // Spinning up HTTP server
    tcp_state = tcp_server_open();

    // Main loop
    while (1)
    {
        debug_printf("Waiting for button press or alarm...\n");
        while (!interrupt_fired)
        {
            __wfi();
        }

        debug_printf("Interrupt fired\n");

        if (shades_toggle_queued)
        {
            if (shades_state == SHADES_OPEN)
                close_shades();
            else
                open_shades();
        }
        else if (shades_open_queued)
        {
            open_shades();
        }
        else if (shades_closed_queued)
        {
            close_shades();
        }
        else if (important_mode_queued)
        {
            debug_printf("Important mode\n");
            important_mode_callback();
        }

        shades_toggle_queued = false;
        shades_open_queued = false;
        shades_closed_queued = false;
        important_mode_queued = false;

        // Set the next alarm
        set_alarm();

        // Make sure TCP server is running
        if (!tcp_state)
        {
            tcp_state = tcp_server_open();
            if (!tcp_state)
            {
                debug_printf("Failed to open TCP server\n");
            }
        }

        // This iteration is done, reset the flag and make sure LED is off
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
    return shades_state == SHADES_CLOSED;
}

void open_shades(void)
{
    if (shades_state == SHADES_OPEN)
    {
        debug_printf("Shades already open\n");
        return;
    }

    debug_printf("Shades opening\n");
    gpio_put(CLOCKWISE_PIN, 1);
    sleep_ms(MOTOR_DURATION_MS);
    gpio_put(CLOCKWISE_PIN, 0);

    shades_state = SHADES_OPEN;
}

void close_shades(void)
{
    if (shades_state == SHADES_CLOSED)
    {
        debug_printf("Shades already closed\n");
        return;
    }

    debug_printf("Shades closing\n");
    gpio_put(COUNTER_CLOCKWISE_PIN, 1);
    sleep_ms(MOTOR_DURATION_MS);
    gpio_put(COUNTER_CLOCKWISE_PIN, 0);

    shades_state = SHADES_CLOSED;
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
    int cycle_count = 0;

    // Blink an LED at 6 hz
    start_blinking_led(6);

    if (shades_state == SHADES_CLOSED)
    {
        open_shades();
    }

    // Keep spinning the motor back and forth until the button is pressed (or time expires)
    interrupt_fired = false;
    while (!interrupt_fired && cycle_count < MAX_IMPORTANT_MODE_CYCLES)
    {
        close_shades();
        sleep_ms(500);
        open_shades();
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

// Button callback
static void gpio_callback(uint gpio, uint32_t events)
{
    if (shades_mode == IMPORTANT)
    {
        important_mode_queued = true;
    }
    else
    {
        shades_toggle_queued = true;
    }

    irq_callback();
}

void queue_open_shades(void)
{
    if (shades_mode == IMPORTANT)
    {
        important_mode_queued = true;
    }
    else
    {
        shades_open_queued = true;
    }
}

void queue_closed_shades(void)
{
    shades_closed_queued = true;
}

int64_t turn_off_motor_callback(alarm_id_t id, __unused void *user_data)
{
    gpio_put(COUNTER_CLOCKWISE_PIN, 0);
    return 0;
}
