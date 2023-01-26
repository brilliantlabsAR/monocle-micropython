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

import bluetooth
import fpga
import time

def capture(url):
  """
  Captures a single image from the camera, and sends it to a url provided. The
  function first tries to send over WiFi. If it cannot reach the internet, it
  will attempt to return it over Bluetooth.
  """
  
  # Start capture using the command 0x5004
  fpga.write(0x5004, [])

  while True:
  
    # Read the bytes remaining in the fifo
    length_array = fpga.read(0x5000, 2)
    length = (length_array[0]<<8)+length_array[1] & 0x0FFF

    if length == 0:
      return

    if length > bluetooth.max_length():
      length = bluetooth.max_length()

    buffer = fpga.read(0x5010, length)

    bluetooth.send(buffer)

def power(power_on):
  return NotImplemented