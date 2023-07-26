import fpga
import display
import math
import struct


SPRITE_NUMBER = 128

sprite_id = 0
sprite_addr = 0x0000


def sprite_reset():
    global sprite_addr
    global sprite_ids

    sprite_addr = 0x0000
    sprite_ids = set()


class Sprite:
    def __init__(self, data, width):
        global sprite_addr
        global sprite_id

        # Validate the parameters
        if len(data) % (4 * width) != 0:
            raise ValueError("data length must formatted as: b'RGBA' * width")
        if width % 32 != 0:
            raise ValueError("width must be a multiple of 32")
        if sprite_id >= SPRITE_NUMBER:
            raise MemoryError("not enough sprites memory slots")

        # Construct the object
        self.width = width
        self.height = len(data) // 4 // width
        self.addr = sprite_addr
        self.id = sprite_id
        sprite_id += 1

        # Send the sprite RGBA data to the FPGA: Address, 128 bytes of data
        for slice in [data[i:i + 128] for i in range(0, len(data), 128)]:
            b = bytearray()
            b.extend(struct.pack('>I', sprite_addr))
            b.extend(slice)
            fpga.write(0x4404, b)
            sprite_addr += len(slice)

        # Send the sprite layout to the FPGA: Width, Height, and Sprite ID
        b = bytearray()
        b.extend(struct.pack('>H', self.id))
        w = (self.width // 32) & 0xF
        h = self.height & 0xFF
        a = (self.addr // 32) & 0xFFFFF
        b.extend(struct.pack('>I', w << 28 | h << 20 | a << 0))
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

        # Pack the fields in a 32-bit integer + 8-bit integer: 5-bytes
        b.extend(struct.pack('>I', x << 20 | y << 8 | z << 4 | i >> 8))
        b.append(i & 0xFF)

        # End marker
        b.extend(b'\x00\xFF\xFF\xFF\xFF')
        print(b)

        # Send the data
        fpga.write(0x4403, b)


display.brightness(0)
