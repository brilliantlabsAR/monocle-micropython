import io
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

def t1():
    print("Test setting and checking the time")
    __test("time.time(1674252171)", None)
    __test("time.time()", 1674252171)
    print("Test invalid values")
    __test("time.time(-1)", ValueError)
    __test("time.zone('-14:00')", ValueError)
    __test("time.zone('15:00')", ValueError)

def t2():
    display.line(0, 0, 640, 400, 0x000099)
    display.show()
    print("blue line from top left to bottom right")
    time.sleep(1)

    display.line(0, 400, 640, 0, 0x990000)
    display.show()
    print("red line from top right to bottom left")
    time.sleep(1)

    #display.text("test", 300, 200, 0xFFFFFF)
    #display.show()
    #print("the white text 'test' in the middle")
    #time.sleep(1)

def all():
    t1()
    t2()
