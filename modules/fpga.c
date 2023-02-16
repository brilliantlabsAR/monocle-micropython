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

STATIC mp_obj_t fpga_read(mp_obj_t addr_16bit, mp_obj_t n)
{
    if (mp_obj_get_int(n) < 1 || mp_obj_get_int(n) > 255)
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("n must be between 1 and 255"));
    }

    // TODO
    // if (app_fpga_get_power_state() == false)
    // {
    //     mp_raise_msg(&mp_type_OSError, "FPGA is not powered");
    // }

    uint16_t addr = mp_obj_get_int(addr_16bit);
    uint8_t addr_bytes[2] = {(uint8_t)(addr >> 8), (uint8_t)addr};

    uint8_t *buffer = m_malloc(mp_obj_get_int(n));

    spi_write(FPGA, addr_bytes, 2, true);
    spi_read(FPGA, buffer, mp_obj_get_int(n));

    mp_obj_t bytes = mp_obj_new_bytes(buffer, mp_obj_get_int(n));

    m_free(buffer);

    return bytes;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(fpga_read_obj, fpga_read);

STATIC mp_obj_t fpga_write(mp_obj_t addr_16bit, mp_obj_t bytes)
{
    size_t n;
    const char *buffer = mp_obj_str_get_data(bytes, &n);

    if (n > 255)
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("input buffer size must be less than 255 bytes"));
    }

    // TODO
    // if (app_fpga_get_power_state() == false)
    // {
    //     mp_raise_msg(&mp_type_OSError, "FPGA is not powered");
    // }

    uint16_t addr = mp_obj_get_int(addr_16bit);
    uint8_t addr_bytes[2] = {(uint8_t)(addr >> 8), (uint8_t)addr};

    if (n == 0)
    {
        spi_write(FPGA, addr_bytes, 2, false);
    }
    else
    {
        spi_write(FPGA, addr_bytes, 2, true);
        spi_write(FPGA, (uint8_t *)buffer, n, false);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(fpga_write_obj, fpga_write);

STATIC mp_obj_t fpga_power(size_t n_args, const mp_obj_t *args)
{
    // TODO
    bool current_power_state = true; // app_fpga_get_power_state();

    if (n_args == 0)
    {
        if (current_power_state)
        {
            return MP_OBJ_NEW_QSTR(MP_QSTR_ON);
        }

        return MP_OBJ_NEW_QSTR(MP_QSTR_OFF);
    }

    if (mp_obj_get_type(args[0]) != &mp_type_bool)
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("input argument must be either True or False"));
    }

    bool new_power_state = (bool)mp_obj_get_int(args[0]);

    if (new_power_state == current_power_state)
    {
        return mp_const_none;
    }

    // TODO
    // app_fpga_set_power_state(new_power_state);

    if (new_power_state == true)
    {
        // TODO
        // app_fpga_boot_stored_bitstream();
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(fpga_power_obj, 0, 1, fpga_power);

STATIC mp_obj_t fpga_status(void)
{
    // TODO
    // if (app_fpga_get_power_state() == false)
    // {
    // return MP_OBJ_NEW_QSTR(MP_QSTR_NOT_POWERED);
    // }

    // if (gpio_get_level(fpga_configuration_done_pin))
    // {
    return MP_OBJ_NEW_QSTR(MP_QSTR_RUNNING);
    // }
    // return MP_OBJ_NEW_QSTR(MP_QSTR_BAD_BITSTREAM);
}
MP_DEFINE_CONST_FUN_OBJ_0(fpga_status_obj, &fpga_status);

STATIC const mp_rom_map_elem_t fpga_module_globals_table[] = {

    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&fpga_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&fpga_write_obj)},
    {MP_ROM_QSTR(MP_QSTR_power), MP_ROM_PTR(&fpga_power_obj)},
    {MP_ROM_QSTR(MP_QSTR_status), MP_ROM_PTR(&fpga_status_obj)},
};
STATIC MP_DEFINE_CONST_DICT(fpga_module_globals, fpga_module_globals_table);

const mp_obj_module_t fpga_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&fpga_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_fpga, fpga_module);
