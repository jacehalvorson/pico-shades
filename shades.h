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

#define MOTOR_DURATION_MS         3200
#define MAX_IMPORTANT_MODE_CYCLES 10

// -------------------Pins--------------------------

#define CLOCKWISE_PIN             0
#define COUNTER_CLOCKWISE_PIN     1
#define MOTOR_ENABLE_PIN          2
#define BUTTON_PIN                3
#define BUTTON_ENABLE_PIN         22

#define NUM_PINS                  5

// -------------------Global Data-------------------

typedef struct pin_t
{
    int number;
    int direction;
    int default_value;
} pin_t;

typedef enum shades_state_t
{
    SHADES_OPEN,
    SHADES_CLOSED
} shades_state_t;

typedef enum shades_mode_t
{
    NORMAL,
    IMPORTANT
} shades_mode_t;

typedef enum shades_action_t
{
    OPEN_SHADES,
    CLOSE_SHADES,
    TOGGLE,
    REPEAT_TOGGLE
} shades_action_t;

// -------------------Constants---------------------

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
static volatile shades_state_t shades_state;
static volatile shades_mode_t shades_mode = NORMAL;
static volatile shades_action_t shades_next_action;

// -------------------Functions---------------------

int main();

bool are_shades_closed(void);
void open_shades(void);
void close_shades(void);
shades_mode_t get_mode(void);
void set_mode(shades_mode_t shades_mode);
void set_next_action(shades_action_t action);

static void important_mode_callback(void);
static void irq_callback(void);
static void gpio_callback(uint gpio, uint32_t events);

#endif // SHADES_H
