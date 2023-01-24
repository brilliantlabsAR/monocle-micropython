/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Raj Nakarja / Silicon Witchery (raj@siliconwitchery.com)
 *              for Brilliant Labs Inc.
 * Authored by: Josuah Demangeon (me@josuah.net)
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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "ble.h"
#include "nrfx_log.h"

#include "driver/bluetooth_data_protocol.h"
#include "driver/bluetooth_low_energy.h"
#include "driver/config.h"
#include "driver/fpga.h"
#include "driver/ov5640.h"
#include "driver/timer.h"
#include "libjojpeg.h"

// List of flags to append in the header when we send file chunks over BLE
enum {
    BLE_FILE_SMALL_FLAG = 0,
    BLE_FILE_START_FLAG,
    BLE_FILE_MIDDLE_FLAG,
    BLE_FILE_END_FLAG,
};

static uint8_t ble_buf[BLE_MAX_MTU_LENGTH];
static uint8_t ble_flag;
static size_t ble_pos;
static size_t ble_len;

static inline size_t data_encode_u32(uint8_t *buf, uint32_t u32)
{
    buf[0] = u32 >> 0;
    buf[1] = u32 >> 8;
    buf[2] = u32 >> 16;
    buf[3] = u32 >> 24;
    return 4;
}

static inline size_t data_encode_str(uint8_t *buf, char const *str)
{
    size_t len = strlen(str);

    buf[0] = len;
    memcpy(buf + 1, str, len);
    return 1 + len;
}

void data_flush_ble_packet(void)
{
    // fill the flag field and send the data
    ble_buf[0] = ble_flag;
    ble_raw_tx(ble_buf, ble_len);

    // reset the lenght, saving room for the flag at the beginning of the buffer
    ble_pos = 1;

    // prepare the next packet's flag, assuming it will be a MIDDLE one,
    // and on the last packet, this callback will not be called automatically,
    // which gives us the opportunity to override that flag for the last packet.
    ble_flag = BLE_FILE_MIDDLE_FLAG;
}

void jojpeg_write(uint8_t const *jpeg_buf, size_t jpeg_len)
{
    for (size_t i = 0; i < jpeg_len; i++) {
        if (ble_pos == ble_len) {
            data_flush_ble_packet();
        }
        assert(ble_pos < ble_len);
        assert(ble_pos > 0);
        ble_buf[ble_pos++] = jpeg_buf[i];
    }
}

void bluetooth_data_camera_capture(char const *filename, uint8_t quality)
{
    // buffer storing RGB data from from the camera, used by the JPEG library
    jojpeg_t ctx;
    uint8_t rgb_buf[OV5640_WIDTH * 8 * 3];

    // ask the FPGA to start a camera capture, and read the data later.
    fpga_cmd(FPGA_CAMERA_CAPTURE);

    // init blueetooth buffer parameters
    ble_len = ble_negotiated_mtu - 3;
    ble_flag = BLE_FILE_START_FLAG;

    // Save room for the flag added later.
    ble_pos = 1;

    // TODO: remove this from the protocol? Set to 0 for now>
    // Insert the filesize
    ble_pos += data_encode_u32(ble_buf + ble_pos, 674075);

    // Add the file name
    ble_pos += data_encode_str(ble_buf + ble_pos, filename);

    // set all the parameters and write the JPEG header
    jojpeg_start(&ctx, OV5640_WIDTH, OV5640_HEIGHT, 3, 0);

    do {
        // get a buffer-ful of RGB data from the camera (via the FPGA)
        //size_t n = fpga_capture_read(rgb_buf, sizeof rgb_buf); // TODO: implement it

        //log("n=%d height=%d", n, ctx.height);

    // enqueue the conversion, letting the callback flush the data over bluetooth
    } while (jojpeg_append_8_rows(&ctx, rgb_buf, sizeof rgb_buf));

    // the callback sets the ble_flag to BLE_MIDDLE when run, instead, here, we want
    // it to be the end packet, unless the callback never ran, which means we have
    // a single small packet.
    ble_flag = (ble_flag == BLE_FILE_MIDDLE_FLAG) ? BLE_FILE_END_FLAG : BLE_FILE_SMALL_FLAG;

    // perform the last data transfer
    data_flush_ble_packet();
}
