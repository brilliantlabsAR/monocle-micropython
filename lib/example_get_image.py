import asyncio
import io
import PIL.Image as Image
import time
from brilliant import Monocle
import binascii

async def get_image():
    async with Monocle() as m:
        await m.send_command("import camera, ubinascii")
        await m.send_command("camera.capture()")
        await asyncio.sleep(3)
        image = bytearray()
        while True:
            cmd = "print(ubinascii.b2a_base64(camera.read(64) or b'').decode('ascii'), end='')"
            base64 = await m.send_command(cmd)
            print(base64)
            if base64 == b"\n":
                break
            image.extend(binascii.a2b_base64(base64))
        return image

image_buffer = asyncio.run(get_image())
print(image_buffer)
Image.open(io.BytesIO(image_buffer))
im = asyncio.run(get_image())
im.show()
