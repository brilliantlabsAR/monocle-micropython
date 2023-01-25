/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Inc (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Inc.
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/repl.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/builtin.h"

#include "shared/readline/readline.h"
#include "shared/runtime/pyexec.h"
#include "genhdr/mpversion.h"

#include "nrfx.h"

#include "nrfx_log.h"
#include "nrf_sdm.h"
#include "nrf_power.h"
#include "nrfx_twi.h"
#include "nrfx_spim.h"
#include "nrfx_systick.h"
#include "nrfx_gpiote.h"
#include "nrf_nvic.h"
#include "nrfx_saadc.h"
#include "nrfx_rtc.h"
#include "nrfx_glue.h"

#include "driver/battery.h"
#include "driver/bluetooth_data_protocol.h"
#include "driver/bluetooth_low_energy.h"
#include "driver/config.h"
#include "driver/ecx336cn.h"
#include "driver/flash.h"
#include "driver/fpga.h"
#include "critical_functions.h"
#include "driver/iqs620.h"
#include "driver/ov5640.h"
#include "driver/spi.h"
#include "driver/timer.h"

nrf_nvic_state_t nrf_nvic_state = {{0}, 0};

extern uint32_t _stack_top;
extern uint32_t _stack_bot;
extern uint32_t _heap_start;
extern uint32_t _heap_end;

const char help_text[] = {
    "Welcome to MicroPython!\n\n"
    "For full documentation, visit: https://docs.brilliantmonocle.com\n"
    "Control commands:\n"
    "  Ctrl-A - enter raw REPL mode\n"
    "  Ctrl-B - enter normal REPL mode\n"
    "  CTRL-C - interrupt a running program\n"
    "  Ctrl-D - reset the device\n"
    "  Ctrl-E - enter paste mode\n\n"
    "To list available modules, type help('modules')\n"
    "For details on a specific module, import it, and then type "
    "help(module_name)\n"};

// TODO
void mp_hal_delay_ms(mp_uint_t ms)
{
    // uint64_t delay = ms * 1000;

    // uint64_t t0 = esp_timer_get_time();

    // while (esp_timer_get_time() - t0 < delay)
    // {
    // vTaskDelay(1);
    // mp_handle_pending(true);
    // }
}

// TODO
void mp_hal_delay_us(mp_uint_t us)
{
    // uint64_t t0 = esp_timer_get_time();

    // while (esp_timer_get_time() - t0 < us)
    // {
    // mp_handle_pending(true);
    // }
}

// TODO
mp_uint_t mp_hal_ticks_ms(void)
{
    return 0;
}

// TODO
mp_uint_t mp_hal_ticks_us(void)
{
    return 0;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs)
{
    // File opening is currently not supported
    mp_raise_OSError(MP_EPERM);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

mp_lexer_t *mp_lexer_new_from_file(const char *filename)
{
    // File opening is currently not supported
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path)
{
    // File opening is currently not supported
    return MP_IMPORT_STAT_NO_EXIST;
}

int mp_hal_stdin_rx_chr(void)
{
    return ble_nus_rx();
}

void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len)
{
    ble_nus_tx(str, len);
}

static void touch_interrupt_handler(nrfx_gpiote_pin_t pin,
                                    nrf_gpiote_polarity_t action)
{
    (void)pin;
    (void)action;
    // TODO
    log("touch event!");
}

/**
 * @brief Main application called from Reset_Handler().
 */
int main(void)
{
    {
        SEGGER_RTT_Init();
        log_clear();
        log("MicroPython on Monocle - " BUILD_VERSION " (" MICROPY_GIT_HASH ") ");
    }

    // Important initialisation to set up power rails and charging/sleep behaviour
    setup_pmic_and_sleep_mode();

    // Setup touch interrupt
    {
        app_err(nrfx_gpiote_init(NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY));
        nrfx_gpiote_in_config_t config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
        app_err(nrfx_gpiote_in_init(TOUCH_INTERRUPT_PIN, &config, touch_interrupt_handler));
        nrfx_gpiote_in_event_enable(TOUCH_INTERRUPT_PIN, true);
    }

    // Setup battery ADC input
    {
        app_err(nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY));

        nrfx_saadc_channel_t channel =
            NRFX_SAADC_DEFAULT_CHANNEL_SE(BATTERY_LEVEL_PIN, 0);

        channel.channel_config.reference = NRF_SAADC_REFERENCE_VDD4;
        channel.channel_config.gain = NRF_SAADC_GAIN1_4;

        app_err(nrfx_saadc_channel_config(&channel));
    }

    // Set up BLE
    ble_init();

    // Setup devices on SPI (FPGA & Flash)
    {
        spi_init(spi2, SPI2_SCK_PIN, SPI2_MOSI_PIN, SPI2_MISO_PIN);

        // Start the FPGA with
        nrf_gpio_cfg_output(FPGA_MODE1_PIN);
        nrf_gpio_pin_write(FPGA_MODE1_PIN, false);

        // Let the FPGA start as soon as it has the power on.
        nrf_gpio_cfg_output(FPGA_RECONFIG_N_PIN);
        nrf_gpio_pin_write(FPGA_RECONFIG_N_PIN, true);

        // fpga_init();

        // flash_init();
        // nrf_gpio_pin_write(FLASH_CS_N_PIN, true);
        // nrf_gpio_cfg_output(FLASH_CS_N_PIN);
    }

    // Setup camera
    {
        // Set to 0V = hold camera in reset.
        // nrf_gpio_pin_write(OV5640_RESETB_N_PIN, false);
        // nrf_gpio_cfg_output(OV5640_RESETB_N_PIN);

        // // Set to 0V = not asserted.
        // nrf_gpio_pin_write(OV5640_PWDN_PIN, false);
        // nrf_gpio_cfg_output(OV5640_PWDN_PIN);
        // ov5640_init();

    }

    // Setup display
    {
        // configure CS pin for the Display (for active low)
        // nrf_gpio_pin_set(ECX336CN_CS_N_PIN);
        // nrf_gpio_cfg_output(ECX336CN_CS_N_PIN);

        // // Set low on boot (datasheet p.11)
        // nrf_gpio_pin_write(ECX336CN_XCLR_PIN, false);
        // nrf_gpio_cfg_output(ECX336CN_XCLR_PIN);
        // ecx336cn_init();
    }

    // Initialise the stack pointer for the main thread
    mp_stack_set_top(&_stack_top);

    // Set the stack limit as smaller than the real stack so we can recover
    mp_stack_set_limit((char *)&_stack_top - (char *)&_stack_bot - 400);

    // Start garbage collection, micropython and the REPL
    gc_init(&_heap_start, &_heap_end);
    mp_init();
    readline_init0();

    // Stay in the friendly or raw REPL until a reset is called
    for (;;)
    {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL)
        {
            if (pyexec_raw_repl() != 0)
            {
                break;
            }
        }
        else
        {
            if (pyexec_friendly_repl() != 0)
            {
                break;
            }
        }
    }

    // On exit, clean up and reset
    gc_sweep_all();
    mp_deinit();
    sd_softdevice_disable();
    NVIC_SystemReset();
}

/**
 * @brief Garbage collection route for nRF.
 */
void gc_collect(void)
{
    // start the GC
    gc_collect_start();

    // Get stack pointer
    uintptr_t sp;
    __asm__("mov %0, sp\n"
            : "=r"(sp));

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
    app_err(1); // exception raised without any handlers for it
    while (1)
    {
    }
}