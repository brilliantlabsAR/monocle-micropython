/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Ltd. (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Ltd.
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
 * @warning CHANGING THIS CODE CAN DAMAGE YOUR HARDWARE.
 *          Read the PMIC datasheet carefully before changing any PMIC settings.
 */

#include <math.h>
#include <string.h>
#include "monocle.h"
#include "nrf_gpio.h"
#include "nrf_sdm.h"
#include "nrf_soc.h"
#include "nrfx_systick.h"
#include "nrf_power.h"
#include "nrfx_timer.h"
#include "nrfx_twim.h"
#include "nrfx_spim.h"

static const nrfx_twim_t i2c_bus_0 = NRFX_TWIM_INSTANCE(0);
static const nrfx_twim_t i2c_bus_1 = NRFX_TWIM_INSTANCE(1);
static const nrfx_spim_t spi_bus_2 = NRFX_SPIM_INSTANCE(2);

bool prevent_sleep_flag = false;

bool force_sleep_flag = false;

static void power_all_rails(bool enable)
{
    if (enable)
    {
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x2E, 0x7F, 0x6F).fail); // Turn on 1.2V with 500mA limit
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x39, 0x1F, 0x1F).fail); // Turn on 1.8V on load switch LSW0
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x2A, 0x7F, 0x7F).fail); // Turn on 2.8V with 333mA limit
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x13, 0x2D, 0x0C).fail); // Enable the 10V boost
        nrfx_systick_delay_ms(10);

        // Wake up the flash
        uint8_t wakeup_device_id[] = {0xAB, 0, 0, 0};
        monocle_spi_write(FLASH, wakeup_device_id, 4, false);

        return;
    }

    app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x13, 0x2D, 0x04).fail); // Turn off 10V on PMIC GPIO2
    nrfx_systick_delay_ms(200);                                          // Let the 10V decay
    app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x2A, 0x0F, 0x0C).fail); // Turn off 2.8V
    app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x39, 0x1F, 0x1C).fail); // Turn off 1.8V on load switch LSW0
    app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x2E, 0x0F, 0x0C).fail); // Turn off 1.2V
}

static void check_if_battery_charging_and_sleep(nrf_timer_event_t event_type,
                                                void *p_context)
{
    (void)event_type;
    (void)p_context;

    // Get the CHG value from STAT_CHG_B
    i2c_response_t charging_response = monocle_i2c_read(PMIC_I2C_ADDRESS, 0x03, 0x0C);
    app_err(charging_response.fail);

    bool charging = charging_response.value;

    if (charging || force_sleep_flag)
    {
        if (prevent_sleep_flag)
        {
            return;
        }

        // Turn off Bluetooth
        app_err(sd_softdevice_disable());

        // Turn off LDO to LEDs
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x3B, 0x1F, 0x0C).fail);

        // Turn off all the FPGA, display and camera rails
        power_all_rails(false);

        // Disconnect AMUX
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x28, 0x0F, 0x00).fail);

        // Put PMIC main bias into low power mode
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x10, 0x20, 0x20).fail);

        // Disable all busses and GPIO pins
        nrfx_twim_uninit(&i2c_bus_0);
        nrfx_twim_uninit(&i2c_bus_1);
        nrfx_spim_uninit(&spi_bus_2);

        for (uint8_t pin = 0; pin < NUMBER_OF_PINS; pin++)
        {
            nrf_gpio_cfg_default(pin);
        }

        // Set the wakeup pin to be the touch input
        nrf_gpio_cfg_sense_input(TOUCH_INTERRUPT_PIN,
                                 NRF_GPIO_PIN_NOPULL,
                                 NRF_GPIO_PIN_SENSE_LOW);

        NRFX_LOG("Going to sleep");

        // Clear the reset reasons
        NRF_POWER->RESETREAS = 0xF000F;

        // Power down
        NRF_POWER->SYSTEMOFF = 1;
        __DSB();

        // We should never return from here. This is just for debug mode
        while (true)
        {
        }
    }
}

