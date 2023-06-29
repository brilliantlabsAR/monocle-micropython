/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Ltd. (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Ltd.
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
#include "monocle.h"
#include "py/runtime.h"

static uint8_t microphone_bit_depth = 16;

static inline void microphone_fpga_read(uint16_t address, uint8_t *buffer, size_t length)
{
    uint8_t address_bytes[2] = {(uint8_t)(address >> 8), (uint8_t)address};

    monocle_spi_write(FPGA, address_bytes, 2, true);

    // Dump the data into a dummy buffer if a buffer isn't provided
    if (buffer == NULL)
    {
        uint8_t dummy_buffer[254];
        monocle_spi_read(FPGA, dummy_buffer, length, false);
        return;
    }

    monocle_spi_read(FPGA, buffer, length, false);
}

static inline void microphone_fpga_write(uint16_t address, uint8_t *buffer, size_t length)
{
    uint8_t address_bytes[2] = {(uint8_t)(address >> 8), (uint8_t)address};

    if (buffer == NULL || length == 0)
    {
        monocle_spi_write(FPGA, address_bytes, 2, false);
        return;
    }

    monocle_spi_write(FPGA, address_bytes, 2, true);
    monocle_spi_write(FPGA, buffer, length, false);
}

static size_t microphone_bytes_available(void)
{
    uint8_t available_bytes[2] = {0, 0};
    microphone_fpga_read(0x5801, available_bytes, sizeof(available_bytes));
    size_t available = (available_bytes[0] << 8 | available_bytes[1]) * 2;

    // Cap to 254 due to SPI DMA limit
    if (available > 254)
    {
        available = 254;
    }

    return available;
}

STATIC mp_obj_t microphone_init(void)
{
    uint8_t fpga_image[4];
    uint8_t module_status[2];

    microphone_fpga_read(0x0001, fpga_image, sizeof(fpga_image));
    microphone_fpga_read(0x5800, module_status, sizeof(module_status));

    if (((module_status[0] & 0x10) != 16) ||
        memcmp(fpga_image, "Mncl", sizeof(fpga_image)))
    {
        mp_raise_NotImplementedError(
            MP_ERROR_TEXT("microphone driver not found on FPGA"));
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(microphone_init_obj, microphone_init);

STATIC mp_obj_t microphone_record(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_sample_rate, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 16000}},
        {MP_QSTR_bit_depth, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 16}},
        {MP_QSTR_seconds, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(5)}}};

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Flush existing data
    while (true)
    {
        size_t available = microphone_bytes_available();

        if (available == 0)
        {
            break;
        }

        microphone_fpga_read(0x5807, NULL, available);
    }

    // Check the given sample rate
    mp_int_t sample_rate = args[0].u_int;

    if (sample_rate != 16000 && sample_rate != 8000)
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("sample rate must be either 16000 or 8000"));
    }

    // Check the currently set sample rate on the FPGA
    uint8_t status_byte;
    microphone_fpga_read(0x0800, &status_byte, sizeof(status_byte));

    // Toggle the sample rate if required
    if (((status_byte & 0x04) == 0x00 && sample_rate == 8000) ||
        ((status_byte & 0x04) == 0x04 && sample_rate == 16000))
    {
        microphone_fpga_write(0x0808, NULL, 0);
    }

    // Check and set bit depth to the global variable
    mp_int_t bit_depth = args[1].u_int;
    if (bit_depth != 16 && bit_depth != 8)
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("bit depth must be either 16 or 8"));
    }
    microphone_bit_depth = bit_depth;

    // Set the block size and request a number of blocks corresponding to seconds
    float block_size;
    sample_rate == 16000 ? (block_size = 0.02) : (block_size = 0.04);

    uint16_t blocks = (uint16_t)(mp_obj_get_float(args[2].u_obj) / block_size);
    uint8_t blocks_bytes[] = {blocks >> 8, blocks};
    microphone_fpga_write(0x0802, blocks_bytes, sizeof(blocks));

    // Trigger capture
    microphone_fpga_write(0x0803, NULL, 0);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(microphone_record_obj, 0, microphone_record);

STATIC mp_obj_t microphone_stop(void)
{
    return mp_const_notimplemented;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(microphone_stop_obj, microphone_stop);

STATIC mp_obj_t microphone_read(mp_obj_t samples)
{
    if (mp_obj_get_int(samples) > 127)
    {
        mp_raise_ValueError(
            MP_ERROR_TEXT("only 127 samples may be read at a time"));
    }

    size_t available = microphone_bytes_available();

    if (available == 0)
    {
        return mp_const_none;
    }

    if (mp_obj_get_int(samples) * 2 < available)
    {
        available = mp_obj_get_int(samples) * 2;
    }

    uint8_t buffer[available];
    microphone_fpga_read(0x5807, buffer, sizeof(buffer));
    return mp_obj_new_bytes(buffer, sizeof(buffer));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microphone_read_obj, microphone_read);

STATIC const mp_rom_map_elem_t microphone_module_globals_table[] = {

    {MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&microphone_init_obj)},
    {MP_ROM_QSTR(MP_QSTR_record), MP_ROM_PTR(&microphone_record_obj)},
    {MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&microphone_stop_obj)},
    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&microphone_read_obj)},
};
STATIC MP_DEFINE_CONST_DICT(microphone_module_globals, microphone_module_globals_table);

const mp_obj_module_t microphone_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&microphone_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_microphone, microphone_module);
