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
#include <math.h>
#include "monocle.h"
#include "extmod/vfs.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/runtime.h"

const struct _mp_obj_type_t device_storage_type;

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

    monocle_flash_read(bufinfo.buf, address, bufinfo.len);

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
        monocle_flash_page_erase(address);
    }

    monocle_flash_write(bufinfo.buf, address, bufinfo.len);

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

        monocle_flash_page_erase(address);
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

    storage_obj_t *self = mp_obj_malloc(storage_obj_t, &device_storage_type);
    self->start = start;
    self->len = length;
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_OBJ_TYPE(
    device_storage_type,
    MP_QSTR_Storage,
    MP_TYPE_FLAG_NONE,
    make_new, storage_make_new,
    print, storage_print,
    locals_dict, &storage_locals_dict);