void monocle_critical_startup(void)
{
    // Enable the the DC/DC convertor
    NRF_POWER->DCDCEN = 1;

    // Enable systick timer functions
    nrfx_systick_init();

    // Set up the I2C buses
    {
        nrfx_twim_config_t bus_0_config = NRFX_TWIM_DEFAULT_CONFIG(
            PMIC_TOUCH_I2C_SCL_PIN,
            PMIC_TOUCH_I2C_SDA_PIN);

        bus_0_config.frequency = NRF_TWIM_FREQ_100K;

        nrfx_twim_config_t bus_1_config = NRFX_TWIM_DEFAULT_CONFIG(
            CAMERA_I2C_SCL_PIN,
            CAMERA_I2C_SDA_PIN);

        bus_1_config.frequency = NRF_TWIM_FREQ_100K;

        app_err(nrfx_twim_init(&i2c_bus_0, &bus_0_config, NULL, NULL));
        app_err(nrfx_twim_init(&i2c_bus_1, &bus_1_config, NULL, NULL));

        nrfx_twim_enable(&i2c_bus_0);
        nrfx_twim_enable(&i2c_bus_1);
    }

    // Check the PMIC and initialize all the settings
    // CAUTION: READ DATASHEET CAREFULLY BEFORE CHANGING THESE
    {
        // Read the PMIC CID
        i2c_response_t resp = monocle_i2c_read(PMIC_I2C_ADDRESS, 0x14, 0x0F);

        if (resp.fail || resp.value != 0x02)
        {
            not_real_hardware_flag = true;
        }

        // Turn off the FPGA, flash, display and camera rails
        power_all_rails(false);

        // Set the SBB drive strength
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x2F, 0x03, 0x01).fail);

        // Adjust SBB2 to 1.2V
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x2D, 0xFF, 0x08).fail);

        // Adjust SBB1 (1.8V main rail) current limit to 500mA
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x2C, 0x30, 0x20).fail);

        // Adjust SBB0 to 2.8V
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x29, 0xFF, 0x28).fail);

        // Configure LEDs on GPIO0 and GPIO1 as open drain outputs. Set to hi-z
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x11, 0x2D, 0x08).fail);
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x12, 0x2D, 0x08).fail);

        // Set LDO1 to 3.3V and turn on (for LEDs)
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x3A, 0xFF, 0x64).fail);
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x3B, 0x1F, 0x0F).fail);

        // Vhot & Vwarm = 45 degrees. Vcool = 15 degrees. Vcold = 0 degrees
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x20, 0xFF, 0x2E).fail);

        // Set CHGIN limit to 475mA
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x21, 0x1C, 0x10).fail);

        // Charge termination current = 5%
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x22, 0x18, 0x00).fail);

        // Set junction regulation temperature to 70 degrees TODO increase this?
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x23, 0xE0, 0x20).fail);

        // Set the fast charge current value to 120mA
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x24, 0xFC, 0x3C).fail);

        // Set the Vcool & Vwarm current to 75mA, and enable the thermistor
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x25, 0xFE, 0x26).fail);

        // Set constant voltage to 4.3V for both fast charge and JEITA
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x26, 0xFC, 0x70).fail);
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x27, 0xFC, 0x70).fail);

        // Connect AMUX to battery voltage
        app_err(monocle_i2c_write(PMIC_I2C_ADDRESS, 0x28, 0x0F, 0x03).fail);
    }

    // Configure the touch IC
    {
        // Read the touch CID
        i2c_response_t resp = monocle_i2c_read(TOUCH_I2C_ADDRESS, 0x00, 0xFF);
        if (resp.fail || resp.value != 0x41)
        {
            app_err(resp.value);
        }

        app_err(monocle_i2c_write(TOUCH_I2C_ADDRESS, 0xD0, 0x60, 0x60).fail); // Ack resets and enable event mode
        app_err(monocle_i2c_write(TOUCH_I2C_ADDRESS, 0xD1, 0xFF, 0x03).fail); // Enable ch0 and ch1
        app_err(monocle_i2c_write(TOUCH_I2C_ADDRESS, 0xD2, 0x20, 0x20).fail); // Disable auto power mode switching
        app_err(monocle_i2c_write(TOUCH_I2C_ADDRESS, 0x40, 0xFF, 0x01).fail); // Enable rx0 to cap sensing
        app_err(monocle_i2c_write(TOUCH_I2C_ADDRESS, 0x41, 0xFF, 0x02).fail); // Enable rx1 to cap sensing
        app_err(monocle_i2c_write(TOUCH_I2C_ADDRESS, 0x43, 0x60, 0x20).fail); // 15pf, 1/8 divider on ch0
        app_err(monocle_i2c_write(TOUCH_I2C_ADDRESS, 0x44, 0x60, 0x20).fail); // 15pf, 1/8 divider on ch1
        app_err(monocle_i2c_write(TOUCH_I2C_ADDRESS, 0x46, 0xFF, 0x1E).fail); // ATI base 75 and target = 30 on ch0
        app_err(monocle_i2c_write(TOUCH_I2C_ADDRESS, 0x47, 0xFF, 0x1E).fail); // ATI base 75 and target = 30 on ch1
        app_err(monocle_i2c_write(TOUCH_I2C_ADDRESS, 0x60, 0xFF, 0x0A).fail); // Proximity thresholds
        app_err(monocle_i2c_write(TOUCH_I2C_ADDRESS, 0x62, 0xFF, 0x0A).fail); // Proximity thresholds
        app_err(monocle_i2c_write(TOUCH_I2C_ADDRESS, 0x61, 0xFF, 0x0A).fail); // Proximity thresholds
        app_err(monocle_i2c_write(TOUCH_I2C_ADDRESS, 0x63, 0xFF, 0x0A).fail); // Proximity thresholds
        app_err(monocle_i2c_write(TOUCH_I2C_ADDRESS, 0xD0, 0x22, 0x22).fail); // Redo ATI and enable event mode

        // TODO optimize this delay
        nrfx_systick_delay_ms(1000);
    }

    // Start SPI before sleeping otherwise we'll crash
    monocle_spi_enable(true);

    // This wont return if Monocle is charging
    check_if_battery_charging_and_sleep(0, NULL);

    // Set up a timer for checking charge state periodically
    {
        nrfx_timer_t timer = NRFX_TIMER_INSTANCE(4);

        nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG;
        timer_config.frequency = NRF_TIMER_FREQ_31250Hz;
        timer_config.bit_width = NRF_TIMER_BIT_WIDTH_24;
        app_err(nrfx_timer_init(&timer,
                                &timer_config,
                                check_if_battery_charging_and_sleep));

        nrfx_timer_extended_compare(&timer, NRF_TIMER_CC_CHANNEL0, 15625,
                                    NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

        nrfx_timer_enable(&timer);
    }

    // Setup GPIOs and set initial values
    {
        nrf_gpio_cfg_output(CAMERA_SLEEP_PIN);
        nrf_gpio_cfg_output(CAMERA_RESET_PIN);
        nrf_gpio_cfg_output(DISPLAY_RESET_PIN);
        nrf_gpio_cfg_output(DISPLAY_CS_PIN);
        nrf_gpio_cfg_output(FPGA_CS_MODE_PIN);

        // Flash CS is open drain with pull up so that the FPGA can use it too
        nrf_gpio_cfg(FLASH_CS_PIN,
                     NRF_GPIO_PIN_DIR_OUTPUT,
                     NRF_GPIO_PIN_INPUT_DISCONNECT,
                     NRF_GPIO_PIN_PULLUP,
                     NRF_GPIO_PIN_S0D1,
                     NRF_GPIO_PIN_NOSENSE);

        // The FPGA RESET pin is both an output, as well as interrupt input
        nrf_gpio_cfg(FPGA_RESET_INT_PIN,
                     NRF_GPIO_PIN_DIR_OUTPUT,
                     NRF_GPIO_PIN_INPUT_CONNECT,
                     NRF_GPIO_PIN_PULLUP,
                     NRF_GPIO_PIN_S0D1,
                     NRF_GPIO_PIN_NOSENSE);

        // TODO Add interrupt functionality to the FPGA_RESET_INT pin

        // Keep camera, display and FPGA in reset
        nrf_gpio_pin_write(CAMERA_RESET_PIN, false);
        nrf_gpio_pin_write(DISPLAY_RESET_PIN, false);
        nrf_gpio_pin_write(FPGA_RESET_INT_PIN, false);

        // Set the chip selects to high
        nrf_gpio_pin_write(DISPLAY_CS_PIN, true);
        nrf_gpio_pin_write(FPGA_CS_MODE_PIN, true);
        nrf_gpio_pin_write(FLASH_CS_PIN, true);
    }
}

