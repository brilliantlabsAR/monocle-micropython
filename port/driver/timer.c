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

#include "driver/config.h"
#include "driver/driver.h"
#include "driver/timer.h"

#define LOG     NRFX_LOG
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
    DRIVER(TIMER);

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
}
