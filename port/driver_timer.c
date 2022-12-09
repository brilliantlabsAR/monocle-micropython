/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

/**
 * Wrapper around the timer interface for sharing it across drivers.
 * @file driver_timer.c
 */

#include <stdint.h>
#include <stdbool.h>

#include "nrf_soc.h"
#include "nrfx_log.h"
#include "nrfx_timer.h"

#include "driver_timer.h"
#include "driver_config.h"

#define LOG(...) NRFX_LOG_ERROR(__VA_ARGS__)
#define ASSERT NRFX_ASSERT

static nrfx_timer_t timer = NRFX_TIMER_INSTANCE(TIMER_INSTANCE);
static nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG;
static timer_handler_t *timer_handlers_list[TIMER_MAX_HANDLERS];

/**
 * The timer hander that dispatches the timer to all the other functions.
 */
static void timer_event_handler(nrf_timer_event_t event, void *ctx)
{
    (void)event;
    (void)ctx;

    for (size_t i = 0; i < TIMER_MAX_HANDLERS; i++)
        if (timer_handlers_list[i] != NULL)
            timer_handlers_list[i]();
}

/**
 * Get a pointer within the array of handlers, for modification purposes.
 * @param ptr A pointer to a function handler, or eventually NULL.
 */
static timer_handler_t **timer_get_handler_slot(timer_handler_t *ptr)
{
    for (size_t i = 0 ; i < TIMER_MAX_HANDLERS; i++)
        if (timer_handlers_list[i] == ptr)
            return &timer_handlers_list[i];
    return NULL;
}

/**
 * Remove a function from the list of timer handlers to execute.
 * @param ptr Function pointer of the timer that was previously added.
 */
void timer_del_handler(timer_handler_t *ptr)
{
    timer_handler_t **slot;

    slot = timer_get_handler_slot(ptr);
    assert(slot != NULL); // should have been added first

    LOG("0x%p", ptr);

    __disable_irq();
    *slot = NULL;
    __enable_irq();
}

void timer_add_handler(timer_handler_t *ptr)
{
    timer_handler_t **slot;

    // Check if the timer is already configured.
    if (timer_get_handler_slot(ptr) != NULL)
        return;

    slot = timer_get_handler_slot(NULL);
    assert(slot != NULL); // misconfiguration of TIMER_MAX_HANDLERS

    LOG("0x%p", ptr);

    __disable_irq();
    *slot = ptr;
    __enable_irq();
}

void timer_init(void)
{
    uint32_t err;

    // Prepare the configuration structure.
    timer_config.mode = NRF_TIMER_MODE_TIMER;
    timer_config.frequency = NRF_TIMER_FREQ_31250Hz;
    timer_config.bit_width = NRF_TIMER_BIT_WIDTH_8;

    err = nrfx_timer_init(&timer, &timer_config, timer_event_handler);
    ASSERT(err == NRFX_SUCCESS);

    // Do not raise an interrupt every time, but on every 100 times -> 31.25 Hz.
    nrfx_timer_extended_compare(&timer, NRF_TIMER_CC_CHANNEL0, 100,
            NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

    // Start the timer, letting timer_add_handler() append more of them while running.
    nrfx_timer_enable(&timer);

    LOG("ready max_handlers=%d", TIMER_MAX_HANDLERS);
}
