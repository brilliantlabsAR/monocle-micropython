#
# This file is part of the MicroPython for Monocle project:
#      https://github.com/brilliantlabsAR/monocle-micropython
#
# Authored by: Josuah Demangeon (me@josuah.net)
#              Raj Nakarja / Brilliant Labs Ltd. (raj@itsbrilliant.co)
#
# ISC Licence
#
# Copyright © 2023 Brilliant Labs Ltd.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#

import device as __device
import display as __display
import camera as __camera
import microphone as __microphone
import touch as __touch
import led as __led
import fpga as __fpga
import bluetooth as __bluetooth
import time as __time
import update as __update_py

def __test(evaluate, expected):
    try:
        response = eval(evaluate)
    except Exception as ex:
        response = type(ex)

    if response == expected:
        print(f"Passed - {evaluate} == {response}")
    else:
        print(f"Failed - {evaluate} == {response}. Expected: {expected}")

# Tests for individual modules
def device_module():

    __test("__device.NAME", 'monocle')
    __test("len(__device.mac_address())", 17)
    __test("len(__device.VERSION)", 12)
    __test("len(__device.GIT_TAG)", 9)
    __test("__device.GIT_REPO", 'https://github.com/brilliantlabsAR/monocle-micropython')
    __test("isinstance(__device.battery_level(), int)", True)
    __test("__device.prevent_sleep(True)", None)
    __test("__device.prevent_sleep(False)", None)
    __test("str(__device.Storage())", 'Storage(start=0x0006d000, len=602112)')

def display_module():

    # Line scanning the screen bound to the left
    __display.line(0,0,   640,200, 0xFFFFFF); __display.show()
    __display.line(0,50,  640,200, 0xFFFFFF); __display.show()
    __display.line(0,100, 640,200, 0xFFFFFF); __display.show()
    __display.line(0,150, 640,200, 0xFFFFFF); __display.show()
    __display.line(0,200, 640,200, 0xFFFFFF); __display.show()
    __display.line(0,250, 640,200, 0xFFFFFF); __display.show()
    __display.line(0,300, 640,200, 0xFFFFFF); __display.show()
    __display.line(0,350, 640,200, 0xFFFFFF); __display.show()
    __display.line(0,400, 640,200, 0xFFFFFF); __display.show()

    # Line scanning the screen bound to the right
    __display.line(640,0,   0,200, 0xFFFFFF); __display.show()
    __display.line(640,50,  0,200, 0xFFFFFF); __display.show()
    __display.line(640,100, 0,200, 0xFFFFFF); __display.show()
    __display.line(640,150, 0,200, 0xFFFFFF); __display.show()
    __display.line(640,200, 0,200, 0xFFFFFF); __display.show()
    __display.line(640,250, 0,200, 0xFFFFFF); __display.show()
    __display.line(640,300, 0,200, 0xFFFFFF); __display.show()
    __display.line(640,350, 0,200, 0xFFFFFF); __display.show()
    __display.line(640,400, 0,200, 0xFFFFFF); __display.show()

    # Line scanning the screen bound to the top
    __display.line(300,0,   0,400, 0xFFFFFF); __display.show()
    __display.line(300,0, 100,400, 0xFFFFFF); __display.show()
    __display.line(300,0, 200,400, 0xFFFFFF); __display.show()
    __display.line(300,0, 300,400, 0xFFFFFF); __display.show()
    __display.line(300,0, 400,400, 0xFFFFFF); __display.show()
    __display.line(300,0, 500,400, 0xFFFFFF); __display.show()
    __display.line(300,0, 600,400, 0xFFFFFF); __display.show()
    __display.line(300,0, 640,400, 0xFFFFFF); __display.show()

    # Line scanning the screen bound to the bottom
    __display.line(  0,0, 300,400, 0xFFFFFF); __display.show()
    __display.line(100,0, 300,400, 0xFFFFFF); __display.show()
    __display.line(200,0, 300,400, 0xFFFFFF); __display.show()
    __display.line(300,0, 300,400, 0xFFFFFF); __display.show()
    __display.line(400,0, 300,400, 0xFFFFFF); __display.show()
    __display.line(500,0, 300,400, 0xFFFFFF); __display.show()
    __display.line(600,0, 300,400, 0xFFFFFF); __display.show()
    __display.line(640,0, 300,400, 0xFFFFFF); __display.show()

    # Test constants
    __test("__display.WIDTH", 640)
    __test("__display.HEIGHT", 400)

def camera_module():

    print("TODO camera module")

def microphone_module():

    print("TODO microphone module")

