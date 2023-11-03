import asyncio
import io
import PIL.Image as Image
from brilliant import Monocle

remote_script = '''
import camera
camera.capture()
while data := camera.read(bluetooth.max_length()):
    bluetooth.send(data)
'''

async def get_image():
    async with Monocle() as m:
        await m.send_command(remote_script)
        from_monocle = await m.get_all_data()
        return Image.open(io.BytesIO(from_monocle))

im = asyncio.run(get_image())
im.show()
