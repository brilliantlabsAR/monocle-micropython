#!/usr/bin/env python3
"""
UART Service
-------------
An example showing how to write a simple program using the Nordic Semiconductor
(nRF) UART service.
"""

import asyncio
import sys
import os
import tty
import termios
import binascii
from itertools import count, takewhile
from typing import Iterator

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
    DATA_TX_CHAR_UUID: "Monocle Raw TX",
    DATA_RX_CHAR_UUID: "Monocle Raw RX",
})

# You can get this function and more from the ``more-itertools`` package.
def sliced(data: bytes, n: int) -> Iterator[bytes]:
    """
    Slices *data* into chunks of size *n*. The last slice may be smaller than
    *n*.
    """
    return takewhile(len, (data[i : i + n] for i in count(0, n)))


async def repl_terminal():
    """This is a simple "terminal" program that uses the Nordic Semiconductor
    (nRF) UART service. It reads from stdin and sends each line of data to the
    remote device. Any data received from the device is printed to stdout.
    """

    # opens sandard output in binary mode
    stdout = os.fdopen(1, 'wb')

    def match_repl_uuid(device: BLEDevice, adv: AdvertisementData):
        # This assumes that the device includes the UART service UUID in the
        # advertising data. This test may need to be adjusted depending on the
        # actual advertising data supplied by the device.
        sys.stderr.write(f"uuids={adv.service_uuids}\n")
        return UART_SERVICE_UUID.lower() in adv.service_uuids

    device = await BleakScanner.find_device_by_filter(match_repl_uuid)

    if device is None:
        sys.stderr.write("no matching device found\n")
        sys.exit(1)

    def handle_disconnect(_: BleakClient):
        sys.stderr.write("\r\nDevice was disconnected.\r\n")

        # cancelling all tasks effectively ends the program
        for task in asyncio.all_tasks():
            task.cancel()

    def handle_repl_rx(_: BleakGATTCharacteristic, data: bytearray):
        stdout.write(data)
        stdout.flush()

    def handle_data_rx(_: BleakGATTCharacteristic, data: bytearray):
        hex = data.hex(' ', 1)
        sys.stderr.write(f'RX: {hex} {data}\r\n')
        sys.stderr.flush()

    def prompt():
        global saved_term
        if sys.stdin.isatty():
            termios.tcsetattr(0, termios.TCSANOW, saved_term)
        line = sys.stdin.buffer.readline()
        tty.setraw(0)
        return line

    async with BleakClient(device, disconnected_callback=handle_disconnect) as client:
        await client.start_notify(UART_TX_CHAR_UUID, handle_repl_rx)
        await client.start_notify(DATA_TX_CHAR_UUID, handle_data_rx)

        loop = asyncio.get_running_loop()
        repl = client.services.get_service(UART_SERVICE_UUID)
        data = client.services.get_service(DATA_SERVICE_UUID)
        repl_rx_char = repl.get_characteristic(UART_RX_CHAR_UUID)
        data_rx_char = data.get_characteristic(DATA_RX_CHAR_UUID)

        # set the terminal to raw I/O: no buffering
        if sys.stdin.isatty():
            tty.setraw(0)

        # Infinite loop to read the input character until the end
        while True:
            ch = await loop.run_in_executor(None, sys.stdin.buffer.read, 1)
            if not ch: # EOF
                break
            if ch == b'\x16': # Ctrl-V
                sys.stderr.write(f'TX: ')
                sys.stderr.flush()
                line = await loop.run_in_executor(None, prompt)
                await client.write_gatt_char(data_rx_char, line)
            else:
                await client.write_gatt_char(repl_rx_char, ch)


if __name__ == "__main__":
    # save the terminal I/O state
    if sys.stdin.isatty():
        saved_term = termios.tcgetattr(0)

    try:
        asyncio.run(repl_terminal())
    except asyncio.CancelledError:
        # task is cancelled on disconnect, so we ignore this error
        pass

    # restore terminal I/O state
    if sys.stdin.isatty():
        termios.tcsetattr(0, termios.TCSANOW, saved_term)
