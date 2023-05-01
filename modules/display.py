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
from __display import *

WIDTH   = 640
HEIGHT  = 400

SPACE_WIDTH = 1
FONT_HEIGHT = 48
FONT_WIDTH = 23 + SPACE_WIDTH

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

class Colored:
  def color(self, color):
    self.col = color

class Line(Colored):
  def __init__(self, x1, y1, x2, y2, color, thickness=1):
    self.x1 = int(x1)
    self.y1 = int(y1)
    self.x2 = int(x2)
    self.y2 = int(y2)
    self.col = color
    self.width = thickness

  def __repr__(self):
    return f'Line({self.x1}, {self.y1}, {self.x2}, {self.y2}, {self.col}, thickness={self.width})'

  def move(self, x, y):
    self.x1 += int(x)
    self.y1 += int(y)
    self.x2 += int(x)
    self.y2 += int(y)
    return self

  def vgr2d(self):
    return vgr2d.Line(self.x1, self.y1, self.x2, self.y2, self.col, self.width)

class Rectangle(Colored):
  def __init__(self, x1, y1, x2, y2, color):
    self.x = min(x1, x2)
    self.y = min(y1, y2)
    self.width = abs(x2 - x1)
    self.height = abs(y2 - y1)
    self.col = color

  def __repr__(self):
    x2 = self.x + self.width
    y2 = self.y + self.height
    return f'Rectangle({self.x}, {self.y}, {x2}, {y2}, {self.col})'

  def move(self, x, y):
    self.x += int(x)
    self.y += int(y)
    return self

  def vgr2d(self):
    v = vgr2d.Rect(self.width, self.height, self.col)
    return v.position(self.x, self.y)

class Polyline(Colored):
  def __init__(self, list, color, thickness=1):
    if len(list) % 2 != 0:
      raise ValueError('list must have odd number of coordinates')
    self.points = []
    for i in range(0, len(list), 2):
      self.points.append((list[i], list[i + 1]))
    self.col = color
    self.width = thickness

  def __repr__(self):
    points = ', '.join([f'{p[0]},{p[1]}' for p in self.points])
    return f'Polyline([{points}], {self.col}, thickness={self.width})'

  def move(self, x, y):
    for point in iter(self.points):
      point[0] += int(x)
      point[1] += int(y)

  def vgr2d(self):
    return vgr2d.Polyline(self.points, self.col, self.width)

class Polygon(Colored):
  def __init__(self, list, color, thickness=1):
    if len(list) % 2 != 0:
      raise ValueError('list must have odd number of coordinates')
    self.points = []
    for i in range(0, len(list), 2):
      self.points.append((list[i], list[i + 1]))
    self.col = color
    self.width = thickness

  def __repr__(self):
    points = ', '.join([f'{p[0]},{p[1]}' for p in self.points])
    return f'Polygon([{points}], {self.col}, thickness={self.width})'

  def move(self, x, y):
    for point in iter(self.points):
      point[0] += int(x)
      point[1] += int(y)

  def vgr2d(self):
    return vgr2d.Polygon(self.points, stroke=None, fill=self.col, width=self.width)

class Text(Colored):
  def __init__(self, string, x, y, color, justify=TOP_LEFT):
    self.x = int(x)
    self.y = int(y)
    self.string = string
    self.col = color
    self.justify(justify)

  def __repr__(self):
    return f"Text('{self.string}', {self.x}, {self.y}, {self.col})"

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
      raise ValueError('unknown justify value')

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
      raise ValueError('unknown justify value')

  def width(self, string):
    return FONT_WIDTH * len(string) - SPACE_WIDTH

  def clip_x(self):
    string = self.string
    x = self.x

    if x < 0:
      i = abs(x) // FONT_WIDTH + 1
      string = string[i:]
      x += i * FONT_WIDTH
    elif x > WIDTH:
      raise ValueError("trying to draw text off screen")
    elif x + self.width(string) > WIDTH:
      overflow_px = x + self.width(string) - WIDTH
      overflow_ch = overflow_px // FONT_WIDTH + 1
      string = string[:-overflow_ch]

    return x, string

  def fbtext(self, buffer):
    x, string = self.clip_x()
    y = self.y

    # Build a buffer to send to the FPGA
    buffer.append((x >> 4) & 0xFF)
    buffer.append(((x << 4) & 0xF0) | ((y >> 8) & 0x0F))
    buffer.append(y & 0xFF)
    buffer.append(self.col)
    i = len(buffer)
    buffer.append(0)
    for c in string.encode('ASCII'):
      buffer.append(c - 32)
      buffer[i] += 1  # increment the length field
    assert(buffer[i] <= 0xFF)

  def move(self, x, y):
    self.x = x
    self.y = y
    return self

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

def show(*args):
  args = flatten(args)

  # 0 is the address of the frame in the framebuffer in use.
  # See https://streamlogic.io/docs/reify/nodes/#fbgraphics
  # Offset: active display offset in buffer used if double buffering
  list = [obj.vgr2d() for obj in args if hasattr(obj, 'vgr2d')]
  vgr2d.display2d(0, list, WIDTH, HEIGHT)

  def has_collision_x(list):
    if len(list) == 0:
      return False
    list = sorted(list, key=lambda obj: obj.x)
    prev = list[0]
    print(list)
    for obj in list[1:]:
      print(f'x: prev={prev} obj={obj}')
      if obj.x < prev.x + prev.width(prev.string):
        # Overlapping on both x and y coordinates
        print('   overlap')
        return True
      prev = obj
    return False

  def has_collision_xy(list):
    if len(list) == 0:
      return False
    sublist = [list[0]]
    prev = list[0]
    for obj in list[1:]:
      if obj.y < prev.y + FONT_HEIGHT:
        # Some overlapping, accumulate the row
        sublist.append(obj)
        print(f'y: prev={prev} obj={obj}')
      else:
        # Since the list is sorted, stop here
        break
      prev = obj
    return has_collision_x(sublist)

  # Text has no wrapper, we implement it locally.
  # See https://streamlogic.io/docs/reify/nodes/#fbtext
  buffer = bytearray(2) # address 0x0000
  list = [obj for obj in args if hasattr(obj, 'fbtext')]
  list = sorted(list, key=lambda obj: obj.y)
  for i in range(len(list)):
    if has_collision_xy(list[i:]):
      raise ValueError(f'{list[i]} collides with another Text()')
    list[i].fbtext(buffer)
  if len(buffer) > 0:
    fpga.write(0x4503, buffer + b'\xFF\xFF\xFF')
