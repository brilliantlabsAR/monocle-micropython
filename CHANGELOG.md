Changelog
=========

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
