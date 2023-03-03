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

static void flash_read(uint8_t *buffer, size_t address, size_t length)
{
    app_err(length > 255);

    uint8_t read_cmd[] = {bit_reverse(0x03),
                          bit_reverse(address >> 16),
                          bit_reverse(address >> 8),
                          bit_reverse(address)};
    spi_write(FLASH, read_cmd, sizeof(read_cmd), true);
    spi_read(FLASH, buffer, length);
}

STATIC mp_obj_t storage_read(mp_obj_t file, mp_obj_t length, mp_obj_t offset)
{
    const char *filename = mp_obj_str_get_str(file);

    if (mp_obj_get_int(length) > (1 << SPIM2_EASYDMA_MAXCNT_SIZE))
    {
        mp_raise_msg_varg(&mp_type_ValueError,
                          MP_ERROR_TEXT("length must be less than %u bytes"),
                          1 << SPIM2_EASYDMA_MAXCNT_SIZE);
    }

    // TODO allow optional length value?

    // TODO allow optional offset value?

    if (strcmp(filename, "FPGA_BITSTREAM") == 0)
    {
        uint8_t buffer[mp_obj_get_int(length)];
        flash_read(buffer, mp_obj_get_int(offset), mp_obj_get_int(length));
        return mp_obj_new_bytes(buffer, mp_obj_get_int(length));
    }

    return mp_const_notimplemented;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(storage_read_obj, storage_read);

STATIC mp_obj_t storage_append(mp_obj_t file, mp_obj_t bytes)
{
    const char *filename = mp_obj_str_get_str(file);

    size_t length;
    const char *data = mp_obj_str_get_data(bytes, &length);

    if (length > (1 << SPIM2_EASYDMA_MAXCNT_SIZE))
    {
        mp_raise_msg_varg(&mp_type_ValueError,
                          MP_ERROR_TEXT("length must be less than %u bytes"),
                          1 << SPIM2_EASYDMA_MAXCNT_SIZE);
    }

    if (strcmp(filename, "FPGA_BITSTREAM") == 0)
    {
        if (fpga_bitstream_programmed_bytes + length >
            0x10000 * reserved_64k_blocks_for_fpga_bitstream)
        {
            mp_raise_ValueError(MP_ERROR_TEXT(
                "data will overflow reserved FPGA bitstream space"));
        }

        // Which page are we on, and how far in are we
        size_t this_page = (size_t)floor(fpga_bitstream_programmed_bytes / 256.0);
        size_t into_page = fpga_bitstream_programmed_bytes % 256;

        NRFX_LOG_ERROR("this_page: %u, into_page: %u", this_page, into_page);

        // Read a page. Two reads as the SPI can't do 256 in one go
        uint8_t page1_data[256];
        size_t this_page_address = this_page * 256;
        flash_read(page1_data, this_page_address, 128);
        flash_read(page1_data + 128, this_page_address + 128, 128);

        NRFX_LOG_ERROR("this_page_address: %u", this_page_address);

        // Incase a second page is needed, we prepare the buffer
        bool double_page_write = false;
        uint8_t page2_data[256];
        size_t next_page_address = this_page_address + 256;

        // Read either two or one page depending on if the data will overflow
        if (length + into_page > 256)
        {
            flash_read(page2_data, next_page_address, 128);
            flash_read(page2_data + 128, next_page_address + 128, 128);
            // memcpy(page1_data + into_page, data, length - into_page);
            // memcpy(page2_data, data + (length - into_page), ...);
            double_page_write = true;
            NRFX_LOG_ERROR("double page write from %u on page 1, to %u on page 2", into_page, length - (256 - into_page));
        }
        else
        {
            // memcpy(page1_data + into_page, data, length);
            NRFX_LOG_ERROR("single page write at byte %u in page, length: %u", into_page, length);
        }

        // TODO write the first page

        // TODO write the second page if it changed
        if (double_page_write)
        {
        }

        ////// TODO
        uint8_t write_enable[] = {bit_reverse(0x06)};
        spi_write(FLASH, write_enable, sizeof(write_enable), false);

        uint8_t page_program[] =
            {bit_reverse(0x02),
             bit_reverse(fpga_bitstream_programmed_bytes >> 16),
             bit_reverse(fpga_bitstream_programmed_bytes >> 8),
             bit_reverse(fpga_bitstream_programmed_bytes)};
        spi_write(FLASH, page_program, sizeof(page_program), true);
        spi_write(FLASH, (uint8_t *)data, length, false);

        fpga_bitstream_programmed_bytes += length;

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