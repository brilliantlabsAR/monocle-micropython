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
def time():
    print("Test setting and checking the time")
    __test("time.time(1674252171)", None)
    __test("time.time()", 1674252171)
    print("Test invalid values")
    __test("time.time(-1)", ValueError)
    __test("time.zone('-14:00')", ValueError)
    __test("time.zone('15:00')", ValueError)

def all():
    time()
