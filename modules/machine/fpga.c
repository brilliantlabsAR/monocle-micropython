/*
 * Copyright (c) 2022 Brilliant Labs
 * Licensed under the MIT License.
 */

#include "py/obj.h"
#include "py/runtime.h"
#include "monocle_fpga.h"

STATIC mp_obj_t machine_fpga_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    (void)type;
    (void)all_args;

    // Parse args.
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // Return the newly created object.
    return MP_OBJ_FROM_PTR(NULL);
}

STATIC void machine_fpga_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)self_in;
    (void)kind;

    mp_printf(print, "FPGA()");
}

STATIC mp_obj_t machine_fpga_write_byte(mp_obj_t addr_obj, mp_obj_t byte_obj)
{
    uint8_t addr = mp_obj_get_int(addr_obj);
    uint8_t byte = mp_obj_get_int(byte_obj);

    fpga_write_byte(addr, byte);
    return mp_const_none;
}

STATIC mp_obj_t machine_fpga_read_byte(mp_obj_t addr_obj)
{
    uint8_t addr = mp_obj_get_int(addr_obj);
    uint8_t byte;

    byte = fpga_read_byte(addr);
    return mp_obj_new_int(byte);
}

STATIC mp_obj_t machine_fpga_test(void)
{
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(machine_fpga_read_byte_obj, &machine_fpga_read_byte);
MP_DEFINE_CONST_FUN_OBJ_2(machine_fpga_write_byte_obj, &machine_fpga_write_byte);
MP_DEFINE_CONST_FUN_OBJ_0(machine_fpga_test_obj, &machine_fpga_test);

STATIC const mp_rom_map_elem_t machine_fpga_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_write_byte),  MP_ROM_PTR(&machine_fpga_write_byte_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_byte),   MP_ROM_PTR(&machine_fpga_read_byte_obj) },
    { MP_ROM_QSTR(MP_QSTR_test),        MP_ROM_PTR(&machine_fpga_test_obj) },
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
