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

#include <math.h>
#include "monocle.h"
#include "nrfx_timer.h"
#include "nrfx_twim.h"
#include "nrfx_systick.h"
#include "nrf_gpio.h"
#include "nrf_soc.h"

static const nrfx_twim_t i2c_bus_0 = NRFX_TWIM_INSTANCE(0);
static const nrfx_twim_t i2c_bus_1 = NRFX_TWIM_INSTANCE(1);

/**
 * @brief Startup and PMIC initialization.
 *
 * @warning CHANGING THIS CODE CAN DAMAGE YOUR HARDWARE.
 *          Read the PMIC datasheet carefully before changing any PMIC settings.
 */

static void check_if_battery_charging_and_sleep(nrf_timer_event_t event_type,
                                                void *p_context)
{
    (void)event_type;
    (void)p_context;

    // Get the CHG value from STAT_CHG_B
    i2c_response_t battery_charging_resp = i2c_read(PMIC_I2C_ADDRESS, 0x03, 0x0C);
    app_err(battery_charging_resp.fail);
    if (battery_charging_resp.value)
    {
        // Turn off all the rails
        // CAUTION: READ DATASHEET CAREFULLY BEFORE CHANGING THESE
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x13, 0x2D, 0x04).fail); // Turn off 10V on PMIC GPIO2
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x3B, 0x1F, 0x0C).fail); // Turn off LDO to LEDs
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x2A, 0x0F, 0x0C).fail); // Turn off 2.7V
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x39, 0x1F, 0x1C).fail); // Turn off 1.8V on load switch
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x2E, 0x0F, 0x0C).fail); // Turn off 1.2V

        // Disconnect AMUX
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x28, 0x0F, 0x00).fail);

        // Put PMIC main bias into low power mode
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x10, 0x20, 0x20).fail);

        nrf_gpio_cfg_sense_input(TOUCH_INTERRUPT_PIN,
                                 NRF_GPIO_PIN_NOPULL,
                                 NRF_GPIO_PIN_SENSE_LOW);

        NRF_POWER->SYSTEMOFF = 1;

        // Never return
        while (1)
        {
        }
    }
}

