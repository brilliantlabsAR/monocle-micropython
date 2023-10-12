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
import struct
import time
from _display import *
import gc

WIDTH = 640
HEIGHT = 400

SPACE_WIDTH = 1
FONT_HEIGHT = 48
FONT_WIDTH = 23 + SPACE_WIDTH

TOP_LEFT = 1
MIDDLE_LEFT = 2
BOTTOM_LEFT = 3
TOP_CENTER = 4
BOTTOM_CENTER = 5
TOP_RIGHT = 6
MIDDLE_CENTER = 7
MIDDLE_RIGHT = 8
BOTTOM_RIGHT = 9

CLEAR = 0x000000
BLACK = 0x000000
RED = 0xAD2323
GREEN = 0x1D6914
BLUE = 0x2A4BD7
CYAN = 0x29D0D0
MAGENTA = 0x8126C0
YELLOW = 0xFFEE33
WHITE = 0xFFFFFF
GRAY1 = 0x1C1C1C
GRAY2 = 0x383838
GRAY3 = 0x555555
GRAY4 = 0x717171
GRAY5 = 0x8D8D8D
GRAY6 = 0xAAAAAA
GRAY7 = 0xC6C6C6
GRAY8 = 0xE2E2E2


FBTEXT_PAGE_SIZE = 1024
FBTEXT_NUM_PAGES = 2
fbtext_addr = 0


sprite_addr = 0x0000


class Colored:
    def color(self, color_rgb):
        self.color_rgb = color_rgb


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


class TextOverlapError(Exception):
    pass


class Text(Colored):
    def __init__(self, string, x, y, color, justify=TOP_LEFT):
        self.x = int(x)
        self.y = int(y)
        self.string = string
        self.justify(justify)
        self.color(color)

    def __repr__(self):
        return f"Text('{self.string}', {self.x}, {self.y}, 0x{self.color_rgb:06x})"

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

    def width(self, string):
        return FONT_WIDTH * len(string)

    def clip_x(self):
        string = self.string
        x = self.x

        if x < 0:
            i = abs(x) // FONT_WIDTH + 1
            string = string[i:]
            x += i * FONT_WIDTH
        if x + self.width(string) > WIDTH:
            overflow_px = x + self.width(string) - WIDTH
            overflow_ch = overflow_px // FONT_WIDTH + 1
            string = string[:-overflow_ch]
        return x, string

    def fbtext(self, buffer):
        x, string = self.clip_x()
        y = self.y

        if len(string) == 0:
            return

        # Build a buffer to send to the FPGA
        buffer.append((x >> 4) & 0xFF)
        buffer.append(((x << 4) & 0xF0) | ((y >> 8) & 0x0F))
        buffer.append(y & 0xFF)
        buffer.append(self.color_index)
        i = len(buffer)
        buffer.append(0)
        for c in string.encode("ASCII"):
            buffer.append(c - 32)
            buffer[i] += 1  # increment the length field
        assert buffer[i] <= 0xFF

    def move(self, x, y):
        self.x += x
        self.y += y
        return self


class Sprite:
    def __init__(self, x, y, z, source):
        self.x = x
        self.y = y
        self.z = z
        self.source = source

    def __repr__(self):
        return f"Sprite({self.x}, {self.y}, {self.z}, {self.source})"

    def move(self, x, y):
        self.x += int(x)
        self.y += int(y)
        return self

    def sprite(self, buffer):
        x = self.x & 0xFFF
        y = self.y & 0xFFF
        z = self.z & 0xF
        id = self.source.id & 0xFFF
        buffer.extend(struct.pack(">I", x << 20 | y << 8 | z << 4 | id >> 8))
        buffer.append(id & 0xFF)


