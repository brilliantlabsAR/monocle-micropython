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

WIDTH = 640
HEIGHT = 400

import time
from sprite import *
from text import *
from vector import *
from color import *
from _display import *


def flatten(o):
    if isinstance(o, tuple) or isinstance(o, list):
        return [i2 for i1 in o for i2 in flatten(i1)]
    else:
        return [o]


def move(*args):
    for arg in flatten(args[:-2]):
        arg.move(args[-2], args[-1])


def show(*args):
    args = flatten(args)

    # Collect everything that must be turned into a vgr2d and render it
    vectors = [arg for arg in args if hasattr(arg, "vgr2d")]
    update_colors(0x4402, vectors)
    show_vgr2d(vectors)

    # Collect everything that must be turned into a sprite and render it
    sprites = [arg for arg in args if hasattr(arg, "sprite")]
    for lst in [arg.sprites() for arg in args if hasattr(arg, "sprites")]:
        sprites += lst
    show_sprites(sprites)

    # Flush the display buffer.
    fpga.write(0x0001, b"")


def clear():
    show()
