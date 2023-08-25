/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Ltd. (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Ltd.
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

#include "nrf_gpio.h"
#include "nrfx_systick.h"
#include "py/runtime.h"

STATIC mp_obj_t camera_sleep(void)
{
    nrf_gpio_pin_write(CAMERA_SLEEP_PIN, true);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(camera_sleep_obj, camera_sleep);

STATIC mp_obj_t camera_wake(void)
{
    nrf_gpio_pin_write(CAMERA_SLEEP_PIN, false);
    nrfx_systick_delay_ms(5);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(camera_wake_obj, camera_wake);

STATIC const mp_rom_map_elem_t camera_module_globals_table[] = {

    {MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&camera_sleep_obj)},
    {MP_ROM_QSTR(MP_QSTR_wake), MP_ROM_PTR(&camera_wake_obj)},
};
STATIC MP_DEFINE_CONST_DICT(camera_module_globals, camera_module_globals_table);

const mp_obj_module_t camera_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&camera_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR__camera, camera_module);