def touch_module():

    __test("__touch.state('A')", False)
    __test("__touch.state('B')", False)
    __test("__touch.state('BOTH')", False)
    __test("__touch.state(__touch.A)", False)
    __test("__touch.state(__touch.B)", False)
    __test("__touch.state(__touch.BOTH)", False)
    __test("__touch.state('B')", False)
    __test("callable(__touch.callback)", True)

def led_module():

    __test("__led.on('RED')", None)
    __test("__led.on(__led.RED)", None)
    __test("__led.off('RED')", None)
    __test("__led.off(__led.RED)", None)
    __test("__led.on('GREEN')", None)
    __test("__led.on(__led.GREEN)", None)
    __test("__led.off('GREEN')", None)
    __test("__led.off(__led.GREEN)", None)

def fpga_module():

    # Test read and check the FPGA chip ID at the same time
    __test("__fpga.read(0x0001, 3)", b'K\x07\x00')

    # Test writes
    __test("__fpga.write(0x0000, b'')", None)
    __test("__fpga.write(0x0000, b'done')", None)
    __test("__fpga.write(0x0000, b'a' * 255)", None)

    # Ensure that ROM strings can't be sent on the SPI
    __test("__fpga.write(0x0000, 'done')", TypeError)

    # Ensure that the min and max transfer sizes are respected
    __test("__fpga.read(0x0000, 256), ", ValueError)
    __test("__fpga.read(0x0000, 0), ", ValueError)
    __test("__fpga.read(0x0000, -1), ", ValueError)
    __test("__fpga.write(0x0000, b'a' * 256)", ValueError)

def bluetooth_module():

    __test("__bluetooth.connected()", True)
    __test("isinstance(__bluetooth.max_length(), int)", True)
    max_length = __bluetooth.max_length()
    __test("__bluetooth.send(b'')", None)
    __test(f"__bluetooth.send(b'a' * {max_length})", None)
    __test(f"__bluetooth.send(b'a' * ({max_length} + 1))", ValueError)
    __test("callable(__bluetooth.receive_callback)", True)

def time_module():

    # Test setting and checking the time
    __test("__time.time(1674252171)", None)
    __test("__time.time()", 1674252171)
    __test("__time.now()", {'timezone': '00:00', 'weekday': 'friday', 'minute': 2, 'day': 20, 'yearday': 20, 'month': 1, 'second': 51, 'hour': 22, 'year': 2023})

    # Check time dict at a specified epoch
    __test("__time.now(1674253104)", {'timezone': '00:00', 'weekday': 'friday', 'minute': 18, 'day': 20, 'yearday': 20, 'month': 1, 'second': 24, 'hour': 22, 'year': 2023})

    # Test timezones
    __test("__time.zone('5:30')", None)
    __test("__time.zone()", '05:30')
    __test("__time.now()", {'timezone': '05:30', 'weekday': 'saturday', 'minute': 32, 'day': 21, 'yearday': 21, 'month': 1, 'second': 51, 'hour': 3, 'year': 2023})
    __test("__time.zone('-12:00')", None)
    __test("__time.now()", {'timezone': '-12:00', 'weekday': 'friday', 'minute': 2, 'day': 20, 'yearday': 20, 'month': 1, 'second': 51, 'hour': 10, 'year': 2023})

    # Test getting epochs from time dict
    __test("__time.mktime({'minute': 18, 'day': 20, 'month': 1, 'second': 24, 'hour': 22, 'year': 2023})", 1674253104)

    # Test sleep
    __test("__time.time(1674252171)", None)
    __test("__time.time()", 1674252171)
    __test("__time.sleep(2)", None)
    __test("__time.time()", 1674252173)
    __test("__time.sleep(0.25)", None)
    __test("__time.time()", 1674252173)

    # Test invalid values
    __test("__time.time(-1)", ValueError)
    __test("__time.zone('-14:00')", ValueError)
    __test("__time.zone('15:00')", ValueError)
    __test("__time.zone('-12:30')", ValueError)

def update_module():

    __test("callable(__update_py.micropython)", True)

    __test("__update_py.Fpga.erase()", None)

    # Ensure that ROM strings can't be sent on the SPI
    __test("__update_py.Fpga.write('done')", TypeError)

    # Write and read back value
    __test("__update_py.Fpga.write(b'done')", None)
    __test("__update_py.Fpga.read(0x0000, 4)", b'done')

    # Check that limits of the FPGA app region are respected
    __test("__update_py.Fpga.read(444430, 4)", b'\xff\xff\xff\xff')
    __test("__update_py.Fpga.read(444430, 8)", ValueError)

def all():
    device_module()
    display_module()
    camera_module()
    microphone_module()
    touch_module()
    led_module()
    fpga_module()
    bluetooth_module()
    time_module()
    update_module()
