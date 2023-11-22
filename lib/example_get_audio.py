import asyncio
import os
from brilliant import Monocle

remote_script = '''
import bluetooth
import microphone
import time

samples = bluetooth.max_length() // 2

microphone.record(seconds=1.0, bit_depth=16, sample_rate=16000)
time.sleep(0.5)  ## A short time is needed to let the FPGA prepare the buffer

num = 0
while num < 2:
    data = microphone.read(samples)
    if data == None:
        num += 1
    else:
        num = 0
        while True:
            try:
                bluetooth.send(data)
                break
            except OSError:
                pass
'''

async def get_audio():
    async with Monocle() as m:
        await m.send_command(remote_script)
        return await m.get_all_data()

f = open("audio.raw", "wb")
raw = asyncio.run(get_audio())
f.write(raw)

# Use the FFmpeg https://ffmpeg.org/ command to convert the input audio to WAV
# You can change the extension from ".wav" to anything you like, like ".mp3"
os.system("ffmpeg -y -f s16be -ar 16000 -i audio.raw audio.wav")
