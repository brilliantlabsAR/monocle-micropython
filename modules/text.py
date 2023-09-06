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
from sprite import SpriteSource, Sprite


SPACE_WIDTH = 20
INTER_CHAR_WIDTH = 32

sprite_source_map = {}


class Text:
    def __init__(self, str, font, color):
        self.str = str
        self.color = color
        self.font = font
        self.x = 20
        self.y = 20

    def to_sprites(self):
        x = self.x
        z = 1
        sprites = list()
        self.load_glyphs()
        for ch in self.str:
            if ch == ' ':
                x += SPACE_WIDTH
                continue
            sprite_source = sprite_source_map[ch]
            sprites.append(Sprite(sprite_source, x, self.y, z))
            x += sprite_source.width + INTER_CHAR_WIDTH
            z += 1
        return sprites

    def load_glyphs(self):
        global sprite_source_map

        color = struct.pack(">I", self.color)
        for ch in self.str:
            if ch == ' ':
                continue
            if ch not in sprite_source_map:
                sprite_source = SpriteSource.from_char(ch, self.font, color)
                sprite_source_map[ch] = sprite_source
