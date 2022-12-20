/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2016 Damien P. George
 * Copyright (c) 2015 - 2018 Glenn Ruben Bakke
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

#include "py/runtime.h"

#include "mphalport.h"
#include "mpconfigport.h"
#include "driver_max77654.h"
#include "modules.h"

typedef struct {
    mp_obj_base_t base;
    mp_uint_t id;
    bool bright;
} led_obj_t;

enum {
    BOARD_LED_RED,
    BOARD_LED_GREEN
};

static led_obj_t led_obj[] = {
    {{&led_type}, BOARD_LED_RED, false},
    {{&led_type}, BOARD_LED_GREEN, false},
};

#define NUM_LEDS MP_ARRAY_SIZE(led_obj)

static void led_set(led_obj_t *led_obj, bool state) {
    switch (led_obj->id) {
        case BOARD_LED_RED:
            max77654_led_red(state);
            break;
        case BOARD_LED_GREEN:
            max77654_led_green(state);
            break;
    }
    led_obj->bright = state;
}

// MicroPython bindings

void led_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    led_obj_t *self = self_in;

    mp_printf(print, "LED(%lu, %s)", self->id, self->bright ? "on" : "off");
}

/// \classmethod \constructor(id)
/// Create an LED object associated with the given LED:
///
///   - `id` is the LED number, 1-4.
STATIC mp_obj_t led_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    // get led number
    mp_int_t id = mp_obj_get_int(args[0]);

    // check led number
    if (!(1 <= id && id <= NUM_LEDS)) {
        mp_raise_ValueError(MP_ERROR_TEXT("LED doesn't exist"));
    }

    // return static led object
    return (mp_obj_t)&led_obj[id - 1];
}

/// \method on()
/// Turn the LED on.
mp_obj_t led_on(mp_obj_t self_in) {
    led_obj_t *self = self_in;
    led_set(self, true);
    return mp_const_none;
}

/// \method off()
/// Turn the LED off.
mp_obj_t led_off(mp_obj_t self_in) {
    led_obj_t *self = self_in;
    led_set(self, false);
    return mp_const_none;
}

/// \method toggle()
/// Toggle the LED between on and off.
mp_obj_t led_toggle(mp_obj_t self_in) {
    led_obj_t *self = self_in;

    led_set(self, !self->bright);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_obj_on_obj, led_on);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_obj_off_obj, led_off);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_obj_toggle_obj, led_toggle);

STATIC const mp_rom_map_elem_t led_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&led_obj_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&led_obj_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_toggle), MP_ROM_PTR(&led_obj_toggle_obj) },
};

STATIC MP_DEFINE_CONST_DICT(led_locals_dict, led_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    led_type,
    MP_QSTR_LED,
    MP_TYPE_FLAG_NONE,
    make_new, led_make_new,
    print, led_print,
    locals_dict, &led_locals_dict
);
