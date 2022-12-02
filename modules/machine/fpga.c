/*
 * Copyright (c) 2022 Brilliant Labs
 * Licensed under the MIT License.
 */

#include "py/obj.h"
#include "py/runtime.h"
#include "monocle_fpga.h"
#include "monocle_config.h"
#include "nrfx_log.h"

STATIC mp_obj_t machine_fpga_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    (void)type;
    (void)all_args;

    // Parse args.
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

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

    // Return the newly created object.
    return MP_OBJ_NEW_SMALL_INT(0);
}

STATIC void machine_fpga_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)self_in;
    (void)kind;

    mp_printf(print, "FPGA()\n");
}

STATIC mp_obj_t machine_fpga_read_byte(mp_obj_t self_in, mp_obj_t addr_in)
{
    uint8_t self = mp_obj_get_int(self_in);
    uint8_t addr = mp_obj_get_int(addr_in);
    (void)self;

    return mp_obj_new_int(fpga_get_register(addr));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_fpga_read_byte_obj, machine_fpga_read_byte);

STATIC mp_obj_t machine_fpga_write_byte(mp_obj_t self_in, mp_obj_t addr_in, mp_obj_t data_in)
{
    uint8_t self = mp_obj_get_int(self_in);
    uint8_t addr = mp_obj_get_int(addr_in);
    uint8_t data = mp_obj_get_int(data_in);
    (void)self;

    fpga_set_register(addr, data);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(machine_fpga_write_byte_obj, &machine_fpga_write_byte);

STATIC const mp_rom_map_elem_t machine_fpga_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_write_byte),  MP_ROM_PTR(&machine_fpga_write_byte_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_byte),   MP_ROM_PTR(&machine_fpga_read_byte_obj) },
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
