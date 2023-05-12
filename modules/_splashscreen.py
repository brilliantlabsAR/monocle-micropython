import fpga, display

if fpga.read(1, 4) == b"Mncl":
    t = display.Text("MONOCLE", 320, 200, display.WHITE, justify=display.MIDDLE_CENTER)
    display.show(t)
    del t

del fpga, display
