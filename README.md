# MicroPython for Monocle

A custom deployment of MicroPython designed specifically for Monocle. Check out the user docs [here](https://docs.brilliant.xyz).

For those of you who want to modify the standard firmware, keep on reading.

## Getting started with development

1. Ensure you have the [ARM GCC Toolchain](https://developer.arm.com/downloads/-/gnu-rm) installed.

1. Ensure you have the [nRF Command Line Tools](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools) installed.

1. Clone this repository along with submodules and build the mpy-cross toolchain:

    ```sh
    git clone https://github.com/brilliantlabsAR/monocle-micropython.git
    cd monocle-micropython

    git submodule update --init
    git -C micropython submodule update --init lib/micropython-lib

    make -C micropython/mpy-cross
    ```

1. You should now be able to build the project by calling `make` from the `monocle-micropython` folder.

    ```sh
    make
    ```

1. Before flashing an nRF5340, you may need to unlock the chip first.

    ```sh
    nrfjprog --recover
    ```

1. You should then be able to flash the device.

    ```sh
    make flash
    ```

### Debugging

1. Open the project in [VSCode](https://code.visualstudio.com).

    There are some build tasks already configured within `.vscode/tasks.json`. Access them by pressing `Ctrl-Shift-P` (`Cmd-Shift-P` on MacOS) → `Tasks: Run Task`.

    1. Build
    1. Build & Flash Chip
    1. Erase & Unlock Chip
    1. Clean

1. Connect your debugger as described [here](https://docs.brilliant.xyz/monocle/monocle/#manually-programming).

1. You many need to unlock the device by using the `Erase Chip` task before programming or debugging.

1. To enable IntelliSense, be sure to select the correct compiler from within VSCode. `Ctrl-Shift-P` (`Cmd-Shift-P` on MacOS) → `C/C++: Select IntelliSense Configuration` → `Use arm-none-eabi-gcc`.

1. Install the [Cortex-Debug](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug) extension for VSCode in order to enable debugging.

1. A debugging launch is already configured within `.vscode/launch.json`. Run the `J-Link` launch configuration from the `Run and Debug` panel, or press `F5`. The project will automatically build and flash before launching.

1. To monitor the logs, run the task `RTT Console` and ensure the `J-Link` launch configuration is running.

### Generating final release `.hex` and DFU `.zip` files

1. Download and install [nrfutil](https://www.nordicsemi.com/Products/Development-tools/nRF-Util) including the `nrf5sdk-tools` package:

    ```sh
    chmod +x nrfutil
    # Make sure to add nrfutil to your path
    nrfutil install nrf5sdk-tools
    ```

1. Generate a settings file:

    ```sh
    nrfutil settings generate --family NRF52 --application build/application.hex --application-version 0 --bootloader-version 0 --bl-settings-version 2 build/settings.hex
    ```

1. Download and install the `mergehex` tool which is a part of the [nRF Command Line Tools](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools).

1. Merge the settings, bootloader, softdevice and application hex files:

    ```sh
    mergehex -m build/settings.hex build/application.hex softdevice/s132_nrf52_7.3.0_softdevice.hex bootloader/build/nrf52832_xxaa_s132.hex -o build/release.hex
    ```

1. Create the DFU zip package using the command:

    ```sh
    nrfutil pkg generate --hw-version 52 --application-version 0 --application build/application.hex --sd-req 0x0124 --key-file bootloader/published_privkey.pem build/release.zip
    ```
    
## FPGA

For information on developing and flashing the FPGA binary. Check the [Monocle FPGA](https://github.com/brilliantlabsAR/monocle-fpga) repository.