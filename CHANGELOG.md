Changelog
=========

v23.019.1521
------------
- Bugfix: Correctly shut down the nRF52832.
  The prototypes using the previous release might have been damaged.

v23.018.1840
------------
- Feature: low power mode when charging in the case.

v23.017.2227
------------
- Feature: rendering library working and flushing to the screen.

v23.017.1326
------------
- Bugfix: Set SPI to a low speed until the display is figured out.

v23.017.1219
------------
- Feature: add a blinking LED codes for the various failure modes for the factory, to remove boards who failed.

v23.016.1142
------------
- Refactor: reduce the number of drivers: added to modles
- Refactor: turn into a fixed startup sequence.
- Feature: add (for now invalid) camera output to jpeg (90s or more!+)

v23.013.2251
------------
- Bugfix for the display on the new MK12 board suggested by Venkat.

v23.012.0631
------------
- Bugfix for the base address of FPGA_CMD_DISPLAY definition.
- Add sample display.show() data.
- Full tracing for the bluetooth module.
- Bugfix for the bluetooth protocol

v23.007.1838
------------
- Implement the `time` module
- Implement the `camera.live()` function
- Bugfix for the systick timer being stuck.

v23.004.0055
------------
- Reorganize all the drivers with a dependency system so that each gets called only once,
  but so that they can be initialized on-demande, and in any order.
- Rework the python API entirely to match the documentation.
- Bugfixes for the drivers, simplified, some rewritten.

v22.346.2141
------------
FPGA: `MK11_FPGA_V0.12.fs`

- Initial pre-release of the new Micropython-based firmware
- Micropython REPL over Bluetooth
- Imported drivers from the previous release
- Partial python interface for controlling the device
- None of the features of the previous releases insofar
- Start sequence with diagnostic for factory testing
- Logs available through RTT

v1.8
----
FPGA: `MK11_FPGA_V0.12.fs`

### New feature implementaion details

- Live Video time increased to 12 Seconds
- LED UI Enabled

### Bugfixes

- BLE Media transfers for Video disabled

### Dependancies

- BLE Stack update to SWL BLE Release 2022-05-09
- nRF SDK 17.0.2

v1.7
----
FPGA: `MK11_FPGA_V0.12.fs`

### New features

- Meadia transfer speed increased
- Adapted Event Scheduler from NRF Library to effectively handle events
- Camera code refactored and opensourced

### Bugfixes

- Touch Flex sensitivity increased

v1.6
----
FPGA: `MK11_FPGA_V0.12.fs`

### New feature implementaion details

- BLE Stack update to SWL BLE Release 2022-05-09
- 5 Sec Live Video change
- Camera code made to be a Lib (support_lib.a)

### Bugfixes

- Factory Mode implemented (Removed OLED notification during Battery Low condition, instead added LED Toggle)
- 1 Sec Delay instead of 18 Sec bootup delay

v1.4
----
FPGA: `MK11_FPGA_V0.12.fs`

### New features

- Flip image to correct optics
- Increase battery termination current cutoff from 5% to 10% (6.8mA) to eliminate CC6 green LED flicker
- Prevent reboot loops by requiring battery be at >20% SoC for boot to proceed
- OTA Protocol change - Added Media Header
- Update LED Blink Codes for Production.

### Bugfixes

- Bluetooth stack bugfix (allow factory reset before pairing)
- Upgrade Bluetooth stack (note: breaks compatibility with previous releases)
- Standby after 5 min or Battery Low
