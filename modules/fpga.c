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
#include "storage.h"
#include "nrf_gpio.h"
#include "py/runtime.h"

STATIC mp_obj_t fpga_read(mp_obj_t addr_16bit, mp_obj_t n)
{
    if (mp_obj_get_int(n) < 1 || mp_obj_get_int(n) > 255)
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("n must be between 1 and 255"));
    }

    uint16_t addr = mp_obj_get_int(addr_16bit);
    uint8_t addr_bytes[2] = {(uint8_t)(addr >> 8), (uint8_t)addr};

    uint8_t *buffer = m_malloc(mp_obj_get_int(n));

    monocle_spi_write(FPGA, addr_bytes, 2, true);
    monocle_spi_read(FPGA, buffer, mp_obj_get_int(n), false);

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

    uint16_t addr = mp_obj_get_int(addr_16bit);
    uint8_t addr_bytes[2] = {(uint8_t)(addr >> 8), (uint8_t)addr};

    if (n == 0)
    {
        monocle_spi_write(FPGA, addr_bytes, 2, false);
    }
    else
    {
        monocle_spi_write(FPGA, addr_bytes, 2, true);
        monocle_spi_write(FPGA, (uint8_t *)buffer, n, false);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(fpga_write_obj, fpga_write);

/// @brief FPGA application object

const struct _mp_obj_type_t fpga_app_type;

static size_t fpga_app_programmed_bytes = 0;

STATIC mp_obj_t fpga_app_read(mp_obj_t address, mp_obj_t length)
{
    if (mp_obj_get_int(address) + mp_obj_get_int(length) > 0x6C80E + 4)
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("address + length cannot exceed 444434 bytes"));
    }

    uint8_t buffer[mp_obj_get_int(length)];

    flash_read(buffer, mp_obj_get_int(address), mp_obj_get_int(length));

    return mp_obj_new_bytes(buffer, mp_obj_get_int(length));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(fpga_app_read_fun_obj, fpga_app_read);
STATIC MP_DEFINE_CONST_STATICMETHOD_OBJ(fpga_app_read_obj, MP_ROM_PTR(&fpga_app_read_fun_obj));

STATIC mp_obj_t fpga_app_write(mp_obj_t bytes)
{
    size_t length;
    const char *data = mp_obj_str_get_data(bytes, &length);

    if (fpga_app_programmed_bytes + length > 0x6C80E + 4)
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("data will overflow the space reserved for the app"));
    }

    flash_write((uint8_t *)data, fpga_app_programmed_bytes, length);

    fpga_app_programmed_bytes += length;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(fpga_app_write_fun_obj, fpga_app_write);
STATIC MP_DEFINE_CONST_STATICMETHOD_OBJ(fpga_app_write_obj, MP_ROM_PTR(&fpga_app_write_fun_obj));

STATIC mp_obj_t fpga_app_delete(void)
{
    for (size_t i = 0; i < 0x6D; i++)
    {
        flash_page_erase(i * 0x1000);
    }

    fpga_app_programmed_bytes = 0;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(fpga_app_delete_fun_obj, fpga_app_delete);
STATIC MP_DEFINE_CONST_STATICMETHOD_OBJ(fpga_app_delete_obj, MP_ROM_PTR(&fpga_app_delete_fun_obj));

bool fpga_app_exists(void)
{
    // Wakeup the flash
    uint8_t wakeup_device_id[] = {0xAB, 0, 0, 0};
    monocle_spi_write(FLASH, wakeup_device_id, 4, true);
    monocle_spi_read(FLASH, wakeup_device_id, 1, false);
    app_err(wakeup_device_id[0] != 0x13 && not_real_hardware_flag == false);

    uint8_t magic_word[4] = "";
    flash_read(magic_word, 0x6C80E, sizeof(magic_word));

    if (memcmp(magic_word, "done", sizeof(magic_word)) == 0)
    {
        return true;
    }

    return false;
}

STATIC const mp_rom_map_elem_t fpga_app_locals_dict_table[] = {

    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&fpga_app_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&fpga_app_write_obj)},
    {MP_ROM_QSTR(MP_QSTR_delete), MP_ROM_PTR(&fpga_app_delete_obj)},
};
STATIC MP_DEFINE_CONST_DICT(fpga_app_locals_dict, fpga_app_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    fpga_app_type,
    MP_QSTR_App,
    MP_TYPE_FLAG_NONE,
    locals_dict, &fpga_app_locals_dict);

/// @brief Global module definition

STATIC const mp_rom_map_elem_t fpga_module_globals_table[] = {

    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&fpga_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&fpga_write_obj)},
    {MP_ROM_QSTR(MP_QSTR_App), MP_ROM_PTR(&fpga_app_type)},
};
STATIC MP_DEFINE_CONST_DICT(fpga_module_globals, fpga_module_globals_table);

const mp_obj_module_t fpga_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&fpga_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_fpga, fpga_module);
