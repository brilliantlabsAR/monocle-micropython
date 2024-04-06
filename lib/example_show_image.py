#!/usr/bin/env python3
"""
An example showing how to display an image by converting it to VGRS
"""

from brilliant import Monocle
import image2vgrs
import asyncio
import struct
import sys
import os

monocle = None

async def from_bluetooth_to_fpga(addr):
    global monocle
    cmd = f"bluetooth.receive_callback(lambda x: fpga.write(0x{addr:04x}, x))"
    await monocle.send_command(cmd)

async def callback_palette(data):
    global monocle
    await from_bluetooth_to_fpga(0x4402)
    await monocle.send_data(data)
    await from_bluetooth_to_fpga(0x4403)

async def callback_pixels(data):
    global monocle
    print('.', end='', flush=True)
    await monocle.send_data(data)

async def show_image(path):
    async with Monocle() as m:
        global monocle
        monocle = m
        await m.send_command('import bluetooth, fpga')
        await image2vgrs.convert(path, callback_palette, callback_pixels)

if __name__ == '__main__':
    asyncio.run(show_image(sys.argv[1]))
