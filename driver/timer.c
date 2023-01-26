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
#include "driver/timer.h"

#include "monocle.h"

#define LEN(x) (sizeof(x) / sizeof *(x))

timer_task_t *timer_1ms[TIMER_MAX_TASKS];
timer_task_t *timer_500ms[TIMER_MAX_TASKS];

static nrfx_timer_t timer = NRFX_TIMER_INSTANCE(TIMER_INSTANCE);
static volatile uint64_t timer_uptime_ms;
uint16_t timer_divider_500ms;

static void timer_call_handlers(timer_task_t **list, size_t task_num)
{
    // call all timer functions that are populted onto the list
    for (size_t i = 0; i < task_num; i++)
        if (list[i] != NULL)
            list[i]();
}

/**
 * The timer hander that dispatches the timer to all the other functions.
 */
static void timer_event_handler(nrf_timer_event_t event, void *ctx)
{
    (void)event;
    (void)ctx;

    // update the current time since timer_start in millisecond.
    timer_uptime_ms++;

    if (timer_divider_500ms++ == 500)
    {
        timer_divider_500ms = 0;
        timer_call_handlers(timer_500ms, LEN(timer_500ms));
    }
    timer_call_handlers(timer_1ms, LEN(timer_1ms));
}

/**
 * Get a pointer within the array of task, for modification purposes.
 * @param fn A pointer to a function, or eventually NULL.
 */
static timer_task_t **timer_get_task_slot(timer_task_t **list, timer_task_t *fn)
{
    for (size_t i = 0; i < TIMER_MAX_TASKS; i++)
        if (list[i] == fn)
            return &list[i];
    return NULL;
}

/**
 * Remove a function from the list of timer task to execute.
 * @param fn Function pointer of the timer that was previously added.
 */
void timer_del_task(timer_task_t **list, timer_task_t *fn)
{
    timer_task_t **slot;

    slot = timer_get_task_slot(list, fn);
    if (slot == NULL)
        return;

    __disable_irq();
    *slot = NULL;
    __enable_irq();
}

void timer_add_task(timer_task_t **list, timer_task_t *fn)
{
    timer_task_t **slot;

    log("0x%p", fn);

    // Check if the timer is already configured.
    if (timer_get_task_slot(list, fn) != NULL)
        return;

    slot = timer_get_task_slot(list, NULL);
    app_err(slot == NULL); // misconfiguration of TIMER_MAX_HANDLERS

    __disable_irq();
    *slot = fn;
    __enable_irq();
}

/**
 * Get the time elapsed since timer_start();
 * @return The uptime in seconds.
 */
uint64_t timer_get_uptime_ms(void)
{
    uint64_t uptime_ms;

    __disable_irq();
    uptime_ms = timer_uptime_ms;
    __enable_irq();
    return uptime_ms;
}

void timer_start(void)
{
    static nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG;

    // Prepare the configuration structure.
    timer_config.frequency = NRF_TIMER_FREQ_125kHz;
    timer_config.mode = NRF_TIMER_MODE_TIMER;
    timer_config.bit_width = NRF_TIMER_BIT_WIDTH_8;

    app_err(nrfx_timer_init(&timer, &timer_config, timer_event_handler));

    // Raise an interrupt every 1ms: 125 kHz / 125
    nrfx_timer_extended_compare(&timer, NRF_TIMER_CC_CHANNEL0, 125,
                                NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

    // Start the timer, letting timer_add_task() append more of them while running.
    nrfx_timer_enable(&timer);
}
