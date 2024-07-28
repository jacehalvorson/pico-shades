#ifndef SHADES_H
#define SHADES_H

#include "hardware/rtc.h"
#include "hardware/sync.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "set_rtc_time.h"
#include "set_alarm.h"
#include "tcp_server.h"
#include "utils.h"

// -------------------Constants---------------------

#define MOTOR_DURATION_MS               2800
#define MAX_IMPORTANT_MODE_DURATION_SEC 60

// -------------------Pins--------------------------

#define CLOCKWISE_PIN                  0
#define COUNTER_CLOCKWISE_PIN          1
#define MOTOR_ENABLE_PIN               2
#define BUTTON_PIN                     3
#define BUTTON_ENABLE_PIN              22

#define NUM_PINS                       5

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

// -------------------Global Data-------------------

static volatile bool interrupt_fired = false;
static volatile bool shades_closed = true;
static          bool important_mode = false;

// -------------------Functions---------------------

int main();

void open_shades(void);
void close_shades(void);
bool are_shades_closed(void);

void important_mode_callback(void);
void irq_callback(void);
void gpio_callback(uint gpio, uint32_t events);

#endif // SHADES_H
