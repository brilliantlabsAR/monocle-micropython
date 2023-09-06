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


class Glyph:
    def __init__(self, file, offset):
        self.beg_x = 0
        self.beg_y = 0
        self.len_x = 0
        self.len_y = 0
        self.width = 0
        self.data = None
        self.file = file
        self.file.seek(offset)

    def read(self):
        # Read the glyph header
        self.beg_x, self.beg_y, self.len_x, self.len_y = self.file.read(4)

        # Compute the size to read including the last byte
        size = self.len_x * self.len_y
        size = (size + 7) // 8

        # Optimization: do not split the row/columns yet
        self.data = self.file.read(size)

        # Get the ceiling with a granularity of 32
        self.width = self.len_x + 32 - ((self.len_x - 1) % 32 + 1)


class Font:
    def __init__(self, path):
        self.file = open(path, "rb")

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

    def glyph_range(self, unicode):
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

    def glyph(self, unicode):

        # Get the range to start scanning from
        range_start, range_address = self.glyph_range(unicode)

        # Scan through the data glyph per glyph
        glyph = Glyph(self.file, 4 + 4 + self.index_size + range_address)

        # Skip several glyphs until we are there
        for _ in range(unicode - range_start):
            glyph.read()
        glyph.read()

        return glyph

    def render(self, glyph, callback, args, bg=b" ", fg=b"#"):

        # Fill empty area above the glyph
        for i in range(glyph.beg_y):
            callback(bg * glyph.width, args)

        # Fill the glyph content
        n = 0
        canvas = bytearray()
        for ch in glyph.data:
            for i in reversed(range(8)):

                # Pad with background at the right
                if n > 0 and n % glyph.len_x == 0:
                    canvas.extend((glyph.width - glyph.len_x) * bg)
                    callback(canvas, args)
                    canvas = bytearray()

                # Fill with foreground or background according to the data
                canvas.extend(fg if ch & (1 << i) else bg)
                n += 1

        # Fill the rest of the line if needed
        while len(canvas) % glyph.width != 0:
            canvas.extend(bg)
        if len(canvas) > 0:
            callback(canvas, args)

        # Fill empty area below the glyph
        for i in range(max(self.height - glyph.len_y - glyph.beg_y, 0)):
            callback(bg * glyph.width, args)

        return canvas
