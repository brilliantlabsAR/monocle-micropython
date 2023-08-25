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

import _camera
import struct
import fpga
import time


_image = fpga.read(0x0001, 4)
_status = fpga.read(0x1000, 1)[0] & 0x10

if _status != 16 or _image != b"Mncl":
    raise (NotImplementedError("camera driver not found on FPGA"))

RGB = "RGB"
JPEG = "JPEG"


def capture():
    _camera.wake()
    fpga.write(0x1003, b"")
    while fpga.read(0x1000, 1) == b"2":
        time.sleep_us(10)


def read(bytes=254):
    if bytes > 254:
        raise ValueError("at most 254 bytes")

    avail = struct.unpack(">H", fpga.read(0x1006, 2))[0]

    if avail == 0:
        _camera.sleep()
        return None

    return fpga.read(0x1007, min(bytes, avail))


def output(x, y, mode):
    return NotImplemented


def zoom(factor):
    return NotImplemented
