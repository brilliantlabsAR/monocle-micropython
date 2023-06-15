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

import device
import display
import camera
import microphone
import touch
import led
import fpga
import bluetooth
import time
import update
import math
import random


def __test(evaluate, expected):
    try:
        response = eval(evaluate)
    except Exception as ex:
        response = type(ex)

    if type(expected) == str:
        response = str(response)

    if response == expected:
        print(f"Passed - {evaluate} == {response}")
    else:
        print(f"Failed - {evaluate} == {response}. Expected: {expected}")


# Tests for individual modules
def device_module():
    __test("device.NAME", "monocle")
    __test("len(device.mac_address())", 17)
    __test("len(device.VERSION)", 12)
    __test("len(device.GIT_TAG)", 9)
    __test("device.GIT_REPO", "https://github.com/brilliantlabsAR/monocle-micropython")
    __test("isinstance(device.battery_level(), int)", True)
    __test("device.prevent_sleep(True)", None)
    __test("device.prevent_sleep(False)", None)
    __test("str(device.Storage())", "Storage(start=0x0006d000, len=602112)")


def display_module():
    __test("display.WIDTH", 640)
    __test("display.HEIGHT", 400)
    __test("display.FONT_HEIGHT", 48)
    __test("display.FONT_WIDTH", 24)
    __test("display.TOP_LEFT > 0", True)
    __test("display.TOP_CENTER > 0", True)
    __test("display.TOP_RIGHT > 0", True)
    __test("display.MIDDLE_LEFT > 0", True)
    __test("display.MIDDLE_CENTER > 0", True)
    __test("display.MIDDLE_RIGHT > 0", True)
    __test("display.BOTTOM_LEFT > 0", True)
    __test("display.BOTTOM_CENTER > 0", True)
    __test("display.BOTTOM_RIGHT > 0", True)

    __test("display.show()", None)
    __test("display.show([])", None)
    __test("display.show([],[])", None)

    __test("display.Text('hello',0,0,0x123456)", "Text('hello', 0, 0, 0x123456)")
    __test(
        "display.Text('hello',0,0,0x123456,justify=display.MIDDLE_CENTER)",
        "Text('hello', -60, -24, 0x123456)",
    )
    __test(
        "display.Rectangle(0,10,20,30,0x123456)", "Rectangle(0, 10, 20, 30, 0x123456)"
    )
    __test("display.Fill(0x123456)", "Rectangle(0, 0, 639, 399, 0x123456)")
    __test(
        "display.Line(0,10,20,30,0x123456)",
        "Line(0, 10, 20, 30, 0x123456, thickness=1)",
    )
    __test(
        "display.Line(0,10,20,30,0x123456,thickness=3)",
        "Line(0, 10, 20, 30, 0x123456, thickness=3)",
    )
    __test(
        "display.VLine(10,20,30,0x123456)",
        "Line(10, 20, 10, 50, 0x123456, thickness=1)",
    )
    __test(
        "display.VLine(10,20,30,0x123456,thickness=3)",
        "Line(10, 20, 10, 50, 0x123456, thickness=3)",
    )
    __test(
        "display.HLine(10,20,30,0x123456)",
        "Line(10, 20, 40, 20, 0x123456, thickness=1)",
    )
    __test(
        "display.HLine(10,20,30,0x123456,thickness=3)",
        "Line(10, 20, 40, 20, 0x123456, thickness=3)",
    )
    __test(
        "display.Polygon([0,10,20,30,40,50],0x123456)",
        "Polygon([0,10, 20,30, 40,50], 0x123456, thickness=1)",
    )
    __test(
        "display.Polygon([0,10,20,30,40,50],0x123456,thickness=3)",
        "Polygon([0,10, 20,30, 40,50], 0x123456, thickness=3)",
    )
    __test(
        "display.Polyline([0,10,20,30,40,50],0x123456)",
        "Polyline([0,10, 20,30, 40,50], 0x123456, thickness=1)",
    )
    __test(
        "display.Polyline([0,10,20,30,40,50],0x123456,thickness=3)",
        "Polyline([0,10, 20,30, 40,50], 0x123456, thickness=3)",
    )

    # Test text placement, movement and colors
    t_tl = display.Text("top left", 0, 0, 0xFF0000, justify=display.TOP_LEFT)
    t_tr = display.Text("top right", 640, 0, 0x00FF00, justify=display.TOP_RIGHT)
    t_bl = display.Text("bot left", 0, 400, 0x0000FF, justify=display.BOTTOM_LEFT)
    t_br = display.Text("bot right", 640, 400, 0xFFFF00, justify=display.BOTTOM_RIGHT)
    display.show(t_tl, t_tr, t_bl, t_br)
    time.sleep(1)

    t_tc = display.Text(".", 320, 0, 0xFFFFFF, justify=display.TOP_CENTER)
    t_ml = display.Text(".", 0, 200, 0xFFFFFF, justify=display.MIDDLE_LEFT)
    t_mr = display.Text(".", 640, 200, 0xFFFFFF, justify=display.MIDDLE_RIGHT)
    t_bc = display.Text(".", 320, 400, 0xFFFFFF, justify=display.BOTTOM_CENTER)
    global t_mc
    t_mc = display.Text(".", 320, 200, 0xFFFFFF, justify=display.MIDDLE_CENTER)
    display.show([t_tl, t_tr, t_bl, t_br], t_tc, t_ml, t_mr, t_bc, t_mc)
    time.sleep(1)

    t_tc.move(0, 100)
    t_bc.move(0, -100)
    t_ml.move(100, 0)
    t_mr.move(-100, 0)
    display.move([t_bl, t_br], 0, -100)
    t_mc.color(0xFF00FF)  # TODO this isn't consistent with move
    display.color([t_ml, t_mr], 0x00FFFF)
    display.show(t_tl, t_tr, t_bl, t_br, t_tc, t_ml, t_mr, t_bc, t_mc)
    time.sleep(1)

    # Test overlapping text
    t_mc.move(0, 70)
    __test(
        "display.show(t_tl,t_tr,t_bl,t_br,t_tc,t_ml,t_mr,t_bc,t_mc)",
        NameError,
    )
    t_mc.move(0, -70)
    display.show(t_tl, t_tr, t_bl, t_br, t_tc, t_ml, t_mr, t_bc, t_mc)
    time.sleep(1)

    # Test shape primitives
    r = display.Rectangle(20, 380, 100, 300, 0xFF0000)
    l = display.Line(0, 0, 640, 400, 0xFFFFFF)
    vl = display.VLine(320, 100, 200, 0x00FF00)
    hl = display.HLine(220, 200, 200, 0x00FF00)
    pl = display.Polyline([320, 100, 420, 200, 320, 300, 220, 200], 0xFFFF00)
    pg = display.Polygon([540, 20, 620, 20, 620, 100], 0x00FFFF)
    display.show(r, l, vl, hl, pg, pl)
    time.sleep(1)


