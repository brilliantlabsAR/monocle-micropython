#
# This file is part of the MicroPython for Monocle project:
#      https://github.com/brilliantlabsAR/monocle-micropython
#
# Authored by: Josuah Demangeon (me@josuah.net)
#              Raj Nakarja / Brilliant Labs Inc (raj@itsbrilliant.co)
#
# ISC Licence
#
# Copyright © 2023 Brilliant Labs Inc.
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

import time
import display

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
def mod_time():
    print("[ time ]")

    print("Test setting and checking the time")
    __test("time.now(1674252171)", None)
    __test("time.now()", 1674252171)
    __test("time.time()", {'timezone': '00:00', 'weekday': 'friday', 'minute': 2, 'day': 20, 'yearday': 20, 'month': 1, 'second': 51, 'hour': 22, 'year': 2023})

    print("Check time dict at a specified epoch")
    __test("time.now(1674253104)", {'timezone': '00:00', 'weekday': 'friday', 'minute': 18, 'day': 20, 'yearday': 20, 'month': 1, 'second': 24, 'hour': 22, 'year': 2023})

    print("Test timezones")
    __test("time.zone('5:30')", None)
    __test("time.zone()", '05:30')
    __test("time.now()", {'timezone': '05:30', 'weekday': 'saturday', 'minute': 32, 'day': 21, 'yearday': 21, 'month': 1, 'second': 51, 'hour': 3, 'year': 2023})
    __test("time.zone('-12:00')", None)
    __test("time.now()", {'timezone': '-12:00', 'weekday': 'friday', 'minute': 2, 'day': 20, 'yearday': 20, 'month': 1, 'second': 51, 'hour': 10, 'year': 2023})

    print("Test getting epochs from time dict")

    print("Test sleep")
    __test("time.time(1674252171)", None)
    __test("time.time()", 1674252171)
    __test("time.sleep(2)", None)
    __test("time.time()", 1674252173)
    __test("time.sleep(0.25)", None)
    __test("time.time()", 1674252173)

    print("Test invalid values")
    __test("time.time(-1)", ValueError)
    __test("time.zone('-14:00')", ValueError)
    __test("time.zone('15:00')", ValueError)
    __test("time.zone('1:20')", ValueError)
    __test("time.sleep(-1)", ValueError)

def mod_display():
    print("[ display ]")

    print("line scanning the screen bound to the left")
    display.line(0,0,   640,200, 0xFFFFFF); display.show()
    display.line(0,50,  640,200, 0xFFFFFF); display.show()
    display.line(0,100, 640,200, 0xFFFFFF); display.show()
    display.line(0,150, 640,200, 0xFFFFFF); display.show()
    display.line(0,200, 640,200, 0xFFFFFF); display.show()
    display.line(0,250, 640,200, 0xFFFFFF); display.show()
    display.line(0,300, 640,200, 0xFFFFFF); display.show()
    display.line(0,350, 640,200, 0xFFFFFF); display.show()
    display.line(0,400, 640,200, 0xFFFFFF); display.show()

    print("line scanning the screen bound to the right")
    display.line(640,0,   0,200, 0xFFFFFF); display.show()
    display.line(640,50,  0,200, 0xFFFFFF); display.show()
    display.line(640,100, 0,200, 0xFFFFFF); display.show()
    display.line(640,150, 0,200, 0xFFFFFF); display.show()
    display.line(640,200, 0,200, 0xFFFFFF); display.show()
    display.line(640,250, 0,200, 0xFFFFFF); display.show()
    display.line(640,300, 0,200, 0xFFFFFF); display.show()
    display.line(640,350, 0,200, 0xFFFFFF); display.show()
    display.line(640,400, 0,200, 0xFFFFFF); display.show()

    print("line scanning the screen bound to the top")
    display.line(300,0,   0,400, 0xFFFFFF); display.show()
    display.line(300,0, 100,400, 0xFFFFFF); display.show()
    display.line(300,0, 200,400, 0xFFFFFF); display.show()
    display.line(300,0, 300,400, 0xFFFFFF); display.show()
    display.line(300,0, 400,400, 0xFFFFFF); display.show()
    display.line(300,0, 500,400, 0xFFFFFF); display.show()
    display.line(300,0, 600,400, 0xFFFFFF); display.show()
    display.line(300,0, 640,400, 0xFFFFFF); display.show()

    print("line scanning the screen bound to the bottom")
    display.line(  0,0, 300,400, 0xFFFFFF); display.show()
    display.line(100,0, 300,400, 0xFFFFFF); display.show()
    display.line(200,0, 300,400, 0xFFFFFF); display.show()
    display.line(300,0, 300,400, 0xFFFFFF); display.show()
    display.line(400,0, 300,400, 0xFFFFFF); display.show()
    display.line(500,0, 300,400, 0xFFFFFF); display.show()
    display.line(600,0, 300,400, 0xFFFFFF); display.show()
    display.line(640,0, 300,400, 0xFFFFFF); display.show()

def run():
    mod_time()
    mod_display()
