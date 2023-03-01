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

def capture(url):
  raise NotImplementedError

def zoom(multiplier):
  raise NotImplementedError

def overlay(enable):
  if enable == True:
    __camera.wake()
    fpga.write(0x1005, "")
    fpga.write(0x3005, "")
  else:
    fpga.write(0x3004, "") # this discards the record buffer
    fpga.write(0x1004, "")
    time.sleep_ms(100);
    __camera.sleep()

def record(enable):
  if enable:
    __camera.wake()
    fpga.write(0x1005, b'') # record on
  else:
    fpga.write(0x1004, b'') # record off
    __camera.sleep()

def replay():
  record(False)
  overlay(True)
  fpga.write(0x3007, b'') # replay once
  time.sleep(4)
  overlay(False)
