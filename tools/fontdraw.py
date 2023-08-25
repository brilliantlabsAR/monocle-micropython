"""
A demo script to test fontgen.py generated files
"""

import sys
import struct

INDEX_RECORD_SIZE = 64 // 8

def get_glyph_range(unicode):
    # Start searching from the middle of the file
    beg = 0
    end = len(font_index) - 1
    i = beg + (end - beg) // 2

    # Inline implementation of binary search to find the glyph
    while beg != end:

        # Decode the u8 and u24 out of the u32
        unicode_len = font_index[i][0] & 0xff
        unicode_start = font_index[i][0] >> 8

        # Should we search lower?
        if unicode < unicode_start:
            end = i

        # Should we search higher?
        elif unicode > unicode_start + unicode_len:
            beg = i

        # That's a hit!
        else:
            return unicode_start, font_index[i][1]

        # Adjust the probe
        i = beg + (end - beg) // 2

    return None

class Glyph:

    def __init__(self, file, offset):
        self.beg_x = 0
        self.beg_y = 0
        self.end_x = 0
        self.end_y = 0
        self.data = None
        self.file = file
        self.file.seek(offset)
        print("offset=0x{:x}".format(offset))

    def read(self):
        # Read the glyph header
        self.beg_x, self.beg_y, self.len_x, self.len_y = f.read(4)

        # Compute the size to read including the last byte
        size = self.len_x * self.len_y
        size = (size + 7) // 8

        # Optimization: do not split the row/columns yet
        self.data = f.read(size)

def get_glyph(file, index_size, unicode):

    # Get the range to start scanning from
    range_start, range_address = get_glyph_range(unicode)

    # Scan through the data glyph per glyph
    glyph = Glyph(file, 4 + 4 + index_size + range_address)

    # Skip several glyphs until we are there
    for _ in range(unicode - range_start):
        glyph.read()
    glyph.read()

    return glyph

with open(sys.argv[1], "rb") as f:
    # Read the index header: (u32)*2
    font_height, index_size = struct.unpack(">II", f.read(4 + 4))

    # Then read the index
    font_index = []
    n = 0
    while n < index_size:
        # Parse a record from the file
        font_index.append(struct.unpack(">II", f.read(4 + 4)))
        n += 4 + 4

    gl = get_glyph(f, index_size, int(sys.argv[2], 0))
    n = 0
    for ch in gl.data:
        for i in reversed(range(8)):
            print("#" if ch & (1 << i) else " ", end="")
            n += 1
            if n % gl.len_x == 0:
                print("|")
