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


class Glyph:
    def __init__(self, font, unicode):
        self.beg_x = 0
        self.beg_y = 0
        self.len_x = 0
        self.len_y = 0
        self.width = 0
        self.data = None
        self.font = font

        # Get the range to start scanning from
        range_start, range_address = font.unicode_range(unicode)

        # Scan through the data glyph per glyph
        font.seek(range_address)

        # Skip several glyphs until we are there
        i = range_start
        for _ in range(unicode - range_start):
            font.read_next_glyph(self)
            i += 1
        font.read_next_glyph(self)

    def draw(self, callback, bg=b" ", fg=b"#"):
        assert len(bg) == len(fg)

        # Fill empty area above the glyph
        for i in range(self.beg_y):
            callback(bg * self.width)

        # Fill the glyph content
        n = 0
        for ch in self.data:
            for i in reversed(range(8)):

                # Pad with background at the left
                if n == 0:
                    canvas = bytearray()
                    canvas.extend(bg * self.beg_x)

                # Fill with foreground or background according to the data
                canvas.extend(fg if ch & (1 << i) else bg)
                n += 1

                # Pad with background at the right
                if n == self.len_x:
                    canvas.extend(bg * (self.width - self.beg_x - self.len_x))
                    callback(canvas)
                    n = 0

        # Fill the rest of the line if needed
        while len(canvas) % (self.width * len(bg)) != 0:
            canvas.extend(bg)
        if len(canvas) > 0:
            callback(canvas)

        # Fill empty area below the glyph
        for i in range(max(self.font.height - self.len_y - self.beg_y, 0)):
            callback(bg * self.width)
