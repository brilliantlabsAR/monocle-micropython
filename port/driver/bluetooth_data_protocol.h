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
 * Custom media transfer protocol  on top of Bluetooth Low Energy.
 */

/**
 * Type of data transaction used in app_start_data_transaction().
 */
typedef enum data_op_t
{
    // Types of transaction which may be selected
    DATA_OP_CAMERA_CAPTURE,     // Capture a single image over Bluetooth or WiFi
    DATA_OP_CAMERA_STREAM,      // Streams frames continuously over WiFi
    DATA_OP_MICROPHONE_STREAM,  // Continuously streams microphone data over Bluetooth or WiFi
    DATA_OP_FIRMWARE_DOWNLOAD,  // Downloads a firmware update over WiFi
    DATA_OP_BITSTREAM_DOWNLOAD, // Downloads an FPGA bitstream over WiFi

    // Stops an ongoing transaction
    DATA_OP_STOP,

    // Return status codes
    DATA_OP_ACCEPTED,            // If the request was accepted
    DATA_OP_ALREADY_IN_PROGRESS, // If a operation is ongoing on the channel
    DATA_OP_NO_INTERNET,         // If there is no internet connection
    DATA_OP_CANNOT_REACH_SERVER, // If the URL cannot be reached
} data_op_t;

/**
 * Starts/stops a data operation of a given type to the mobile over BLE, or WiFi to a server.
 * @param channel: Type of operation to request.
 * @param url: URL to download/upload the request. If NULL, Bluetooth will be used.
 * @return One of the status codes from data_op_t.
 */
data_op_t app_data_operation(data_op_t channel);
