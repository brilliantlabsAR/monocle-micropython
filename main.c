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

#include "monocle.h"
#include "touch.h"
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

#include "driver/bluetooth_data_protocol.h"
#include "driver/bluetooth_low_energy.h"
#include "driver/config.h"
#include "driver/flash.h"

nrf_nvic_state_t nrf_nvic_state = {{0}, 0};

extern uint32_t _stack_top;
extern uint32_t _stack_bot;
extern uint32_t _heap_start;
extern uint32_t _heap_end;

// Indicate that SPI completed the transfer from the interrupt handler to main loop.
static volatile bool m_xfer_done = true;

// ECX336CN datasheet section 10.1
uint8_t const ecx336cn_config[] = {
    [0x00] = 0x9E, // [0]=0 -> enter power save mode
    [0x01] = 0x20,
    /* * */
    [0x03] = 0x20, // 1125
    [0x04] = 0x3F,
    [0x05] = 0xC8, // 1125  DITHERON, LUMINANCE=0x00=2000cd/m2=medium (Datasheet 10.8)
    /* * */
    [0x07] = 0x40,
    [0x08] = 0x80, // Luminance adjustment: OTPCALDAC_REGDIS=0 (preset mode per reg 5), white chromaticity: OTPDG_REGDIS=0 (preset mode, default)
    /* * */
    [0x0A] = 0x10,
    /* * */
    [0x0F] = 0x56,
    /* * */
    [0x20] = 0x01,
    /* * */
    [0x22] = 0x40,
    [0x23] = 0x40,
    [0x24] = 0x40,
    [0x25] = 0x80,
    [0x26] = 0x40,
    [0x27] = 0x40,
    [0x28] = 0x40,
    [0x29] = 0x0B,
    [0x2A] = 0xBE, // CALDAC=190 (ignored, since OTPCALDAC_REGDIS=0)
    [0x2B] = 0x3C,
    [0x2C] = 0x02,
    [0x2D] = 0x7A,
    [0x2E] = 0x02,
    [0x2F] = 0xFA,
    [0x30] = 0x26,
    [0x31] = 0x01,
    [0x32] = 0xB6,
    /* * */
    [0x34] = 0x03,
    [0x35] = 0x60, // 1125
    /* * */
    [0x37] = 0x76,
    [0x38] = 0x02,
    [0x39] = 0xFE,
    [0x3A] = 0x02,
    [0x3B] = 0x71, // 1125
    /* * */
    [0x3D] = 0x1B,
    /* * */
    [0x3F] = 0x1C,
    [0x40] = 0x02, // 1125
    [0x41] = 0x4D, // 1125
    [0x42] = 0x02, // 1125
    [0x43] = 0x4E, // 1125
    [0x44] = 0x80,
    /* * */
    [0x47] = 0x2D, // 1125
    [0x48] = 0x08,
    [0x49] = 0x01, // 1125
    [0x4A] = 0x7E, // 1125
    [0x4B] = 0x08,
    [0x4C] = 0x0A, // 1125
    [0x4D] = 0x04, // 1125
    /* * */
    [0x4F] = 0x3A, // 1125
    [0x50] = 0x01, // 1125
    [0x51] = 0x58, // 1125
    [0x52] = 0x01,
    [0x53] = 0x2D,
    [0x54] = 0x01,
    [0x55] = 0x15, // 1125
    /* * */
    [0x57] = 0x2B,
    [0x58] = 0x11, // 1125
    [0x59] = 0x02,
    [0x5A] = 0x11, // 1125
    [0x5B] = 0x02,
    [0x5C] = 0x25,
    [0x5D] = 0x04, // 1125
    [0x5E] = 0x0B, // 1125
    /* * */
    [0x60] = 0x23,
    [0x61] = 0x02,
    [0x62] = 0x1A, // 1125
    /* * */
    [0x64] = 0x0A, // 1125
    [0x65] = 0x01, // 1125
    [0x66] = 0x8C, // 1125
    [0x67] = 0x30, // 1125
    /* * */
    [0x69] = 0x00, // 1125
    /* * */
    [0x6D] = 0x00, // 1125
    /* * */
    [0x6F] = 0x60,
    /* * */
    [0x79] = 0x68,
};

void spim_event_handler(nrfx_spim_evt_t const *p_event, void *p_context)
{
    m_xfer_done = true;
}

void spi_chip_select(uint8_t cs_pin)
{
    nrf_gpio_pin_clear(cs_pin);
    nrfx_systick_delay_us(100);
}

void spi_chip_deselect(uint8_t cs_pin)
{
    nrf_gpio_pin_set(cs_pin);
    nrfx_systick_delay_us(100);
}

static void spi_xfer_chunk(nrfx_spim_xfer_desc_t *xfer)
{
    const nrfx_spim_t spi2 = NRFX_SPIM_INSTANCE(2);

    // wait for any pending SPI operation to complete
    while (!m_xfer_done)
        sd_app_evt_wait();

    // Start the transaction and wait for the interrupt handler to warn us it is done.
    m_xfer_done = false;
    app_err(nrfx_spim_xfer(&spi2, xfer, 0));
    while (!m_xfer_done)
        __WFE();
}

