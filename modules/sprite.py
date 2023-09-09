#
# This file is part of the MicroPython for Monocle project:
#      https://github.com/brilliantlabsAR/monocle-micropython
#
# Authored by: Josuah Demangeon (me@josuah.net)
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


sprite_address = 0x0000


class SpriteSource:
    def __init__(self, height, width, active_width):
        global sprite_address

        if width % 32 != 0:
            raise ValueError("width must be a multiple of 32")
        self.width = width
        # Horizontal size with actual data
        self.active_width = active_width
        self.height = height
        self.addr = sprite_address
        self.id = None

    def __repr__(self):
        return f"SpriteSource([0x{self.addr:X}], {self.width}x{self.height})"

    @classmethod
    def from_glyph(cls, glyph, color):
        source = cls(glyph.font.height, glyph.width, glyph.len_x)
        glyph.render(source.add_data, fg=color, bg=b"\x00\x00\x00\x00")
        return source

    @classmethod
    def from_char(cls, char, font, color):
        return cls.from_glyph(font.glyph(ord(char)), color)

    def add_data(self, data):
        global sprite_address
        if len(data) != 128:
            raise ValueError("data must be 128 bytes long")
        fpga.write(0x4404, struct.pack(">I", sprite_address) + data)
        sprite_address += len(data)

    def describe(self, buffer):
        width = (self.width // 32) & 0xF
        height = self.height & 0xFF
        addr = (self.addr // 128) & 0xFFFFF
        buffer.extend(struct.pack(">I", width << 28 | height << 20 | addr << 0))


class Sprite:
    def __init__(self, source, x, y, z):
        self.x = x
        self.y = y
        self.z = z
        self.source = source

    def __repr__(self):
        return f"Sprite({self.source}, {self.x}, {self.y}, {self.z})"

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


def show_sprites(sprites):
    # Send layout description data to the FPGA
    buffer = bytearray()
    buffer.extend(b"\x00\x00")
    for id, item in enumerate(set(item.source for item in sprites)):
        item.id = id
        item.describe(buffer)
    print(f"fpga.write(0x4402, {buffer})")
    fpga.write(0x4402, buffer)

    # Send placement data to the FPGA
    buffer = bytearray()
    buffer.extend(b"\x00\x00")
    for item in sprites:
        item.sprite(buffer)
    buffer.extend(b"\x00\xFF\xFF\xFF\xFF")
    print(f"fpga.write(0x4403, {buffer})")
    fpga.write(0x4403, buffer)
