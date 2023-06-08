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

import fpga, struct, _compression


_image = fpga.read(0x0001, 4)
_status = fpga.read(0x5800, 1)[0] & 0x10
if _status != 16 or _image != b"Mncl":
    raise (NotImplementedError("microphone driver not found on FPGA"))


def _flush():
    count = 0
    while True:
        available = 2 * int.from_bytes(fpga.read(0x5801, 2), "big")
        if available == 0:
            break
        fpga.read(0x5807, min(254, available))
        count += min(254, available)


def record(sample_rate=16000, seconds=1.0):
    # TODO possible to pass sample rate to FPGA?

    _flush()

    # Set window size. Resolution is 20ms
    # TODO possible to omit window size? This allows explicit stop/start
    n = int(seconds / 0.02)
    fpga.write(0x0802, int.to_bytes(n, 2, "big"))

    # Trigger capture
    fpga.write(0x0803, "")


def read(samples=-1):
    if samples > 127:
        raise (ValueError("only 127 samples may be read at a time"))

    available = 2 * int.from_bytes(fpga.read(0x5801, 2), "big")

    if available == 0:
        return None

    available = min(available, 254)

    if samples == -1:
        data = fpga.read(0x5807, available)
    else:
        data = fpga.read(0x5807, min(samples * 2, available))

    ret_data = []

    for i in range(len(data) / 2):
        ret_data.append(struct.unpack(">h", data[i * 2 : i * 2 + 2])[0])

    return ret_data


def stop():
    # TODO send stop command
    pass


def compress(data):
    return _compression.delta_encode(data)


# TODO add callback handler when keyword detection is available
