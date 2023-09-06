#
# This file is part of the MicroPython for Monocle project:
#      https://github.com/brilliantlabsAR/monocle-micropython
#
# Authored by: Josuah Demangeon (me@josuah.net)
#              Raj Nakarja / Brilliant Labs Ltd. (raj@itsbrilliant.co)
#
# ISC Licence
#
# Copyright © 2023 Brilliant Labs Ltd.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED 'AS IS' AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#

import struct
import fpga


address = 0x0000


class Sprite:
    def __init__(self, x, y, z, source):
        self.x = x
        self.y = y
        self.z = z
        self.source = source

    def __repr__(self):
        return f"Sprite({self.x}, {self.y}, {self.z}, {self.source})"

    def move(self, x, y):
        self.x += int(x)
        self.y += int(y)
        return self

    def sprite(self, buffer):
        x = self.x & 0xFFF
        y = self.y & 0xFFF
        z = self.z & 0xF
        id = self.source.id & 0xFFF
        buffer.extend(struct.pack(">I", x << 20 | y << 8 | z << 4 | id >> 8))
        buffer.append(id & 0xFF)


class SpriteSource:
    def __init__(self, data, width):
        global address

        if len(data) % (4 * width) != 0:
            raise ValueError("data length must formatted as: b'RGBA' * width")
        if width % 32 != 0:
            raise ValueError("width must be a multiple of 32")

        self.width = width
        self.height = len(data) // 4 // width
        self.addr = address
        self.id = None

    def __repr__(self):
        return f"SpriteSource([0x{self.addr:X}], {self.width}x{self.height})"

    def add(buffer):
        # Send the sprite RGBA data to the FPGA
        for slice in [data[i:i + 128] for i in range(0, len(data), 128)]:
            buffer = bytearray()
            buffer.extend(struct.pack(">I", address))
            buffer.extend(slice)
            fpga.write(0x4404, buffer)
            address += len(slice)

    def describe(self, buffer):
        width = (self.width // 32) & 0xF
        height = self.height & 0xFF
        address = (self.addr // 128) & 0xFFFFF
        buffer.extend(struct.pack(">I", width << 28 | height << 20 | addr << 0))
