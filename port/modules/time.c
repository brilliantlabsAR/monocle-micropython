/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Glenn Ruben Bakke
 * Copyright (c) 2022 Brilliant Labs Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <time.h>

#include "py/qstr.h"
#include "py/nlr.h"
#include "py/runtime.h"

#include "nrfx_systick.h"

#include "driver/nrfx.h"
#include "driver/timer.h"

uint64_t time_at_init;
uint64_t time_zone_offset;

STATIC mp_obj_t time___init__(void)
{
    nrfx_init();
    timer_init();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(time___init___obj, time___init__);

STATIC mp_obj_t time_epoch(size_t n_args, const mp_obj_t *args)
{
    if (n_args == 0)
        return mp_obj_new_int(time_at_init + timer_get_uptime_ms() / 1000);
    if (n_args == 1)
        time_at_init = mp_obj_get_int(args[0]) - timer_get_uptime_ms() / 1000;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_epoch_obj, 0, 1, time_epoch);

STATIC mp_obj_t time_ticks_ms(void)
{
    return mp_obj_new_int(timer_get_uptime_ms());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(time_ticks_ms_obj, time_ticks_ms);

STATIC mp_obj_t time_sleep(mp_obj_t secs_in)
{
    uint64_t secs = mp_obj_get_int(secs_in);

    nrfx_systick_delay_ms(secs * 1000);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(time_sleep_obj, time_sleep);

STATIC mp_obj_t time_sleep_ms(mp_obj_t msecs_in)
{
    uint64_t msecs = mp_obj_get_int(msecs_in);

    nrfx_systick_delay_ms(msecs);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(time_sleep_ms_obj, time_sleep_ms);

STATIC mp_obj_t time_zone(mp_obj_t hours_in, mp_obj_t minutes_in)
{
    int16_t hours = mp_obj_get_int(hours_in);
    int16_t minutes = mp_obj_get_int(minutes_in);

    time_zone_offset = hours * 3600 + minutes * 60;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(time_zone_obj, time_zone);

static inline bool isleap(int year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

static inline int mdays(int mon, int year)
{
    return (mon == 2) ? (28 + isleap(year)) : (30 + (mon + (mon > 7)) % 2);
}

STATIC mp_obj_t time_time(size_t n_args, const mp_obj_t *args)
{
    mp_obj_t dict_out = mp_obj_new_dict(0);
    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(dict_out);
    time_t t;
    int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, days;

    // get the time to convert from one way or another
    if (n_args == 0)
        t = time_at_init + timer_get_uptime_ms() / 1000;
    else
        t = mp_obj_get_int(args[0]);

    // apply the time zone offset to get local time
    t += time_zone_offset;

    // compute time components
    tm_sec = t % 60; t /= 60;
    tm_min = t % 60; t /= 60;
    tm_hour = t % 24; t /= 24;

    // compute date components
    for (tm_year = 1970; t >= (days = (365 + isleap(tm_year))); tm_year++)
        t -= days;
    for (tm_mon = 1; t >= (days = mdays(tm_mon, tm_year)); tm_mon++)
        t -= days;
    tm_mday = t + 1;

    // fill the fields of a python dict
    mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_sec), MP_OBJ_NEW_SMALL_INT(tm_sec));
    mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_min), MP_OBJ_NEW_SMALL_INT(tm_min));
    mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_hour), MP_OBJ_NEW_SMALL_INT(tm_hour));
    mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_mday), MP_OBJ_NEW_SMALL_INT(tm_mday));
    mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_mon), MP_OBJ_NEW_SMALL_INT(tm_mon));
    mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_year), MP_OBJ_NEW_SMALL_INT(tm_year));

    // return the dict
    return dict_out;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_time_obj, 0, 1, time_time);

STATIC mp_obj_t time_mktime(mp_obj_t dict)
{
    int sec  = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_sec)));
    int min  = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_min)));
    int hour = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_hour)));
    int day  = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_mday)));
    int mon  = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_mon)));
    int year = mp_obj_get_int(mp_obj_dict_get(dict, MP_ROM_QSTR(MP_QSTR_year)));

    // accumulate seconds from the time
    sec = sec + min * 60 + hour * 3600;

    // accumulate seconds from the date
    for (mon--; mon > 0; mon--)
    	day = day + mdays(mon, year);
    day = day - 1
        + year / 400 * 146097
        + year % 400 / 100 * 36524
        + year % 100 / 4 * 1461
        + year % 4 / 1 * 365;
    sec += (day - 719527) * 86400;

    // the result as an integer
    return mp_obj_new_int(sec);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(time_mktime_obj, time_mktime);

STATIC const mp_rom_map_elem_t time_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_time) },
    { MP_ROM_QSTR(MP_QSTR___init__),            MP_ROM_PTR(&time___init___obj) },

    // methods
    { MP_ROM_QSTR(MP_QSTR_epoch),               MP_ROM_PTR(&time_epoch_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_ms),            MP_ROM_PTR(&time_ticks_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep),               MP_ROM_PTR(&time_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep_ms),            MP_ROM_PTR(&time_sleep_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_mktime),              MP_ROM_PTR(&time_mktime_obj) },
    { MP_ROM_QSTR(MP_QSTR_time),                MP_ROM_PTR(&time_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_zone),                MP_ROM_PTR(&time_zone_obj) },
};
STATIC MP_DEFINE_CONST_DICT(time_module_globals, time_module_globals_table);

const mp_obj_module_t time_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&time_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_time, time_module);
