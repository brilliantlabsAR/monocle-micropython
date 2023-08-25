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
        unicode_len = font_index[i][0] & 0x000000ff
        unicode_start = font_index[i][0] >> 8

        # Should we search lower?
        if unicode < unicode_start:
            end = i

        # Should we search higher?
        elif unicode > unicode_start + unicode_len:
            beg = i

        # That's a hit!
        else:
            return unicode_start, font_index[i][0]

        # Adjust the probe
        i = beg + (end - beg) // 2

    return None

class Glyph:

    def __init__(self, file, data_offset):
        self.beg_x = 0
        self.beg_y = 0
        self.end_x = 0
        self.end_y = 0
        self.file = file
        self.file.seek(data_offset, 0)
        self.data = None

    def read(self):
        # Read the glyph header
        self.beg_x = f.read(1)[0]
        self.beg_y = f.read(1)[0]
        self.len_x = f.read(1)[0]
        self.len_y = f.read(1)[0]

        # Compute the size to read including the last byte
        size = self.len_x * self.len_y
        size = (size + 7) // 8

        # Optimization: do not split the row/columns yet
        self.data = f.read(size)

def get_glyph(file, unicode):

    # Get the range to start scanning from
    range_start, range_address = get_glyph_range(unicode)

    # Scan through the data glyph per glyph
    glyph = Glyph(file, 4 + 4 + index_size)

    # Skip several glyphs until we are there
    for _ in range(unicode - range_start):
        glyph.read()
    glyph.read()

    return glyph

with open(sys.argv[1], "rb") as f:

    # Skip (u32)reserved
    struct.unpack(">I", f.read(4))

    # Read the index size
    index_size = struct.unpack(">I", f.read(4))[0]

    # Then read the index
    font_index = []
    n = 0
    while n < index_size:
        # Read a record from the file
        index_record = f.read(INDEX_RECORD_SIZE)
        n += INDEX_RECORD_SIZE

        # Unpack the data from the record
        unicode_ref = struct.unpack(">I", index_record[0:4])[0]
        glyph_address = struct.unpack(">I", index_record[4:8])[0]

        # Add both to the array as a compact tuple of 2 ints
        font_index.append((unicode_ref, glyph_address))

    print(sys.argv[2])
    gl = get_glyph(f, int(sys.argv[2], 0))
    n = 0
    for ch in gl.data:
        for i in reversed(range(8)):
            print("#" if ch & (1 << i) else " ", end="")
            n += 1
            if n % gl.len_x == 0:
                print("|")
