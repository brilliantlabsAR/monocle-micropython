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
 * Configuratin file for the drivers.
 * GPIO pins used, I2C addresses, peripheral instances in use...
 */

#include "pinout.h"
// Bluetooth params

#define BLE_DEVICE_NAME "Monocle"

// 0 is reserved for SoftDevice
#define TIMER_INSTANCE 2
#define TIMER_MAX_HANDLERS 2

// I2C addresses

// SPI

#define SPI_MAX_XFER_LEN 255

// OV5640

#define OV5640_WIDTH 1280
#define OV5640_HEIGHT 720

// ECX664CN

#define ECX336CN_WIDTH 640
#define ECX336CN_HEIGHT 400

// LIGJOJPEG

#define JOJPEG_WIDTH_MAX OV5640_WIDTH

// LIGGFX

#define GFX_OBJ_NUM

// Driver declaration

#define DRIVER(name)                    \
    {                                   \
        static bool ready = false;      \
        if (ready)                      \
            return;                     \
        else                            \
            ready = true;               \
        PRINTF("DRIVER(%s)\r\n", name); \
    }