def camera_module():
    print("TODO camera module")


def microphone_module():
    __test("microphone.record()", None)
    __test("microphone.record(seconds=6.5)", None)
    time.sleep(0.5)
    __test("len(microphone.read())", 127)
    __test("len(microphone.read(samples=10))", 10)
    __test("len(microphone.read(samples=128))", ValueError)
    __test("microphone.compress([23432,24399,24300,24500])", b"[\x88")


def touch_module():
    __test("touch.state('A')", False)
    __test("touch.state('B')", False)
    __test("touch.state('BOTH')", False)
    __test("touch.state(touch.A)", False)
    __test("touch.state(touch.B)", False)
    __test("touch.state(touch.BOTH)", False)
    __test("touch.state('B')", False)
    __test("callable(touch.callback)", True)


def led_module():
    __test("led.on('RED')", None)
    __test("led.on(led.RED)", None)
    __test("led.off('RED')", None)
    __test("led.off(led.RED)", None)
    __test("led.on('GREEN')", None)
    __test("led.on(led.GREEN)", None)
    __test("led.off('GREEN')", None)
    __test("led.off(led.GREEN)", None)


def fpga_module():
    # Test read returns correct length
    __test("len(fpga.read(0x0001, 4))", 4)

    # Test writes
    __test("fpga.write(0x0000, b'')", None)
    __test("fpga.write(0x0000, b'done')", None)
    __test("fpga.write(0x0000, b'a' * 255)", None)

    # Ensure that the min and max transfer sizes are respected
    __test("fpga.read(0x0000, 256), ", ValueError)
    __test("fpga.read(0x0000, 0), ", ValueError)
    __test("fpga.read(0x0000, -1), ", ValueError)
    __test("fpga.write(0x0000, b'a' * 256)", ValueError)


