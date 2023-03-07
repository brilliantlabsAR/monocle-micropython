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
#include "py/mphal.h"
#include "py/runtime.h"

static const size_t reserved_64k_blocks_for_fpga_bitstream = 7;

static size_t fpga_bitstream_programmed_bytes = 0;

static bool flash_is_busy(void)
{
    uint8_t status_cmd[] = {bit_reverse(0x05)};
    spi_write(FLASH, status_cmd, sizeof(status_cmd), true);
    spi_read(FLASH, status_cmd, sizeof(status_cmd));

    if ((status_cmd[0] & bit_reverse(0x01)) == 0)
    {
        return false;
    }

    return true;
}

static void flash_read(uint8_t *buffer, size_t address, size_t length)
{
    app_err(length > 255);

    while (flash_is_busy())
    {
        mp_hal_delay_ms(1);
    }

    uint8_t read_cmd[] = {bit_reverse(0x03),
                          bit_reverse(address >> 16),
                          bit_reverse(address >> 8),
                          bit_reverse(address)};
    spi_write(FLASH, read_cmd, sizeof(read_cmd), true);
    spi_read(FLASH, buffer, length);
}

static void flash_write(uint8_t *buffer, size_t address, size_t length)
{
    app_err(length > 255);

    while (flash_is_busy())
    {
        mp_hal_delay_ms(1);
    }

    uint8_t write_enable_cmd[] = {bit_reverse(0x06)};
    spi_write(FLASH, write_enable_cmd, sizeof(write_enable_cmd), false);

    uint8_t page_program_cmd[] = {bit_reverse(0x02),
                                  bit_reverse(address >> 16),
                                  bit_reverse(address >> 8),
                                  bit_reverse(address)};
    spi_write(FLASH, page_program_cmd, sizeof(page_program_cmd), true);
    spi_write(FLASH, buffer, length, false);
}

STATIC mp_obj_t storage_read(mp_obj_t file, mp_obj_t file_length, mp_obj_t offset)
{
    const char *file_name = mp_obj_str_get_str(file);

    if (strcmp(file_name, "FPGA_BITSTREAM") == 0)
    {
        uint8_t buffer[mp_obj_get_int(file_length)];

        size_t bytes_read = 0;
        while (bytes_read < mp_obj_get_int(file_length))
        {
            size_t max_readable_length = MIN(mp_obj_get_int(file_length) - bytes_read, 255);

            flash_read(buffer + bytes_read,
                       mp_obj_get_int(offset) + bytes_read,
                       max_readable_length);

            bytes_read += max_readable_length;
        }

        return mp_obj_new_bytes(buffer, mp_obj_get_int(file_length));
    }

    return mp_const_notimplemented;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(storage_read_obj, storage_read);

STATIC mp_obj_t storage_append(mp_obj_t file, mp_obj_t bytes)
{
    const char *file_name = mp_obj_str_get_str(file);

    size_t file_length;
    const char *file_data = mp_obj_str_get_data(bytes, &file_length);

    if (strcmp(file_name, "FPGA_BITSTREAM") == 0)
    {
        if (fpga_bitstream_programmed_bytes + file_length >
            0x10000 * reserved_64k_blocks_for_fpga_bitstream)
        {
            mp_raise_ValueError(MP_ERROR_TEXT(
                "file length overflows the reserved space for the bitstream"));
        }

        size_t bytes_written = 0;
        while (bytes_written < file_length)
        {
            size_t page_number = (size_t)floor(fpga_bitstream_programmed_bytes / 256.0);
            size_t page_address = page_number * 256;

            size_t offset_in_page = fpga_bitstream_programmed_bytes % 256;
            size_t bytes_left_in_page = 256 - offset_in_page;

            // Read the page in two chunks as the SPI can't do 256 in one go
            uint8_t page_data[256];
            flash_read(page_data, page_address, 128);
            flash_read(page_data + 128, page_address + 128, 128);

            size_t bytes_appended = MIN(file_length - bytes_written, bytes_left_in_page);
            memcpy(page_data + offset_in_page, file_data + bytes_written, bytes_appended);

            flash_write(page_data, page_address, 128);
            flash_write(page_data + 128, page_address + 128, 128);

            fpga_bitstream_programmed_bytes += bytes_appended;
            bytes_written += bytes_appended;
        }
        return mp_const_none;
    }

    return mp_const_notimplemented;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(storage_append_obj, storage_append);

STATIC mp_obj_t storage_delete(mp_obj_t file)
{
    const char *file_name = mp_obj_str_get_str(file);

    if (strcmp(file_name, "FPGA_BITSTREAM") == 0)
    {
        for (size_t i = 0; i < reserved_64k_blocks_for_fpga_bitstream; i++)
        {
            uint8_t write_enable[] = {bit_reverse(0x06)};
            spi_write(FLASH, write_enable, sizeof(write_enable), false);

            uint32_t address_24bit = 0x10000 * i;
            uint8_t block_erase[] = {bit_reverse(0xD8),
                                     bit_reverse(address_24bit >> 16),
                                     0, // Bottom bytes of address are always 0
                                     0};
            spi_write(FLASH, block_erase, sizeof(block_erase), false);

            while (flash_is_busy())
            {
                mp_hal_delay_ms(10);
            }
        }

        fpga_bitstream_programmed_bytes = 0;

        return mp_const_none;
    }

    return mp_const_notimplemented;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(storage_delete_obj, storage_delete);

STATIC mp_obj_t storage_list(void)
{
    return mp_const_notimplemented;
}
MP_DEFINE_CONST_FUN_OBJ_0(storage_list_obj, storage_list);

STATIC mp_obj_t storage_mem_free(void)
{
    return mp_const_notimplemented;
}
MP_DEFINE_CONST_FUN_OBJ_0(storage_mem_free_obj, storage_mem_free);

STATIC const mp_rom_map_elem_t storage_module_globals_table[] = {

    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&storage_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_append), MP_ROM_PTR(&storage_append_obj)},
    {MP_ROM_QSTR(MP_QSTR_delete), MP_ROM_PTR(&storage_delete_obj)},
    {MP_ROM_QSTR(MP_QSTR_list), MP_ROM_PTR(&storage_list_obj)},
    {MP_ROM_QSTR(MP_QSTR_mem_free), MP_ROM_PTR(&storage_mem_free_obj)},
    {MP_ROM_QSTR(MP_QSTR_MEM_TOTAL),
     MP_ROM_INT(1048576 - (reserved_64k_blocks_for_fpga_bitstream * 8192))},
    {MP_ROM_QSTR(MP_QSTR_FPGA_BITSTREAM), MP_ROM_QSTR(MP_QSTR_FPGA_BITSTREAM)},
};
STATIC MP_DEFINE_CONST_DICT(storage_module_globals, storage_module_globals_table);

const mp_obj_module_t storage_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&storage_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_storage, storage_module);