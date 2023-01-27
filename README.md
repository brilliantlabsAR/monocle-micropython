# MicroPython for Monocle

A custom deployment of MicroPython designed specifically for Monocle. Read the [docs](https://docs.brilliantmonocle.com).

## Flash you Monocle


1. Download the latest `.hex` file from the [releases page](https://github.com/brilliantlabsAR/monocle-micropython/releases).

1. Ensure you have the [nRF Command Line Tools](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools) installed.

1. Connect your debugger as described [here](https://docs.brilliantmonocle.com/monocle/monocle/#manually-programming).

1. Flash your device using the command:

```sh
nrfjprog --program *.hex --chiperase -f nrf52 --verify --reset
```

---

## Getting started with development

1. Ensure you have the [ARM GCC Toolchain](https://developer.arm.com/downloads/-/gnu-rm) installed.

1. Ensure you have the [nRF Command Line Tools](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools) installed.

1. Clone this repository, submodules and build:

    ```sh
    # Clone and jump into this repo
    git clone https://github.com/brilliantlabsAR/monocle-micropython.git
    cd monocle-micropython

    # Initialize the submodules
    git submodule update --init
    git -C micropython submodule update --init lib/micropython-lib
    ```

1. You can now close the terminal and open the project in [VSCode](https://code.visualstudio.com).

    There are three build tasks already configured and ready for use. Access them by pressing `Ctrl-Shift-P` (`Cmd-Shift-P` on MacOS) → `Tasks: Run Task`.

    1. Build (`Ctrl-B` / `Cmd-B`)
    1. Build & flash
    1. Clean

## FPGA

For information on developing and flashing the FPGA binary. Check the [Monocle FPGA](https://github.com/brilliantlabsAR/monocle-fpga) repository.