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
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#

import vgr2d

BLACK   = 0
RED     = 1
GREEN   = 2
BLUE    = 3
CYAN    = 4
MAGENTA = 5
YELLOW  = 6
WHITE   = 7
GRAY1   = 8
GRAY2   = 9
GRAY3   = 10
GRAY4   = 11
GRAY5   = 12
GRAY6   = 13
GRAY7   = 14
GRAY8   = 15

class Line(vgr2d.Line):
    type = "vgr2d"
    move = vgr2d.Line.position

class Rect(vgr2d.Rect):
    type = "vgr2d"
    move = vgr2d.Rect.position

class Polyline(vgr2d.Polyline):
    type = "vgr2d"
    move = vgr2d.Polyline.position

class Polygon(vgr2d.Polygon):
    type = "vgr2d"
    move = vgr2d.Polygon.position

class Text:
    type = "text"

    def __init__(self, str, color):
        self.x = 0
        self.y = 0
        self.str = str
        self.color = color

    def move(self, x, y):
        self.x = x
        self.y = y

def show_text(list):
    for text in list:
        assert len(text) <= 0xFF
        header = bytearray(5 + len(text))
        header[0] = (text.x >> 4) & 0xFF
        header[1] = ((text.x << 4) & 0xF0) | ((text.y >> 8) & 0x0F)
        header[2] = text.y & 0xFF
        header[3] = text.color
        header[4] = len(text)
        fpga.write(0x__03, header + text)

def show(list):
    # 0 is the address of the frame in the framebuffer in use.
    # See https://streamlogic.io/docs/reify/nodes/#fbgraphics
    # Offset: active display offset in buffer used if double buffering
    vgr2d.display2d(0, [x for x in list if x.type == "vgr2d"])

    # Text has no wrapper, we implement it locally.
    # See https://streamlogic.io/docs/reify/nodes/#fbtext
    show_text([x for x in list if x.type == "text"])
