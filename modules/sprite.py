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


class Sprite:
    class Placement:
        def __init__(self, sprite, x, y, z):
            self.x = x
            self.y = y
            self.z = z
            self.sprite = sprite

        def __repr__(self):
            return f"Sprite.Placement({self.sprite}, {self.x}, {self.y}, {self.z})"

        def move(self, x, y):
            self.x += int(x)
            self.y += int(y)
            return self

        def encode(self, buffer):
            x = self.x & 0xFFF
            y = self.y & 0xFFF
            z = self.z & 0xF
            id = self.sprite.id & 0xFFF
            buffer.extend(struct.pack(">I", x << 20 | y << 8 | z << 4 | id >> 8))
            buffer.append(id & 0xFF)

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
        return f"Sprite([0x{self.addr:X}], {self.width}x{self.height})"

    def data(self, data):
        global sprite_address
        if len(data) % 128 != 0:
            raise ValueError("data must be a multiple of 128 bytes (32 pixels) long")

        # Send the sprite RGBA data to the FPGA
        for slice in [data[i:i + 128] for i in range(0, len(data), 128)]:
            fpga.write(0x4504, struct.pack(">I", sprite_address) + slice)
        sprite_address += len(data)

    def encode(self, buffer):
        width = (self.width // 32) & 0xF
        height = self.height & 0xFF
        addr = (self.addr // 128) & 0xFFFFF
        buffer.extend(struct.pack(">I", width << 28 | height << 20 | addr << 0))

    def draw(self, x, y, z):
        return self.Placement(self, x, y, z)


def show_sprites(placement_list):
    # Send layout description data to the FPGA
    buffer = bytearray()
    buffer.extend(b"\x00\x00")
    for id, sprite in enumerate(set(placement.sprite for placement in placement_list)):
        sprite.id = id
        sprite.encode(buffer)
    print(f"fpga.write(0x4502, {buffer})")
    fpga.write(0x4502, buffer)

    # Send placement data to the FPGA
    buffer = bytearray()
    buffer.extend(b"\x00\x00")
    for placement in placement_list:
        placement.encode(buffer)
    buffer.extend(b"\x00\xFF\xFF\xFF\xFF")
    print(f"fpga.write(0x4503, {buffer})")
    fpga.write(0x4503, buffer)
    fpga.write(0x4501, b"")
