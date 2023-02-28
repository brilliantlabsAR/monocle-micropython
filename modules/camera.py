#
# This file is part of the MicroPython for Monocle project:
#      https://github.com/brilliantlabsAR/monocle-micropython
#
# Authored by: Josuah Demangeon (me@josuah.net)
#              Raj Nakarja / Brilliant Labs Inc (raj@itsbrilliant.co)
#
# ISC Licence
#
# Copyright © 2023 Brilliant Labs Inc.
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

import __camera
import bluetooth
import fpga
import time
import gc

def capture(url):
  """
  Send the captured image over Bluetooth.
  """
  __camera.wake()
  fpga.write(0x1009, b'') # camera on
  fpga.write(0x1006, b'') # camera capture
  time.sleep_ms(60) # let the FPGA work a bit

  buffer = bytearray(bluetooth.max_length())
  buffer[1:4] = b'\x00\x00\x00\x00' # file size
  buffer[5] = len(url)
  buffer[6:] = url[:255].encode('utf-8')

  flag = 1 # START
  offset = 6 + len(url)

  while True:
    # Read the bytes remaining in the fifo
    length_bytes = fpga.read(0x5000, 2)
    length = (length_bytes[0] << 8 | length_bytes[1]) & 0x0FFF

    if length == 0:
      break

    if length > bluetooth.max_length() - offset:
      length = bluetooth.max_length() - offset
    else:
      flag = 0 if (flag == 1) else 3 # SMALL or END

    buffer[0] = flag
    buffer[offset:] = fpga.read(0x5010, length)

    while True:
      try:
        bluetooth.send(buffer)
      except OSError:
        continue
      break

    offset = 1
    flag = 2 # MIDDLE
    gc.collect()

  __camera.sleep()
  gc.collect()

def zoom(multiplier):
  raise NotImplementedError

def overlay(enable):
  if enable == True:
    __camera.wake()
    fpga.write(0x1005, "")
    fpga.write(0x3005, "")
  else:
    fpga.write(0x3004, "")
    fpga.write(0x1004, "")
    time.sleep_ms(100);
    __camera.sleep()
