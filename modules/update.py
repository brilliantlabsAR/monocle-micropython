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

import _update as __update
import time as __time


def micropython():
    if __update.connected() == False:
        return

    print("Monocle will reset if not updated within 5 minutes.\n")
    print("If an update fails, simply connect to DFUTarg again.\n")
    print("Monocle will now enter update mode")
    __time.sleep(1)
    print("\b.")
    __time.sleep(1)
    print("\b.")
    __time.sleep(1)
    print("\b.")
    __update.nrf52()


class Fpga:
    def read(address, length):
        return __update.read_fpga_app(address, length)

    def write(data):
        return __update.write_fpga_app(data)

    def erase():
        return __update.erase_fpga_app()
