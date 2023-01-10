/*
 * This file is part of the MicroPython for Frame project:
 *      https://github.com/Itsbrilliantlabs/frame-micropython
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
#include "driver/timer.h"

/**
 * @brief List of states for the data operations state machine.
 */
typedef enum data_state_t
{
    DATA_STATE_IDLE,
    DATA_STATE_GET_CAM_METADATA,
    DATA_STATE_BLE_CAM_SMALL_PAYLOAD,
    DATA_STATE_BLE_CAM_DATA_START,
    DATA_STATE_BLE_CAM_DATA_MIDDLE,
    DATA_STATE_BLE_CAM_DATA_END,
} data_state_t;

/**
 * @brief State space for the data operations state machine.
 */
struct data_state_space_t
{                                                             // ------------------------------------
    struct data_input                                         // Inputs
    {                                                         // ------------------------------------
        bool camera_capture_flag;                             // Setting this flag starts a single camera capture. It's automatically cleared when read
        bool camera_stream_flag;                              // Setting this flag starts a continuos camera capture. It's not automatically cleared, stop must be used
        bool microphone_stream_flag;                          // Setting this flag starts a continuos microphone capture. It's not automatically cleared, stop must be used
        bool firmware_download_flag;                          // Setting this flag starts a firmware update. It's automatically cleared when read
        bool bitstream_download_flag;                         // Setting this flag starts a bitstream update. It's automatically cleared when read
        bool stop_flag;                                       // Setting this flag stops one of the above transfers
    } input;                                                  // ------------------------------------
    struct data_state                                         // Internal state variables
    {                                                         // ------------------------------------
        data_state_t current;                                 // Current state of the state machine. Set automatically within the state machine
        data_state_t next;                                    // Next state to go into. Set this value in the state switch() logic to change state
        uint32_t seconds_elapsed;                             // How long we've been in the current state for. Set automatically within the state machine, and reset when a state changes
    } state;                                                  // ------------------------------------
    struct data_output                                        // Outputs
    {                                                         // ------------------------------------
        struct data_output_file                               // Metadata of the file to send
        {                                                     // ------------
            uint32_t size;                                    // File size
            char name[50];                                    // File name string. 50byte limit
        } file;                                               // ------------
        struct data_output_ble                                // Buffer payload and lengths for Bluetooth transfers
        {                                                     // ------------
            uint8_t buffer[BLE_MAX_MTU_LENGTH];               // Buffer containing a single BLE payload
            uint16_t mtu;                                     // MTU length - 3
            uint16_t sent_bytes;                              // How many bytes of the current file have been sent so far
        } ble;                                                // ------------
        bool no_ble_error_flag;                               // Goes high if BLE is not connected ot enabled on the host. Must be cleared after read
        bool no_internet_error_flag;                          // Goes high if there is no internet connection. Must be cleared after read
    } output;                                                 // ------------------------------------
} data = {
    .state.current = DATA_STATE_IDLE,
    .state.next = DATA_STATE_IDLE,
    .state.seconds_elapsed = 0,
};

// TODO remove this when no longer needed
#include "dummy_data.h"

static inline size_t strnlen(const char *s, size_t maxlen)
{
    char *cp;

    cp = memchr(s, '\0', maxlen);
    return (cp == NULL) ? maxlen : cp - s;
}

static bool read_and_clear(bool *flag)
{
    // If the flag is true
    if (*flag)
    {
        // Clear the flag
        *flag = false;

        // Return true
        return true;
    }

    // Otherwise return false
    return false;
}

static inline size_t data_encode_u32(uint8_t *buf, uint32_t u32)
{
    buf[0] = u32 >> 24;
    buf[1] = u32 >> 16;
    buf[2] = u32 >> 8;
    buf[3] = u32 >> 0;
    return 4;
}

static inline size_t data_encode_str(uint8_t *buf, char *name)
{
    size_t len = strlen(name);

    buf[0] = len;
    memcpy(buf + 1, name, len);
    return 1 + len;
}

static inline size_t data_encode_mem(uint8_t *buf, uint8_t const *mem, size_t len)
{
    memcpy(buf, mem, len);
    return 1 + len;
}

