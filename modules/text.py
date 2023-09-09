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


# TODO legacy support for Noa Android/iOS app
FONT_HEIGHT = 48
FONT_WIDTH = 24


class Text:
    def __init__(self, str, x, y, color, justify=TOP_LEFT, font=SYSTEM_FONT):
        self.str = str
        self.color = (color << 8) | 0xFF
        self.font = font
        self.x = x
        self.y = y
        self.justify(justify)

    def justify(self, justify):
        left = (TOP_LEFT, MIDDLE_LEFT, BOTTOM_LEFT)
        center = (TOP_CENTER, MIDDLE_CENTER, BOTTOM_CENTER)
        right = (TOP_RIGHT, MIDDLE_RIGHT, BOTTOM_RIGHT)

        if justify in left:
            self.x = self.x
        elif justify in center:
            self.x = self.x - self.width(self.string) // 2
        elif justify in right:
            self.x = self.x - self.width(self.string)
        else:
            raise ValueError("unknown justify value")

        top = (TOP_LEFT, TOP_CENTER, TOP_RIGHT)
        middle = (MIDDLE_LEFT, MIDDLE_CENTER, MIDDLE_RIGHT)
        bottom = (BOTTOM_LEFT, BOTTOM_CENTER, BOTTOM_RIGHT)

        if justify in top:
            self.y = self.y
        elif justify in middle:
            self.y = self.y - FONT_HEIGHT // 2
        elif justify in bottom:
            self.y = self.y - FONT_HEIGHT
        else:
            raise ValueError("unknown justify value")

    def move(self, x, y):
        self.x += int(x)
        self.y += int(y)
        return self

    def width(self, string):
        # TODO legacy support for Noa Android/iOS app
        return FONT_WIDTH * len(string)

    def sprites(self):
        x = self.x
        y = self.y
        z = 1 # first valid z-index
        sprites = list()
        for ch in self.str:
            if ch == ' ':
                # TODO legacy support for Noa Android/iOS app
                x += FONT_WIDTH
                continue
            sprite = self.font.sprite(ord(ch), self.color)
            sprites.append(sprite.draw(x, y, z))

            # TODO legacy support for Noa Android/iOS app
            #x += sprite.active_width
            x += FONT_WIDTH

            # TODO waiting a bugfix
            y += sprite.height

            # TODO really necessary? Any upper bound?
            z += 1
        return sprites
