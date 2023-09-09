#
# This file is part of the MicroPython for Monocle project:
#      https://github.com/brilliantlabsAR/monocle-micropython
#
# Authored by: Josuah Demangeon (me@josuah.net)
#
# ISC Licence
#
# Copyright © 2023 Brilliant Labs Ltd.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED 'AS IS' AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#


import fpga


CLEAR = 0x000000
BLACK = 0x000000
RED = 0xAD2323
GREEN = 0x1D6914
BLUE = 0x2A4BD7
CYAN = 0x29D0D0
MAGENTA = 0x8126C0
YELLOW = 0xFFEE33
WHITE = 0xFFFFFF
GRAY1 = 0x1C1C1C
GRAY2 = 0x383838
GRAY3 = 0x555555
GRAY4 = 0x717171
GRAY5 = 0x8D8D8D
GRAY6 = 0xAAAAAA
GRAY7 = 0xC6C6C6
GRAY8 = 0xE2E2E2


class Colored:
    def color(self, color_rgb):
        self.color_rgb = color_rgb


def color(*args):
    for arg in flatten(args[:-1]):
        arg.color(args[-1])


def update_colors(addr, obj_list, dump=False):
    # new buffer for the FPGA API, starting with address 0x0000
    buffer = bytearray(2)

    # collect the colors from the various objects
    color_list = []
    for obj in obj_list:
        if obj.color_rgb in color_list:
            # deduplicate the color by using the existing index
            obj.color_index = color_list.index(obj.color_rgb)

        else:
            # Add a new color if enough room
            if len(color_list) > 128:
                raise ValueError("more than 128 different color unsupported")

            # Add to the list for future reference
            obj.color_index = len(color_list)
            color_list.append(obj.color_rgb)

            # Add to the buffer for the FPGA
            buffer.append(obj.color_rgb >> 16)
            buffer.append(obj.color_rgb >> 8)
            buffer.append(obj.color_rgb >> 0)

    # Flush the buffer, we are done
    fpga.write(addr, buffer)

    # Hexdump the buffer if requested
    if dump:
        print("".join("%02X" % x for x in buffer))
