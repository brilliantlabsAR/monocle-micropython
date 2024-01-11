#!/usr/bin/env python3
"""
An example showing how to connect to the Monocle and store a file.
"""

import asyncio
import sys
import os

from bleak import BleakClient, BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic
from bleak.backends.device import BLEDevice
from bleak.backends.scanner import AdvertisementData
from bleak.uuids import register_uuids


class Monocle:
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
        self.lock = asyncio.Lock()

    async def __aenter__(self):
        await self.connect()
        self.log(await self.get_all_uart())
        return self

    async def __aexit__(self, *args):
        await self.disconnect()

    async def connect(self):
        device = await BleakScanner.find_device_by_filter(self.match_uart_uuid)
        if device is None:
            print("no matching device found\n")
            exit(1)
        self.client = BleakClient(device, disconnected_callback=self.handle_disconnect)
        await self.client.connect()
        await self.init_uart_service()
        await self.init_data_service()
        await self.set_monocle_raw_mode()

    async def disconnect(self):
        try:
            await self.client.disconnect()
        except asyncio.exceptions.CancelledError:
            pass

    def log(self, msg):
        if "DEBUG" in os.environ:
            print(msg, flush=True)

    def err(self, msg):
        print(msg, flush=True)

    def match_uart_uuid(self, device:BLEDevice, adv:AdvertisementData):
        self.log(f"uuids={adv.service_uuids}")
        return self.UART_SERVICE_UUID.lower() in adv.service_uuids

    def handle_disconnect(self, _:BleakClient):
        self.log("Device was disconnected.")
        for task in asyncio.all_tasks():
            task.cancel()

    def handle_uart_rx(self, _:BleakGATTCharacteristic, data:bytearray):
        self.log(f"handle_uart_rx: {data}")
        self.uart_rx_buf.extend(data)

    def handle_data_rx(self, _:BleakGATTCharacteristic, data:bytearray):
        self.log(f"handle_data_rx: {data}")
        self.data_rx_buf.extend(data)

    async def get_char_uart(self):
        while len(self.uart_rx_buf) == 0:
            await asyncio.sleep(0.01)
        c = self.uart_rx_buf[0]
        del self.uart_rx_buf[0]
        return c

    async def get_char_data(self):
        while len(self.data_rx_buf) == 0:
            await asyncio.sleep(0.01)
        c = self.data_rx_buf[0]
        del self.data_rx_buf[0]
        return c

    async def get_line_uart(self, delim=b"\n"):
        buf = bytearray()
        while not (c := await self.get_char_uart()) in delim:
            buf.append(c)
        return buf

    async def get_line_data(self, delim="\n"):
        buf = bytearray()
        while not (c := await self.get_char_data()) in delim:
            buf.append(c)
        return buf

    async def get_all_data(self):
        async with self.lock:
            buf = self.data_rx_buf
            self.data_rx_buf = bytearray()
            return buf;

    async def get_all_uart(self):
        async with self.lock:
            buf = self.uart_rx_buf
            self.uart_rx_buf = bytearray()
            return buf;

    async def send_data(self, data):
        rx = self.data_rx_char
        mtu = rx.max_write_without_response_size
        if len(data) > mtu:
            raise ValueError(f"data ({len(data)}) larger than maximum ({mtu})")
        await self.client.write_gatt_char(rx, data)

    async def send_uart(self, data):
        rx = self.uart_rx_char
        mtu = rx.max_write_without_response_size
        for i in range(0, len(data) + 1, mtu):
            await self.client.write_gatt_char(rx, data[i:i + mtu])

    async def send_command(self, cmd):
        x = await self.get_all_data()
        self.log(f"send_command: flushing data: {x}")
        x = await self.get_all_uart()
        self.log(f"send_command: flushing uart: {x}")
        self.log(f"send_command: len={len(cmd)} cmd='{cmd}'")
        await self.send_uart(cmd.encode("ascii") + b"\x04")
        ok = bytes((await self.get_char_uart(), await self.get_char_uart()))
        if ok != b"OK":
            print(f"response is {bytes(ok)} instead of 'OK'")
            await self.get_all_data()
            return None
        result = await self.get_line_uart(delim=b"\x04")
        error = await self.get_line_uart(delim=b"\x04")
        if error != b"":
            print("ERROR on the monocle:")
            print("===")
            print(error.decode('ascii'), end="")
            print("===")
        return result

    async def init_uart_service(self):
        await self.client.start_notify(self.UART_TX_CHAR_UUID, self.handle_uart_rx)
        uart_service = self.client.services.get_service(self.UART_SERVICE_UUID)
        self.uart_rx_char = uart_service.get_characteristic(self.UART_RX_CHAR_UUID)
        self.uart_rx_buf = bytearray()

    async def init_data_service(self):
        await self.client.start_notify(self.DATA_TX_CHAR_UUID, self.handle_data_rx)
        data_service = self.client.services.get_service(self.DATA_SERVICE_UUID)
        self.data_rx_char = data_service.get_characteristic(self.DATA_RX_CHAR_UUID)
        self.data_rx_buf = bytearray()

    async def set_monocle_raw_mode(self):
        await self.client.write_gatt_char(self.uart_rx_char, b"\x01 \x04")
        while await self.get_line_uart(delim=b"\r\n\x04") != b">OK":
            pass
