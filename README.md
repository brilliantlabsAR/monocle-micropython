# MicroPython for Monocle

A custom deployment of MicroPython designed specifically for Monocle. Read the [docs](https://docs.brilliantmonocle.com).

## Flash a release


1. Download the latest `.hex` file from the [releases page](https://github.com/brilliantlabsAR/monocle-micropython/releases).

1. Ensure you have the [nRF Command Line Tools](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools) installed.

1. Connect your debugger as described [here](https://docs.brilliantmonocle.com/monocle/monocle/#manually-programming).

1. Flash your device using the command:

```sh
nrfjprog --program *.hex --chiperase -f nrf52 --verify --reset
```

---

## Local development

### A note about Nix and direnv

If you use the [Nix package manager](https://nixos.org/), you can set up a development environment instantly with `nix develop` or `nix-shell` after cloning the repository and initializing the submodules. Additionally, if you have [direnv](https://direnv.net/) installed, you can also run `direnv allow` to automatically enter the Nix environment when your shell or text editor enters the repository directory. See also the VSCode [direnv extension](https://marketplace.visualstudio.com/items?itemName=mkhl.direnv).

### Getting started

1. Ensure you have the [ARM GCC Toolchain](https://developer.arm.com/downloads/-/gnu-rm) installed.

   (This step is unnecessary if you are using Nix.)

1. Ensure you have the [nRF Command Line Tools](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools) installed.

   (This step is unnecessary if you are using Nix.)

1. Clone this repository, submodules and build:

    ```sh
    # Clone and jump into this repo
    git clone https://github.com/brilliantlabsAR/monocle-micropython.git
    cd monocle-micropython

    # Initialize the submodules
    git submodule update --init
    git -C micropython submodule update --init lib/micropython-lib
    ```

1. You can now close the terminal and open the project in [VSCode](https://code.visualstudio.com) or your favorite text editor.

    In VSCode, there are three build tasks already configured and ready for use. Access them by pressing `Ctrl-Shift-P` (`Cmd-Shift-P` on MacOS) → `Tasks: Run Task`.

    1. Build (`Ctrl-B` / `Cmd-B`)

       This corresponds to the command `make`.
    1. Build & flash

       This corresponds to the command `make flash`.
    1. Clean

       This corresponds to the command `make clean`.
    1. Release

       This corresponds to the command `make release`. See the below section for more details.

### Generating final release `.hex` and DFU `.zip` files

As mentioned above, the following steps (apart from the installation of the required Nordic SDK packages) can be done via `make release` instead.

1. Download and install [nrfutil](https://www.nordicsemi.com/Products/Development-tools/nRF-Util) including the `nrf5sdk-tools` package:

    ```sh
    chmod +x nrfutil
    # Make sure to add nrfutil to your path
    nrfutil install nrf5sdk-tools
    ```

   (This step is unnecessary if you are using Nix.)

1. Download and install the `mergehex` tool which is a part of the [nRF Command Line Tools](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools).

   (This step is unnecessary if you are using Nix.)

1. Generate a settings file:

    ```sh
    nrfutil settings generate --family NRF52 --application build/application.hex --application-version 0 --bootloader-version 0 --bl-settings-version 2 build/settings.hex
    ```

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
