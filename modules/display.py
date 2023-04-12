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

WIDTH   = 640
HEIGHT  = 400

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

class DisplayElem:
  def __init__(self):
    self.position = (0, 0)

  def __repr__(self):
    x = self.position[0]
    y = self.position[1]
    exclude = ("position")
    args = [f"{k}={v}" for k, v in self.__dict__.items() if k not in exclude]
    args = ",".join(sorted(args))
    return f"{self.__class__.__name__}({args})@({x},{y})"

  def move(self, x, y):
    self.position = (int(x), int(y))
    return self

class Line(DisplayElem):
  type = "vgr2d"

  def __init__(self, x1, y1, x2, y2, width=1, color=WHITE):
    super().__init__()
    self.point1 = (x1, y1)
    self.point2 = (x2, y2)
    self.width = width
    self.color = color

  def vgr2d(self):
    v = vgr2d.Line(*self.point1, *self.point2, self.color, self.width)
    return v.position(*self.position)

class Rect(DisplayElem):
  type = "vgr2d"

  def __init__(self, width, height, color=WHITE):
    super().__init__()
    self.width = width
    self.height = height
    self.position = (0, 0)
    self.color = color

  def vgr2d(self):
    v = vgr2d.Rect(self.width, self.height, self.color)
    return v.position(*self.position)

class Polyline(DisplayElem):
  type = "vgr2d"

  def __init__(self, points, width=1, color=WHITE):
    super().__init__()
    self.points = points
    self.width = width
    self.color = color

  def vgr2d(self):
    v = vgr2d.Polyline(self.points, self.color, self.width)
    return v.position(*self.position)

class Polygon(DisplayElem):
  type = "vgr2d"

  def __init__(self, points, stroke=WHITE, fill=None, width=1):
    super().__init__()
    if (stroke is None) == (fill is None):
      raise TypeError("exactly one of fill= or stroke= must be set")
    self.points = points
    self.stroke = stroke
    self.fill = fill
    self.width = width

  def vgr2d(self):
    v = vgr2d.Polygon(self.points, stroke=self.stroke, fill=self.fill, width=self.width)
    return v.position(*self.position)

class Text(DisplayElem):
  type = "text"

  def __init__(self, string, color=WHITE):
    super().__init__()
    self.string = string
    self.color = color

  def move(self, x, y):
    self.position = (int(x), int(y))
    return self

def __show_text(list):
  for text in list:
    x = text.position[0]
    y = text.position[1]
    buffer = bytearray(7)
    buffer[0] = 0
    buffer[1] = 0
    buffer[2] = (x >> 4) & 0xFF
    buffer[3] = ((x << 4) & 0xF0) | ((y >> 8) & 0x0F)
    buffer[4] = y & 0xFF
    buffer[5] = text.color
    buffer[6] = 0
    for c in text.string.encode("ASCII"):
      buffer.append(c - 32)
      buffer[6] += 1
    buffer += b"\xFF\xFF\xFF"
    assert len(buffer) <= 0xFF
    fpga.write(0x4503, buffer)

def move(list, x, y):
  min = (WIDTH, HEIGHT)
  for obj in list:
    pos = obj.position
    if pos[0] < min[0]:
      min[0] = pos[0]
    if pos[1] < min[1]:
      min[1] = pos[1]
  for obj in list:
    obj.move(obj.position[0] - min[0] + x, obj.position[1] - min[1] + y)

def show(list):
  # 0 is the address of the frame in the framebuffer in use.
  # See https://streamlogic.io/docs/reify/nodes/#fbgraphics
  # Offset: active display offset in buffer used if double buffering
  vgr2d.display2d(0, [x.vgr2d() for x in list if x.type == "vgr2d"])

  # Text has no wrapper, we implement it locally.
  # See https://streamlogic.io/docs/reify/nodes/#fbtext
  __show_text([x for x in list if x.type == "text"])
