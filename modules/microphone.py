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

import fpga

__microphone_opened = False


class Microphone:
    def __init__(self, callback, sample_rate):
        global __microphone_opened
        if callable(callback) == False:
            raise TypeError("callback must be a callable function")
        if __microphone_opened:
            raise RuntimeError("microphone is already started")
        __microphone_opened = True
        self.callback = callback
        self.sample_rate = sample_rate

    def read(self, samples=-1):
        ## TODO fpga.read limited to 256
        available = int.from_bytes(fpga.read(0x1C01, 2), "big")
        if samples == -1:
            data = fpga.read(0x1C02, available)
        else:
            data = fpga.read(0x1C02, min(samples, available))
        return data

    def close(self):
        global __microphone_opened
        __microphone_opened = False


def open(callback, sample_rate=16000):
    return Microphone(callback, sample_rate)
