/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon <me@josuah.net>
 *
 * ISC Licence
 *
 * Copyright © 2022 Brilliant Labs Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * Wrapper around the timer interface for sharing it across drivers.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nrf_soc.h"
#include "nrfx_log.h"
#include "nrfx_timer.h"
#include "nrfx_systick.h"

#include "driver/config.h"
#include "driver/nrfx.h"
#include "driver/timer.h"

#define ASSERT  NRFX_ASSERT

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

    // call all timer functions
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
    if (slot == NULL)
        return;

    __disable_irq();
    *slot = NULL;
    __enable_irq();
}

void timer_add_handler(timer_handler_t *ptr)
{
    timer_handler_t **slot;

    LOG("0x%p", ptr);

    // Check if the timer is already configured.
    if (timer_get_handler_slot(ptr) != NULL)
        return;

    slot = timer_get_handler_slot(NULL);
    assert(slot != NULL); // misconfiguration of TIMER_MAX_HANDLERS

    __disable_irq();
    *slot = ptr;
    __enable_irq();
}

static volatile uint64_t timer_frequency = 0;
static volatile uint64_t timer_ticks = 0;
static volatile uint64_t timer_uptime = 0;

/**
 * Update the current time since timer_init in second.
 */
static void timer_tick_handler(void)
{
    if (++timer_ticks >= timer_frequency) {
        timer_ticks = 0;
        timer_uptime++;
    }
}

/**
 * Get the time elapsed since timer_init();
 * @return The uptime in seconds.
 */
uint64_t timer_get_uptime_s(void)
{
    uint64_t uptime;

    __disable_irq();
    uptime = timer_uptime;
    __enable_irq();
    return uptime;
}

void timer_init(void)
{
    uint32_t err;

    DRIVER("TIMER");
    nrfx_init();

    err = nrfx_timer_init(&timer, &timer_config, timer_event_handler);
    ASSERT(err == NRFX_SUCCESS);

    // Prepare the configuration structure.
    timer_config.mode = NRF_TIMER_MODE_TIMER;
    timer_config.frequency = NRF_TIMER_FREQ_31250Hz;
    timer_config.bit_width = NRF_TIMER_BIT_WIDTH_8;

    // Raise an interrupt every 125 times: 31250 Hz / 125 = 250 Hz.
    nrfx_timer_extended_compare(&timer, NRF_TIMER_CC_CHANNEL0, 125,
            NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
    timer_frequency = 250 * 500; // TODO: why x500 ?

    // Add our own timer for computing maintaining a clock
    timer_add_handler(timer_tick_handler);

    // Start the timer, letting timer_add_handler() append more of them while running.
    nrfx_timer_enable(&timer);
}
