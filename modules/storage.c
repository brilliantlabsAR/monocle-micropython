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

#include <string.h>
#include <math.h>
#include "monocle.h"
#include "extmod/vfs.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/runtime.h"

/// @brief Low level helpers

static bool flash_is_busy(void)
{
    uint8_t status_cmd[] = {0x05};
    monocle_spi_write(FLASH, status_cmd, sizeof(status_cmd), true);
    monocle_spi_read(FLASH, status_cmd, sizeof(status_cmd), false);

    if ((status_cmd[0] & 0x01) == 0)
    {
        return false;
    }

    return true;
}

static void flash_read(uint8_t *buffer, size_t address, size_t length)
{
    if (address + length > 0x100000)
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("address + length cannot exceed 1048576 bytes"));
    }

    while (flash_is_busy())
    {
        mp_hal_delay_ms(1);
    }

    uint8_t read_cmd[] = {0x03,
                          address >> 16,
                          address >> 8,
                          address};
    monocle_spi_write(FLASH, read_cmd, sizeof(read_cmd), true);

    size_t bytes_read = 0;
    while (bytes_read < length)
    {
        // nRF DMA can only handle 255 bytes at a time
        size_t max_readable_length = MIN(length - bytes_read, 255);

        // If we're going to need another transfer, keep cs held
        bool hold = length - bytes_read > 255;

        monocle_spi_read(FLASH, buffer + bytes_read, max_readable_length, hold);

        bytes_read += max_readable_length;
    }
}

static void flash_write(uint8_t *buffer, size_t address, size_t length)
{
    if (address + length > 0x100000)
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("address + length cannot exceed 1048576 bytes"));
    }

    NRFX_LOG("Writing %u bytes to flash", length);

    size_t bytes_written = 0;
    while (bytes_written < length)
    {
        size_t bytes_left_in_page_from_address_offset = 256 - (address % 256);
        size_t bytes_left_to_write = length - bytes_written;

        size_t max_writable_length = MIN(bytes_left_in_page_from_address_offset,
                                         bytes_left_to_write);

        // nRF DMA can only handle 255 bytes at a time
        max_writable_length = MIN(max_writable_length, 255);

        while (flash_is_busy())
        {
            mp_hal_delay_ms(1);
        }

        uint8_t write_enable_cmd[] = {0x06};
        monocle_spi_write(FLASH, write_enable_cmd, sizeof(write_enable_cmd), false);

        size_t address_offset = address + bytes_written;
        NRFX_LOG(" - %u bytes from 0x%08x", max_writable_length, address_offset);

        uint8_t page_program_cmd[] = {0x02,
                                      address_offset >> 16,
                                      address_offset >> 8,
                                      address_offset};
        monocle_spi_write(FLASH, page_program_cmd, sizeof(page_program_cmd), true);

        // If we're going to need another transfer, keep cs held
        bool hold = bytes_left_to_write > 255;

        monocle_spi_write(FLASH, buffer + bytes_written, max_writable_length, hold);

        bytes_written += max_writable_length;
    }
}

static void flash_page_erase(size_t address)
{
    if (address % 0x1000)
    {
        mp_raise_ValueError(MP_ERROR_TEXT(
            "address must be aligned to a page size of 4096 bytes"));
    }

    while (flash_is_busy())
    {
        mp_hal_delay_ms(1);
    }

    uint8_t write_enable_cmd[] = {0x06};
    monocle_spi_write(FLASH, write_enable_cmd, sizeof(write_enable_cmd), false);

    uint8_t sector_erase[] = {0x20,
                              address >> 16,
                              address >> 8,
                              address};
    monocle_spi_write(FLASH, sector_erase, sizeof(sector_erase), false);
}

/// @brief SPI flash partition object

const struct _mp_obj_type_t storage_flash_device_type;

typedef struct _storage_obj_t
{
    mp_obj_base_t base;
    uint32_t start;
    uint32_t len;
} storage_obj_t;

mp_obj_t storage_readblocks(size_t n_args, const mp_obj_t *args)
{
    storage_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    uint32_t block_num = mp_obj_get_int(args[1]);
    mp_obj_t buffer = args[2];

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buffer, &bufinfo, MP_BUFFER_WRITE);

    mp_int_t address = self->start + (block_num * 4096);

    if (n_args == 4)
    {
        uint32_t offset = mp_obj_get_int(args[3]);
        address += offset;
    }

    flash_read(bufinfo.buf, address, bufinfo.len);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(storage_readblocks_obj, 3, 4, storage_readblocks);

