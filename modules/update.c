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
#include "py/runtime.h"

STATIC mp_obj_t update_nrf52(void)
{
    monocle_enter_bootloader();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(update_nrf52_obj, update_nrf52);

const struct _mp_obj_type_t fpga_app_type;

static size_t fpga_app_programmed_bytes = 0;

STATIC mp_obj_t update_fpga_app_read(mp_obj_t address, mp_obj_t length)
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
STATIC MP_DEFINE_CONST_FUN_OBJ_2(update_read_fpga_app_obj, update_fpga_app_read);

STATIC mp_obj_t update_fpga_app_write(mp_obj_t bytes)
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
STATIC MP_DEFINE_CONST_FUN_OBJ_1(update_write_fpga_app_obj, update_fpga_app_write);

STATIC mp_obj_t update_fpga_app_delete(void)
{
    for (size_t i = 0; i < 0x6D; i++)
    {
        flash_page_erase(i * 0x1000);
    }

    fpga_app_programmed_bytes = 0;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(update_erase_fpga_app_obj, update_fpga_app_delete);

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

STATIC const mp_rom_map_elem_t update_module_globals_table[] = {

    {MP_ROM_QSTR(MP_QSTR_nrf52), MP_ROM_PTR(&update_nrf52_obj)},
    {MP_ROM_QSTR(MP_QSTR_read_fpga_app), MP_ROM_PTR(&update_read_fpga_app_obj)},
    {MP_ROM_QSTR(MP_QSTR_write_fpga_app), MP_ROM_PTR(&update_write_fpga_app_obj)},
    {MP_ROM_QSTR(MP_QSTR_erase_fpga_app), MP_ROM_PTR(&update_erase_fpga_app_obj)},
};
STATIC MP_DEFINE_CONST_DICT(update_module_globals, update_module_globals_table);

const mp_obj_module_t update_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&update_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR___update, update_module);