"""
A demo script to test fontgen.py generated files with font.py
"""

import sys
from font import *

def callback(canvas, args):
    args.append(canvas)

font = Font(sys.argv[1])
table = [""] * font.height

for ch in sys.argv[2]:
    gl = font.glyph(ord(ch))
    buf = []
    font.render(gl, callback, buf)
    table = [s + buf[i].decode('utf-8') for i, s in enumerate(table)]

for row in table:
    print(row)
