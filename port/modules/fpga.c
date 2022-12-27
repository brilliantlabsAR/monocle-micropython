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

#include "driver/config.h"
#include "driver/fpga.h"
#include "driver/max77654.h"
#include "driver/spi.h"

STATIC mp_obj_t mod_fpga___init__(void)
{
    // dependencies:
    fpga_init();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_fpga___init___obj, mod_fpga___init__);

static inline void write_addr(uint16_t addr)
{
    uint8_t buf[] = { addr >> 0, addr >> 8, };
    spi_write(buf, sizeof buf);
}

STATIC mp_obj_t fpga_read(mp_obj_t addr_in, mp_obj_t len_in)
{
    uint16_t addr = mp_obj_get_int(addr_in);
    size_t len = mp_obj_get_int(len_in);

    // Allocate a buffer for reading data into
    uint8_t *out_data = m_malloc(len);

    // Create a list where we'll return the bytes
    mp_obj_t return_list = mp_obj_new_list(0, NULL);

    // Read on the SPI using the command and address given
    spi_chip_select(SPIM0_FPGA_CS_PIN);
    write_addr(addr);
    spi_read(out_data, len);
    spi_chip_deselect(SPIM0_FPGA_CS_PIN);

    // Copy the read bytes into the list object
    for (size_t i = 0; i < len; i++)
    {
        mp_obj_list_append(return_list, MP_OBJ_NEW_SMALL_INT(out_data[i]));
    }

    // Free the temporary buffer
    m_free(out_data);

    // Return the list
    return MP_OBJ_FROM_PTR(return_list);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(fpga_read_obj, fpga_read);

STATIC mp_obj_t fpga_write(mp_obj_t addr_in, mp_obj_t list_in)
{
    uint16_t addr = mp_obj_get_int(addr_in);

    // Extract the buffer of elements and size from the python object.
    size_t len = 0;
    mp_obj_t *list = NULL;
    mp_obj_list_get(list_in, &len, &list);

    // Create a contiguous region with the bytes to read in.
    uint8_t *in_data = m_malloc(len);

    // Copy the write bytes into the a continuous buffer
    for (size_t i = 0; i < len; i++)
    {
        in_data[i] = mp_obj_get_int(list[i]);
    }

    spi_chip_select(SPIM0_FPGA_CS_PIN);
    write_addr(addr);
    spi_write(in_data, len);
    spi_chip_deselect(SPIM0_FPGA_CS_PIN);

    // Free the temporary buffer
    m_free(in_data);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(fpga_write_obj, &fpga_write);

STATIC mp_obj_t fpga_status(void)
{
    return MP_OBJ_NEW_SMALL_INT(fpga_system_id());
}
MP_DEFINE_CONST_FUN_OBJ_0(fpga_status_obj, &fpga_status);

STATIC const mp_rom_map_elem_t fpga_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_fpga) },
    { MP_ROM_QSTR(MP_QSTR___init__),    MP_ROM_PTR(&mod_fpga___init___obj) },

    // methods
    { MP_ROM_QSTR(MP_QSTR_write),       MP_ROM_PTR(&fpga_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),        MP_ROM_PTR(&fpga_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_status),      MP_ROM_PTR(&fpga_status_obj) },
};
STATIC MP_DEFINE_CONST_DICT(fpga_module_globals, fpga_module_globals_table);

const mp_obj_module_t fpga_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&fpga_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_fpga, fpga_module);
