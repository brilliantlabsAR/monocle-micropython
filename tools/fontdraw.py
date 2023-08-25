"""
A demo script to test fontgen.py generated files
"""

import sys
import struct

INDEX_RECORD_SIZE = 64 // 8

class Glyph:

    def __init__(self, file, offset):
        self.beg_x = 0
        self.beg_y = 0
        self.end_x = 0
        self.end_y = 0
        self.data = None
        self.file = file
        self.file.seek(offset)

    def read(self):
        # Read the glyph header
        self.beg_x, self.beg_y, self.len_x, self.len_y = self.file.read(4)

        # Compute the size to read including the last byte
        size = self.len_x * self.len_y
        size = (size + 7) // 8

        # Optimization: do not split the row/columns yet
        self.data = self.file.read(size)

class Font:

    def __init__(self, path):
        self.file = open(path, "rb")

        # Read the index header: (u32)*2
        record = self.file.read(4 + 4)
        self.font_height, self.index_size = struct.unpack(">II", record)

        # Then read the index
        self.font_index = []
        n = 0
        while n < self.index_size:
            # Parse a record from the file
            self.font_index.append(struct.unpack(">II", self.file.read(8)))
            n += 8

    def glyph_range(self, unicode):
        # Start searching from the middle of the file
        beg = 0
        end = len(self.font_index) - 1
        i = beg + (end - beg) // 2

        # Inline implementation of binary search to find the glyph
        while beg != end:

            # Decode the u8 and u24 out of the u32
            unicode_len = self.font_index[i][0] & 0xff
            unicode_start = self.font_index[i][0] >> 8

            # Should we search lower?
            if unicode < unicode_start:
                end = i

            # Should we search higher?
            elif unicode > unicode_start + unicode_len:
                beg = i

            # That's a hit!
            else:
                return unicode_start, self.font_index[i][1]

            # Adjust the probe
            i = beg + (end - beg) // 2

        return None

    def glyph(self, unicode):

        # Get the range to start scanning from
        range_start, range_address = self.glyph_range(unicode)

        # Scan through the data glyph per glyph
        glyph = Glyph(self.file, 4 + 4 + self.index_size + range_address)

        # Skip several glyphs until we are there
        for _ in range(unicode - range_start):
            glyph.read()
        glyph.read()

        return glyph

    def draw(self, glyph):
        n = 0
        for ch in glyph.data:
            for i in reversed(range(8)):
                print("#" if ch & (1 << i) else " ", end="")
                n += 1
                if n % glyph.len_x == 0:
                    print("|")
        if n % glyph.len_x != 0:
            print('', end='\r')

if __name__ == "__main__":
    font = Font(sys.argv[1])
    for ch in sys.argv[2]:
        glyph = font.glyph(ord(ch))
        print(ch)
        font.draw(glyph)
