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

import sys
import struct
from sprite import Sprite
from glyph import Glyph
from font_unifont import UNIFONT_BIN as font_data


class BytesFile:
    def __init__(self, data):
        self.data = data
        self.offset = 0

    def read(self, size):
        self.offset += size
        return self.data[self.offset - size : self.offset]

    def seek(self, size):
        self.offset = size

    def tell(self):
        return self.offset


class Font:
    def __init__(self, file):
        if isinstance(file, str):
            file = open(file, "rb")
        self.file = file
        self.cache = {}
        self.height = None
        self.index_size = None

        # Read the index header: (u32)*2
        u32x2 = self.file.read(4 + 4)
        self.height, self.index_size = struct.unpack(">II", u32x2)

    def read_next_index(self):
        # Check if the end is reached
        if self.file.tell() >= 4 + 4 + self.index_size:
            return None

        # Parse a record from the file
        return struct.unpack(">II", self.file.read(8))

    def read_next_glyph(self, glyph):
        # Read the glyph header
        glyph.beg_x, glyph.beg_y, glyph.len_x, glyph.len_y = self.file.read(4)

        # Compute the size to read including the last byte
        size = glyph.len_x * glyph.len_y
        size = (size + 7) // 8

        # Optimization: do not split the row/columns yet
        glyph.data = self.file.read(size)

        # Round width to the upper slice of 32
        n = glyph.beg_x + glyph.len_x
        glyph.width = n + 32 - ((n - 1) % 32 + 1)

    def unicode_range(self, unicode):
        # Rewind to the beginning of the index
        self.file.seek(4 + 4)

        # Inline implementation of binary search to find the glyph
        while (row := self.read_next_index()) is not None:
            # Decode the u24 and u8 out of the u32
            unicode_len = row[0] & 0xff
            unicode_beg = row[0] >> 8

            # Check if it is withing the current range
            if unicode > unicode_beg and unicode < unicode_beg + unicode_len:
                return unicode_beg, row[1]

        # Reached the end of the index without finding the glyph
        raise ValueError("glyph not found in font")

    def seek(self, address):
        self.file.seek(4 + 4 + self.index_size + address)

    def sprite(self, unicode, color):
        # Use the per-font caching of the sprite
        if (unicode, color) in self.cache:
            return self.cache[(unicode, color)]

        # Get the glyph for this char from the font
        glyph = Glyph(self, unicode)

        # Build a new sprite from the glyph metadata
        active_width = glyph.beg_x + glyph.len_x
        sprite = Sprite(self.height, glyph.width, active_width)

        # Fill the sprite with the glyph data
        fg = struct.pack('>I', color)
        glyph.draw(sprite.data, fg=fg, bg=b"\x00\x00\x00\x00")

        # Cache the new sprite and return it
        self.cache[(unicode, color)] = sprite
        return sprite


SYSTEM_FONT = Font(BytesFile(font_data))
