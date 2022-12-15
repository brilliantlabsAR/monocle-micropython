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
 * Driver for the SPI Sony OLED Microdisplay.
 * No public datasheet seem to be published on the web.
 * The data path is connected directly to the data path to FPGA, the
 * MCU only has access to the SPI configuration interface.
 * After the OLED configuration is done, the luminance (brightness)
 * should be set and the display is ready to receive data.
 */

/** See ECX336CN datasheet section 10.8; luminance values are in cd/m^2 */
typedef enum {
    ECX336CN_DIM     = 1,    // 750  cd/m2
    ECX336CN_LOW     = 2,    // 1250 cd/m2
    ECX336CN_MEDIUM  = 0,    // 2000 cd/m2, this is the default
    ECX336CN_DEFAULT = 0,
    ECX336CN_HIGH    = 3,    // 3000 cd/m2
    ECX336CN_BRIGHT  = 4,    // 4000 cd/m2
} ecx336cn_luminance_t;

void ecx336cn_prepare(void);
void ecx336cn_init(void);
void ecx336cn_deinit(void);
void ecx336cn_set_luminance(ecx336cn_luminance_t level);
void ecx336cn_sleep(void);
void ecx336cn_awake(void);
