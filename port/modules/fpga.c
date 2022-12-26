/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon <me@josuah.net>
 *
 * ISC Licence
 *
 * Copyright © 2022 Brilliant Labs Inc.
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

#include <stddef.h>

#include "py/obj.h"
#include "py/objarray.h"
#include "py/runtime.h"

#include "nrfx_log.h"

#include "driver/fpga.h"
#include "driver/spi.h"
#include "driver/config.h"

static inline void write_addr(mp_obj_t obj)
{
    uint8_t buf[] = {
        mp_obj_get_int(obj) >> 0,
        mp_obj_get_int(obj) >> 8,
    };
    spi_write(buf, sizeof buf);
}

STATIC mp_obj_t fpga_read(mp_obj_t addr_in, mp_obj_t bytearray_in)
{
    mp_obj_array_t *bytearray = MP_OBJ_TO_PTR(bytearray_in);

    spi_chip_select(SPIM0_FPGA_CS_PIN);
    write_addr(addr_in);
    spi_read(bytearray->items, bytearray->len);
    spi_chip_deselect(SPIM0_FPGA_CS_PIN);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(fpga_read_obj, fpga_read);

STATIC mp_obj_t fpga_write(mp_obj_t addr_in, mp_obj_t bytearray_in)
{
    mp_obj_array_t *bytearray = MP_OBJ_TO_PTR(bytearray_in);

    spi_chip_select(SPIM0_FPGA_CS_PIN);
    write_addr(addr_in);
    spi_write(bytearray->items, bytearray->len);
    spi_chip_deselect(SPIM0_FPGA_CS_PIN);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(fpga_write_obj, &fpga_write);

STATIC mp_obj_t fpga_status(void)
{
    return MP_OBJ_NEW_SMALL_INT(fpga_system_id());
}
MP_DEFINE_CONST_FUN_OBJ_0(fpga_status_obj, &fpga_status);

STATIC const mp_rom_map_elem_t fpga_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_write),  MP_ROM_PTR(&fpga_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),   MP_ROM_PTR(&fpga_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_status),     MP_ROM_PTR(&fpga_status_obj) },
};
STATIC MP_DEFINE_CONST_DICT(fpga_module_globals, fpga_module_globals_table);

const mp_obj_module_t fpga_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&fpga_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_fpga, fpga_module);
