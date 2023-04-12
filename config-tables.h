/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Ltd. (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Ltd.
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

typedef struct display_config_t
{
    uint8_t address;
    uint8_t value;
} display_config_t;

const display_config_t display_config[] = {
    {0x00, 0x92},
    {0x01, 0x20},
    {0x02, 0x00},
    {0x03, 0x20},
    {0x04, 0x3F},
    {0x05, 0xC8},
    {0x06, 0x00},
    {0x07, 0x40},
    {0x08, 0x80},
    {0x09, 0x00},
    {0x0A, 0x10},
    {0x0B, 0x00},
    {0x0C, 0x00},
    {0x0D, 0x00},
    {0x0E, 0x00},
    {0x0F, 0x56},
    {0x10, 0x00},
    {0x11, 0x00},
    {0x12, 0x00},
    {0x13, 0x00},
    {0x14, 0x00},
    {0x15, 0x00},
    {0x16, 0x00},
    {0x17, 0x00},
    {0x18, 0x00},
    {0x19, 0x00},
    {0x1A, 0x00},
    {0x1B, 0x00},
    {0x1C, 0x00},
    {0x1D, 0x00},
    {0x1E, 0x00},
    {0x1F, 0x00},
    {0x20, 0x01},
    {0x21, 0x00},
    {0x22, 0x40},
    {0x23, 0x40},
    {0x24, 0x40},
    {0x25, 0x80},
    {0x26, 0x40},
    {0x27, 0x40},
    {0x28, 0x40},
    {0x29, 0x0B},
    {0x2A, 0xBE},
    {0x2B, 0x3C},
    {0x2C, 0x02},
    {0x2D, 0x7A},
    {0x2E, 0x02},
    {0x2F, 0xFA},
    {0x30, 0x26},
    {0x31, 0x01},
    {0x32, 0xB6},
    {0x33, 0x00},
    {0x34, 0x03},
    {0x35, 0x5A},
    {0x36, 0x00},
    {0x37, 0x76},
    {0x38, 0x02},
    {0x39, 0xFE},
    {0x3A, 0x02},
    {0x3B, 0x0D},
    {0x3C, 0x00},
    {0x3D, 0x1B},
    {0x3E, 0x00},
    {0x3F, 0x1C},
    {0x40, 0x01},
    {0x41, 0xF3},
    {0x42, 0x01},
    {0x43, 0xF4},
    {0x44, 0x80},
    {0x45, 0x00},
    {0x46, 0x00},
    {0x47, 0x2D},
    {0x48, 0x08},
    {0x49, 0x01},
    {0x4A, 0x7E},
    {0x4B, 0x08},
    {0x4C, 0x0A},
    {0x4D, 0x04},
    {0x4E, 0x00},
    {0x4F, 0x3A},
    {0x50, 0x01},
    {0x51, 0x58},
    {0x52, 0x01},
    {0x53, 0x2D},
    {0x54, 0x01},
    {0x55, 0x15},
    {0x56, 0x00},
    {0x57, 0x2B},
    {0x58, 0x11},
    {0x59, 0x02},
    {0x5A, 0x11},
    {0x5B, 0x02},
    {0x5C, 0x25},
    {0x5D, 0x04},
    {0x5E, 0x0B},
    {0x5F, 0x00},
    {0x60, 0x23},
    {0x61, 0x02},
    {0x62, 0x1A},
    {0x63, 0x00},
    {0x64, 0x0A},
    {0x65, 0x01},
    {0x66, 0x8C},
    {0x67, 0x30},
    {0x68, 0x00},
    {0x69, 0x00},
    {0x6A, 0x00},
    {0x6B, 0x00},
    {0x6C, 0x00},
    {0x6D, 0x00},
    {0x6E, 0x00},
    {0x6F, 0x60},
    {0x70, 0x00},
    {0x71, 0x00},
    {0x72, 0x00},
    {0x73, 0x00},
    {0x74, 0x00},
    {0x75, 0x00},
    {0x76, 0x00},
    {0x77, 0x00},
    {0x78, 0x00},
    {0x79, 0x68},
    {0x7A, 0x00},
    {0x7B, 0x00},
    {0x7C, 0x00},
    {0x7D, 0x00},
    {0x7E, 0x00},
    {0x7F, 0x00},
    {0x00, 0x93},
};

typedef struct camera_config_t
{
    uint16_t address;
    uint8_t value;
} camera_config_t;

// Useful resources for the camera configuration. The table below is based on
// the Linux driver, along with some tweaks to enable MIPI and set resolution
// https://github.com/adafruit/Adafruit_CircuitPython_OV5640/blob/main/adafruit_ov5640.py
// https://github.com/torvalds/linux/blob/master/drivers/media/i2c/ov5640.c

const camera_config_t camera_config[] = {
    {0x3008, 0x42},       // Enter software standby mode
    {0x3103, 0x03},       // Undocumented. Enables PLL
    {0x3017, 0xff},       // IO control enables all pins as outputs
    {0x3018, 0xff},       // IO control enables all pins as outputs
    {0x3035, 0x41},       // PLL control slow down all clocks by 4
    {0x3037, 0x13},       // PLL control root divide by 2
    {0x3108, 0x01},       // Undocumented
    {0x3630, 0x36},       // Undocumented
    {0x3631, 0x0e},       // Undocumented
    {0x3632, 0xe2},       // Undocumented
    {0x3633, 0x12},       // Undocumented
    {0x3621, 0xe0},       // Undocumented
    {0x3704, 0xa0},       // Undocumented
    {0x3703, 0x5a},       // Undocumented
    {0x3715, 0x78},       // Undocumented
    {0x3717, 0x01},       // Undocumented
    {0x370b, 0x60},       // Undocumented
    {0x3705, 0x1a},       // Undocumented
    {0x3905, 0x02},       // Undocumented
    {0x3906, 0x10},       // Undocumented
    {0x3901, 0x0a},       // Undocumented
    {0x3731, 0x12},       // Undocumented
    {0x3600, 0x08},       // VCM control debug mode
    {0x3601, 0x33},       // VCM control debug mode
    {0x302d, 0x60},       // Datasheet says not to change this, but it's set in the linux driver
    {0x3620, 0x52},       // Undocumented
    {0x371b, 0x20},       // Undocumented
    {0x471c, 0x50},       // Undocumented
    {0x3a13, 0x43},       // Exposure control pre-gain = x1.047
    {0x3a18, 0x00},       // Exposure control gain ceiling MSB
    {0x3a19, 0xf8},       // Exposure control gain ceiling LSB. Total = x15.5
    {0x3635, 0x13},       // Undocumented
    {0x3636, 0x03},       // Undocumented
    {0x3634, 0x40},       // Undocumented
    {0x3622, 0x01},       // Undocumented
    {0x3c01, 0xa4},       // 50/60Hz flicker detection into auto mode
    {0x3c04, 0x28},       // 50/60Hz flicker detection low sum threshold
    {0x3c05, 0x98},       // 50/60Hz flicker detection high sum threshold
    {0x3c06, 0x00},       // 50/60Hz flicker detection light meter threshold 1 MSB
    {0x3c07, 0x08},       // 50/60Hz flicker detection light meter threshold 1 LSB
    {0x3c08, 0x00},       // 50/60Hz flicker detection light meter threshold 2 MSB
    {0x3c09, 0x1c},       // 50/60Hz flicker detection light meter threshold 2 LSB
    {0x3c0a, 0x9c},       // 50/60Hz flicker detection sample number MSB
    {0x3c0b, 0x40},       // 50/60Hz flicker detection sample number LSB
    {0x3810, 0x00},       // Timing Hoffset[11:8]
    {0x3811, 0x10},       // Timing Hoffset[7:0] HOFFSET = 16 = 0x10
    {0x3812, 0x00},       // Timing Voffset[10:8]
    {0x3813, 0x2e},       // Timing Voffset[7:0] VOFFSET = 46 = 0x2E
    {0x4740, 0x21},       // VSync
    {0x3820, 0x47},       // Flip image vertically
    {0x3814, 0x31},       // Timing Y increment
    {0x3815, 0x31},       // Timing X increment
    {0x3800, 0x00},       // Timing X start address MSB
    {0x3801, 0x10},       // Timing X start address LSB
    {0x3802, 0x00},       // Timing Y start address MSB
    {0x3803, 0x0E},       // Timing Y start address LSB
    {0x3804, 0x0a},       // Timing X end address MSB
    {0x3805, 0x2f},       // Timing X end address LSB
    {0x3806, 0x06},       // Timing Y end address MSB
    {0x3807, 0xa9},       // Timing Y end address LSB
    {0x3808, 640 >> 8},   // Timing X output size MSB
    {0x3809, 640 & 0xFF}, // Timing X output size LSB. 640px
    {0x380a, 400 >> 8},   // Timing Y output size MSB
    {0x380b, 400 & 0xFF}, // Timing Y output size LSB. 400px
    {0x380c, 0x05},       // Timing X total size MSB
    {0x380d, 0xF8},       // Timing X total size MSB
    {0x380e, 0x03},       // Timing Y total size MSB
    {0x380f, 0x84},       // Timing Y total size LSB
    {0x3618, 0x00},       // Undocumented
    {0x3612, 0x29},       // Undocumented
    {0x3708, 0x64},       // Undocumented
    {0x3709, 0x52},       // Undocumented
    {0x370c, 0x03},       // Undocumented
    {0x3a02, 0x03},       // Auto exposure 60Hz max output limit MSB
    {0x3a03, 0xd8},       // Auto exposure 60Hz max output limit LSB
    {0x3a08, 0x01},       // Auto exposure 50Hz bandwidth MSB
    {0x3a09, 0x27},       // Auto exposure 50Hz bandwidth LSB
    {0x3a0a, 0x00},       // Auto exposure 60Hz bandwidth MSB
    {0x3a0b, 0xf6},       // Auto exposure 60Hz bandwidth LSB
    {0x3a0e, 0x03},       // Auto exposure 50Hz max bands in one frame
    {0x3a0d, 0x04},       // Auto exposure 60Hz max bands in one frame
    {0x3a14, 0x03},       // Auto exposure 50Hz max output limit MSB
    {0x3a15, 0xd8},       // Auto exposure 50Hz max output limit LSB
    {0x4001, 0x02},       // Black level control start line
    {0x4004, 0x02},       // Black level control line number
    {0x3000, 0x00},       // Enable all system blocks
    {0x3004, 0xff},       // Enable all clocks
    {0x3006, 0xc3},       // Disable JPEG x2 clock
    {0x302e, 0x08},       // Undocumented
    {0x4300, 0x30},       // Format YUV422 and sequence YUYV
    {0x501f, 0x00},       // ISP format control as YUV422
    {0x4407, 0x04},       // JPEG quantization scale
    {0x440e, 0x00},       // JPEG control debug mode
    {0x460b, 0x35},       // VFIFO debug mode
    {0x460c, 0x20},       // DVP PCLK in auto mode
    {0x4837, 0x16},       // MIPI pixel clock period
    {0x3824, 0x04},       // PCLK divider not used if in auto mode as above
    {0x5000, 0xa7},       // ISP enable everything in CONTORL 00
    {0x5001, 0xa3},       // ISP enable everything in CONTROL 01 except UV average
    {0x5002, 0x80},       // ISP enable scaling in CONTROL 02 (undocumented: guessed)
    {0x3503, 0x00},       // Auto exposure and auto gain on
    {0x5180, 0xff},       // Auto white balance B block
    {0x5181, 0xf2},       // Auto white balance max step local, max step fast, one zone
    {0x5182, 0x00},       // Auto white balance local counters
    {0x5183, 0x14},       // Auto white balance SIMF and window
    {0x5186, 0x09},       // Auto white balance advanced control register
    {0x5187, 0x09},       // Auto white balance advanced control register
    {0x5188, 0x09},       // Auto white balance advanced control register
    {0x5189, 0x88},       // Auto white balance advanced control register
    {0x518a, 0x54},       // Auto white balance advanced control register
    {0x518b, 0xee},       // Auto white balance advanced control register
    {0x518c, 0xb2},       // Auto white balance advanced control register
    {0x518d, 0x50},       // Auto white balance advanced control register
    {0x518e, 0x34},       // Auto white balance advanced control register
    {0x518f, 0x6b},       // Auto white balance advanced control register
    {0x5190, 0x46},       // Auto white balance advanced control register
    {0x5191, 0xf8},       // Auto white balance top limit
    {0x5192, 0x04},       // Auto white balance bottom limit
    {0x5193, 0x70},       // Auto white balance red limit
    {0x5194, 0xf0},       // Auto white balance green limit
    {0x5195, 0xf0},       // Auto white balance blue limit
    {0x5197, 0x01},       // Auto white balance local limit
    {0x5198, 0x04},       // Auto white balance debug
    {0x5199, 0x6c},       // Auto white balance debug
    {0x519a, 0x04},       // Auto white balance debug
    {0x519b, 0x00},       // Auto white balance debug
    {0x519c, 0x09},       // Auto white balance debug
    {0x519d, 0x2b},       // Auto white balance debug
    {0x519e, 0x38},       // Auto white balance debug
    {0x5381, 0x1e},       // Color matrix CMX1 for Y
    {0x5382, 0x5b},       // Color matrix CMX2 for Y
    {0x5383, 0x08},       // Color matrix CMX3 for Y
    {0x5384, 0x0a},       // Color matrix CMX4 for U
    {0x5385, 0x7e},       // Color matrix CMX5 for U
    {0x5386, 0x88},       // Color matrix CMX6 for U
    {0x5387, 0x7c},       // Color matrix CMX7 for V
    {0x5388, 0x6c},       // Color matrix CMX8 for V
    {0x5389, 0x10},       // Color matrix CMX9 for V
    {0x538a, 0x01},       // Color matrix sign for CMX9
    {0x538b, 0x98},       // Color matrix sign for CMX8 - CMX1
    {0x5300, 0x08},       // Colour interpolation sharpen threshold 1
    {0x5301, 0x30},       // Colour interpolation sharpen threshold 2
    {0x5302, 0x10},       // Colour interpolation sharpen mt offset 1
    {0x5303, 0x00},       // Colour interpolation sharpen mt offset 2
    {0x5304, 0x08},       // Colour interpolation DNS threshold 1
    {0x5305, 0x30},       // Colour interpolation DNS threshold 2
    {0x5306, 0x08},       // Colour interpolation DNS offset 1
    {0x5307, 0x16},       // Colour interpolation DNS offset 2
    {0x5309, 0x08},       // Colour interpolation sharpen TH threshold 1
    {0x530a, 0x30},       // Colour interpolation sharpen TH threshold 2
    {0x530b, 0x04},       // Colour interpolation sharpen TH offset 1
    {0x530c, 0x06},       // Colour interpolation sharpen TH offset 2
    {0x5480, 0x01},       // Gamma control bias on
    {0x5481, 0x08},       // Gamma control YST00
    {0x5482, 0x14},       // Gamma control YST01
    {0x5483, 0x28},       // Gamma control YST02
    {0x5484, 0x51},       // Gamma control YST03
    {0x5485, 0x65},       // Gamma control YST04
    {0x5486, 0x71},       // Gamma control YST05
    {0x5487, 0x7d},       // Gamma control YST06
    {0x5488, 0x87},       // Gamma control YST07
    {0x5489, 0x91},       // Gamma control YST08
    {0x548a, 0x9a},       // Gamma control YST09
    {0x548b, 0xaa},       // Gamma control YST0A
    {0x548c, 0xb8},       // Gamma control YST0B
    {0x548d, 0xcd},       // Gamma control YST0C
    {0x548e, 0xdd},       // Gamma control YST0D
    {0x548f, 0xea},       // Gamma control YST0E
    {0x5490, 0x1d},       // Gamma control YST0F
    {0x5580, 0x02},       // Special digital effects saturation enable
    {0x5583, 0x40},       // Special digital effects saturation U
    {0x5584, 0x10},       // Special digital effects saturation V
    {0x5589, 0x10},       // Special digital effects UV adjust threshold 1
    {0x558a, 0x00},       // Special digital effects UV adjust threshold 2 MSB
    {0x558b, 0xf8},       // Special digital effects UV adjust threshold 2 LSB
    {0x5800, 0x23},       // Lens correction green matrix
    {0x5801, 0x14},       // Lens correction green matrix
    {0x5802, 0x0f},       // Lens correction green matrix
    {0x5803, 0x0f},       // Lens correction green matrix
    {0x5804, 0x12},       // Lens correction green matrix
    {0x5805, 0x26},       // Lens correction green matrix
    {0x5806, 0x0c},       // Lens correction green matrix
    {0x5807, 0x08},       // Lens correction green matrix
    {0x5808, 0x05},       // Lens correction green matrix
    {0x5809, 0x05},       // Lens correction green matrix
    {0x580a, 0x08},       // Lens correction green matrix
    {0x580b, 0x0d},       // Lens correction green matrix
    {0x580c, 0x08},       // Lens correction green matrix
    {0x580d, 0x03},       // Lens correction green matrix
    {0x580e, 0x00},       // Lens correction green matrix
    {0x580f, 0x00},       // Lens correction green matrix
    {0x5810, 0x03},       // Lens correction green matrix
    {0x5811, 0x09},       // Lens correction green matrix
    {0x5812, 0x07},       // Lens correction green matrix
    {0x5813, 0x03},       // Lens correction green matrix
    {0x5814, 0x00},       // Lens correction green matrix
    {0x5815, 0x01},       // Lens correction green matrix
    {0x5816, 0x03},       // Lens correction green matrix
    {0x5817, 0x08},       // Lens correction green matrix
    {0x5818, 0x0d},       // Lens correction green matrix
    {0x5819, 0x08},       // Lens correction green matrix
    {0x581a, 0x05},       // Lens correction green matrix
    {0x581b, 0x06},       // Lens correction green matrix
    {0x581c, 0x08},       // Lens correction green matrix
    {0x581d, 0x0e},       // Lens correction green matrix
    {0x581e, 0x29},       // Lens correction green matrix
    {0x581f, 0x17},       // Lens correction green matrix
    {0x5820, 0x11},       // Lens correction green matrix
    {0x5821, 0x11},       // Lens correction green matrix
    {0x5822, 0x15},       // Lens correction green matrix
    {0x5823, 0x28},       // Lens correction green matrix
    {0x5824, 0x46},       // Lens correction blue and red matrix
    {0x5825, 0x26},       // Lens correction blue and red matrix
    {0x5826, 0x08},       // Lens correction blue and red matrix
    {0x5827, 0x26},       // Lens correction blue and red matrix
    {0x5828, 0x64},       // Lens correction blue and red matrix
    {0x5829, 0x26},       // Lens correction blue and red matrix
    {0x582a, 0x24},       // Lens correction blue and red matrix
    {0x582b, 0x22},       // Lens correction blue and red matrix
    {0x582c, 0x24},       // Lens correction blue and red matrix
    {0x582d, 0x24},       // Lens correction blue and red matrix
    {0x582e, 0x06},       // Lens correction blue and red matrix
    {0x582f, 0x22},       // Lens correction blue and red matrix
    {0x5830, 0x40},       // Lens correction blue and red matrix
    {0x5831, 0x42},       // Lens correction blue and red matrix
    {0x5832, 0x24},       // Lens correction blue and red matrix
    {0x5833, 0x26},       // Lens correction blue and red matrix
    {0x5834, 0x24},       // Lens correction blue and red matrix
    {0x5835, 0x22},       // Lens correction blue and red matrix
    {0x5836, 0x22},       // Lens correction blue and red matrix
    {0x5837, 0x26},       // Lens correction blue and red matrix
    {0x5838, 0x44},       // Lens correction blue and red matrix
    {0x5839, 0x24},       // Lens correction blue and red matrix
    {0x583a, 0x26},       // Lens correction blue and red matrix
    {0x583b, 0x28},       // Lens correction blue and red matrix
    {0x583c, 0x42},       // Lens correction blue and red matrix
    {0x583d, 0xce},       // Lens correction BR offset
    {0x5025, 0x00},       // Undocumented
    {0x3a0f, 0x30},       // Auto exposure stable high range limit (enter)
    {0x3a10, 0x28},       // Auto exposure stable low range limit (enter)
    {0x3a1b, 0x30},       // Auto exposure stable high range limit (go out)
    {0x3a1e, 0x26},       // Auto exposure stable low range limit (go out)
    {0x3a11, 0x60},       // Auto exposure fast zone high limit
    {0x3a1f, 0x14},       // Auto exposure fast zone low limit
    {0x3008, 0x02},       // Bring out of reset
};
