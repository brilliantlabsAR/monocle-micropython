/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon (name@email.com)
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

#include "driver_fpga.h"
#include "driver_spi.h"
#include "driver_config.h"

#include "machine_fpga.h"

typedef struct {
    mp_obj_base_t base;
} machine_fpga_obj_t;

static machine_fpga_obj_t fpga_obj = {{&machine_fpga_type}};

STATIC mp_obj_t machine_fpga_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    machine_fpga_obj_t *self = &fpga_obj;
    (void)type;
    (void)all_args;

    // Parse args.
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // Return the newly created object.
    return MP_OBJ_FROM_PTR(self);
}

STATIC void machine_fpga_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)self_in;
    (void)kind;

    mp_printf(print, "FPGA(\n");
    fpga_check_reg(FPGA_SYSTEM_CONTROL);
    fpga_check_reg(FPGA_DISPLAY_CONTROL);
    fpga_check_reg(FPGA_MEMORY_CONTROL);
    fpga_check_reg(FPGA_LED_CONTROL);
    fpga_check_reg(FPGA_CAMERA_CONTROL);
    fpga_check_reg(FPGA_SYSTEM_STATUS);
    fpga_check_reg(FPGA_WR_BURST_SIZE_LO);
    fpga_check_reg(FPGA_WR_BURST_SIZE_HI);
    fpga_check_reg(FPGA_BURST_WR_DATA);
    fpga_check_reg(FPGA_RD_BURST_SIZE_LO);
    fpga_check_reg(FPGA_RD_BURST_SIZE_HI);
    fpga_check_reg(FPGA_BURST_RD_DATA);
    fpga_check_reg(FPGA_CAPTURE_CONTROL);
    fpga_check_reg(FPGA_CAPTURE_STATUS);
    fpga_check_reg(FPGA_CAPTURE_SIZE_0);
    fpga_check_reg(FPGA_CAPTURE_SIZE_1);
    fpga_check_reg(FPGA_CAPTURE_SIZE_2);
    fpga_check_reg(FPGA_CAPTURE_SIZE_3);
    fpga_check_reg(FPGA_CAPT_FRM_CHECKSUM_0);
    fpga_check_reg(FPGA_CAPT_FRM_CHECKSUM_1);
    fpga_check_reg(FPGA_REPLAY_RATE_CONTROL);
    fpga_check_reg(FPGA_MIC_CONTROL);
    fpga_check_reg(FPGA_CAPT_BYTE_COUNT_0);
    fpga_check_reg(FPGA_CAPT_BYTE_COUNT_1);
    fpga_check_reg(FPGA_CAPT_BYTE_COUNT_2);
    fpga_check_reg(FPGA_CAPT_BYTE_COUNT_3);
    fpga_check_reg(FPGA_VERSION_MINOR);
    fpga_check_reg(FPGA_VERSION_MAJOR);
    mp_printf(print, ")");
}

STATIC mp_obj_t machine_fpga_spi_read(mp_obj_t self_in, mp_obj_t addr_in)
{
    (void)self_in;
    (void)addr_in;
    // use spi_write instead, which fills the bytearray with the value read.
    return mp_const_notimplemented;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_fpga_spi_read_obj, machine_fpga_spi_read);

