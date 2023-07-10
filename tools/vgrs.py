import sys
from PIL import Image

WIDTH = 640
HEIGHT = 400
DEPTH = 3

# color scheme to update manually (for now) according to the data
palette = {}
palette[0x0FF] = b'\xFF\x00\x00' # Color 0
palette[0x1FF] = b'\xFF\x66\x00' # Color 1
palette[0x2FF] = b'\xFF\xFF\x00' # Color 2
palette[0x3FF] = b'\x00\xFF\x00' # Color 3
palette[0x4FF] = b'\x00\x00\xFF' # Color 4
palette[0x5FF] = b'\xFF\x00\x66' # Color 5
palette[0x6FF] = b'\xFF\x00\xAA' # Color 6

# read input hexadecimal string and decode it into a bytearray
buf = bytearray()
for line in sys.stdin.readlines():
    buf.extend(bytes.fromhex(line.rstrip()))

# prepare a memory canvas to store the image to be drawn
rgb = bytearray(WIDTH * HEIGHT * DEPTH)

# reset the current values for the pointer that draws onto the RGB canvas
cur_x = 0
cur_y = 0
cur_color = b'\x00\x00\x00'

# iterate through pair of bytes, forming an uint16_t containing a command
it = iter(buf)
for b1 in it:
    b2 = next(it)
    num = ((b1 & 0x0F) << 8) + b2

    # dispatch the command to draw on the canvas accordingly

    if b1 & 0xF0 == 0x00:
        cur_color = palette[num]
        print(f'{b1:02X} {b2:02X} SetColor: {cur_color} (0x{num:X})')

    elif b1 & 0xF0 == 0x80:
        cur_x += num // 8
        print(f'{b1:02X} {b2:02X} Skip: 0x{num:X}')

    elif b1 & 0xF0 == 0xA0:
        cur_x = num // 8
        cur_y += 1
        print(f'{b1:02X} {b2:02X} JumpToColumn: 0x{cur_x:X}')

    elif b1 & 0xF0 == 0xC0:
        for i in range(num // 8):
            rgb[cur_y * WIDTH * DEPTH + cur_x * DEPTH + 0] = cur_color[0]
            rgb[cur_y * WIDTH * DEPTH + cur_x * DEPTH + 1] = cur_color[1]
            rgb[cur_y * WIDTH * DEPTH + cur_x * DEPTH + 2] = cur_color[2]
            cur_x += 1
        print(f'{b1:02X} {b2:02X} Put: 0x{num // 8:X}')

    else:
        print(f'{b1:02X} {b2:02X}')

# save the result as PNG image
im = Image.frombuffer('RGB', (640, 400), rgb, 'raw', 'RGB', 0, 1)
im.save('vgrs.png')
