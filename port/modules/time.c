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

#include "py/qstr.h"
#include "py/nlr.h"
#include "py/runtime.h"

#include "nrfx_systick.h"

#include "driver/nrfx.h"
#include "driver/timer.h"

uint64_t time_at_init;

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
    {
        return mp_obj_new_int(time_at_init + timer_get_uptime_s());
    }
    if (n_args == 1)
    {
        time_at_init = mp_obj_get_int(args[0]) - timer_get_uptime_s();
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_epoch_obj, 0, 1, time_epoch);

STATIC const mp_rom_map_elem_t time_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_time) },
    { MP_ROM_QSTR(MP_QSTR___init__),            MP_ROM_PTR(&time___init___obj) },

    // methods
    { MP_ROM_QSTR(MP_QSTR_epoch),               MP_ROM_PTR(&time_epoch_obj) },
};
STATIC MP_DEFINE_CONST_DICT(time_module_globals, time_module_globals_table);

const mp_obj_module_t time_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&time_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_time, time_module);