void monocle_enter_bootloader(void)
{
    // Set the persistent memory flag telling the bootloader to go into DFU mode
    sd_power_gpregret_set(0, 0xB1);

    // Reset the CPU, giving control to the bootloader
    NVIC_SystemReset();
}

void monocle_fpga_reset(bool reboot)
{
    // CAUTION: READ DATASHEET CAREFULLY BEFORE CHANGING THESE

    if (!reboot)
    {
        power_all_rails(false);

        // Hold reset
        nrf_gpio_pin_write(FPGA_RESET_INT_PIN, false);
        nrfx_systick_delay_ms(25);

        power_all_rails(true);

        return;
    }

    power_all_rails(true);

    // Check flash for a valid FPGA image
    uint8_t magic_word[4] = "";
    monocle_flash_read(magic_word, 0x6C80E, sizeof(magic_word));

    // Set the FPGA MODE1 pin accordingly
    if (memcmp(magic_word, "done", sizeof(magic_word)) == 0)
    {
        NRFX_LOG("Booting FPGA from SPI flash");
        nrf_gpio_pin_write(FPGA_CS_MODE_PIN, true);
    }
    else
    {
        NRFX_LOG("Booting FPGA from internal flash");
        nrf_gpio_pin_write(FPGA_CS_MODE_PIN, false);
    }

    // Boot
    monocle_spi_enable(false);
    nrf_gpio_pin_write(FPGA_RESET_INT_PIN, true);
    nrfx_systick_delay_ms(200); // Should boot within 142ms @ 25MHz
    monocle_spi_enable(true);

    // Release the mode pin so it can be used as chip select
    nrf_gpio_pin_write(FPGA_CS_MODE_PIN, true);
}
