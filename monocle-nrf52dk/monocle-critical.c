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

#include <math.h>
#include <string.h>
#include "monocle.h"
#include "nrf_gpio.h"
#include "nrf_sdm.h"
#include "nrf_soc.h"
#include "nrfx_systick.h"
#include "nrf_power.h"
#include "nrfx_timer.h"
#include "nrfx_gpiote.h"
#include "nrfx_twim.h"
#include "nrfx_spim.h"

static const nrfx_twim_t i2c_bus = NRFX_TWIM_INSTANCE(0);

bool prevent_sleep_flag = true;
bool force_sleep_flag = false;

// Magic number which doesn't interfere with the bootloader flag bits
static const uint32_t safe_mode_flag = 0x06;

static void check_if_battery_charging_and_sleep(nrf_timer_event_t event_type,
                                                void *p_context)
{
    // TODO: use a hardware button for this
    (void)event_type;
    (void)p_context;
}


static void touch_a_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    (void)pin;
    (void)action;
    touch_event_handler(TOUCH_A);
}

static void touch_b_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    (void)pin;
    (void)action;
    touch_event_handler(TOUCH_B);
}

void monocle_critical_startup(void)
{
    // Enable the the DC/DC convertor
    NRF_POWER->DCDCEN = 1;

    // Enable systick timer functions
    nrfx_systick_init();

    // Set up the I2C buses
    {
        nrfx_twim_config_t bus_config = NRFX_TWIM_DEFAULT_CONFIG(
            I2C_SCL_PIN,
            I2C_SDA_PIN);

        bus_config.frequency = NRF_TWIM_FREQ_100K;

        app_err(nrfx_twim_init(&i2c_bus, &bus_config, NULL, NULL));

        nrfx_twim_enable(&i2c_bus);
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
        nrf_gpio_cfg_input(BUTTON_1_PIN, NRF_GPIO_PIN_PULLUP);
        nrf_gpio_cfg_input(BUTTON_2_PIN, NRF_GPIO_PIN_PULLUP);
        nrf_gpio_cfg_input(BUTTON_3_PIN, NRF_GPIO_PIN_PULLUP);
        nrf_gpio_cfg_input(BUTTON_4_PIN, NRF_GPIO_PIN_PULLUP);
        nrf_gpio_cfg_output(LED_1_PIN);
        nrf_gpio_cfg_output(LED_2_PIN);
        nrf_gpio_cfg_output(LED_3_PIN);
        nrf_gpio_cfg_output(LED_4_PIN);
        nrf_gpio_pin_write(LED_1_PIN, !false);
        nrf_gpio_pin_write(LED_2_PIN, !false);
        nrf_gpio_pin_write(LED_3_PIN, !false);
        nrf_gpio_pin_write(LED_4_PIN, !false);
    }
}

void monocle_enter_bootloader(void)
{
    // Set the persistent memory flag telling the bootloader to go into DFU mode
    app_err(sd_power_gpregret_set(0, 0xB1));

    // Resets the CPU, giving control to the bootloader
    NVIC_SystemReset();
}

void monocle_enter_safe_mode(void)
{
    // Set the persistent memory flag for safe mode
    app_err(sd_power_gpregret_set(0, safe_mode_flag));

    NVIC_SystemReset();
}

bool monocle_started_in_safe_mode(void)
{
    uint32_t register_value;
    app_err(sd_power_gpregret_get(0, &register_value));

    // Clear magic number once read
    app_err(sd_power_gpregret_clr(0, safe_mode_flag));

    // Extra configuration that needed to be done after nrfx config on main()
    nrfx_gpiote_in_config_t config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
    config.pull = NRF_GPIO_PIN_PULLUP;
    app_err(nrfx_gpiote_in_init(BUTTON_1_PIN, &config, &touch_a_handler));
    app_err(nrfx_gpiote_in_init(BUTTON_2_PIN, &config, &touch_b_handler));
    nrfx_gpiote_in_event_enable(BUTTON_1_PIN, true);
    nrfx_gpiote_in_event_enable(BUTTON_2_PIN, true);

    return register_value & safe_mode_flag;
}

void monocle_fpga_reset(bool reboot)
{
    (void)reboot;
}