mp_obj_t storage_writeblocks(size_t n_args, const mp_obj_t *args)
{
    storage_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    uint32_t block_num = mp_obj_get_int(args[1]);
    mp_obj_t buffer = args[2];

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buffer, &bufinfo, MP_BUFFER_WRITE);

    mp_int_t address = self->start + (block_num * 4096);

    if (n_args == 4)
    {
        uint32_t offset = mp_obj_get_int(args[3]);
        address += offset;
    }
    else
    {
        flash_page_erase(address);
    }

    flash_write(bufinfo.buf, address, bufinfo.len);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(storage_writeblocks_obj, 3, 4, storage_writeblocks);

mp_obj_t storage_ioctl(mp_obj_t self_in, mp_obj_t op_in, mp_obj_t arg_in)
{
    storage_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_int_t op = mp_obj_get_int(op_in);
    switch (op)
    {
    case MP_BLOCKDEV_IOCTL_INIT:
    {
        return MP_OBJ_NEW_SMALL_INT(0);
    }

    case MP_BLOCKDEV_IOCTL_DEINIT:
    {
        return MP_OBJ_NEW_SMALL_INT(0);
    }

    case MP_BLOCKDEV_IOCTL_SYNC:
    {
        return MP_OBJ_NEW_SMALL_INT(0);
    }

    case MP_BLOCKDEV_IOCTL_BLOCK_COUNT:
    {
        return MP_OBJ_NEW_SMALL_INT(self->len / 4096);
    }

    case MP_BLOCKDEV_IOCTL_BLOCK_SIZE:
    {
        return MP_OBJ_NEW_SMALL_INT(4096);
    }

    case MP_BLOCKDEV_IOCTL_BLOCK_ERASE:
    {
        mp_int_t block_num = mp_obj_get_int(arg_in);
        mp_int_t address = self->start + (block_num * 4096);

        if ((address & 0x3) || (address % 4096 != 0))
        {
            return MP_OBJ_NEW_SMALL_INT(-MP_EIO);
        }

        flash_page_erase(address);
        return MP_OBJ_NEW_SMALL_INT(0);
    }

    default:
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(storage_ioctl_obj, storage_ioctl);

STATIC const mp_rom_map_elem_t storage_locals_dict_table[] = {

    {MP_ROM_QSTR(MP_QSTR_readblocks), MP_ROM_PTR(&storage_readblocks_obj)},
    {MP_ROM_QSTR(MP_QSTR_writeblocks), MP_ROM_PTR(&storage_writeblocks_obj)},
    {MP_ROM_QSTR(MP_QSTR_ioctl), MP_ROM_PTR(&storage_ioctl_obj)},
};
STATIC MP_DEFINE_CONST_DICT(storage_locals_dict, storage_locals_dict_table);

STATIC void storage_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    storage_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "Storage(start=0x%08x, len=%u)", self->start, self->len);
}

STATIC mp_obj_t storage_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_start, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0x6D000}},
        {MP_QSTR_length, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0x93000}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t start = args[0].u_int;
    mp_int_t length = args[1].u_int;

    if (start < 0x6D000)
    {
        mp_raise_ValueError(MP_ERROR_TEXT(
            "start must be equal or higher than the FPGA bitstream end address of 0x6D000"));
    }

    if (length < 0)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("length cannot be less than zero"));
    }

    if (length + start > 0x100000)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("start + length must be less than 0x100000"));
    }

    storage_obj_t *self = mp_obj_malloc(storage_obj_t, &storage_flash_device_type);
    self->start = start;
    self->len = length;
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_OBJ_TYPE(
    storage_flash_device_type,
    MP_QSTR_Partition,
    MP_TYPE_FLAG_NONE,
    make_new, storage_make_new,
    print, storage_print,
    locals_dict, &storage_locals_dict);

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

bool monocle_check_for_valid_bitstream(void)
{
    // Wakeup the flash
    uint8_t wakeup_device_id[] = {0xAB, 0, 0, 0};
    monocle_spi_write(FLASH, wakeup_device_id, 4, true);
    monocle_spi_read(FLASH, wakeup_device_id, 1, false);
    app_err(wakeup_device_id[0] != 0x13 && not_real_hardware_flag == false);

    uint8_t magic_word[4] = "";
    flash_read(magic_word, 0x6C80E, sizeof(magic_word));

    if (memcmp(magic_word, "\xFE\xED\xC0\xDE", sizeof(magic_word)) == 0)
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

/// @brief Globals and module definition

STATIC const mp_rom_map_elem_t storage_module_globals_table[] = {

    {MP_ROM_QSTR(MP_QSTR_Partition), MP_ROM_PTR(&storage_flash_device_type)},
    {MP_ROM_QSTR(MP_QSTR_App), MP_ROM_PTR(&fpga_app_type)},
};
STATIC MP_DEFINE_CONST_DICT(storage_module_globals, storage_module_globals_table);

const mp_obj_module_t storage_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&storage_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_storage, storage_module);