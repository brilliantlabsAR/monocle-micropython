/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Inc (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Inc.
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

#pragma once

#include "nrf_timer.h"
#include "mpconfigport.h"

typedef int mp_int_t;
typedef unsigned int mp_uint_t;
typedef long mp_off_t;

int mp_hal_generate_random_seed(void);
void mp_hal_timer_1ms_callback(nrf_timer_event_t event, void *context);
void mp_hal_print_strn(void *env, const char *str, size_t len);
mp_uint_t mp_hal_ticks_ms(void);
mp_uint_t mp_hal_ticks_us(void);
void mp_hal_delay_ms(mp_uint_t ms);
void mp_hal_delay_us(mp_uint_t us);
void mp_hal_set_interrupt_char(char c);
