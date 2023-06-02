/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon <me@josuah.net>
 *
 * ISC Licence
 *
 * Copyright © 2022 Brilliant Labs Ltd.
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

#include <stddef.h>
#include <stdlib.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/misc.h"
#include "speex/speex.h"

void *speex_encoder_state;
SpeexBits speex_encoder_bits;
char speex_data[512];

/* The frame size in hardcoded for this sample code but it doesn't have to be */
#define FRAME_SIZE_MAX 256

#include "nrfx_log.h"

STATIC mp_obj_t microphone___init__(void) {
    speex_encoder_state = speex_encoder_init(&speex_nb_mode);
    speex_encoder_ctl(speex_encoder_state, SPEEX_SET_QUALITY, (int[]){8});
    speex_bits_init(&speex_encoder_bits);

    NRFX_LOG("microphone___init__");
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(microphone___init___obj, &microphone___init__);

STATIC mp_obj_t microphone_compress(mp_obj_t buffer) {
    int16_t samples[FRAME_SIZE_MAX];
    mp_buffer_info_t bufinfo;
    size_t n;

    mp_get_buffer_raise(buffer, &bufinfo, MP_BUFFER_READ);

    if (bufinfo.len > FRAME_SIZE_MAX * 2)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("buffer must be 256 bytes max"));
    }
    if (bufinfo.len % 2 != 0)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("must be int16_t encoded as big-endian"));
    }

    for (int i = 0; i < bufinfo.len / 2; i++)
    {
        uint8_t *buf = bufinfo.buf;
        union { int16_t i16; uint16_t u16; } value;

        value.u16 = (8 << buf[i * 2 + 0]) | (0 << buf[i * 2 + 1]);
        samples[i] = value.i16;
    }

    speex_bits_reset(&speex_encoder_bits);
    speex_encode_int(speex_encoder_state, samples, &speex_encoder_bits);
    n = speex_bits_write(&speex_encoder_bits, speex_data, sizeof(speex_data));

    // Only one concurrent conversion supported, we can return a global array.
    return mp_obj_new_bytes((uint8_t *)speex_data, n);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microphone_compress_obj, &microphone_compress);

STATIC const mp_rom_map_elem_t microphone_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&microphone___init___obj)},
    {MP_ROM_QSTR(MP_QSTR_compress), MP_ROM_PTR(&microphone_compress_obj)},
};
STATIC MP_DEFINE_CONST_DICT(microphone_module_globals, microphone_module_globals_table);

const mp_obj_module_t microphone_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&microphone_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR__microphone, microphone_module);