void spi_read(uint8_t *buf, size_t len)
{
    for (size_t n = 0; len > 0; len -= n, buf += n)
    {
        n = MIN(255, len);
        nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_RX(buf, n);
        spi_xfer_chunk(&xfer);
    }
}

void spi_write(uint8_t const *buf, size_t len)
{
    for (size_t n = 0; len > 0; len -= n, buf += n)
    {
        n = MIN(255, len);
        nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(buf, n);
        spi_xfer_chunk(&xfer);
    }
}

static inline const void ecx336cn_write_byte(uint8_t addr, uint8_t data)
{
    spi_chip_select(ECX336CN_CS_N_PIN);
    spi_write(&addr, 1);
    spi_write(&data, 1);
    spi_chip_deselect(ECX336CN_CS_N_PIN);
}

static inline uint8_t ecx336cn_read_byte(uint8_t addr)
{
    uint8_t data;

    ecx336cn_write_byte(0x80, 0x01);
    ecx336cn_write_byte(0x81, addr);

    spi_chip_select(ECX336CN_CS_N_PIN);
    spi_write(&addr, 1);
    spi_read(&data, 1);
    spi_chip_deselect(ECX336CN_CS_N_PIN);

    return data;
}

static void touch_interrupt_handler(nrfx_gpiote_pin_t pin,
                                    nrf_gpiote_polarity_t polarity)
{
    (void)pin;
    (void)polarity;

    /*
    // Read the interrupt registers
    i2c_response_t global_reg_0x11 = i2c_read(TOUCH_I2C_ADDRESS, 0x11, 0xFF);
    i2c_response_t sar_ui_reg_0x12 = i2c_read(TOUCH_I2C_ADDRESS, 0x12, 0xFF);
    i2c_response_t sar_ui_reg_0x13 = i2c_read(TOUCH_I2C_ADDRESS, 0x13, 0xFF);
    */
    touch_action_t touch_action = A_TOUCH; // TODO this should be decoded from the I2C responses
    touch_event_handler(touch_action);
}

/**
 * @brief Main application called from Reset_Handler().
 */
int main(void)
{
    // Set up debug logging
    {
        SEGGER_RTT_Init();
        log_clear();
        log("MicroPython on Monocle - " BUILD_VERSION " (" MICROPY_GIT_HASH ") ");
    }

    // Set up the PMIC and go to sleep if on charge
    monocle_critical_startup();

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

        channel.channel_config.reference = NRF_SAADC_REFERENCE_INTERNAL;
        channel.channel_config.gain = NRF_SAADC_GAIN1_2;

        app_err(nrfx_saadc_channel_config(&channel));
    }

    // Set up BLE
    ble_init();

    // Set up the SPI bus to the FPGA and external flash IC
    {
        const nrfx_spim_t spi2 = NRFX_SPIM_INSTANCE(2);

        nrfx_spim_config_t config = NRFX_SPIM_DEFAULT_CONFIG(
            SPI2_SCK_PIN, SPI2_MOSI_PIN, SPI2_MISO_PIN, NRFX_SPIM_PIN_NOT_USED);

        config.frequency = NRF_SPIM_FREQ_4M;
        config.mode = NRF_SPIM_MODE_3;
        config.bit_order = NRF_SPIM_BIT_ORDER_LSB_FIRST;

        app_err(nrfx_spim_init(&spi2, &config, spim_event_handler, NULL));
    }

    // Check if external flash has an FPGA image and boot it
    {
        // nrf_gpio_pin_set(FLASH_CS_N_PIN);
        // nrf_gpio_cfg_output(FLASH_CS_N_PIN);

        // Let the FPGA start as soon as it has the power on.
        nrf_gpio_cfg_output(FPGA_RECONFIG_N_PIN);
        nrf_gpio_pin_set(FPGA_RECONFIG_N_PIN);
    }

    // Setup camera
    {
        // Set to 0V = hold camera in reset.
        // nrf_gpio_pin_clear(OV5640_RESETB_N_PIN);
        // nrf_gpio_cfg_output(OV5640_RESETB_N_PIN);

        // // Set to 0V = not asserted.
        // nrf_gpio_pin_clear(OV5640_PWDN_PIN);
        // nrf_gpio_cfg_output(OV5640_PWDN_PIN);
        // ov5640_init();
    }

    // Setup display
    {
        // configure CS pin for the Display (for active low)
        nrf_gpio_pin_set(ECX336CN_CS_N_PIN);
        nrf_gpio_cfg_output(ECX336CN_CS_N_PIN);

        ecx336cn_write_byte(0x00, 0x9E); // enter power saving mode (YUV)
        // it is now possible to turn off the 10V rail

        for (size_t i = 0; i < sizeof(ecx336cn_config) / sizeof(*ecx336cn_config); i++)
        {
            ecx336cn_write_byte(i, ecx336cn_config[i]);
        }

        // the 10V power rail needs to be turned back on first
        ecx336cn_write_byte(0x00, 0x9F); // exit power saving mode (YUV)

        // check that 0x29 changed from default 0x0A to 0x0B
        app_err(ecx336cn_read_byte(0x29) != 0x0B);
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
void nlr_jump_fail(void *val)
{
    app_err((uint32_t)val);
    NVIC_SystemReset();
}
