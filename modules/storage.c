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

STATIC mp_obj_t storage_read(mp_obj_t file, mp_obj_t length, mp_obj_t offset)
{
    const char *filename = mp_obj_str_get_str(file);

    // TODO handle offsets or lengths which are too large

    // TODO allow optional length value?

    // TODO allow optional offset value?

    if (strcmp(filename, "FPGA_BITSTREAM") == 0)
    {
        uint32_t address = mp_obj_get_int(offset);

        uint8_t read[] = {bit_reverse(0x03),
                          bit_reverse(address >> 16),
                          bit_reverse(address >> 8),
                          bit_reverse(address)};
        spi_write(FLASH, read, sizeof(read), true);

        uint8_t buffer[mp_obj_get_int(length)];

        spi_read(FLASH, buffer, mp_obj_get_int(length));

        return mp_obj_new_bytes(buffer, mp_obj_get_int(length));
    }

    return mp_const_notimplemented;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(storage_read_obj, storage_read);

STATIC mp_obj_t storage_append(mp_obj_t file, mp_obj_t bytes)
{
    const char *filename = mp_obj_str_get_str(file);

    if (strcmp(filename, "FPGA_BITSTREAM") == 0)
    {
        size_t length;
        const char *data = mp_obj_str_get_data(bytes, &length);

        if (fpga_bitstream_programmed_bytes + length >
            0x10000 * reserved_64k_blocks_for_fpga_bitstream)
        {
            mp_raise_ValueError(MP_ERROR_TEXT(
                "data will overflow reserved FPGA bitstream space"));
        }

        uint8_t write_enable[] = {bit_reverse(0x06)};
        spi_write(FLASH, write_enable, sizeof(write_enable), false);

        uint8_t page_program[] =
            {bit_reverse(0x02),
             bit_reverse(fpga_bitstream_programmed_bytes >> 16),
             bit_reverse(fpga_bitstream_programmed_bytes >> 8),
             bit_reverse(fpga_bitstream_programmed_bytes)};
        spi_write(FLASH, page_program, sizeof(page_program), true);
        spi_write(FLASH, (uint8_t *)data, length, true);

        fpga_bitstream_programmed_bytes += length;

        // If the currently written data isn't aligned to a page boundary
        // if (fpga_bitstream_programmed_bytes % 256)
        // {
        //     size_t remaining_bytes_in_page =
        //         256 - (fpga_bitstream_programmed_bytes % 256);

        //     if (length <= remaining_bytes_in_page)
        //     {

        //     }

        //     bytes_written += remaining_bytes_in_page;
        // }

        // Write the remaining data from the page boundary in 256byte chunks

        // Write a partial page

        return mp_const_none;
    }

    return mp_const_notimplemented;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(storage_append_obj, storage_append);

STATIC mp_obj_t storage_delete(mp_obj_t file)
{
    const char *filename = mp_obj_str_get_str(file);

    if (strcmp(filename, "FPGA_BITSTREAM") == 0)
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

            while (true)
            {
                uint8_t status[] = {bit_reverse(0x05)};
                spi_write(FLASH, status, sizeof(status), true);
                spi_read(FLASH, status, sizeof(status));

                // If no longer busy
                if ((status[0] & bit_reverse(0x01)) == 0)
                {
                    break;
                }

                mp_hal_delay_ms(10);
            }
        }

        fpga_bitstream_programmed_bytes = 0;

        return mp_const_none;
    }

    return mp_const_notimplemented;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(storage_delete_obj, storage_delete);

STATIC const mp_rom_map_elem_t storage_module_globals_table[] = {

    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&storage_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_append), MP_ROM_PTR(&storage_append_obj)},
    {MP_ROM_QSTR(MP_QSTR_delete), MP_ROM_PTR(&storage_delete_obj)},
    {MP_ROM_QSTR(MP_QSTR_FPGA_BITSTREAM), MP_ROM_QSTR(MP_QSTR_FPGA_BITSTREAM)},
};
STATIC MP_DEFINE_CONST_DICT(storage_module_globals, storage_module_globals_table);

const mp_obj_module_t storage_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&storage_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_storage, storage_module);