/**
 * @brief State machine which handles all data operations such as OTA and file
 *        transfers.
 */
static void data_state_machine(void)
{
    // List of flags to append in the header when we send file chunks over BLE
    enum ble_file_flag
    {
        BLE_FILE_SMALL_FLAG = 0,
        BLE_FILE_START_FLAG = 1,
        BLE_FILE_MIDDLE_FLAG = 2,
        BLE_FILE_END_FLAG = 3,
    };

    // // State machine logic
    switch (data.state.current)
    {
    case DATA_STATE_IDLE:
    LOG("DATA_STATE_IDLE");
    {
        // If a stop is requested
        if (read_and_clear(&data.input.stop_flag))
        {
            // Clear the streaming flag
            data.input.camera_stream_flag = false;
        }

        // If a camera capture or stream is requested
        if (read_and_clear(&data.input.camera_capture_flag) ||
            data.input.camera_stream_flag)
        {
            // Go get the image metadata
            data.state.next = DATA_STATE_GET_CAM_METADATA;
            break;
        }

        // TODO microphone

        // TODO firmware update

        // TODO bitstream update

        // Stop the timer callback if there's nothing to do
        timer_del_handler(&data_state_machine);
        break;
    }

    case DATA_STATE_GET_CAM_METADATA:
    LOG("DATA_STATE_GET_CAM_METADATA");
    {
        // Get the file size
        data.output.file.size = sizeof(dummy_small_file); // TODO spi

        // Get the filename
        size_t len = snprintf(data.output.file.name, sizeof data.output.file.name, "test_file.jpg"); // TODO spi

        // Set the MTU length
        data.output.ble.mtu = ble_negotiated_mtu - 3;

        // Reset the number of sent bytes
        data.output.ble.sent_bytes = 0;

        // If the payload is smaller than a single payload
        if (data.output.file.size + len + 6 <= data.output.ble.mtu)
        {
            // Go to small payload state
            data.state.next = DATA_STATE_BLE_CAM_SMALL_PAYLOAD;
            break;
        }

        // Otherwise go to data start state
        data.state.next = DATA_STATE_BLE_CAM_DATA_START;
        break;
    }

    case DATA_STATE_BLE_CAM_SMALL_PAYLOAD:
    LOG("DATA_STATE_BLE_CAM_SMALL_PAYLOAD");
    {
        size_t i = 0;

        // Append the small file flag
        data.output.ble.buffer[i++] = BLE_FILE_SMALL_FLAG;

        // Insert the filesize
        i += data_encode_u32(data.output.ble.buffer + i, data.output.file.size);

        // Add the file name
        i += data_encode_str(data.output.ble.buffer + i, data.output.file.name);

        // Append the data into the remaining buffer space
        i += data_encode_mem(data.output.ble.buffer + i, dummy_small_file, data.output.file.size);

        // Send the data
        ble_raw_tx(data.output.ble.buffer, i);

        // Return to IDLE
        data.state.next = DATA_STATE_IDLE;
        break;
    }

    case DATA_STATE_BLE_CAM_DATA_START:
    LOG("DATA_STATE_BLE_CAM_DATA_START");
    {
        size_t i = 0, len;

        // Append the start file flag
        data.output.ble.buffer[i++] = BLE_FILE_START_FLAG;

        // Insert the filesize
        i += data_encode_u32(data.output.ble.buffer + i, data.output.file.size);

        // Add the file name
        i += data_encode_str(data.output.ble.buffer + i, data.output.file.name);

        // Append the data into the remaining buffer space
        len = data.output.ble.mtu - i;
        i += data_encode_mem(data.output.ble.buffer + i, dummy_large_file, len);

        // Send the data. If not successful
        ble_raw_tx(data.output.ble.buffer, i);

        // Increment the sent bytes
        data.output.ble.sent_bytes += len;

        // If there is less than one MTU length worth of data length
        if (data.output.file.size <= data.output.ble.sent_bytes + data.output.ble.mtu - 1)
        {
            // Go to the end data state
            data.state.next = DATA_STATE_BLE_CAM_DATA_END;
            break;
        }

        // Otherwise go to middle data state
        data.state.next = DATA_STATE_BLE_CAM_DATA_MIDDLE;
        break;
    }

    case DATA_STATE_BLE_CAM_DATA_MIDDLE:
    LOG("DATA_STATE_BLE_CAM_DATA_MIDDLE");
    {
        size_t i = 0, len;

        // Append the middle of file flag
        data.output.ble.buffer[i++] = BLE_FILE_MIDDLE_FLAG;

        // Append the data into the remaining buffer space
        len = data.output.ble.mtu - i;
        i += data_encode_mem(data.output.ble.buffer + i,
                dummy_large_file + data.output.ble.sent_bytes,
                len);

        // Send the data. If not successful
        ble_raw_tx(data.output.ble.buffer, i);

        // Increment the sent bytes
        data.output.ble.sent_bytes += len;

        // If the user cancels the transfer
        if (data.input.stop_flag)
        {
            // Return to IDLE
            data.state.next = DATA_STATE_IDLE;
            break;
        }

        // If there is less than one MTU length worth of data length
        if (data.output.file.size <=
            data.output.ble.sent_bytes + data.output.ble.mtu - 1)
        {
            // Go to the end data state
            data.state.next = DATA_STATE_BLE_CAM_DATA_END;
            break;
        }

        // Otherwise stay in the middle state
        break;
    }

    case DATA_STATE_BLE_CAM_DATA_END:
    LOG("DATA_STATE_BLE_CAM_DATA_END");
    {
        size_t i = 0;

        // Add the end flag
        data.output.ble.buffer[i++] = BLE_FILE_END_FLAG;

        // Append the data into the remaining buffer space
        i += data_encode_mem(data.output.ble.buffer + i,
               dummy_large_file + data.output.ble.sent_bytes, // TODO spi
               data.output.file.size - data.output.ble.sent_bytes);

        // Send the data
        ble_raw_tx(data.output.ble.buffer, i);

        // Return to IDLE
        data.state.next = DATA_STATE_IDLE;
        break;
    }
    }

    // Increment the seconds timer
    data.state.seconds_elapsed++;

    // If we're changing states
    if (data.state.current != data.state.next)
    {
        // Reset the elapsed time in the current state
        data.state.seconds_elapsed = 0;

        // Set the current state to the next state ready for next entry
        data.state.current = data.state.next;
    }
}

