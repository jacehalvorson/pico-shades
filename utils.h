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

// -------------------Global Data-------------------

volatile static int blinking_led_on = 0;

// -------------------Functions---------------------

int connect_to_wifi(char *wifi_ssid, char *wifi_password);

int blink_led(const int frequency_hz, const int duration_ms);
bool led_blink_timer_callback(repeating_timer_t *rt);

#endif // UTILS_H
