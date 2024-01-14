#!/usr/bin/env python3
"""
An example showing how to connect to the Monocle and store a file.
"""

from brilliant import Monocle
import asyncio
import sys
import os

async def upload_file(file):
    async with Monocle() as m:
        print(f"uploading {file} ", end="")
        await m.send_command(f"f = open('{file}', 'wb')")
        with open(file, "rb") as f:
            while data := f.read(100):
                print(end=".", flush=True)
                await m.send_command(f"f.write({bytes(data).__repr__()})")
        await m.send_command("f.close()")
        print(" done")

for file in sys.argv[1:]:
    asyncio.run(upload_file(file))
