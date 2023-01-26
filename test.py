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
    # line scanning the screen bound to the left
    display.line(0,0,   640,200, 0xFFFFFF); display.show()
    display.line(0,50,  640,200, 0xFFFFFF); display.show()
    display.line(0,100, 640,200, 0xFFFFFF); display.show()
    display.line(0,150, 640,200, 0xFFFFFF); display.show()
    display.line(0,200, 640,200, 0xFFFFFF); display.show()
    display.line(0,250, 640,200, 0xFFFFFF); display.show()
    display.line(0,300, 640,200, 0xFFFFFF); display.show()
    display.line(0,350, 640,200, 0xFFFFFF); display.show()
    display.line(0,400, 640,200, 0xFFFFFF); display.show()

    # line scanning the screen bound to the right
    display.line(640,0,   0,200, 0xFFFFFF); display.show()
    display.line(640,50,  0,200, 0xFFFFFF); display.show()
    display.line(640,100, 0,200, 0xFFFFFF); display.show()
    display.line(640,150, 0,200, 0xFFFFFF); display.show()
    display.line(640,200, 0,200, 0xFFFFFF); display.show()
    display.line(640,250, 0,200, 0xFFFFFF); display.show()
    display.line(640,300, 0,200, 0xFFFFFF); display.show()
    display.line(640,350, 0,200, 0xFFFFFF); display.show()
    display.line(640,400, 0,200, 0xFFFFFF); display.show()

    # line scanning the screen bound to the top
    display.line(300,0,   0,400, 0xFFFFFF); display.show()
    display.line(300,0, 100,400, 0xFFFFFF); display.show()
    display.line(300,0, 200,400, 0xFFFFFF); display.show()
    display.line(300,0, 300,400, 0xFFFFFF); display.show()
    display.line(300,0, 400,400, 0xFFFFFF); display.show()
    display.line(300,0, 500,400, 0xFFFFFF); display.show()
    display.line(300,0, 600,400, 0xFFFFFF); display.show()
    display.line(300,0, 640,400, 0xFFFFFF); display.show()

    # line scanning the screen bound to the bottom
    display.line(  0,0, 300,400, 0xFFFFFF); display.show()
    display.line(100,0, 300,400, 0xFFFFFF); display.show()
    display.line(200,0, 300,400, 0xFFFFFF); display.show()
    display.line(300,0, 300,400, 0xFFFFFF); display.show()
    display.line(400,0, 300,400, 0xFFFFFF); display.show()
    display.line(500,0, 300,400, 0xFFFFFF); display.show()
    display.line(600,0, 300,400, 0xFFFFFF); display.show()
    display.line(640,0, 300,400, 0xFFFFFF); display.show()

def all():
    t1()
    t2()
