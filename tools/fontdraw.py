"""
A demo script to test fontgen.py generated files

    font_format: (u32)reserved, (u32)index_size, font_index, font_data

    font_index: [ (u64)index_record ]*N
    index_record: (u24)unicode_start, (u8)unicode_count, (u32)glyph_address

    font_data: [ (u16)glyph_header, glyph_data ]*N
    glyph_header: (u4)data_height, (u4)data_width, (u4)beg_x, (u4)beg_y
    glyph_data: [ (u1)data_bit ]*N

"""

INDEX_RECORD_SIZE = 64 // 8

import struct

with open(sys.argv[1], "rb") as f:

    # Skip (u32)reserved
    struct.unpack(">I", f.read(4))

    # Read the index size
    index_size = struct.unpack(">I", f.read(4))

    # Then read the index
    font_index = []
    while index_size > 0:
        # Read a record from the file
        index_record = f.read(INDEX_RECORD_SIZE)
        index_size -= INDEX_RECORD_SIZE

        # Unpack the data from the record
        unicode_ref = struct.unpack(">I", index_record[0:4])
        glyph_address = struct.unpack(">I", index_record[4:8])

        # Add both to the array as a compact tuple of 2 ints
        font_index.append((unicode_ref, glyph_address))

    # Then read the rest of the file as font data
    font_data = f.read()

def get_glyph_address_range(unicode):
    # Start searching from the middle of the file
    beg = 0
    end = len(font_index)
    i = beg + (end - beg) // 2

    # Inline implementation of binary search to find the glyph
    while beg != end:

        # Decode the u8 and u24 out of the u32
        unicode_len = font_index[i][0] & 0b000000ff
        unicode_start = font_index[i][0] >> 8

        # Should we search lower?
        if unicode < unicode_start:
            end = i

        # Should we search higher?
        else unicode > unicode_start + unicode_len:
            beg = i

        # That's a hit!
        else:
            return font_index[i][1], unicode_len

        # Adjust the probe
        i = beg + (end - beg) // 2

    return None

def read_glyph():
    

def get_glyph_address(unicode):
    # Get the range to start scanning from
    range_start, range_len = get_glyph_address(unicode)

    # Scan through the data glyph per glyph
    
