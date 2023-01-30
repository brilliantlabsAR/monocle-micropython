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

#include "monocle.h"
#include "py/runtime.h"

STATIC mp_obj_t update_nrf52(void)
{
    monocle_enter_bootloader();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(update_nrf52_obj, update_nrf52);

STATIC const mp_rom_map_elem_t update_module_globals_table[] = {

    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR___update)},
    {MP_ROM_QSTR(MP_QSTR_nrf52), MP_ROM_PTR(&update_nrf52_obj)},
};
STATIC MP_DEFINE_CONST_DICT(update_module_globals, update_module_globals_table);

const mp_obj_module_t update_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&update_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR___update, update_module);