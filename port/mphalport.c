/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Glenn Ruben Bakke
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "py/mpstate.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "driver/bluetooth_low_energy.h"

#include "nrf_soc.h"

/** Help text that is shown with the help() command.  */
const char mp_hal_help_text[] = {
    "Welcome to MicroPython!\n\n"
    "For micropython help, visit: https://docs.micropython.org\n"
    "For hardware help, visit: https://docs.siliconwitchery.com\n\n"
    "Control commands:\n"
    "  Ctrl-A - enter raw REPL mode\n"
    "  Ctrl-B - enter normal REPL mode\n"
    "  CTRL-C - interrupt a running program\n"
    "  Ctrl-D - reset the device\n"
    "  Ctrl-E - enter paste mode\n\n"
    "To list available modules, type help('modules')\n"
    "For details on a specific module, import it, and then type help(module_name)\n"
};

mp_uint_t mp_hal_ticks_ms(void)
{
    return 0;
}

/**
 * @brief Sends data to BLE central device.
 * @param str: String to send.
 * @param len: Length of string.
 */
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len)
{
    ble_nus_tx(str, len);
}

void mp_hal_stdout_tx_strn_cooked(const char *str, mp_uint_t len)
{
    mp_hal_stdout_tx_strn(str, len);
}

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags)
{
    uintptr_t ret = 0;
    if ((poll_flags & MP_STREAM_POLL_RD) && ble_nus_is_rx_pending())
        ret |= MP_STREAM_POLL_RD;
    return ret;
}

/**
 * @brief Takes a single character from the received data buffer, and sends it
 *        to the micropython parser.
 */
int mp_hal_stdin_rx_chr(void)
{
    return ble_nus_rx();
}

void mp_hal_stdout_tx_str(const char *str)
{
    mp_hal_stdout_tx_strn(str, strlen(str));
}

void mp_hal_delay_us(mp_uint_t us)
{
    uint32_t now;
    if (us == 0)
        return;
    now = mp_hal_ticks_us();
    while (mp_hal_ticks_us() - now < us);
}

void mp_hal_delay_ms(mp_uint_t ms)
{
    uint32_t now;
    if (ms == 0)
        return;
    now = mp_hal_ticks_ms();
    while (mp_hal_ticks_ms() - now < ms)
        MICROPY_EVENT_POLL_HOOK
}

/**
 * @brief function called by Micropython to enter bootloader mode.
 * See also "port/mpconfigport.h".
 */
NORETURN void mp_hal_enter_bootloader(void)
{
    // Set the persistent memory flag telling the bootloaer to go into DFU mode.
    sd_power_gpregret_set(0, 0xB1);

    // Reset the CPU, giving controll to the bootloader.
    NVIC_SystemReset();
}