def bluetooth_module():
    __test("bluetooth.connected()", True)
    __test("isinstance(bluetooth.max_length(), int)", True)
    max_length = bluetooth.max_length()
    time.sleep(0.1)
    __test("bluetooth.send(b'')", None)
    time.sleep(0.1)
    __test(f"bluetooth.send(b'a' * {max_length})", None)
    time.sleep(0.1)
    __test(f"bluetooth.send(b'a' * ({max_length} + 1))", ValueError)
    __test("callable(bluetooth.receive_callback)", True)


def time_module():
    # Test setting and checking the time
    __test("time.time(1674252171)", None)
    __test("time.time()", 1674252171)
    __test(
        "time.now()",
        {
            "timezone": "00:00",
            "weekday": "friday",
            "minute": 2,
            "day": 20,
            "yearday": 20,
            "month": 1,
            "second": 51,
            "hour": 22,
            "year": 2023,
        },
    )

    # Check time dict at a specified epoch
    __test(
        "time.now(1674253104)",
        {
            "timezone": "00:00",
            "weekday": "friday",
            "minute": 18,
            "day": 20,
            "yearday": 20,
            "month": 1,
            "second": 24,
            "hour": 22,
            "year": 2023,
        },
    )

    # Test timezones
    __test("time.zone('5:30')", None)
    __test("time.zone()", "05:30")
    __test(
        "time.now()",
        {
            "timezone": "05:30",
            "weekday": "saturday",
            "minute": 32,
            "day": 21,
            "yearday": 21,
            "month": 1,
            "second": 51,
            "hour": 3,
            "year": 2023,
        },
    )
    __test("time.zone('-12:00')", None)
    __test(
        "time.now()",
        {
            "timezone": "-12:00",
            "weekday": "friday",
            "minute": 2,
            "day": 20,
            "yearday": 20,
            "month": 1,
            "second": 51,
            "hour": 10,
            "year": 2023,
        },
    )

    # Test getting epochs from time dict
    __test(
        "time.mktime({'minute': 18, 'day': 20, 'month': 1, 'second': 24, 'hour': 22, 'year': 2023})",
        1674253104,
    )

    # Test sleep
    __test("time.time(1674252171)", None)
    __test("time.time()", 1674252171)
    __test("time.sleep(2)", None)
    __test("time.time()", 1674252173)
    __test("time.sleep(0.25)", None)
    __test("time.time()", 1674252173)

    # Test invalid values
    __test("time.time(-1)", ValueError)
    __test("time.zone('-14:00')", ValueError)
    __test("time.zone('15:00')", ValueError)
    __test("time.zone('-12:30')", ValueError)


def update_module():
    __test("callable(update.micropython)", True)
    __test("callable(update.Fpga.erase)", True)
    __test("callable(update.Fpga.write)", True)

    # Check that limits of the FPGA app region are respected
    __test("len(update.Fpga.read(444430, 4))", 4)
    __test("update.Fpga.read(444430, 8)", ValueError)


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
