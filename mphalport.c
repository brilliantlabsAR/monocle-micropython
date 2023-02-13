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

#include "py/builtin.h"
#include "py/mperrno.h"
#include "py/lexer.h"
#include "py/runtime.h"
#include "mpconfigport.h"
#include "nrfx_rtc.h"

const char help_text[] = {
    "Welcome to MicroPython!\n\n"
    "For full documentation, visit: https://docs.brilliantmonocle.com\n"
    "Control commands:\n"
    "  Ctrl-A - enter raw REPL mode\n"
    "  Ctrl-B - enter normal REPL mode\n"
    "  CTRL-C - interrupt a running program\n"
    "  Ctrl-D - reset the device\n"
    "  Ctrl-E - enter paste mode\n\n"
    "To list available modules, type help('modules')\n"
    "For details on a specific module, import it, and then type "
    "help(module_name)\n"};

static nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(1);

static bool rtc_wakeup_flag;

mp_uint_t mp_hal_ticks_ms(void)
{
    uint32_t value = nrfx_rtc_counter_get(&rtc);

    // Correct for the slightly faster tick frequency of 1024Hz
    float ms = (float)value / 1024 * 1000;

    return (mp_uint_t)ms;
}

mp_uint_t mp_hal_ticks_cpu(void)
{
    // This doesn't seem to be used by anything so it's not implemented
    return 0;
}

void rtc_event_handler(nrfx_rtc_int_type_t int_type)
{
    rtc_wakeup_flag = true;
}

void mp_hal_delay_ms(mp_uint_t ms)
{
    nrfx_rtc_cc_set(&rtc,
                    0,
                    nrfx_rtc_counter_get(&rtc) + (ms * 1000 / 1024),
                    true);

    rtc_wakeup_flag = false;

    while (!rtc_wakeup_flag)
    {
        MICROPY_EVENT_POLL_HOOK;
    }
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs)
{
    // File opening is currently not supported
    mp_raise_OSError(MP_EPERM);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

mp_lexer_t *mp_lexer_new_from_file(const char *filename)
{
    // File opening is currently not supported
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path)
{
    // File opening is currently not supported
    return MP_IMPORT_STAT_NO_EXIST;
}

int mp_hal_generate_random_seed(void)
{
    return 0;
}
