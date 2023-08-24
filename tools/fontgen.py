"""
A font conversion tool taking BDF files as input and generating a custom
compact bitmap format as output:

    font_format: (u32)reserved, (u32)header_size, font_index, font_data

    font_index: [ (u64)index_record ]*N
    index_record: (u24)unicode_start, (u8)unicode_count, (u32)glyph_address

    font_data: [ (u16)glyph_header, glyph_data ]*N
    glyph_header: (u4)data_height, (u4)data_width, (u4)offset_x, (u4)offset_y
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

font_index = bytearray()

font_data = bytearray()

def push_glyph():
    glyph = next_glyph()

    # If no more room for the current codepoint
    if glyph.codepoint != prev_codepoint + 1 or prev_codepoint + 1 > 0xFF:
        pass

    append
