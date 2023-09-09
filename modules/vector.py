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

import vgr2d
import fpga
import gc
from display import WIDTH, HEIGHT
from color import Colored


class Line(Colored):
    def __init__(self, x1, y1, x2, y2, color, thickness=1):
        if thickness > 18:
            raise ValueError("max thickness is 18")
        self.x1 = int(x1)
        self.y1 = int(y1)
        self.x2 = int(x2)
        self.y2 = int(y2)
        self.width = thickness
        self.color(color)

    def __repr__(self):
        return f"Line({self.x1}, {self.y1}, {self.x2}, {self.y2}, 0x{self.color_rgb:06x}, thickness={self.width})"

    def move(self, x, y):
        self.x1 += int(x)
        self.y1 += int(y)
        self.x2 += int(x)
        self.y2 += int(y)
        return self

    def vgr2d(self):
        return vgr2d.Line(
            self.x1, self.y1, self.x2, self.y2, self.color_index, self.width
        )


class HLine(Line):
    def __init__(self, x, y, width, color, thickness=1):
        super().__init__(x, y, x + width, y, color, thickness=thickness)


class VLine(Line):
    def __init__(self, x, y, height, color, thickness=1):
        super().__init__(x, y, x, y + height, color, thickness=thickness)


class Rectangle(Colored):
    def __init__(self, x1, y1, x2, y2, color):
        self.x = min(x1, x2)
        self.y = min(y1, y2)
        self.width = abs(x2 - x1)
        self.height = abs(y2 - y1)
        self.color(color)

    def __repr__(self):
        x2 = self.x + self.width
        y2 = self.y + self.height
        return f"Rectangle({self.x}, {self.y}, {x2}, {y2}, 0x{self.color_rgb:06x})"

    def move(self, x, y):
        self.x += int(x)
        self.y += int(y)
        return self

    def vgr2d(self):
        v = vgr2d.Rect(self.width, self.height, self.color_index)
        return v.position(self.x, self.y)


class Fill(Rectangle):
    def __init__(self, color):
        super().__init__(0, 0, WIDTH - 1, HEIGHT - 1, color)


class Polyline(Colored):
    def __init__(self, coordinates, color, thickness=1):
        if len(coordinates) % 2 != 0:
            raise ValueError("coordinates must have odd number of values")
        self.points = []
        for i in range(0, len(coordinates), 2):
            self.points.append((coordinates[i], coordinates[i + 1]))
        self.width = thickness
        self.color(color)

    def __repr__(self):
        points = ", ".join([f"{p[0]},{p[1]}" for p in self.points])
        return f"Polyline([{points}], 0x{self.color_rgb:06x}, thickness={self.width})"

    def move(self, x, y):
        for i, value in enumerate(self.points):
            self.points[i] = (value[0] + int(x), value[1] + int(y))

    def vgr2d(self):
        return vgr2d.Polyline(self.points, self.color_index, self.width)


class Polygon(Colored):
    def __init__(self, coordinates, color, thickness=1):
        if len(coordinates) % 2 != 0:
            raise ValueError("coordinates must have odd number of values")
        self.points = []
        for i in range(0, len(coordinates), 2):
            self.points.append((coordinates[i], coordinates[i + 1]))
        self.width = thickness
        self.color(color)

    def __repr__(self):
        points = ", ".join([f"{p[0]},{p[1]}" for p in self.points])
        return f"Polygon([{points}], 0x{self.color_rgb:06x}, thickness={self.width})"

    def move(self, x, y):
        for i, value in enumerate(self.points):
            self.points[i] = (value[0] + int(x), value[1] + int(y))

    def vgr2d(self):
        return vgr2d.Polygon(
            self.points, stroke=None, fill=self.color_index, width=self.width
        )


def show_vgr2d(vgr2d_list, dump=False):
    # 0 is the address of the frame in the framebuffer in use.
    # See https://streamlogic.io/docs/reify/nodes/#fbgraphics
    # Offset: active display offset in buffer used if double buffering
    vgr2d.display2d(0, [obj.vgr2d() for obj in vgr2d_list], WIDTH, HEIGHT)
    gc.collect() # memory optimization to reduce fragmentation
