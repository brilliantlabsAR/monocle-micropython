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

#include <time.h>
#include <stdlib.h>

#include "py/runtime.h"
#include "shared/timeutils/timeutils.h"
#include "extmod/utime_mphal.h"
#include "nrfx_systick.h"
#include "mphalport.h"

uint64_t time_since_boot;
uint64_t time_zone_offset;

STATIC mp_obj_t time_now(size_t n_args, const mp_obj_t *args)
{
    if (n_args == 0)
    {
        return mp_obj_new_int(time_since_boot + mp_hal_ticks_ms() / 1000);
    }
    else
    {
        mp_int_t now_s = mp_obj_get_int(args[0]);
        mp_int_t uptime_s = mp_hal_ticks_ms() / 1000;
        if (now_s < uptime_s)
        {
            mp_raise_ValueError(MP_ERROR_TEXT("time too low"));
        }
        time_since_boot = now_s - uptime_s;
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_now_obj, 0, 1, time_now);

STATIC mp_obj_t time_zone(size_t n_args, const mp_obj_t *args)
{
    if (n_args == 0)
    {
        return mp_obj_new_int(time_zone_offset);
    }
    else
    {
        char const *s =  mp_obj_str_get_str(args[1]);
        char *end = NULL;

        long hours = strtol(s, &end, 10);
        if (end == s || *end != ':')
        {
            mp_raise_ValueError(MP_ERROR_TEXT("invalid hour format"));
        }

        s = end + 1;

        long minutes = strtol(s, &end, 10);
        if (end == s || *end != '\0' || minutes < 0)
        {
            mp_raise_ValueError(MP_ERROR_TEXT("invalid minute format"));
        }

        time_zone_offset = hours * 3600 + minutes * 60;
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_zone_obj, 0, 1, time_zone);

STATIC mp_obj_t time_time(size_t n_args, const mp_obj_t *args)
{
    mp_int_t seconds;

    // Get current date and time.
    if (n_args == 0 || args[0] == mp_const_none)
    {
        seconds = time_since_boot + mp_hal_ticks_ms() / 1000;
    }
    else
    {
        seconds = mp_obj_get_int(args[0]);
    }

    // Convert given seconds to tuple.
    timeutils_struct_time_t tm;
    timeutils_seconds_since_epoch_to_struct_time(seconds, &tm);
    mp_obj_t tuple[8] = {
        tuple[0] = mp_obj_new_int(tm.tm_year),
        tuple[1] = mp_obj_new_int(tm.tm_mon),
        tuple[2] = mp_obj_new_int(tm.tm_mday),
        tuple[3] = mp_obj_new_int(tm.tm_hour),
        tuple[4] = mp_obj_new_int(tm.tm_min),
        tuple[5] = mp_obj_new_int(tm.tm_sec),
        tuple[6] = mp_obj_new_int(tm.tm_wday),
        tuple[7] = mp_obj_new_int(tm.tm_yday),
    };
    return mp_obj_new_tuple(8, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_time_obj, 0, 1, time_time);

STATIC mp_obj_t time_mktime(mp_obj_t dict)
{
    if (!mp_obj_is_type(dict, &mp_type_dict))
    {
        mp_raise_TypeError(MP_ERROR_TEXT("argument must be a dict"));
    }
    mp_int_t year = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_year)));
    mp_int_t mon  = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_mon)));
    mp_int_t mday = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_mday)));
    mp_int_t hour = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_hour)));
    mp_int_t min  = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_min)));
    mp_int_t sec  = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_sec)));
    return mp_obj_new_int(timeutils_mktime(year, mon, mday, hour, min, sec));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(time_mktime_obj, time_mktime);

STATIC const mp_rom_map_elem_t time_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__),     MP_ROM_QSTR(MP_QSTR_time)},

    {MP_ROM_QSTR(MP_QSTR_mktime),       MP_ROM_PTR(&time_mktime_obj)},
    {MP_ROM_QSTR(MP_QSTR_time),         MP_ROM_PTR(&time_time_obj)},
    {MP_ROM_QSTR(MP_QSTR_zone),         MP_ROM_PTR(&time_zone_obj)},
    {MP_ROM_QSTR(MP_QSTR_now),          MP_ROM_PTR(&time_now_obj)},
    {MP_ROM_QSTR(MP_QSTR_sleep),        MP_ROM_PTR(&mp_utime_sleep_obj)},
    {MP_ROM_QSTR(MP_QSTR_sleep_ms),     MP_ROM_PTR(&mp_utime_sleep_ms_obj)},
    {MP_ROM_QSTR(MP_QSTR_ticks_add),    MP_ROM_PTR(&mp_utime_ticks_add_obj)},
    {MP_ROM_QSTR(MP_QSTR_ticks_cpu),    MP_ROM_PTR(&mp_utime_ticks_cpu_obj)},
    {MP_ROM_QSTR(MP_QSTR_ticks_diff),   MP_ROM_PTR(&mp_utime_ticks_diff_obj)},
    {MP_ROM_QSTR(MP_QSTR_ticks_ms),     MP_ROM_PTR(&mp_utime_ticks_ms_obj)},
};
STATIC MP_DEFINE_CONST_DICT(time_module_globals, time_module_globals_table);

const mp_obj_module_t time_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&time_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_time, time_module);
