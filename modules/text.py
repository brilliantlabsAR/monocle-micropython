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
from sprite import Sprite, show_sprites
from font import SYSTEM_FONT


TOP_LEFT = 1
MIDDLE_LEFT = 2
BOTTOM_LEFT = 3
TOP_CENTER = 4
BOTTOM_CENTER = 5
TOP_RIGHT = 6
MIDDLE_CENTER = 7
MIDDLE_RIGHT = 8
BOTTOM_RIGHT = 9

SPACE_WIDTH = 20
INTER_CHAR_WIDTH = 4


class Text:
    def __init__(self, str, x, y, color, font=SYSTEM_FONT):
        self.str = str
        self.color = color
        self.font = font
        self.x = x
        self.y = y

    def sprites(self):
        x = self.x
        y = self.y
        z = 1 # first valid z-index
        sprites = list()
        for ch in self.str:
            if ch == ' ':
                x += SPACE_WIDTH
                continue
            sprite = self.font.sprite(ord(ch), self.color)
            sprites.append(sprite.draw(x, y, z))
            x += sprite.active_width + INTER_CHAR_WIDTH
            y += sprite.height # TODO remove
            z += 1
        return sprites
