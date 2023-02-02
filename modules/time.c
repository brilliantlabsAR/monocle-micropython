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

#include <stdlib.h>
#include <stdio.h>

#include "py/runtime.h"
#include "shared/timeutils/timeutils.h"
#include "extmod/utime_mphal.h"
#include "nrfx_systick.h"
#include "mphalport.h"

uint64_t time_at_boot_s;
static int8_t time_zone_hour_offset;
static uint8_t time_zone_minute_offset;

STATIC uint64_t _gettime(void)
{
    return time_at_boot_s + mp_hal_ticks_ms() / 1000;
}

STATIC mp_obj_t time_zone(size_t n_args, const mp_obj_t *args)
{
    if (n_args == 0)
    {
        char timezone_string[] = "+00:00";
        snprintf(timezone_string,
                sizeof timezone_string,
                "%02d:%02u",
                time_zone_hour_offset,
                time_zone_minute_offset);

        return mp_obj_new_str(timezone_string, strlen(timezone_string));
    }

    int hour = 0;
    int minute = 0;

    if (sscanf(mp_obj_str_get_str(args[0]), "%d:%d", &hour, &minute) != 2)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("must be '+hh:mm' or '-hh:mm'"));
    }

    if (hour < -12.0 || hour > 14.0)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("hour value must be between -12 and +14"));
    }

    if (minute != 0 && minute != 30 && minute != 45)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("minute value must be either 00, 30, or 45"));
    }

    if ((hour == -12.0 || hour == 14.0) && minute != 0.0)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("when hour is -12 or 14, minutes must be 0"));
    }

    time_zone_hour_offset = hour;
    time_zone_minute_offset = minute;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_zone_obj, 0, 1, time_zone);

STATIC mp_obj_t time_now(size_t n_args, const mp_obj_t *args)
{
    timeutils_struct_time_t tm;
    mp_int_t now;

    if (n_args == 0)
    {
        now = _gettime();
    }
    else if (n_args == 1)
    {
        if (mp_obj_get_int(args[0]) < 0)
        {
            mp_raise_ValueError(MP_ERROR_TEXT("value given must be positive"));
        }
        now = mp_obj_get_int(args[0]);
    }

    now += time_zone_hour_offset;
    if (time_zone_hour_offset >= 0)
    {
        now += time_zone_minute_offset;
    }
    else
    {
        now -= time_zone_minute_offset;
    }

    timeutils_seconds_since_epoch_to_struct_time(now, &tm);

    mp_obj_dict_t *dict = mp_obj_new_dict(0);

    mp_obj_dict_store(dict,
                      MP_ROM_QSTR(MP_QSTR_year),
                      mp_obj_new_int(tm.tm_year));

    mp_obj_dict_store(dict,
                      MP_ROM_QSTR(MP_QSTR_year),
                      mp_obj_new_int(tm.tm_year));

    mp_obj_dict_store(dict,
                      MP_ROM_QSTR(MP_QSTR_month),
                      mp_obj_new_int(tm.tm_mon));

    mp_obj_dict_store(dict,
                      MP_ROM_QSTR(MP_QSTR_day),
                      mp_obj_new_int(tm.tm_mday));

    mp_obj_dict_store(dict,
                      MP_ROM_QSTR(MP_QSTR_hour),
                      mp_obj_new_int(tm.tm_hour));

    mp_obj_dict_store(dict,
                      MP_ROM_QSTR(MP_QSTR_minute),
                      mp_obj_new_int(tm.tm_min));

    mp_obj_dict_store(dict,
                      MP_ROM_QSTR(MP_QSTR_second),
                      mp_obj_new_int(tm.tm_sec));

    qstr weekday;

    switch (tm.tm_wday)
    {
    case 0:
        weekday = MP_QSTR_monday;
        break;

    case 1:
        weekday = MP_QSTR_tuesday;
        break;

    case 2:
        weekday = MP_QSTR_wednesday;
        break;

    case 3:
        weekday = MP_QSTR_thursday;
        break;

    case 4:
        weekday = MP_QSTR_friday;
        break;

    case 5:
        weekday = MP_QSTR_saturday;
        break;

    case 6:
        weekday = MP_QSTR_sunday;
        break;

    default:
        weekday = MP_QSTR_;
        break;
    }

    mp_obj_dict_store(dict,
                      MP_ROM_QSTR(MP_QSTR_weekday),
                      MP_OBJ_NEW_QSTR(weekday));

    mp_obj_dict_store(dict,
                      MP_ROM_QSTR(MP_QSTR_yearday),
                      mp_obj_new_int(tm.tm_yday));

    char timezone_string[] = "+00:00";
    snprintf(timezone_string,
            sizeof(timezone_string),
            "%02d:%02u",
            time_zone_hour_offset,
            time_zone_minute_offset);

    mp_obj_dict_store(dict,
                      MP_ROM_QSTR(MP_QSTR_timezone),
                      mp_obj_new_str(timezone_string, strlen(timezone_string)));

    return MP_OBJ_FROM_PTR(dict);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_now_obj, 0, 1, time_now);

STATIC mp_obj_t time_time(size_t n_args, const mp_obj_t *args)
{
    if (n_args == 0)
    {
        return mp_obj_new_int(_gettime());
    }

    mp_int_t now_s = mp_obj_get_int(args[0]);
    mp_int_t uptime_s = mp_hal_ticks_ms() / 1000;

    if (now_s < uptime_s)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("value given must be positive"));
    }
    time_at_boot_s = now_s - uptime_s;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_time_obj, 0, 1, time_time);

STATIC mp_obj_t time_mktime(mp_obj_t dict)
{
    mp_int_t year, mon, mday, hour, min, sec;

    if (!mp_obj_is_type(dict, &mp_type_dict))
    {
        mp_raise_TypeError(MP_ERROR_TEXT("argument must be a dict"));
    }

    year = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_year)));
    mon  = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_month)));
    mday = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_day)));
    hour = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_hour)));
    min  = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_minute)));
    sec  = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_second)));

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
