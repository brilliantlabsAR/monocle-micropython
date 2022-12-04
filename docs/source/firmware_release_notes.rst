Current Release
===============

Monocle Fimrware Version V1.8
-----------------------------

+----------+--------------------+
| Hardware | MK11 V1.0          |
+==========+====================+
| MCU      | MK11_MCU_V1.8.hex  |
+----------+--------------------+
| FPGA     | MK11_FPGA_V0.12.fs |
+----------+--------------------+

New feature implementaion details
---------------------------------
- Live Video time increased to 12 Seconds
- LED UI Enabled

Bugs Solved
-----------
- BLE Media transfers for Video disabled

TAG Version
-----------
*v1.8*

Milestone
---------
*Release v1.8*

New features in the next Release
--------------------------------
* Code Structuring
* Update FPGA Support

Dependancies
------------
* BLE Stack update to SWL BLE Release 2022-05-09
* nRF SDK 17.0.2

Previous Releases
=================

Monocle Fimrware Version V1.7
-----------------------------

+----------+--------------------+
| Hardware | MK11 V1.0          |
+==========+====================+
| MCU      | MK11_MCU_V1.7.hex  |
+----------+--------------------+
| FPGA     | MK11_FPGA_V0.12.fs |
+----------+--------------------+

New feature implementaion details
---------------------------------
- Meadia transfer speed increased
- Adapted Event Scheduler from NRF Library to effectively handle events
- Camera code refactored and opensourced

Bugs Solved
-----------
- Touch Flex sensitivity increased

TAG Version
-----------
*v1.7*

Milestone
---------
*Release v1.7*

Monocle Fimrware Version V1.6
-----------------------------

+----------+--------------------+
| Hardware | MK11 V1.0          |
+==========+====================+
| MCU      | MK11_MCU_V1.6.hex  |
+----------+--------------------+
| FPGA     | MK11_FPGA_V0.12.fs |
+----------+--------------------+

New feature implementaion details
---------------------------------
* BLE Stack update to SWL BLE Release 2022-05-09
* 5 Sec Live Video change
* Camera code made to be a Lib (support_lib.a)

Bugs Solved
-----------
* Factory Mode implemented (Removed OLED notification during Battery Low condition, instead added LED Toggle)
* 1 Sec Delay instead of 18 Sec bootup delay

TAG Version
-----------
*v1.6*

Milestone
---------
*Release v1.6*

Monocle Fimrware Version V1.4
-----------------------------

+----------+--------------------+
| Hardware | MK11 V1.0          |
+==========+====================+
| MCU      | MK11_MCU_V1.4.hex  |
+----------+--------------------+
| FPGA     | MK11_FPGA_V0.12.fs |
+----------+--------------------+

New feature implementaion details
---------------------------------
- Flip image to correct optics
- Increase battery termination current cutoff from 5% to 10% (6.8mA) to eliminate CC6 green LED flicker
- Prevent reboot loops by requiring battery be at >20% SoC for boot to proceed
- OTA Protocol change - Added Media Header
- Update LED Blink Codes for Production.

Bugs Solved
-----------
- Bluetooth stack bugfix (allow factory reset before pairing)
- Upgrade Bluetooth stack (note: breaks compatibility with previous releases)
- Standby after 5 min or Battery Low

TAG Version
-----------
*v1.4*

Milestone
---------
*Release v1.4*
