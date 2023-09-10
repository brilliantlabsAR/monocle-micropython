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
from font.unifont import UNIFONT_BIN


class BytesFile:
    def __init__(self, data):
        self.data = data
        self.offset = 0

    def read(self, size):
        self.offset += size
        return self.data[self.offset - size : self.offset]

    def seek(self, size):
        self.offset = size


class Font:
    def __init__(self, file):
        if isinstance(file, str):
            file = open(file, "rb")
        self.file = file
        self.cache = {}
        self.height = None
        self.index_size = None
        self.index = []

        # Read the index header: (u32)*2
        record = self.file.read(4 + 4)
        self.height, self.index_size = struct.unpack(">II", record)

        # Then read the index
        self.index = []
        n = 0
        while n < self.index_size:
            # Parse a record from the file
            self.index.append(struct.unpack(">II", self.file.read(8)))
            n += 8
            x = self.index[len(self.index) - 1]

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
        # Start searching from the middle of the file
        beg = 0
        end = len(self.index)
        i = beg + (end - beg) // 2

        # Inline implementation of binary search to find the glyph
        while beg != end:
            # Decode the u8 and u24 out of the u32
            unicode_len = self.index[i][0] & 0xff
            unicode_start = self.index[i][0] >> 8

            # Should we search lower?
            if unicode < unicode_start:
                end = i

            # Should we search higher?
            elif unicode > unicode_start + unicode_len:
                beg = i + 1

            # That's a hit!
            else:
                return unicode_start, self.index[i][1]

            # Adjust the probe
            i = beg + (end - beg) // 2

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


SYSTEM_FONT = Font(BytesFile(UNIFONT_BIN))
