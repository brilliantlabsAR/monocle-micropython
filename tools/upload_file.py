#!/usr/bin/env python3
"""
An example showing how to connect to the Monocle and store a file.
"""

import asyncio
import sys

from bleak import BleakClient, BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic
from bleak.backends.device import BLEDevice
from bleak.backends.scanner import AdvertisementData
from bleak.uuids import register_uuids


UART_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
UART_RX_CHAR_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
UART_TX_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

DATA_SERVICE_UUID = "e5700001-7bac-429a-b4ce-57ff900f479d"
DATA_RX_CHAR_UUID = "e5700002-7bac-429a-b4ce-57ff900f479d"
DATA_TX_CHAR_UUID = "e5700003-7bac-429a-b4ce-57ff900f479d"

register_uuids({
    DATA_SERVICE_UUID: "Monocle Raw Serivce",
    DATA_TX_CHAR_UUID: "Monocle Raw RX",
    DATA_RX_CHAR_UUID: "Monocle Raw TX",
})

repl_rx_event = asyncio.Event()
repl_rx_last = None
data_rx_event = asyncio.Event()
repl_rx_last = None

async def upload_file(file):

    def match_repl_uuid(device: BLEDevice, adv: AdvertisementData):
        print(f"uuids={adv.service_uuids}")
        return UART_SERVICE_UUID.lower() in adv.service_uuids

    def handle_disconnect(_: BleakClient):
        print("\nDevice was disconnected.")
        for task in asyncio.all_tasks():
            task.cancel()

    def handle_repl_tx(_: BleakGATTCharacteristic, data: bytearray):
        # Here, handle data sent by the Monocle with `print()`
        global repl_rx_last
        repl_rx_last = data
        repl_rx_event.set()

    def handle_data_tx(_: BleakGATTCharacteristic, data: bytearray):
        # Here, handle data sent by the Monocle with `bluetooth.send()`
        global data_rx_last
        data_rx_last = data
        data_rx_event.set()
        
    device = await BleakScanner.find_device_by_filter(match_repl_uuid)
    if device is None:
        print("no matching device found\n")
        exit(1)

    async def send_command(client, cmd):
        global repl_rx_event
        global repl_rx_last

        await client.write_gatt_char(repl_rx_char, cmd)
        await repl_rx_event.wait()
        repl_rx_event.clear()
        print(repl_rx_last)
        assert repl_rx_last[0:2] == b'OK'

    print("Connected to the Monocle")

    async with BleakClient(device, disconnected_callback=handle_disconnect) as client:
        await client.start_notify(UART_TX_CHAR_UUID, handle_repl_tx)
        await client.start_notify(DATA_TX_CHAR_UUID, handle_data_tx)

        repl_service = client.services.get_service(UART_SERVICE_UUID)
        data_service = client.services.get_service(DATA_SERVICE_UUID)
        repl_rx_char = repl_service.get_characteristic(UART_RX_CHAR_UUID)
        data_rx_char = data_service.get_characteristic(DATA_RX_CHAR_UUID)
 
        # Example application: upload a file to the Monocle
        loop = asyncio.get_running_loop()

        global repl_rx_event
        global repl_rx_last

        print(">>> switching the terminal to raw mode")
        await client.write_gatt_char(repl_rx_char, b'\x01')
        await repl_rx_event.wait()
        await asyncio.sleep(1)
        repl_rx_event.clear()
        print(repl_rx_last)

        print(">>> opening a file to write to")
        await send_command(client, ''f"f = open('{file}', 'wb')\x04".encode("ascii"))

        print(">>> writing the data to the file")
        with open(file, 'rb') as f:
            size = repl_rx_char.max_write_without_response_size - len("f.write(r'''''')\x04")
            while data := f.read(size):
                sep = b"'''" if b'"""' in data else b'"""'
                await send_command(client, b"f.write(r" + sep + data + sep + b")\x04")

        print(">>> closing the file")
        await send_command(client, b"f.close()\x04")

        print(">>> switch back to friendly mode")
        await client.write_gatt_char(repl_rx_char, b"\x02")

        print(">>> upload done!")

if __name__ == "__main__":
    file = sys.argv[1]
    try:
        asyncio.run(upload_file(file))
    except asyncio.CancelledError:
        pass
