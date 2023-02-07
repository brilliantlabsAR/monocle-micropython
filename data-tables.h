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

#pragma once
#include <stdint.h>

typedef struct {
    uint16_t addr;
    uint8_t value;
} ov5640_conf_t;

extern const size_t ecx336cn_config_len;
extern const uint8_t *ecx336cn_config_p;

extern const size_t ov5640_lightmode_len;
extern const uint8_t (*ov5640_lightmode_p)[7];

extern const size_t ov5640_saturation_len;
extern const uint8_t (*ov5640_saturation_p)[6];

extern const size_t ov5640_effects_len;
extern const uint8_t (*ov5640_effects_p)[3];

extern const size_t ov5640_rgb565_len;
extern const ov5640_conf_t *ov5640_rgb565_p;

extern const size_t ov5640_rgb565_1x_len;
extern const ov5640_conf_t *ov5640_rgb565_1x_p;

extern const size_t ov5640_rgb565_2x_len;
extern const ov5640_conf_t *ov5640_rgb565_2x_p;

extern const size_t ov5640_uxga_init_len;
extern const ov5640_conf_t *ov5640_uxga_init_p;

extern const size_t ov5640_yuv422_direct_len;
extern const ov5640_conf_t *ov5640_yuv422_direct_p;

extern const size_t ov5640_af_config_len;
extern const uint8_t *ov5640_af_config_p;
