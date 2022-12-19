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
 * Driver for configuring the SPI-controlled FPGA.
 * It controls the low-level read/write with the registers and bulk data transfer.
 * It provides a higher level API for:
 * - the FPGA itself,
 * - the Camera data path,
 * - the Microdisplay data path,
 * - the Microphone data,
 */

void fpga_prepare(void);
void fpga_init(void);
void fpga_deinit(void);
uint32_t fpga_system_id(void);
uint32_t fpga_system_version(void);
void fpga_camera_zoom(uint8_t zoom_level);
void fpga_camera_stop(void);
void fpga_camera_start(void);
void fpga_camera_capture(void);
void fpga_camera_off(void);
void fpga_camera_on(void);
void fpga_live_video_start(void);
void fpga_live_video_stop(void);
void fpga_live_video_replay(void);
void fpga_graphics_off(void);
void fpga_graphics_on(void);
void fpga_graphics_clear(void);
void fpga_graphics_swap_buffer(void);
void fpga_graphics_set_write_base(uint32_t base);
void fpga_graphics_write_data(uint8_t *buf, size_t len);
uint16_t fpga_capture_read_status(void);
void fpga_capture_read_data(uint8_t *buf, size_t len);

// debug
void fpga_check_pins(char const *msg);
#define fpga_check_reg(reg) NRFX_LOG("[0x%02X] %-s=%d", reg, #reg, fpga_read_register(reg))

