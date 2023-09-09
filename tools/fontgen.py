"""
A font conversion tool taking BDF files as input and generating a custom
compact bitmap format as output:

    font_format: (u32)font_height, (u32)index_size, font_index, font_data

    font_index: [ (u64)index_record ]*N
    index_record: (u24)unicode_start, (u8)unicode_count, (u32)glyph_address

    font_data: [ (u16)glyph_header, glyph_data ]*N
    glyph_header: (u8)beg_x, (u8)beg_y, (u8)len_x, (u8)len_y
    glyph_data: [ (u1)data_bit ]*N

The font_index can be looked-up using a binary search to find a glyph block,
and then scan the glyph data within it:

If searching the unicode codepoint U+30F3:

1. A binary search would allow to find the range (U+3000, U+3100),
   which will point at the position in the glyph data

2. The glyphs start with a header specifying the size of the glpyh
   data, which allows to jump to the next header.

3. Starting from U+3000, 0x3F (63) glyphs need to be skipped,
   then the glyph data can be used to paint it.
"""

import struct
import sys
import re
import binascii
import unicodedata
from bdfparser import Font

font_index = bytearray()
font_data = bytearray()

def add_index_record(unicode_start, unicode_end, glyph_address):
    len = unicode_end - unicode_start + 1
    assert len > 0 and len <= 0xff

    font_index.extend(struct.pack(">I", unicode_start)[1:])
    font_index.extend(struct.pack(">B", unicode_end - unicode_start + 1))
    font_index.extend(struct.pack(">I", glyph_address))

def get_y_bounding_box(bitmap):
    beg_y = None
    end_y = 0
    for i, row in enumerate(bitmap):
        if '1' in row:
            if beg_y is None:
                beg_y = i
            end_y = i
    beg_y = beg_y or 0
    beg_y = 0 if end_y == beg_y else beg_y
    return beg_y, end_y + 1

def get_x_bounding_box(bitmap):
    beg_x = None
    end_x = 0
    for i in range(len(bitmap[0])):
        if '1' in [x[i] for x in bitmap]:
            if beg_x is None:
                beg_x = i
            end_x = i
    return beg_x or 0, end_x + 1

def add_glyph_header(beg_x, beg_y, end_x, end_y):
    font_data.append(beg_x)
    font_data.append(beg_y)
    font_data.append(end_x - beg_x)
    font_data.append(end_y - beg_y)

def add_glyph_bitmap(bitmap):
    i = 0
    ch = 0
    for row in bitmap:
        for bit in row:
            assert bit in ('0', '1')

            ch <<= 1
            if bit == '1':
                ch |= 1
            i += 1
            if i % 8 == 0:
                font_data.append(ch)
                ch = 0

    # There is an incomplete byte after the rest
    if i % 8 > 0:
        font_data.append(ch << (8 - i % 8))

def add_glyph(glyph):
    # Get a Bitmap out of the font, cropping it as small as possible
    glyph_bitmap = glyph.draw(1)

    # Format the data into the bitmap, skipping the empty rows
    bitmap = [x.replace('2', '1') for x in glyph.draw(0).todata()]

    # Crop the top and bottom margins
    beg_y, end_y = get_y_bounding_box(bitmap)
    bitmap = bitmap[beg_y:end_y]

    # Crop the left and right margins
    beg_x, end_x = get_x_bounding_box(bitmap)
    bitmap = [row[beg_x:end_x] for row in bitmap]

    # Add the glyph metadata
    font_data.extend(bytes((beg_x, beg_y, end_x - beg_x, end_y - beg_y)))

    # Loop over every bit of every row and append them back-to-back
    add_glyph_bitmap(bitmap)

def read_filter(file):
    ranges = []
    for line in open(file, "r"):
        range_beg = line[0:8]
        range_end = line[9:17]
        assert len(range_beg) == 8
        assert len(range_end) == 8
        assert line[8] == ' '
        assert line[17] == ' ' or line[17] == ''
        range_beg = struct.unpack('>I', binascii.a2b_hex(range_beg))[0]
        range_end = struct.unpack('>I', binascii.a2b_hex(range_end))[0]
        ranges.append((range_beg, range_end))
    return ranges

def build_font(font, filter):
    unicode_start = filter[0][0]
    unicode_count = 0
    glyph_address = 0

    # Add all glyphs one by one to the font_index and font_data bytearrays()
    unicode_prev = -1
    for gl in font.iterglyphs(order=1, r=ranges):

        print(gl.chr(), gl.meta["glyphname"])

        # The glyph data block is very straightforward, always the same: add the
        # glyph, but we must keep track of the address.
        add_glyph(gl)

        # If no more room for the current codepoint, push the record, switch to
        # the next
        if unicode_prev >= 0:
            if gl.cp() != unicode_prev + 1 or gl.cp() >= unicode_start + 0xff:
                add_index_record(unicode_start, unicode_prev, glyph_address)
                glyph_address = address_prev
                unicode_start = gl.cp()

        unicode_prev = gl.cp()
        address_prev = len(font_data)

    add_index_record(unicode_start, unicode_prev, glyph_address)

def write_output(file, font_height):
    with open(file, "wb") as f:
        f.write(struct.pack(">I", font_height))
        f.write(struct.pack(">I", len(font_index)))
        f.write(font_index)
        f.write(font_data)

if len(sys.argv) != 4:
    print("usage: {} source.bdf selection.txt output.bin".format(sys.argv[0]))
    exit(1)

source_file = sys.argv[1]
filter_file = sys.argv[2]
output_file = sys.argv[3]

ranges = read_filter(filter_file)
font = Font(source_file)
build_font(font, ranges)
write_output(output_file, int(font.props["pixel_size"]))
