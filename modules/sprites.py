import fpga
import display
import math
import struct
display.brightness(0)

a = []
b = []
for i in range(8):
    a.extend([0xA0,0x00,0xA0,0xFF])
    b.extend([0x00,0xA0,0xA0,0xFF])
for i in range(8):
    a.extend([0x00,0xA0,0xA0,0xFF])
    b.extend([0xA0,0x00,0xA0,0xFF])
for i in range(8):
    a.extend([0xA0,0x00,0xA0,0xFF])
    b.extend([0x00,0xA0,0xA0,0xFF])
for i in range(8):
    a.extend([0x00,0xA0,0xA0,0xFF])
    b.extend([0xA0,0x00,0xA0,0xFF])
addr = 0
for i in range(15):
    d = [(addr>>24)&0xFF, (addr>>16)&0xFF, (addr>>8)&0xFF, addr&0xFF]
    if i < 5 or i >= 10:
        d.extend(a)
    else:
        d.extend(b)
    fpga.write(0x4404,bytes(d))
    addr += len(a)

for i in range(32):
    d = [(addr>>24)&0xFF, (addr>>16)&0xFF, (addr>>8)&0xFF, addr&0xFF]
    for j in range(i):
        d.extend([0xFF,0xFF,0xFF,0xFF])
    for j in range(32-i):
        d.extend([0x00,0x00,0x00,0x00])
    fpga.write(0x4404,bytes(d))
    addr += 128

# Describe the sprites layout
fpga.write(0x4402,bytes([0x00,0x00,0x10,0xf0,0x00,0x00,0x12,0x00,0x00,0x0f]))

id0 = 0
x0 = 0x0A10
y0 = 0x00

x0h = x0 >> 8
x0l = x0 & 0xFF

id1 = 1
x1 = 0x0890
y1 = 0x30

x1h = x1 >> 8
x1l = x1 & 0xFF

fpga.write(0x4403,bytes([0x00,0x00, x0h,x0l,y0,0x10,id0, x1h,x1l,y1,0x10,id1, 0x00, 0xFF,0xFF,0xFF,0xFF]))

SPRITE_NUMBER = 128

sprite_ids = set()
sprite_addr = 0x0000

def sprite_new_id():
    global sprite_ids

    # Get the first ID that is not already the table
    id = min(set(range(SPRITE_NUMBER)) - sprite_ids)

    # Mark it as used and return it
    sprite_ids.add(id)
    return id

def sprite_alloc_mem(size):
    global sprite_addr

    addr = sprite_addr
    sprite_addr += size
    return addr

def sprite_reset():
    global sprite_addr
    global sprite_ids

    sprite_addr = 0x0000
    sprite_ids = set()


class Sprite:
    def __init__(self, data, width):
        global sprite_addr

        self.width = width
        self.height = len(data) // 4 // width
        self.id = sprite_new_id()
        self.addr = sprite_addr
        if len(data) % (4 * width) != 0:
            raise ValueError("data length must formatted as: b'RGBA' * width")
        if self.id is None:
            raise MemoryError("not enough sprites slot")

        # Send the sprite RGBA data to the FPGA: Address, 128 bytes of data
        for slice in [data[i:i + 128] for i in range(0, len(data), 128)]:
            b = bytearray()
            b.extend(struct.pack('>H', sprite_addr))
            b.extend(slice)
            print(b)
            fpga.write(0x4404, b)
            sprite_addr += len(slice)

        # Send the sprite layout to the FPGA: Width, Height, and Sprite ID
        b = bytearray()
        b.extend(struct.pack('>H', self.id))
        w = math.ceil(self.width / 32) & 0xF
        h = self.height & 0xFF
        i = (self.id // 32) & 0xFFFFF
        b.extend(struct.pack('>I', w << 28 | h << 20 | i << 0))
        print(b)
        fpga.write(0x4402, b)

    def __repr__(self):
        return f'Sprite(w={self.width}, h={self.height}, id={self.id}, addr=0x{self.addr:04X})'

    def place(self, x, y, z):
        b = bytearray()
        b.extend(b'\x00\x00')

        # X, Y coordinates, Z stacking, Sprite ID
        x = x & 0xFFF
        y = y & 0xFFF
        z = z & 0xF
        i = self.id & 0xFFF

        # pack the fields in a 32-bit integer + 8-bit integer: 5-bytes
        b.extend(struct.pack('>I', x << 20 | y << 8 | z << 4 | i >> 8))
        b.append(i & 0xFF)

        # End marker
        b.extend(b'\x00\xFF\xFF\xFF\xFF')
        print(b)

        # Send the data
        fpga.write(0x4403, b)


from math import sin, cos
import time
for i in range(1, 200):
    i1 = i / 4
    s1 = (int(cos(i1) * 8 + 8), int(sin(i1) * 100 + 100), 1)
    i2 = (i + 5) / 4
    s2 = (int(cos(i2) * 8 + 8), int(sin(i2) * 100 + 100), 1)
    place((s1, s2))
    time.sleep(0.2)
