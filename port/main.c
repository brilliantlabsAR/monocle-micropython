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

#include "nrfx_log.h"
#include "nrf_sdm.h"
#include "nrfx_twi.h"
#include "nrfx_spim.h"
#include "nrfx_systick.h"

#include "driver/battery.h"
#include "driver/bluetooth_data_protocol.h"
#include "driver/bluetooth_low_energy.h"
#include "driver/config.h"
#include "driver/ecx336cn.h"
#include "driver/flash.h"
#include "driver/fpga.h"
#include "driver/i2c.h"
#include "driver/iqs620.h"
#include "driver/jojpeg.h"
#include "driver/max77654.h"
#include "driver/nrfx.h"
#include "driver/ov5640.h"
#include "driver/spi.h"
#include "driver/timer.h"

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

/**
 * @brief Garbage collection route for nRF.
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

/**
 * @brief Called if an exception is raised outside all C exception-catching handlers.
 */
NORETURN void nlr_jump_fail(void *val)
{
    (void)val;
    assert(!"exception raised without any handlers for it");
}

/**
 * @brief Main application called from Reset_Handler().
 */
int main(void)
{
    // All logging through SEGGER RTT interface
    SEGGER_RTT_Init();
    PRINTF("\r\n" "Brilliant Monocle " BUILD_VERSION " " GIT_COMMIT "\r\n\r\n");

    // Initialise drivers

    LOG("BLE");
    ble_init();

    LOG("NRFX");
    nrfx_init();

    LOG("I2C");
    i2c_init(i2c0, I2C0_SCL_PIN, I2C0_SDA_PIN);
    i2c_init(i2c1, I2C1_SCL_PIN, I2C1_SDA_PIN);

    LOG("SPI");
    spi_init(spi2, SPI2_SCK_PIN, SPI2_MOSI_PIN, SPI2_MISO_PIN);

    LOG("TIMER");
    //timer_init();

    LOG("MAX77654");
    max77654_init();
    nrfx_systick_delay_ms(1);
    max77654_rail_1v2(true); // TODO: we are turning the FPGA rail off to debug the flash
    nrfx_systick_delay_ms(1);
    max77654_rail_1v8(true);
    nrfx_systick_delay_ms(1);
    max77654_rail_2v7(true);
    nrfx_systick_delay_ms(100); // wait that all the chips start
    max77654_rail_10v(true);
    nrfx_systick_delay_ms(10);

    LOG("FPGA");
    fpga_init();

    LOG("ECX336CN");
    ecx336cn_init();

    LOG("setup done");

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
    //pyexec_frozen_module("main.py");

    // REPL mode can change, or it can request a soft reset
    for (int stop = false; !stop;)
    {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL)
        {
            stop = pyexec_raw_repl();
        }
        else
        {
            stop = pyexec_friendly_repl();
        }
        LOG("switching the interpreter mode");
    }

    // Deinitialize the board and power things off early
    //power_off(); // TODO: implement this?

    // Garbage collection ready to exit
    gc_sweep_all(); // TODO optimize away GC if space needed later

    // Deinitialize the runtime.
    mp_deinit();

    // Stop the softdevice
    sd_softdevice_disable();

    // Reset chip
    NVIC_SystemReset();
}

NORETURN void __assert_func(const char *file, int line, const char *func, const char *expr)
{
    LOG("%s:%d: %s: %s", file, line, func, expr);
    for (;;) __asm__("bkpt");
}
