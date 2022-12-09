/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#ifndef DRIVER_DFU_H
#define DRIVER_DFU_H

/**
 * Device Firmware Upgrade module for rebooting into bootloader mode.
 * @defgroup dfu
 */

_Noreturn void dfu_reboot_bootloader(void);

#endif
