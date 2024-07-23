#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#include "pico/cyw43_arch.h"

// -------------------Constants---------------------

#define DEBUG

#ifdef DEBUG
    #define debug_printf(...) printf(__VA_ARGS__)
#else
    #define debug_printf(...)
#endif

#define MICROSECONDS_PER_SECOND 1000000

// -------------------Global Data-------------------

volatile static int blinking_led_on = 0;
static repeating_timer_t led_blink_timer;

// -------------------Functions---------------------

int connect_to_wifi(char *wifi_ssid, char *wifi_password);

void start_blinking_led(const int frequency_hz);
void stop_blinking_led(void);
bool led_blink_timer_callback(repeating_timer_t *rt);

#endif // UTILS_H
