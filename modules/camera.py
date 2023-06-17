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

import _camera as __camera
import bluetooth as __bluetooth
import fpga as __fpga
import time as __time

RGB = "RGB"
YUV = "YUV"
JPEG = "JPEG"


__image = __fpga.read(0x0001, 4)
__status1 = __fpga.read(0x1000, 1)[0] & 0x10
__status2 = __fpga.read(0x5000, 1)[0] & 0x10
if __status1 != 16 or __status2 != 16: # or __image != b"Mncl":
    raise (NotImplementedError("camera driver not found on FPGA"))


_capture_on = False
_capture_y = 0


def capture(x, y, format):
    global _capture_y
    global _capture_on

    __camera.wake()
    __fpga.write(0x1005, b'')
    __fpga.write(0x3005, b'')

    __time.sleep_ms(100)
    __camera.write(0x3002, 0x00) # SYSTEM_RESET_2 - set everything on
    __time.sleep_ms(100)
    __camera.write(0x3006, 0xFF) # CLOCK_ENABLE_2 - enable all clocks
    __time.sleep_ms(100)
    __camera.write(0x3821, 0x27) # TIMING_TC_REG_21 - enable JPEG, binning, mirror
    __time.sleep_ms(100)
    __camera.write(0x4300, 0x30) # FORMAT_CONTROL_0 - YUV422, YUYV
    __time.sleep_ms(100)
    __camera.write(0x501f, 0x00) # FORMAT_MUX_CONTROL - select YUV422
    __time.sleep_ms(100)
    __camera.write(0x4713, 0x02) # JPG_MODE_SELECT - compression mode 2
    __time.sleep_ms(100)
    __camera.write(0x460c, 0x22) # VFIFO_CTRL0C - undocumented bit
    __time.sleep_ms(100)
    __camera.write(0x3824, 0x04) # DVP_PCLK - clock divider value
    __time.sleep_ms(100)
    __camera.write(0x460b, 0x35) # DEBUG_MODE - undocumented
    __time.sleep_ms(100)
    __fpga.write(0x5004, b'')
    __time.sleep_ms(100)

    _capture_on = True
    _capture_y = y


def capture_off():
    __fpga.write(0x1004, "") # OVCAM_PAUSE_REQ
    __camera.sleep()


def read(bytes=254):
    if not _capture_on:
        raise ValueError("no ongoing capture()")
    if bytes > 254:
        raise ValueError("at most 254 bytes")
    __time.sleep_ms(10)
    return __fpga.read(0x5005, bytes)


def record(enable):
    if enable:
        # Overlay seems to be required for recording.
        overlay(True)
        __fpga.write(0x1005, b"")  # record on
    else:
        # This pauses the image, ready for replaying
        __fpga.write(0x1004, b"")  # record off
        __camera.sleep()


def replay():
    # Stop recording to avoid output mixing live and recorded feeds
    record(False)

    # This will trigger one replay of the recorded feed
    __fpga.write(0x3007, b"")  # replay once
    __time.sleep(4)
