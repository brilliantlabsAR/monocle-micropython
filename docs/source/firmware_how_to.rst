Getting Started with the Firmware
=================================

Building
--------

.. code::

   # install the dependencies
   pip install --user intelhex bleak
   
   # import the submodules
   git submodule init nrfx
   git submodule init micropython
   make -C micropython/ports/nrf/ submodules
   
   # build the firmware image
   make

This should go through everything required to get a `build/firmware.hex` file built-up.

Some more dependencies might need to be installed, such as a cross compiler toolchain:

.. code::

   # On Ubuntu/Debian-based systems (including Windows WSL):
   sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi gdb-multiarch
   
   # On Fedora-based systems:
   sudo yum install arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib gdb-multiarch
   
   # On Arch-based systems:
   sudo pacman -Syu arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib gdb-multiarch
   
   # On MacOS:
   brew install arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib gdb-multiarch


Flashing to the Monocle
-----------------------

The devkit can be flashed through the SWD debugger interface with a dongle such as `st-link/v2 <https://www.adafruit.com/product/2548>`_ or `J-Link <https://www.adafruit.com/product/3571>`_.

The connection to the Monocle board is as follow:

.. code::

   ┌─────────────────────────────────┐
   │ () ┌───────────────────────┐ () │
   │    │  o o o o o o o o o o  │    │
   │    │  o o o o o │ o o o o  │    │
   │    └────────│─│ │───│──────┘    │
   │             │ │ │   └─────────────── RESET
   │        ┌────│ │ └─────────────────── GND    st-link
   │        │    │ └───────────────────── SWCLK
   │        │    └─────────────────────── SWDIO
   │        │               │        │
   │        │               │        │
   │        │               │        │
   │        └───────────────┘        │
   │                                 │
   │            |||||||              │
   │                                 │
   │             ┌──  ──┐            │
   │             └──────┘            │
   │                 └─────────────────── SEGGER J-Link
   │                                 │
   :                                 :

Then, the flash command can be issued.

Flashing is done as for any ARM microcontroller

- With `OpenOCD <https://openocd.org/>`_ (supporting both st-link/v2/v3 or J-Link)
- WIth `nrfjprog <https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools/download>`_ (supporting J-Link only)

Once everything is connected:

.. code::

   # If using OpenOCD with an st-link/v2/v3:
   make flash_openocd_stlink
   
   # If using OpenOCD with a J-Link:
   make flash_openocd_jlink
   
   # If using nrfjprog with a J-Link:
   make flash_nrfjprog_jlink


Debugging the firmware with GDB
-------------------------------

You will need `gdb-multiarch` installed on your system.

Then, with the SWD debugger connected as it already is when flashing:

.. code::

   # If using OpenOCD with an st-link/v2/v3:
   make gdb_openocd_stlink
   
   # If using OpenOCD with a J-Link:
   make gdb_openocd_jlink
   
   # If using nrfjprog with a J-Link:
   make gdb_segger_jlink

Then in another terminal, start GDB:

.. code::

   make gdb


Connecting to the Monocle
-------------------------

A `MicroPython <https://micropython.org/>`_ REPL is running on the Monocle.
You can connect to it over the Bluetooth RFCOMM service.


From a computer
^^^^^^^^^^^^^^^

A `serial_console.py` script is provided for connecting over Bluetooth.
You can invoke it with `python3 serial_console.py` or:

.. code::

   make shell

This should give you access to a MicroPython REPL running on the Monocle.

If the connection does not happen, you may need to enable Bluetooth on your system.
For instance, on Linux, you need to start the `bluetoothd` service.
You can then scan the existing devices with `bluetoothctl` or `sudo hcitool lescan`.


From a phone
^^^^^^^^^^^^

You may use a RFCOMM serial console to connect to the shell.

- Android: `Serial Bluetooth Terminal <https://play.google.com/store/apps/details?id=de.kai_morich.serial_bluetooth_terminal>`_
- iOS: `Bluetooth Terminal <https://apps.apple.com/us/app/bluetooth-terminal/id1058693037>`_

You would need to setup macros or switch to hex mode to enter the various CTRL keys:
``CTRL-A`` is ``01``, ``CTRL-B`` is ``02``, ``CTRL-D`` is ``04`` in hex mode.


Troubleshooting
---------------
The community chat is present at `MONOCLE/#support <https://discord.com/channels/963222352534048818/976634834879385621>`_ on Discord.

Future development
------------------

* Audio transfer from Monocle Hardware to Phone Application
* Reliable transfer of data to phone
* Data tranfer from Phone to Monocle Hardware
* FPGA Upgrade feature
