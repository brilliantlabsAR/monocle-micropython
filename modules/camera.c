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
#include "nrf_gpio.h"
#include "py/runtime.h"

STATIC mp_obj_t camera_sleep(void)
{
    nrf_gpio_pin_write(CAMERA_SLEEP_PIN, true);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(camera_sleep_obj, camera_sleep);

STATIC mp_obj_t camera_wake(void)
{
    nrf_gpio_pin_write(CAMERA_SLEEP_PIN, false);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(camera_wake_obj, camera_wake);

// To set the zoom level to 1.5:
/*
fpga.write(0x3004, b'') # live off
__camera.zoom(1.5)
fpga.write(0x1005, b'') # record on
fpga.write(0x3005, b'') # live on
*/
// Only values between 0 (no effect) and 1.8 (no more output) seems to work.
// There might be another configuration value to adjust aroung some threshold.
STATIC mp_obj_t camera_zoom(mp_obj_t zoom)
{
    if (mp_obj_get_float(zoom) < 1)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("min zoom is 1"));
    }

    float width = 2607 / mp_obj_get_float(zoom);
    float height = 1705 / mp_obj_get_float(zoom);

    app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0x00).fail); // Start group 0

#define CAM_CROP_LEFT     0
#define CAM_CROP_TOP      0
#define CAM_CROP_WIDTH    (uint16_t)(width)
#define CAM_CROP_HEIGHT   (uint16_t)(height)
#define CAM_CROP_RIGHT    (CAM_CROP_LEFT + CAM_CROP_WIDTH - 1)
#define CAM_CROP_BOTTOM   (CAM_CROP_TOP + CAM_CROP_HEIGHT - 1)

    app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3800, 0xFF, CAM_CROP_LEFT >> 8).fail);       // Timing X start address MSB
    app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3801, 0xFF, CAM_CROP_LEFT & 0xFF).fail);     // Timing X start address LSB
    app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3802, 0xFF, CAM_CROP_TOP >> 8).fail);        // Timing Y start address MSB
    app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3803, 0xFF, CAM_CROP_TOP & 0xFF).fail);      // Timing Y start address LSB
    app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3804, 0xFF, CAM_CROP_RIGHT >> 8).fail);      // Timing X end address MSB
    app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3805, 0xFF, CAM_CROP_RIGHT & 0xFF).fail);    // Timing X end address LSB
    app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3806, 0xFF, CAM_CROP_BOTTOM >> 8).fail);     // Timing Y end address MSB
    app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3807, 0xFF, CAM_CROP_BOTTOM & 0xFF).fail);   // Timing Y end address LSB

    app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0x10).fail); // End group 0
    app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0xA0).fail); // Launch group 0

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_zoom_obj, camera_zoom);

STATIC const mp_rom_map_elem_t camera_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&camera_sleep_obj)},
    {MP_ROM_QSTR(MP_QSTR_wake), MP_ROM_PTR(&camera_wake_obj)},
    {MP_ROM_QSTR(MP_QSTR_zoom), MP_ROM_PTR(&camera_zoom_obj)},
};
STATIC MP_DEFINE_CONST_DICT(camera_module_globals, camera_module_globals_table);

const mp_obj_module_t camera_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&camera_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR___camera, camera_module);
