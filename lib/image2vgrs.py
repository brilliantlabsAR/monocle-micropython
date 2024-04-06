# Converter from an image to a format understood by:
# https://streamlogic.io/docs/reify/nodes/#fbgraphics

from PIL import Image
from io import StringIO
import sys
import struct
import asyncio

callback_palette = None
callback_pixels = None
buffer = bytearray()
threshold = 128
data_pos = 0

async def put_palette(im):
    """Extract the palette from an image and convert it to VGRS format"""
    global callback_palette
    assert(len(im.palette.colors) == 16)
    b = bytearray(struct.pack('>H', 0x0000))
    for x in im.palette.colors:
        b.extend(struct.pack('>BBB', x[0], x[1], x[2]))
    await callback_palette(b)

async def cmd_PutColorSpan(color_index, num_pixels):
    """Select a color and immediately put a span on a same command"""
    await send_pixels(struct.pack('>BB', color_index, num_pixels), False)

async def cmd_NextLinePos(x):
    await send_pixels(struct.pack('>H', 0xA000 | x), False)

async def put_row(px, row, width):
    for col in range(0, width):
        await cmd_PutColorSpan(px[col, row], 16) # TODO why 16 and not just 1?
    await cmd_NextLinePos(0)

async def put_pixels(im):
    width, height = im.size
    px = im.load()
    for row in range(0, height):
        await put_row(px, row, width)

async def send_pixels(data, flush):
    global buffer
    global callback
    global data_pos

    buffer.extend(data)
    if len(buffer) >= threshold - 2 or flush:
        await callback_pixels(struct.pack('>H', data_pos) + buffer[:threshold])
        buffer = buffer[threshold:]
        data_pos += len(data)

async def convert(path, cb1, cb2):
    """import an image and convert it to VGRS format"""
    global callback_palette
    global callback_pixels
    global data_pos
    im = Image.open(path).convert('P', palette=Image.ADAPTIVE, colors=16)

    callback_palette = cb1
    callback_pixels = cb2
    data_pos = 0

    await put_palette(im)
    await put_pixels(im)
    await send_pixels(b'', True)

if __name__ == '__main__':
    async def callback(data):
        print(data)
    asyncio.run(convert(sys.argv[1], callback, callback))