class SpriteSource:
    def __init__(self, data, width):
        global sprite_addr

        if len(data) % (4 * width) != 0:
            raise ValueError("data length must formatted as: b'RGBA' * width")
        if width % 32 != 0:
            raise ValueError("width must be a multiple of 32")

        self.width = width
        self.height = len(data) // 4 // width
        self.addr = sprite_addr
        self.id = None

        # Send the sprite RGBA data to the FPGA
        for slice in [data[i:i + 128] for i in range(0, len(data), 128)]:
            buffer = bytearray()
            buffer.extend(struct.pack(">I", sprite_addr))
            buffer.extend(slice)
            fpga.write(0x4404, buffer)
            sprite_addr += len(slice)

    def __repr__(self):
        return f"SpriteSource([0x{self.addr:X}], {self.width}x{self.height})"

    def sprite_describe(self, buffer):
        width = (self.width // 32) & 0xF
        height = self.height & 0xFF
        addr = (self.addr // 32) & 0xFFFFF
        buffer.extend(struct.pack(">I", width << 28 | height << 20 | addr << 0))


def flatten(o):
    if isinstance(o, tuple) or isinstance(o, list):
        return [i2 for i1 in o for i2 in flatten(i1)]
    else:
        return [o]


def move(*args):
    for arg in flatten(args[:-2]):
        arg.move(args[-2], args[-1])


def color(*args):
    for arg in flatten(args[:-1]):
        arg.color(args[-1])


def update_colors(addr, obj_list, dump=False):
    # new buffer for the FPGA API, starting with address 0x0000
    buffer = bytearray(2)

    # collect the colors from the various objects
    color_list = []
    for obj in obj_list:
        if obj.color_rgb in color_list:
            # deduplicate the color by using the existing index
            obj.color_index = color_list.index(obj.color_rgb)

        else:
            # Add a new color if enough room
            if len(color_list) > 128:
                raise ValueError("more than 128 different color unsupported")

            # Add to the list for future reference
            obj.color_index = len(color_list)
            color_list.append(obj.color_rgb)

            # Add to the buffer for the FPGA
            buffer.append(obj.color_rgb >> 16)
            buffer.append(obj.color_rgb >> 8)
            buffer.append(obj.color_rgb >> 0)

    # Flush the buffer, we are done
    fpga.write(addr, buffer)

    # Hexdump the buffer if requested
    if dump:
        print("".join("%02X" % x for x in buffer))


def show_sprite(sprites):

    # Send layout description data to the FPGA
    buffer = bytearray()
    buffer.extend(b"\x00\x00")
    for id, item in enumerate(set(item.source for item in sprites)):
        item.id = id
        item.sprite_describe(buffer)
    fpga.write(0x4402, buffer)

    # Send placement data to the FPGA
    buffer = bytearray()
    buffer.extend(b"\x00\x00")
    for item in sprites:
        item.sprite(buffer)
    buffer.extend(b"\x00\xFF\xFF\xFF\xFF")
    fpga.write(0x4403, buffer)


def show_fbtext(fbtext_list):
    global fbtext_addr

    update_colors(0x4502, fbtext_list)

    # Text has no wrapper, we implement it locally.
    # See https://streamlogic.io/docs/reify/nodes/#fbtext
    buffer = bytearray(struct.pack(">H", fbtext_addr))
    fbtext_list = sorted(fbtext_list, key=lambda obj: obj.x)

    # Check for overlapping text
    def box(obj):
        x2 = obj.x + FONT_WIDTH * len(obj.string)
        y2 = obj.y + FONT_HEIGHT
        return obj.x, x2, obj.y, y2
    for a in fbtext_list:
        ax1, ax2, ay1, ay2 = box(a)
        for b in fbtext_list:
            if a is b:
                continue
            bx1, bx2, by1, by2 = box(b)
            if ax1 <= bx2 and ax2 >= bx1:
                if ay1 <= by2 and ay2 >= by1:
                    raise TextOverlapError(f"{a} overlaps with {b}")

    # Render the text
    for obj in fbtext_list:
        obj.fbtext(buffer)
    if len(buffer) > 0:
        addr = bytearray(struct.pack(">H", fbtext_addr))
        fpga.write(0x4501, addr)
        fpga.write(0x4503, buffer + b"\xFF\xFF\xFF")
        fbtext_addr += FBTEXT_PAGE_SIZE
        fbtext_addr %= FBTEXT_PAGE_SIZE * FBTEXT_NUM_PAGES
        time.sleep_ms(20) # ensure the buffer swap has happened


def show_vgr2d(vgr2d_list, dump=False):
    update_colors(0x4402, vgr2d_list, dump=dump)

    # 0 is the address of the frame in the framebuffer in use.
    # See https://streamlogic.io/docs/reify/nodes/#fbgraphics
    # Offset: active display offset in buffer used if double buffering
    vgr2d.display2d(0, [obj.vgr2d() for obj in vgr2d_list], WIDTH, HEIGHT, dump=dump)
    gc.collect() # memory optimization to reduce fragmentation


def show(*args, dump=False):
    args = flatten(args)
    show_vgr2d([obj for obj in args if hasattr(obj, "vgr2d")], dump=dump)
    show_fbtext([obj for obj in args if hasattr(obj, "fbtext")])
    show_sprite([obj for obj in args if hasattr(obj, "sprite")])


def clear():
    show(Text(" ", 0, 0, 0x000000))


def reset():
    global sprite_addr
    sprite_addr = 0x0000
    clear()
