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

typedef enum {
    DRIVER_BATTERY,
    DRIVER_BLE,
    DRIVER_ECX336CN,
    DRIVER_FLASH,
    DRIVER_FPGA,
    DRIVER_I2C,
    DRIVER_IQS620,
    DRIVER_MAX77654,
    DRIVER_OV5640,
    DRIVER_SPI,
    DRIVER_TIMER,
    DRIVER_TOUCH,
} driver_num_t;

#define DRIVER(name) if (driver_ready(DRIVER_ ## name, # name)) return

bool driver_ready(uint8_t num, char const *name);
void driver_reset(void);
void driver_self_test(void);
