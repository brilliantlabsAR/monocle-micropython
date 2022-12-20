/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Raj Nakarja - Silicon Witchery AB
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

#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/repl.h"
#include "py/runtime.h"
#include "py/stackctrl.h"

#include "shared/readline/readline.h"
#include "shared/runtime/pyexec.h"

#include "nrf_sdm.h"
#include "nrfx_systick.h"

#include "driver_ble.h"
#include "driver_board.h"
#include "driver_battery.h" // debug

/** Variable that holds the Softdevice NVIC state.  */
nrf_nvic_state_t nrf_nvic_state = {{0}, 0};

/** This is the top of stack pointer as set in the nrf52832.ld file */
extern uint32_t _stack_top;

/** This is the bottom of stack pointer as set in the nrf52832.ld file */
extern uint32_t _stack_bot;

/** This is the start of the heap as set in the nrf52832.ld file */
extern uint32_t _heap_start;

/** This is the end of the heap as set in the nrf52832.ld file */
extern uint32_t _heap_end;

/** Help text that is shown with the help() command.  */
const char help_text[] = {
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
    "For details on a specific module, import it, and then type help(module_name)\n"};

/**
 * Called if an exception is raised outside all C exception-catching handlers.
 */
_Noreturn void nlr_jump_fail(void *val)
{
    (void)val;
    assert(!"exception raised without any handlers for it");
}

#include "nrfx_log.h"
#define LOG NRFX_LOG_ERROR

/**
 * Main application called from Reset_Handler().
 */
int main(void)
{
    // All logging through SEGGER RTT interface
    SEGGER_RTT_Init();
    LOG("Monocle firmware "BUILD_VERSION" "GIT_COMMIT);

    // Initialise BLE, also used for logging
    ble_init();

    // Configure the hardware and IO pins
    board_init();

    // Initialise the stack pointer for the main thread
    mp_stack_set_top(&_stack_top);

    // Set the stack limit as smaller than the real stack so we can recover
    mp_stack_set_limit((char *)&_stack_top - (char *)&_stack_bot - 400);

    // Initialise the garbage collector
    gc_init(&_heap_start, &_heap_end); // TODO optimize away GC if space needed later

    // Initialise the micropython runtime
    mp_init();

    // Initialise the readline module for REPL
    readline_init0();

    // If main.py exits, fallback to a REPL.
    // TODO: add an #ifdef to reboot instead of running the REPL.
    pyexec_frozen_module("main.py");

    // REPL mode can change, or it can request a soft reset
    for (int stop = false; !stop;) {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
            stop = pyexec_raw_repl();
        } else {
            stop = pyexec_friendly_repl();
        }
        LOG("switching the interpreter mode");
    }

    // Deinitialize the board and power things off early
    board_deinit();

    // Garbage collection ready to exit
    gc_sweep_all(); // TODO optimize away GC if space needed later

    // Deinitialize the runtime.
    mp_deinit();

    // Stop the softdevice
    sd_softdevice_disable();

    // Reset chip
    NVIC_SystemReset();
}

/**
 * If an assert is triggered, the firmware reboots into bootloader mode
 * pending for a bugfix to be flashed, then finally booting to firmware.
 */
_Noreturn void __assert_func(const char *file, int line, const char *func, const char *expr)
{
    LOG("%s:%d: %s: %s", file, line, func, expr);
    for (;;) __asm__("bkpt");
}

/**
 * Garbage collection route for nRF.
 */
void gc_collect(void)
{
    // start the GC
    gc_collect_start();

    // Get stack pointer
    uintptr_t sp;
    __asm__("mov %0, sp\n" : "=r"(sp));

    // Trace the stack, including the registers
    // (since they live on the stack in this function)
    gc_collect_root((void **)sp, ((uint32_t)&_stack_top - sp) / sizeof(uint32_t));

    // end the GC
    gc_collect_end();
}
