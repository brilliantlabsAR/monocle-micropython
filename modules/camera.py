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

import _camera as camera
import bluetooth as bluetooth
import fpga as fpga
import time as time

RGB = "RGB"
YUV = "YUV"
JPEG = "JPEG"


_image = fpga.read(0x0001, 4)
_status1 = fpga.read(0x1000, 1)[0] & 0x10
_status2 = fpga.read(0x5000, 1)[0] & 0x10
if _status1 != 16 or _status2 != 16: # or _image != b"Mncl":
    raise (NotImplementedError("camera driver not found on FPGA"))


_camera_on = False
_capture_on = False


def overlay(enable=None):
    if enable == None:
        global _overlay_state
        return _overlay_state
    if enable == True:
        _overlay_state = True
    else:
        fpga.write(0x3004, "")
        fpga.write(0x1004, "")
        time.sleep_ms(100)
        camera.sleep()
        _overlay_state = False


def configure(x, y, format):
    global _camera_on

    # Wake the camera
    camera.wake()

    # Enable the camera core in the FPGA
    fpga.write(0x1005, "")


def capture():
    global _camera_on
    global _capture_on

    if not _camera_on:
        configure(640, 400, JPEG)

    # Trigger a capture on the FPGA
    fpga.write(0x5004, b'')
    time.sleep_ms(2000)

    # Keep track of the ongoing capture
    _capture_on = True


def read(bytes=254):
    if not _capture_on:
        raise ValueError("no ongoing capture()")
    if bytes > 254:
        raise ValueError("at most 254 bytes")

    # Read available byte count
    avail = struct.unpack('>H', fpga.read(0x1006, 2))[0]
    if avail == 0:
        return None
    if avail <= bytes:
        bytes = avail

    # Read and return the JPEG data
    return fpga.read(0x5005, bytes)


def wait_data():
    while struct.unpack('>H', fpga.read(0x1006, 2))[0] == 0:
        time.sleep_us(10)


def off():
    fpga.write(0x1004, "") # OVCAM_PAUSE_REQ
    camera.sleep()
    _camera_on = False
    _capture_on = False


def record(enable):
    if enable:
        # Overlay seems to be required for recording.
        overlay(True)
        fpga.write(0x1005, b"")  # record on
    else:
        # This pauses the image, ready for replaying
        fpga.write(0x1004, b"")  # record off
        camera.sleep()


def replay():
    # Stop recording to avoid output mixing live and recorded feeds
    record(False)

    # This will trigger one replay of the recorded feed
    fpga.write(0x3007, b"")  # replay once
    time.sleep(4)
