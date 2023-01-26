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

/*
 * Driver for configuring the SPI-controlled FPGA.
 * It controls the low-level read/write with the registers and bulk data transfer.
 * It provides a higher level API for:
 * - the FPGA itself,
 * - the Camera data path,
 * - the Microdisplay data path,
 * - the Microphone data,
 */

#define FPGA_SYSTEM_ID          0x0001
#define FPGA_SYSTEM_VERSION     0x0002
#define FPGA_CAMERA_ZOOM        0x1002
#define FPGA_CAMERA_STOP        0x1004
#define FPGA_CAMERA_START       0x1005
#define FPGA_CAMERA_CAPTURE     0x1006
#define FPGA_CAMERA_OFF         0x1008
#define FPGA_CAMERA_ON          0x1009
#define FPGA_LIVEVIDEO_START    0x3005
#define FPGA_LIVEVIDEO_STOP     0x3004
#define FPGA_LIVEVIDEO_REPLAY   0x3007
#define FPGA_GRAPHICS_OFF       0x4404
#define FPGA_GRAPHICS_ON        0x4405
#define FPGA_GRAPHICS_CLEAR     0x4406
#define FPGA_GRAPHICS_SWAP      0x4407
#define FPGA_GRAPHICS_BASE      0x4410
#define FPGA_GRAPHICS_DATA      0x4411
#define FPGA_CAPTURE_STATUS     0x5000
#define FPGA_CAPTURE_DATA       0x5010

void fpga_init(void);
void fpga_cmd_read(uint16_t cmd, uint8_t *buf, size_t len);
void fpga_cmd_write(uint16_t cmd, const uint8_t *buf, size_t len);
void fpga_cmd(uint16_t cmd);

// debug
void fpga_check_pins(char const *msg);
