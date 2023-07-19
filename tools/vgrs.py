import sys
from PIL import Image

WIDTH = 640
HEIGHT = 400
DEPTH = 3

# the first line has the color palette in a list of "key=value" strings
# separated by space
palette = {}
buf = bytes.fromhex(sys.stdin.readline().rstrip())
it = iter(buf)

# Decode the RGB palette coming as first line
i = 0
assert(next(it) == 0x00) # Skip address field, first byte
assert(next(it) == 0x00) # Skip address field, second byte
while True:
    try:
        palette[i] = bytes((next(it), next(it), next(it)))
    except StopIteration:
        break
    i += 1
print(palette)

# Read input hexadecimal string and decode it into a bytearray
buf = bytearray()
for line in sys.stdin.readlines():
    buf.extend(bytes.fromhex(line.rstrip()))

# Prepare a memory canvas to store the image to be drawn
rgb = bytearray(WIDTH * HEIGHT * DEPTH)

# Cursor drawing onto the RGB canvas
cur_x = 0
cur_y = 0
cur_color = b'\x00\x00\x00'

def cmd_PutSpan(arg):
    global cur_x
    global cur_y
    global cur_color

    for i in range(arg):
        assert cur_x < WIDTH * 16
        rgb[cur_y * WIDTH * DEPTH + cur_x // 16 * DEPTH + 0] = cur_color[0]
        rgb[cur_y * WIDTH * DEPTH + cur_x // 16 * DEPTH + 1] = cur_color[1]
        rgb[cur_y * WIDTH * DEPTH + cur_x // 16 * DEPTH + 2] = cur_color[2]
        cur_x += 1

def cmd_SetColor(arg):
    global cur_color

    # Update the current color using an index from the palette
    cur_color = palette[arg]

def cmd_SkipColumns(arg):
    global cur_x

    # No data for these many pixels
    cur_x += arg

def cmd_JumpToLine(arg):
    global cur_x
    global cur_y

    # No data for these many lines
    cur_x = 0
    cur_y = arg

def cmd_NextLinePos(arg):
    global cur_x
    global cur_y

    # Jump to the column given on the next row
    cur_x = arg
    cur_y += 1

# iterate through pair of bytes, forming an uint16_t containing a command
it = iter(buf)
while True:
    try:
        b1 = next(it)
        b2 = next(it)
    except StopIteration:
        break
    num = (b1 << 8) | b2
    cmd = (b1 & 0b11110000) >> 4

    print(f"{b1:02X} {b2:02X} ", end="")

    # dispatch the command to draw on the canvas accordingly

    if cmd & 0b1000 == 0:
        arg1 = (num & 0b_01111111_00000000) >> 8
        arg2 = (num & 0b_00000000_11111111)
        cmd_SetColor(arg1)
        cmd_PutSpan(arg2)
        print(f"SetColor: {list(cur_color)}; PutSpan: 0x{arg2:X}")

    elif cmd == 0x8 or cmd == 0x9:
        arg = num & 0b_00011111_11111111
        cmd_SkipColumns(arg)
        print(f"SkipColumns: 0x{arg:X}")

    elif cmd == 0xA or cmd == 0xB:
        arg = num & 0b_00011111_11111111
        cmd_NextLinePos(arg)
        print(f"NextLinePos: 0x{arg:X}")

    elif cmd == 0xC:
        arg = num & 0b_00001111_11111111
        cmd_PutSpan(arg)
        print(f"PutSpan: 0x{arg:X}")

    elif cmd == 0xF:
        arg = num & 0b_00001111_11111111
        cmd_JumpToLine(arg)
        print(f"JumpToLine: 0x{arg:X}")

    else:
        print("")

# save the result as PNG image
im = Image.frombuffer('RGB', (WIDTH, HEIGHT), rgb, 'raw', 'RGB', 0, 1)
im.save('vgrs.png')
