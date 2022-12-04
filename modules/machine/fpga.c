/*
 * Copyright (c) 2022 Brilliant Labs
 * Licensed under the MIT License.
 */

#include "py/obj.h"
#include "py/runtime.h"
#include "monocle_fpga.h"
#include "monocle_config.h"
#include "nrfx_log.h"
#include "fpga.h"

typedef struct {
    mp_obj_base_t base;
    uint8_t id;
} machine_fpga_obj_t;

static machine_fpga_obj_t fpga_obj = {{&machine_fpga_type}, 0};

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
    machine_fpga_obj_t *self = MP_OBJ_TO_PTR(self_in);
    (void)kind;

    assert(self->id == 0);
    mp_printf(print, "FPGA(");
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
    mp_printf(print, ")\n");
}

STATIC mp_obj_t machine_fpga_spi_read(mp_obj_t self_in, mp_obj_t addr_in)
{
    machine_fpga_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t addr = mp_obj_get_int(addr_in);

    assert(self->id == 0);
    return mp_obj_new_int(fpga_read_register(addr));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_fpga_spi_read_obj, machine_fpga_spi_read);

STATIC mp_obj_t machine_fpga_spi_write(mp_obj_t self_in, mp_obj_t addr_in, mp_obj_t data_in)
{
#if 0
    machine_fpga_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t addr = mp_obj_get_int(addr_in);
    uint8_t *data = mp_obj_get_int(data_in);

    assert(self->id == 0);
    fpga_set_register(addr, data);
#endif
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(machine_fpga_spi_write_obj, &machine_fpga_spi_write);

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