STATIC mp_obj_t machine_fpga_spi_write(mp_obj_t self_in, mp_obj_t bytearray_in)
{
    (void)self_in;
    mp_obj_array_t *bytearray = MP_OBJ_TO_PTR(bytearray_in);

    spi_chip_select(SPIM0_FPGA_CS_PIN);
    spi_xfer(bytearray->items, bytearray->len);
    spi_chip_deselect(SPIM0_FPGA_CS_PIN);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(machine_fpga_spi_write_obj, &machine_fpga_spi_write);

STATIC const mp_rom_map_elem_t machine_fpga_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_spi_write),  MP_ROM_PTR(&machine_fpga_spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_spi_read),   MP_ROM_PTR(&machine_fpga_spi_read_obj) },

    { MP_ROM_QSTR(MP_QSTR_FPGA_SYSTEM_CONTROL),      MP_OBJ_NEW_SMALL_INT(FPGA_SYSTEM_CONTROL) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_DISPLAY_CONTROL),     MP_OBJ_NEW_SMALL_INT(FPGA_DISPLAY_CONTROL) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_MEMORY_CONTROL),      MP_OBJ_NEW_SMALL_INT(FPGA_MEMORY_CONTROL) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_LED_CONTROL),         MP_OBJ_NEW_SMALL_INT(FPGA_LED_CONTROL) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_CAMERA_CONTROL),      MP_OBJ_NEW_SMALL_INT(FPGA_CAMERA_CONTROL) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_SYSTEM_STATUS),       MP_OBJ_NEW_SMALL_INT(FPGA_SYSTEM_STATUS) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_WR_BURST_SIZE_LO),    MP_OBJ_NEW_SMALL_INT(FPGA_WR_BURST_SIZE_LO) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_WR_BURST_SIZE_HI),    MP_OBJ_NEW_SMALL_INT(FPGA_WR_BURST_SIZE_HI) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_BURST_WR_DATA),       MP_OBJ_NEW_SMALL_INT(FPGA_BURST_WR_DATA) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_RD_BURST_SIZE_LO),    MP_OBJ_NEW_SMALL_INT(FPGA_RD_BURST_SIZE_LO) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_RD_BURST_SIZE_HI),    MP_OBJ_NEW_SMALL_INT(FPGA_RD_BURST_SIZE_HI) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_BURST_RD_DATA),       MP_OBJ_NEW_SMALL_INT(FPGA_BURST_RD_DATA) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_CAPTURE_CONTROL),     MP_OBJ_NEW_SMALL_INT(FPGA_CAPTURE_CONTROL) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_CAPTURE_STATUS),      MP_OBJ_NEW_SMALL_INT(FPGA_CAPTURE_STATUS) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_CAPTURE_SIZE_0),      MP_OBJ_NEW_SMALL_INT(FPGA_CAPTURE_SIZE_0) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_CAPTURE_SIZE_1),      MP_OBJ_NEW_SMALL_INT(FPGA_CAPTURE_SIZE_1) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_CAPTURE_SIZE_2),      MP_OBJ_NEW_SMALL_INT(FPGA_CAPTURE_SIZE_2) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_CAPTURE_SIZE_3),      MP_OBJ_NEW_SMALL_INT(FPGA_CAPTURE_SIZE_3) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_CAPT_FRM_CHECKSUM_0), MP_OBJ_NEW_SMALL_INT(FPGA_CAPT_FRM_CHECKSUM_0) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_CAPT_FRM_CHECKSUM_1), MP_OBJ_NEW_SMALL_INT(FPGA_CAPT_FRM_CHECKSUM_1) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_REPLAY_RATE_CONTROL), MP_OBJ_NEW_SMALL_INT(FPGA_REPLAY_RATE_CONTROL) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_MIC_CONTROL),         MP_OBJ_NEW_SMALL_INT(FPGA_MIC_CONTROL) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_CAPT_BYTE_COUNT_0),   MP_OBJ_NEW_SMALL_INT(FPGA_CAPT_BYTE_COUNT_0) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_CAPT_BYTE_COUNT_1),   MP_OBJ_NEW_SMALL_INT(FPGA_CAPT_BYTE_COUNT_1) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_CAPT_BYTE_COUNT_2),   MP_OBJ_NEW_SMALL_INT(FPGA_CAPT_BYTE_COUNT_2) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_CAPT_BYTE_COUNT_3),   MP_OBJ_NEW_SMALL_INT(FPGA_CAPT_BYTE_COUNT_3) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_VERSION_MINOR),       MP_OBJ_NEW_SMALL_INT(FPGA_VERSION_MINOR) },
    { MP_ROM_QSTR(MP_QSTR_FPGA_VERSION_MAJOR),       MP_OBJ_NEW_SMALL_INT(FPGA_VERSION_MAJOR) },
};
STATIC MP_DEFINE_CONST_DICT(machine_fpga_locals_dict, machine_fpga_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_fpga_type,
    MP_QSTR_FPGA,
    MP_TYPE_FLAG_NONE,
    make_new, machine_fpga_make_new,
    print, machine_fpga_print,
    locals_dict, &machine_fpga_locals_dict
);
