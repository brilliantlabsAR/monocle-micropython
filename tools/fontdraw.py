"""
A demo script to test fontgen.py generated files with font.py
"""

import sys
from font import *

ft = Font(sys.argv[1])
table = [""] * ft.height

for ch in sys.argv[2]:
    gl = ft.glyph(ord(ch))
    buf = []
    gl.render(lambda x: buf.append(x))
    table = [s + buf[i].decode('utf-8') for i, s in enumerate(table)]

for row in table:
    print(row)
