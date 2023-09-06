"""
A demo script to test fontgen.py generated files with font.py
"""

import sys
from font import *

def draw(font_height, canvas, width, table):
    assert len(canvas) % width == 0
    canvas = canvas.decode()
    for i in range(min(len(canvas) // width, font_height)):
        table[i] += canvas[i * width:(i + 1) * width] + " "

font = Font(sys.argv[1])
table = [""] * font.height

for ch in sys.argv[2]:
    gl = font.glyph(ord(ch))
    draw(font.height, font.render(gl), gl.width, table)

for row in table:
    print(row)
