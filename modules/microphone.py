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


def record(sample_rate=16000):
    # TODO pass sample rate to FPGA
    fpga.write(0x1C04, int.to_bytes(sample_rate, 2, "big"))

    # TODO send flush buffer command
    fpga.write(0x1C03, b"\x01")

    # TODO send record command
    fpga.write(0x1C04, b"\x02")


def read(samples=-1):
    available = int.from_bytes(fpga.read(0x1C01, 2), "big")
    available = min(available, 255)

    if samples == -1:
        data = fpga.read(0x1C02, available)
    else:
        data = fpga.read(0x1C02, min(samples, available))
    return data


def stop():
    # TODO send stop command
    fpga.write(0x1C04, b"\x03")


# TODO add callback handler when keyword detection is available