void monocle_critical_startup(void)
{
    // Enable the the DC/DC convertor
    NRF_POWER->DCDCEN = 0x00000001;

    // Set up the I2C buses
    {
        nrfx_twim_config_t bus_0_config = NRFX_TWIM_DEFAULT_CONFIG(PMIC_TOUCH_I2C_SCL_PIN,
                                                                   PMIC_TOUCH_I2C_SDA_PIN);
        bus_0_config.frequency = NRF_TWIM_FREQ_100K;

        nrfx_twim_config_t bus_1_config = NRFX_TWIM_DEFAULT_CONFIG(CAMERA_I2C_SCL_PIN,
                                                                   CAMERA_I2C_SDA_PIN);
        bus_1_config.frequency = NRF_TWIM_FREQ_100K;

        app_err(nrfx_twim_init(&i2c_bus_0, &bus_0_config, NULL, NULL));
        app_err(nrfx_twim_init(&i2c_bus_1, &bus_1_config, NULL, NULL));

        nrfx_twim_enable(&i2c_bus_0);
        nrfx_twim_enable(&i2c_bus_1);
    }

    // Check the PMIC and initialize battery charger settings
    {
        // Read the PMIC CID
        i2c_response_t resp = i2c_read(PMIC_I2C_ADDRESS, 0x14, 0x0F);

        if (resp.fail || resp.value != 0x02)
        {
            app_err(resp.value);
        }

        // Vhot & Vwarm = 45 degrees. Vcool = 15 degrees. Vcold = 0 degrees
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x20, 0xFF, 0x2E).fail);

        // Set CHGIN limit to 475mA
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x21, 0x1C, 0x10).fail);

        // Charge termination current = 5%
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x22, 0x18, 0x00).fail);

        // Set junction regulation temperature to 70 degrees
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x23, 0xE0, 0x20).fail);

        // Set the fast charge current value to 75mA
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x24, 0xFC, 0x24).fail);

        // Set the Vcool & Vwarm current to 75mA, and enable the thermistor
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x25, 0xFE, 0x26).fail);

        // Set constant voltage to 4.3V for both fast charge and JEITA
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x26, 0xFC, 0x70).fail);
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x27, 0xFC, 0x70).fail);
    }

    // Configure the touch IC
    {
        // Read the touch CID
        i2c_response_t resp = i2c_read(TOUCH_I2C_ADDRESS, 0x00, 0xFF);
        if (resp.fail || resp.value != 0x41)
        {
            app_err(resp.value);
        }

        app_err(i2c_write(TOUCH_I2C_ADDRESS, 0xD0, 0x60, 0x60).fail); // Ack resets and enable event mode
        app_err(i2c_write(TOUCH_I2C_ADDRESS, 0xD1, 0xFF, 0x03).fail); // Enable ch0 and ch1
        app_err(i2c_write(TOUCH_I2C_ADDRESS, 0xD2, 0x20, 0x20).fail); // Disable auto power mode switching // TODO enable ULP mode
        app_err(i2c_write(TOUCH_I2C_ADDRESS, 0x40, 0xFF, 0x01).fail); // Enable rx0 to cap sensing
        app_err(i2c_write(TOUCH_I2C_ADDRESS, 0x41, 0xFF, 0x02).fail); // Enable rx1 to cap sensing
        app_err(i2c_write(TOUCH_I2C_ADDRESS, 0x43, 0x60, 0x20).fail); // 15pf, 1/8 divider on ch0
        app_err(i2c_write(TOUCH_I2C_ADDRESS, 0x44, 0x60, 0x20).fail); // 15pf, 1/8 divider on ch1
        app_err(i2c_write(TOUCH_I2C_ADDRESS, 0x46, 0xFF, 0x1E).fail); // ATI base 75 and target = 30 on ch0
        app_err(i2c_write(TOUCH_I2C_ADDRESS, 0x47, 0xFF, 0x1E).fail); // ATI base 75 and target = 30 on ch1
        app_err(i2c_write(TOUCH_I2C_ADDRESS, 0x60, 0xFF, 0x0A).fail); // Proximity thresholds
        app_err(i2c_write(TOUCH_I2C_ADDRESS, 0x62, 0xFF, 0x0A).fail); // Proximity thresholds
        app_err(i2c_write(TOUCH_I2C_ADDRESS, 0x61, 0xFF, 0x0A).fail); // Proximity thresholds
        app_err(i2c_write(TOUCH_I2C_ADDRESS, 0x63, 0xFF, 0x0A).fail); // Proximity thresholds
        app_err(i2c_write(TOUCH_I2C_ADDRESS, 0xD0, 0x22, 0x22).fail); // Redo ATI and enable event mode
                                                                      // TODO what interrupts are enabled?

        // Delay to complete configuration
        for (int i = 0; i < 10000000; i++)
        {
            __asm volatile("nop");
        }
    }

    check_if_battery_charging_and_sleep(0, NULL); // This won't return if charging

    // Set up a timer for checking charge state periodically
    {
        nrfx_timer_t timer = NRFX_TIMER_INSTANCE(4);

        nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG;
        timer_config.frequency = NRF_TIMER_FREQ_31250Hz;
        timer_config.bit_width = NRF_TIMER_BIT_WIDTH_24;
        app_err(nrfx_timer_init(&timer,
                                &timer_config,
                                check_if_battery_charging_and_sleep));

        nrfx_timer_extended_compare(&timer, NRF_TIMER_CC_CHANNEL0, 156250,
                                    NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

        nrfx_timer_enable(&timer);
    }

    // TODO ignoring all display-related functions below also work

    // Power up everything for normal operation.
    // CAUTION: READ DATASHEET CAREFULLY BEFORE CHANGING THESE
    {
        nrfx_systick_init();

        // Tell the FPGA to start with the embedded flash.
        nrf_gpio_pin_clear(FPGA_MODE1_PIN);
        nrf_gpio_cfg_output(FPGA_MODE1_PIN);

        // Display: 9. Power Supply Sequence (datasheet p.11)
        nrf_gpio_pin_clear(ECX336CN_XCLR_PIN);
        nrf_gpio_cfg_output(ECX336CN_XCLR_PIN);

        // Set SBB2 to 1.2V and turn on
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x2D, 0xFF, 0x08).fail);
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x2E, 0x4F, 0x4F).fail);

        // Set LDO0 to load switch mode and turn on
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x39, 0x1F, 0x1F).fail);

        // Display: 9. Power Supply Sequence (datasheet p.11)
        nrfx_systick_delay_ms(1);

        // Set SBB0 to 2.8V and turn on
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x29, 0xFF, 0x28).fail);
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x2A, 0x4F, 0x4F).fail);

        // Configure LEDs on GPIO0 and GPIO1 as open drain outputs. Set to hi-z
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x11, 0x2D, 0x08).fail);
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x12, 0x2D, 0x08).fail);

        // Set LDO1 to 3.3V and turn on
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x3A, 0xFF, 0x64).fail);
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x3B, 0x1F, 0x0F).fail);

        // Display: 9. Power Supply Sequence (datasheet p.11)
        nrf_gpio_pin_set(ECX336CN_XCLR_PIN);

        // Enable the 10V boost
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x13, 0x2D, 0x0C).fail);

        // Connect AMUX to battery voltage
        app_err(i2c_write(PMIC_I2C_ADDRESS, 0x28, 0x0F, 0x03).fail);
    }
}

/**
 * @brief Bootloader entry function.
 */

void monocle_enter_bootloader(void)
{
    // Set the persistent memory flag telling the bootloader to go into DFU mode.
    sd_power_gpregret_set(0, 0xB1);

    // Reset the CPU, giving control to the bootloader.
    NVIC_SystemReset();
}
