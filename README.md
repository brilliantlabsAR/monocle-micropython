# MicroPython-based Monocle Firmware

This is a port of MicroPython for running in the Nordic nRF51832 chip inside the Monocle.

It also cointains drivers for controlling the rest of the circuit,
some of which are directly accessible from python for driving the monocle.

To drive the Monocle, a Bluetooth serial service (RFCOMM) is broadcasted.

The `serial_console.py` script permits to connect to it from the local computer.
A working [bleak](https://bleak.readthedocs.io/en/latest/) is required for it.

Building
--------
```sh
# install the dependencies
pip install --user intelhex bleak

# import the submodules
git submodule init nrfx
git submodule init micropython
make -C micropython/ports/nrf/ submodules

# build the firmware image
make
```

This should go through everything required to get a `build/firmware.hex` file built-up.

Some more dependencies might need to be installed, such as a cross compiler toolchain:

```sh
# On Ubuntu/Debian-based systems (including Windows WSL):
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi

# On Fedora-based systems:
sudo yum install arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib

# On Arch-based systems:
sudo pacman -Syu arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib

# On MacOS:
brew install arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib
```

Flashing to the Monocle
-----------------------
The devkit can be flashed through the SWD debugger interface with a dongle such as [STlinkV2][1] or [J-Link][2].

[1]: https://www.adafruit.com/product/2548
[2]: https://www.adafruit.com/product/3571

The connection to the Monocle board is as follow:

```

                  ┌─────────────────── GND     -> to programmer dongle
┌─────────────────│───────────────┐
│ () ┌────────────│──────────┐ () │
│    │  o o o o o o o o o o  │    │
│    │  o o o o o o o o o o  │    │
│    └────────│─│  ───│──────┘    │
│             │ │     └─────────────── RESET   -> to programmer dongle
│        ┌────│ │ ───────┐        │
│        │    │ └───────────────────── SWCLK   -> to programmer dongle
│        │    └─────────────────────── SWDIO   -> to programmer dongle
│        │               │        │
│        │               │        │
│        │               │        │
│        └───────────────┘        │
│                                 │
│            |||||||              │
│                                 │
│             ┌──  ──┐            │
│             └──────┘            │
:                                 :
:                                 :
```

Then, the flash command can be issued.

Flashing is done as for any ARM microcontroller

* With [OpenOCD][3] (supporting both st-link/v2 or J-Link)
* WIth [nrfjprog][4] (supporting J-Link only)

[3]: https://openocd.org/
[4]: https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools/download
