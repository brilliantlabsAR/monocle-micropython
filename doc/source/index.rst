Monocle documentation
=====================

This is a port of MicroPython for running in the Nordic nRF51832 chip inside the Monocle.

It also cointains drivers for controlling the rest of the circuit,
some of which are directly accessible from python for driving the Monocle.

To drive the Monocle, a Bluetooth serial service (RFCOMM) is broadcasted.

The `serial_console.py` script permits to connect to it from the local computer.
A working [bleak](https://bleak.readthedocs.io/en/latest/) is required for it.

.. toctree::
   :maxdepth: 5
   :caption: Contents:

   /getting-started
   /drivers/html/index.rst