/**
 * @brief Starts/stops a data operation of a given type to the mobile over
 *        BLE, or WiFi to a server.
 * @param op: Type of operation to request.
 * @return One of the status codes from data_op_t.
 */
bool app_data_operation(data_op_t op)
{
    if (data.state.current != DATA_STATE_IDLE)
        return false;

    // Based on the requested action
    switch (op)
    {
    // If a single camera capture
    case DATA_OP_CAMERA_CAPTURE:
    LOG("DATA_OP_CAMERA_CAPTURE");
    {
        // Initiate a capture
        data.input.camera_capture_flag = true;

        break;
    }

    // If a continuos camera stream
    case DATA_OP_CAMERA_STREAM:
    LOG("DATA_OP_CAMERA_STREAM");
    {
        // Initiate a capture
        data.input.camera_stream_flag = true;

        break;
    }

    // If a continuos microphone stream
    case DATA_OP_MICROPHONE_STREAM:
    LOG("DATA_OP_MICROPHONE_STREAM");
    {
        break;
    }

    // If a firmware download is requested
    case DATA_OP_FIRMWARE_DOWNLOAD:
    LOG("DATA_OP_FIRMWARE_DOWNLOAD");
    {
        break;
    }

    // If a bitstream update is requested
    case DATA_OP_BITSTREAM_DOWNLOAD:
    LOG("DATA_OP_BITSTREAM_DOWNLOAD");
    {
        break;
    }

    // If a stop command is requested
    case DATA_OP_STOP:
    LOG("DATA_OP_STOP");
    {
        data.input.stop_flag = true;
        break;
    }

    default:
        break;
    }

    // Start the timer
    timer_add_handler(&data_state_machine);

    return true;
}
