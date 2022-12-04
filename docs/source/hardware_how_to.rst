Get Started with the Developer Board
====================================

This section describes all the necessary steps to connect the Monocle Developer Board for firmware and FPGA development. 

Setup
-----

.. image:: images/Monocle_devboard_back.png
  :alt: Monocle Developer Board Connections



The picture above shows the various components connected to the `Developer Board <https://github.com/Itsbrilliantlabs/monocle-boards/blob/main/Monocle%20dev%20board%20v1.0.pdf>`_
  - Micro USB cable for power
  - JLink programmer required for firmware development/debug
  - `Gowin programmer cable <https://www.gowinsemi.com/en/support/devkits_detail/3/>`_ required for FPGA programming/debug

Note that the Developer board allows you to connect either a 10-pin JTAG/SWD cable or a larger 20-pin connector.

Suggested setups:

- Firmware Developers: Any JTAG debugger that works with the Segger Embedded Studio would work well.
  A variety of JLink programmers have been found to work well.

- FPGA Developers: You will most likely need to also do some firmware modifications if you decide to change the FPGA code so access to a SWD compatible programmer is highly recommended.
  In addition, you will need a `Gowin programmer cable <https://www.gowinsemi.com/en/support/devkits_detail/3/>`_.
