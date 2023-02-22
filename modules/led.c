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

#include "py/runtime.h"
#include "monocle.h"

enum
{
    LED_RED,
    LED_GREEN
};

static mp_obj_t led_on(mp_obj_t led_in)
{
    qstr led = mp_obj_str_get_qstr(led_in);

    if ((led != MP_QSTR_RED) &&
        (led != MP_QSTR_GREEN))
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("must be led.RED or led.GREEN"));
    }

    if (led == MP_QSTR_RED)
    {
        monocle_set_led(RED_LED, true);
        return mp_const_none;
    }

    monocle_set_led(GREEN_LED, true);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_on_obj, led_on);

static mp_obj_t led_off(mp_obj_t led_in)
{
    qstr led = mp_obj_str_get_qstr(led_in);

    if ((led != MP_QSTR_RED) &&
        (led != MP_QSTR_GREEN))
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("must be led.RED or led.GREEN"));
    }

    if (led == MP_QSTR_RED)
    {
        monocle_set_led(RED_LED, false);
        return mp_const_none;
    }

    monocle_set_led(GREEN_LED, false);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_off_obj, led_off);

STATIC const mp_rom_map_elem_t led_module_globals_table[] = {

    {MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&led_on_obj)},
    {MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&led_off_obj)},
    {MP_ROM_QSTR(MP_QSTR_RED), MP_ROM_QSTR(MP_QSTR_RED)},
    {MP_ROM_QSTR(MP_QSTR_GREEN), MP_ROM_QSTR(MP_QSTR_GREEN)},
};
STATIC MP_DEFINE_CONST_DICT(led_module_globals, led_module_globals_table);

const mp_obj_module_t led_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&led_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_led, led_module);
