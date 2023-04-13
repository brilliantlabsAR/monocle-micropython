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
import fpga
from __display import *

WIDTH   = 640
HEIGHT  = 400

FONT_HEIGHT = 64
FONT_WIDTH = 32

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

TOP_LEFT        = 1
MIDDLE_LEFT     = 2
BOTTOM_LEFT     = 3
TOP_CENTER      = 4
BOTTOM_CENTER   = 5
TOP_RIGHT       = 6
MIDDLE_CENTER   = 7
MIDDLE_RIGHT    = 8
BOTTOM_RIGHT    = 9

class Line:
  type = "vgr2d"

  def __init__(self, x1, y1, x2, y2, color, thickness=1):
    self.x1 = int(x1)
    self.y1 = int(y1)
    self.x2 = int(x2)
    self.y2 = int(y2)
    self.color = color
    self.width = thickness

  def __repr__(self):
    return f"Line({self.x1}, {self.y1}, {self.x2}, {self.y2}, {self.color}, thickness={self.width})"

  def move(self, x, y):
    self.x1 += int(x)
    self.y1 += int(y)
    self.x2 += int(x)
    self.y2 += int(y)
    return self

  def vgr2d(self):
    return vgr2d.Line(self.x1, self.y1, self.x2, self.y2, self.color, self.width)

class Rectangle:
  type = "vgr2d"

  def __init__(self, x1, y1, x2, y2, color):
    self.x = min(x1, x2)
    self.y = min(y1, y2)
    self.width = abs(x2 - x1)
    self.height = abs(y2 - y1)
    self.color = color

  def __repr__(self):
    x2 = self.x + self.width
    y2 = self.y + self.height
    return f"Rectangle({self.x}, {self.y}, {x2}, {y2}, {self.color})"

  def move(self, x, y):
    self.x += int(x)
    self.y += int(y)
    return self

  def vgr2d(self):
    v = vgr2d.Rect(self.width, self.height, self.color)
    return v.position(x, y)

class Polyline:
  type = "vgr2d"

  def __init__(self, *args, thickness=1):
    it = iter(args)
    self.points = [(x, y) for x, y in zip(it, it)]
    self.color = next(it)
    self.width = thickness

  def __repr__(self):
    points = ", ".join([f"{p[0]},{p[1]}" for p in self.points])
    return f"Polyline({points}, {self.color}, thickness={self.width})"

  def move(self, x, y):
    for point in iter(self.points):
      point[0] += int(x)
      point[1] += int(y)

  def vgr2d(self):
    return vgr2d.Polyline(self.points, self.color, self.width)

class Polygon:
  type = "vgr2d"

  def __init__(self, *args, thickness=1):
    it = iter(args)
    self.points = [(x, y) for x, y in zip(it, it)]
    self.fill = next(it)
    self.width = thickness

  def __repr__(self):
    points = ", ".join([f"{p[0]},{p[1]}" for p in self.points])
    return f"Polyline({points}, {self.fill}, thickness={self.width})"

  def move(self, x, y):
    for point in iter(self.points):
      point[0] += int(x)
      point[1] += int(y)

  def vgr2d(self):
    return vgr2d.Polygon(self.points, stroke=None, fill=self.fill, width=self.width)

class Text:
  type = "text"

  def __init__(self, string, x, y, color, justify=TOP_LEFT):
    self.x = int(x)
    self.y = int(y)
    self.string = string
    self.color = color
    self.justify = justify

  def __repr__(self):
    return f"Text('{self.string}', {self.x}, {self.y}, {self.color}, justify={self.justify})"

  def show(self):
    left = (TOP_LEFT, MIDDLE_LEFT, BOTTOM_LEFT)
    center = (TOP_CENTER, MIDDLE_CENTER, BOTTOM_CENTER)
    right = (TOP_RIGHT, MIDDLE_RIGHT, BOTTOM_RIGHT)

    if self.justify in left:
      x = int(self.x)
    elif self.justify in center:
      x = int(self.x) - FONT_WIDTH * len(self.string) // 2
    elif self.justify in right:
      x = int(self.x) - FONT_WIDTH * len(self.string)
    else:
      raise TypeError("unknown justify value")

    top = (TOP_LEFT, TOP_CENTER, TOP_RIGHT)
    middle = (MIDDLE_LEFT, MIDDLE_CENTER, MIDDLE_RIGHT)
    bottom = (BOTTOM_LEFT, BOTTOM_CENTER, BOTTOM_RIGHT)

    if self.justify in top:
      y = int(self.y) - FONT_HEIGHT
    elif self.justify in middle:
      y = int(self.y) - FONT_HEIGHT // 2
    elif self.justify in bottom:
      y = int(self.y)
    else:
      raise TypeError("unknown justify value")

    buffer = bytearray(7)
    buffer[0] = 0
    buffer[1] = 0
    buffer[2] = (x >> 4) & 0xFF
    buffer[3] = ((x << 4) & 0xF0) | ((y >> 8) & 0x0F)
    buffer[4] = y & 0xFF
    buffer[5] = self.color
    buffer[6] = 0
    for c in self.string.encode("ASCII"):
      buffer.append(c - 32)
      buffer[6] += 1
    buffer += b"\xFF\xFF\xFF"
    assert len(buffer) <= 0xFF
    fpga.write(0x4503, buffer)

  def move(self, x, y):
    self.x = x
    self.y = y
    return self

def __flatten(list):
  return [flatten(item) if hasattr(item, '__iter__') else item for item in list]

def move(*args):
  for arg in __flatten(args[:-2]):
    arg.move(arg, args[-2], args[-1])

def color(*args):
  for arg in __flatten(args[:-1]):
    arg.color(arg, args[-2], args[-1])

def show(*args):
  args = __flatten(args)

  # 0 is the address of the frame in the framebuffer in use.
  # See https://streamlogic.io/docs/reify/nodes/#fbgraphics
  # Offset: active display offset in buffer used if double buffering
  vgr2d.display2d(0, [obj.vgr2d() for obj in args if obj.type == "vgr2d"])

  # Text has no wrapper, we implement it locally.
  # See https://streamlogic.io/docs/reify/nodes/#fbtext
  for obj in args:
    if obj.type == "text":
      obj.show()
