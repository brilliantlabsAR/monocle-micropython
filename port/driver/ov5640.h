/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon <me@josuah.net>
 *
 * ISC Licence
 *
 * Copyright © 2022 Brilliant Labs Inc.
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

/**
 * Driver for the I²C OV5640 camera sensor.
 * https://cdn.sparkfun.com/datasheets/Sensors/LightImaging/
 * The parallel data path is connected to FPGA. The MCU only has access to the I²C configuration interface.
 * - 5MP max resolutiono
 * - Recording happens at 15 FPS, and the FPGA triple every frame to match the 50 FPS of the ECX334CN display.
 * - When using the 4x digital zoom, the camera outputs 640x400 video.
 */

#define OV5640_CHIPIDH          0x300A ///< OV5640 Chip ID Register address, high byte
#define OV5640_CHIPIDL          0x300B ///< OV5640 Chip ID Register address, low byte
#define OV5640_ID               0x5640 ///< OV5640 Chip ID, expected value
#define OV5640_FPS              15     ///< frames per second, as implemented in camera configuration

#define TRANSFER_CMPLT 0x00u
#define TRANSFER_ERROR 0x01u
#define I2C_SLAVE_ADDR OV5640_ADDR

// Suggested sequence of calls from main program:
// Initialize MCU pins & I2C interface (done at start of program, in board init)
//  ov5640_init();
// Enable 24MHz pixel clock to the OV5640
//  ov5640_pwr_on(); (sets control pin states; in some MK versions turns on 2.8V)
//  ov5640_rgb565_mode(); -or- ov5640_yuv422_mode();
//  ov5640_mode_1x();
// if using other display: ov5640_reduce_size(new display resolution which should be < 640x400);
//  ov5640_focus_init(); 
//  ov5640_light_mode(0);
//  ov5640_color_saturation(3);
//  ov5640_brightness(4);
//  ov5640_contrast(3);
//  ov5640_sharpness(33);
//  ov5640_focus_constant(); // if AF lens module
//  ov5640_flip(1); // if needed
//  ov5640_mirror(1); // if needed

// Functions return true on success, false on failure

void ov5640_prepare(void);
void ov5640_init(void);
void ov5640_deinit(void);
void ov5640_pwr_on(void);
void ov5640_pwr_sleep(void);
void ov5640_pwr_wake(void);
//void ov5640_pwr_off(void);

void ov5640_mode_1x(void);
void ov5640_mode_2x(void);
void ov5640_reduce_size(uint16_t Hpixels, uint16_t Vpixels);
void ov5640_light_mode(uint8_t mode);
void ov5640_color_saturation(uint8_t sat);
void ov5640_brightness(uint8_t bright);
void ov5640_contrast(uint8_t contrast);
void ov5640_sharpness(uint8_t sharp);
void ov5640_special_effects(uint8_t eft);
void ov5640_flash_ctrl(bool on);
void ov5640_mirror(bool on);
void ov5640_flip(bool on);

void ov5640_outsize_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height);
void ov5640_focus_init(void);

