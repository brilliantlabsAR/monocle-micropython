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
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi gdb-multiarch

# On Fedora-based systems:
sudo yum install arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib gdb-multiarch

# On Arch-based systems:
sudo pacman -Syu arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib gdb-multiarch

# On MacOS:
brew install arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib gdb-multiarch
```

Flashing to the Monocle
-----------------------
The devkit can be flashed through the SWD debugger interface with a dongle such as [st-link/v2][1] or [J-Link][2].

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

* With [OpenOCD][3] (supporting both st-link/v2/v3 or J-Link)
* WIth [nrfjprog][4] (supporting J-Link only)

[3]: https://openocd.org/
[4]: https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools/download

Once everything is connected:

```
# If using OpenOCD with an st-link/v2/v3:
make flash_openocd_stlink

# If using OpenOCD with a J-Link:
make flash_openocd_jlink

# If using nrfjprog with a J-Link:
make flash_nrfjprog_jlink
```


Debugging the firmware with GDB
-------------------------------
You will need `gdb-multiarch` installed on your system.

Then, with the SWD debugger connected as it already is when flashing:

```
# If using OpenOCD with an st-link/v2/v3:
make gdb_openocd_stlink

# If using OpenOCD with a J-Link:
make gdb_openocd_jlink

# If using nrfjprog with a J-Link:
make gdb_segger_jlink
```

Then in another terminal, start GDB:

```
make gdb
```

Connecting to the monocle
-------------------------
A `serial_console.py` script is provided for connecting over bluetooth.
You can invoke it with `python3 serial_console.py` or:

```
make shell
```

If the connection does not happen, you may need to enable Bluetooth on your system.

For instance, on Linux, you need to start the `bluetoothd` service.
You can then scan the existing devices with `bluetoothctl` or `sudo hcitool lescan`.

On a phone, you may use a RFCOMM serial console to connect to the shell.

* Android: [Serial Bluetooth Terminal][1]
* iOS: [Bluetooth Terminal][2]

[1]: https://play.google.com/store/apps/details?id=de.kai_morich.serial_bluetooth_terminal
[2]: https://apps.apple.com/us/app/bluetooth-terminal/id1058693037

You would need to setup macros or switch to hex mode to enter the various CTRL keys:
`CTRL-A` is `01`, `CTRL-B` is `02`, `CTRL-D` is `04` in hex mode.

Troubleshooting
---------------
The community chat is present at [MONOCLE/#support][3] on Discord.

[3]: https://discord.com/channels/963222352534048818/976634834879385621
