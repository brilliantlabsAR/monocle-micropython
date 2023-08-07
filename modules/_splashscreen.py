import fpga, display


def __splashscreen():
    logo = display.Text(
        "MONOCLE", 320, 200, display.WHITE, justify=display.MIDDLE_CENTER
    )
    bars_top = []
    bars_bottom = []

    starting_location = 185

    ax = 68 + starting_location + 70
    bx = 40 + starting_location + 70
    cx = 0 + starting_location + 70
    dx = 28 + starting_location + 70

    for i in range(6):
        if i == 0:
            color = 0xFF0000
        elif i == 1:
            color = 0xFFAA00
        elif i == 2:
            color = 0xFFFF00
        elif i == 3:
            color = 0x008800
        elif i == 4:
            color = 0x0000FF
        else:
            color = 0xAA00FF

        bars_top.append(display.Polygon([ax, 100, bx, 100, cx, 170, dx, 170], color))
        ax += 28
        bx += 28
        cx += 28
        dx += 28

    ax = 68 + starting_location
    bx = 40 + starting_location
    cx = 0 + starting_location
    dx = 28 + starting_location

    for i in range(6):
        if i == 0:
            color = 0xFF0000
        elif i == 1:
            color = 0xFFAA00
        elif i == 2:
            color = 0xFFFF00
        elif i == 3:
            color = 0x008800
        elif i == 4:
            color = 0x0000FF
        else:
            color = 0xAA00FF

        bars_bottom.append(display.Polygon([ax, 230, bx, 230, cx, 300, dx, 300], color))
        ax += 28
        bx += 28
        cx += 28
        dx += 28

    display.show(logo, bars_top, bars_bottom)


if fpga.read(1, 4) == b"Mncl":
    __splashscreen()

del fpga, display, __splashscreen
