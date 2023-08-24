"""
A font conversion tool taking BDF files as input and generating a custom
compact bitmap format as output:

    font_format: (u32)reserved, (u32)index_size, font_index, font_data

    font_index: [ (u64)index_record ]*N
    index_record: (u24)unicode_start, (u8)unicode_count, (u32)glyph_address

    font_data: [ (u16)glyph_header, glyph_data ]*N
    glyph_header: (u4)len_x, (u4)len_y, (u4)beg_x, (u4)beg_y
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
import unicodedata
import sys
from bdfparser import Font

font_index = bytearray()
font_data = bytearray()
unicode_start = 0
unicode_count = 0
prev_codepoint = 0
glyph_address = 0

def add_index_record(unicode_start, unicode_count, glyph_address):
    assert unicode_count <= 0xff

    font_index.extend(struct.pack(">I", unicode_start)[0:3])
    font_index.extend(struct.pack(">B", unicode_count))
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
    return beg_y, end_y

def get_x_bounding_box(bitmap):
    beg_x = None
    end_x = 0
    for i in range(len(bitmap[0])):
        if '1' in [x[i] for x in bitmap]:
            if beg_x is None:
                beg_x = i
            end_x = i
    return beg_x, end_x

def add_glyph_header(beg_x, beg_y, end_x, end_y):
    len_x = end_x - beg_x
    len_y = end_y - beg_y
    font_data.append((len_x << 4) & 0b11110000 | len_y & 0b00001111)
    font_data.append((beg_x << 4) & 0b11110000 | beg_y & 0b00001111)

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
        font_data.append(ch)

def add_glyph(glyph):
    # Get a Bitmap out of the font, cropping it as small as possible
    glyph_bitmap = glyph.draw(1)

    # Format the data into the bitmap, skipping the empty rows
    bitmap = [x.replace('2', '1') for x in glyph.draw(0).todata()]

    # Crop the top and bottom margins
    beg_y, end_y = get_y_bounding_box(bitmap)
    bitmap = bitmap[beg_y:end_y+1]

    # Crop the left and right margins
    beg_x, end_x = get_y_bounding_box(bitmap)
    bitmap = [row[beg_x:end_x+1] for row in bitmap]

    # Add the glyph metadata
    font_data.extend(struct.pack(">I", ))

    # Loop over every bit of every row and append them back-to-back
    add_glyph_data(bitmap)

if len(sys.argv) != 2:
    print("usage: {} source_file.bdf output_file.bin".format(sys.argv[0]))
    exit(1)

font = Font(sys.argv[1])

# Add all glyphs one by one to the font_index and font_data bytearrays()
for glyph in font.iterglyphs(order=1):

    # The glyph data block is very straightforward, always the same: add the
    # glyph, but we must keep track of the address.
    add_glyph(glyph)

    # If we add another code point in the same range
    if (glyph.cp() == prev_codepoint + 1 and
            glyph.cp() <= unicode_start + 0xff):
        unicode_count += 1

    # If no more room for the current codepoint, push the record, switch to
    # the next
    else:
        add_index_record(unicode_start, unicode_count, glyph_address)
        glyph_address = len(font_data)
        unicode_start = glyph.cp()
        unicode_count = 0

    prev_codepoint = glyph.cp()

# The last record
add_index_record(unicode_start, unicode_count, glyph_address)

with open(sys.argv[2], "wb") as f:
	f.write(struct.pack(">I", 0))
	f.write(struct.pack(">I", len(font_index)))
	f.write(font_index)
	f.write(font_data)
