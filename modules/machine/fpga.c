/*
 * Copyright (c) 2022 Brilliant Labs
 * Licensed under the MIT License.
 */

#include "py/obj.h"
#include "py/runtime.h"
#include "monocle_fpga.h"

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

MP_DEFINE_CONST_FUN_OBJ_1(machine_fpga_read_byte_obj, &machine_fpga_read_byte);
MP_DEFINE_CONST_FUN_OBJ_2(machine_fpga_write_byte_obj, &machine_fpga_write_byte);
