/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

/**
 * Wrapper around the timer interface for sharing it across drivers.
 * @file monocle_timer.c
 */

#include <stdint.h>
#include <stdbool.h>

#include "nrfx_timer.h"
#include "nrfx_log.h"

#include "monocle_timer.h"
#include "monocle_config.h"

#define LOG(...) NRFX_LOG_ERROR(__VA_ARGS__)
#define ASSERT NRFX_ASSERT

nrfx_timer_t timer = NRFX_TIMER_INSTANCE(TIMER_INSTANCE);
nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG;
timer_handler_t *timer_handlers_list[TIMER_MAX_CALLBACKS];

/**
 * Get a pointer within the array of callback, for modification purposes.
 */
static timer_handler_t **timer_get_handler_slot(timer_handler_t *ptr)
{
    for (size_t i = 0 ; i < TIMER_MAX_CALLBACKS; i++)
        if (timer_handlers_list[i] == ptr)
            return &timer_handlers_list[i];
    return NULL;
}

/**
 * Remove a function from the list of timer handlers to execute.
 * @param ptr Function pointer of the timer that was previously added.
 */
static void timer_del_handler(timer_handler_t *ptr)
{
}

void timer_add_handlier()
{
}

void timer_init(void)
{
    // Prepare the configuration structure.
    timer_config.mode = NRF_TIMER_MODE_TIMER;
    timer_config.frequency = NRF_TIMER_FREQ_1MHz;
    timer_config.bit_width = NRF_TIMER_BIT_WIDTH_8;

    err = nrfx_timer_init(&timer, &timer_config, timer_handler);
    ASSERT(err == NRFX_SUCCESS);

    // Do not raise an interrupt on every MHz, but on every 100 MHz.
    nrfx_timer_extended_compare(&touch_timer, NRF_TIMER_CC_CHANNEL0, 100, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

    // Start the timer.
    nrfx_timer_enable(&timer);
}

void timer_add_handler_handler_t cb)
{

}

