/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Raj Nakarja - Silicon Witchery AB
 * Authored by: Josuah Demangeon <me@josuah.net>
 *
 * ISC Licence
 *
 * Copyright © 2022 Brilliant Labs Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * Bluetooth Device Firmware Upgrade (DFU)
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nrf_soc.h"
#include "nrfx_log.h"

#include "driver_dfu.h"

#define LOG NRFX_LOG_ERROR

/**
 * Magic pattern written to GPREGRET register to signal between main
 * app and DFU. The 3 lower bits are assumed to be used for signalling
 * purposes.
 */
#define BOOTLOADER_DFU_GPREGRET         (0xB0)

/**
 * Bit mask to signal from main application to enter DFU mode using a
 * buttonless service.
 */
#define BOOTLOADER_DFU_START_BIT_MASK   (0x01)

/**
 * Magic number to signal that bootloader should enter DFU mode because of
 * signal from Buttonless DFU in main app.
 */
#define BOOTLOADER_DFU_START            (BOOTLOADER_DFU_GPREGRET | BOOTLOADER_DFU_START_BIT_MASK)

_Noreturn void dfu_reboot_bootloader(void)
{
    LOG("resetting system now");

    // Set the flag telling the bootloaer to go into DFU mode.
    sd_power_gpregret_set(0, BOOTLOADER_DFU_START);

    // Reset the CPU, giving controll to the bootloader.
    NVIC_SystemReset();
}
