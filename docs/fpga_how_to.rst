Get Started with FPGA development
=================================

Steps
-----
1. Install the toolchain by registering for a GoWin account and download the software at: https://www.gowinsemi.com/en/support/download_eda/. This will require you to request a license.
2. Follow instructions to install the license and enable the toolchain.
3. Open the GoWin tool, open the fpga_proj.gprj file from the File -> Open menu
4. Right click on the "Place & Route" and click on Run.

.. image:: images/how_to_fpga_1.png
  :alt: Screenshot.

5. This will result in a bitfile that can be used to program the flash on the Monocle.
6. Once the Gowin toolchain is installed, open the Gowin programmer application.
7. Select USB Cable setting and set the port to be channel 1 per the screenshot below and hit the Save button.

.. image:: images/how_to_fpga_2.png
  :alt: Screenshot.

8. Select the Series to be GW1N, Device to be GW1N-9 and operation to be Read Device Codes
9. Click on the Green Program/Configure button to read out the ID of the FPGA.
10. You should get the JTAG ID of the FPGA as seen in the screenshot above.

Future development
------------------

* Video Compression for faster Video transfer from Monocle to Phone
