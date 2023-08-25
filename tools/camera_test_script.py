import asyncio
import sys

from bleak import BleakClient, BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic
from bleak.backends.device import BLEDevice
from bleak.backends.scanner import AdvertisementData

from PIL import Image
import io

REPL_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
REPL_TX_CHAR_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
REPL_RX_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

DATA_SERVICE_UUID = "e5700001-7bac-429a-b4ce-57ff900f479d"
DATA_TX_CHAR_UUID = "e5700002-7bac-429a-b4ce-57ff900f479d"
DATA_RX_CHAR_UUID = "e5700003-7bac-429a-b4ce-57ff900f479d"

file = [
    """import bluetooth""",
    """import camera""",
    """import time""",
    """camera.capture()""",
    """time.sleep_ms(250)""",
    """def send(data):""",
    """    while True:""",
    """        try:""",
    """            bluetooth.send(data)""",
    """            break""",
    """        except OSError:""",
    """            pass""",
    """send("START")""",
    """while True:""",
    """    image_data = camera.read(bluetooth.max_length())""",
    """    if image_data == None:""",
    """        break""",
    """    send(image_data)""",
    """send("END")""",
]

image_data = b""


async def main():
    def match_repl_uuid(device: BLEDevice, adv: AdvertisementData):
        return REPL_SERVICE_UUID in adv.service_uuids

    def repl_rx(_: BleakGATTCharacteristic, data: bytearray):
        # print(data.decode("utf-8").strip("\r\n")) # Uncomment for debug
        pass

    def data_rx(_: BleakGATTCharacteristic, data: bytearray):
        global image_data
        if data == b"START":
            print("Downloading image data\n")

        elif data == b"END":
            print("Saving image")
            image = Image.open(io.BytesIO(bytes(image_data)))
            image.show()
            print("Done")
            sys.exit(0)

        else:
            image_data += data
            print(f"\033[FDownloaded {len(image_data)} bytes")

    print("Searching for device")

    device = await BleakScanner.find_device_by_filter(match_repl_uuid)

    if device is None:
        print("No device found")
        sys.exit(1)

    print("Connected")

    async with BleakClient(device) as c:
        await c.start_notify(REPL_RX_CHAR_UUID, repl_rx)
        await c.start_notify(DATA_RX_CHAR_UUID, data_rx)

        repl = c.services.get_service(REPL_SERVICE_UUID)
        repl_tx = repl.get_characteristic(REPL_TX_CHAR_UUID)

        await c.write_gatt_char(repl_tx, b"\x03\x01")
        await c.write_gatt_char(repl_tx, b"f=open('main.py', 'w')\x04")

        await c.write_gatt_char(repl_tx, b"f.write('''")

        for line in file:
            await c.write_gatt_char(repl_tx, line.encode() + b"\n")

        await c.write_gatt_char(repl_tx, b"''')\x04")

        await c.write_gatt_char(repl_tx, b"f.close()\x04")
        await c.write_gatt_char(repl_tx, b"\x04")

        while True:
            await asyncio.sleep(1)


asyncio.run(main())
