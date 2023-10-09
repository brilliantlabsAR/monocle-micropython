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


class MonocleScript:
    UART_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
    UART_RX_CHAR_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
    UART_TX_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

    DATA_SERVICE_UUID = "e5700001-7bac-429a-b4ce-57ff900f479d"
    DATA_RX_CHAR_UUID = "e5700002-7bac-429a-b4ce-57ff900f479d"
    DATA_TX_CHAR_UUID = "e5700003-7bac-429a-b4ce-57ff900f479d"

    def __init__(self):
        register_uuids({
            self.DATA_SERVICE_UUID: "Monocle Data Serivce",
            self.DATA_TX_CHAR_UUID: "Monocle Data RX",
            self.DATA_RX_CHAR_UUID: "Monocle Data TX",
        })

    @classmethod
    async def run(cls, *args):
        self = cls()

        device = await BleakScanner.find_device_by_filter(self.match_uart_uuid)
        if device is None:
            print("no matching device found\n")
            exit(1)
        print("Connected to the Monocle")

        async with BleakClient(device, disconnected_callback=self.handle_disconnect) as c:
            self.client = c
            await self.init_uart_service()
            await self.init_data_service()
            await self.set_monocle_raw_mode()
            await self.script(*args)
            await self.client.write_gatt_char(self.uart_rx_char, b"\x02")

    def match_uart_uuid(self, device:BLEDevice, adv:AdvertisementData):
        print(f"uuids={adv.service_uuids}")
        return self.UART_SERVICE_UUID.lower() in adv.service_uuids

    def handle_disconnect(self, _:BleakClient):
        print("\nDevice was disconnected.")
        exit(1)

    def handle_uart_tx(self, _:BleakGATTCharacteristic, data:bytearray):
        # Here, handle data sent by the Monocle with `print()`
        self.uart_rx_last = data
        self.uart_rx_event.set()

    def handle_data_tx(self, _:BleakGATTCharacteristic, data:bytearray):
        # Here, handle data sent by the Monocle with `bluetooth.send()`
        self.data_rx_last = data
        self.data_rx_event.set()

    async def send_command(self, cmd):
        await self.client.write_gatt_char(self.uart_rx_char, cmd.encode('ascii') + b'\x04')
        await self.uart_rx_event.wait()
        self.uart_rx_event.clear()
        print(self.uart_rx_last)
        assert self.uart_rx_last[0:2] == b'OK'

    async def init_uart_service(self):
        await self.client.start_notify(self.UART_TX_CHAR_UUID, self.handle_uart_tx)
        uart_service = self.client.services.get_service(self.UART_SERVICE_UUID)
        self.uart_rx_char = uart_service.get_characteristic(self.UART_RX_CHAR_UUID)
        self.uart_rx_event = asyncio.Event()
        self.uart_rx_last = None

    async def init_data_service(self):
        await self.client.start_notify(self.DATA_TX_CHAR_UUID, self.handle_data_tx)
        data_service = self.client.services.get_service(self.DATA_SERVICE_UUID)
        self.data_rx_char = data_service.get_characteristic(self.DATA_RX_CHAR_UUID)
        self.data_rx_event = asyncio.Event()
        self.data_rx_last = None
  
    async def set_monocle_raw_mode(self):
        await self.client.write_gatt_char(self.uart_rx_char, b'\x01')
        await self.uart_rx_event.wait()
        await asyncio.sleep(0.5)
        self.uart_rx_event.clear()


class UploadFileScript(MonocleScript):
    """
    Example application: upload a file to the Monocle
    """
    async def script(self, file):

        print(">>> opening a file to write to")
        await self.send_command(f"f = open('{file}', 'wb')")

        print(">>> writing the data to the file")
        with open(file, 'rb') as f:
            while data := f.read(100):
                print(data)
                await self.send_command(f"f.write({bytes(data).__repr__()})")

        print(">>> closing the file")
        await self.send_command("f.close()")
 
        print(">>> script done")


if __name__ == "__main__":
    asyncio.run(UploadFileScript.run(sys.argv[1]))
