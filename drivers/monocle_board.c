/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#include "monocle_board.h"
#include "nrfx_systick.h"
#include <stdint.h>
#include <stdbool.h>
#include "monocle_max77654.h"
#include "monocle_config.h"

void board_pin_off(uint8_t pin)
{
    nrf_gpio_pin_write(pin, 0);
}

void board_pin_on(uint8_t pin)
{
    nrf_gpio_pin_write(pin, 1);
}

void board_aux_power_on(void)
{
    // Advised not to pause the debugger in this function to preserve
    // power-on timing requirements

    // FPGA requires VCC(1.2V) before VCCX(2.7V) or VCCO(1.8V).
    max77654_rail_1v2_on(true);

    // Camera requires 1.8V before 2.7V.
    max77654_rail_1v8sw_on(true);

    // 1.8V ramp rate is slower than 2.7V; without this, 1.8V reaches
    // target voltage 0.2s _after_ 2.7V
    nrfx_systick_delay_ms(1);
    max77654_rail_2v7_on(true);

    // Rise time measured as 1.2ms; give FPGA time to boot, while MODE1
    // is still held low.
    nrfx_systick_delay_ms(2);

    max77654_rail_10v_on(true);
}

void board_aux_power_off(void)
{
    max77654_rail_vled_on(false);
    max77654_rail_10v_on(false);
    max77654_rail_2v7_on(false);
    max77654_rail_1v8sw_on(false);
    max77654_rail_1v2_on(false);
}

/**
 * Initializ the BSP handling for the board.
 * Configure output pins and set to initial state.
 * @param init_flags Flags specify what to initialize (LEDs/buttons).
 *  See @ref BSP_BOARD_INIT_FLAGS.
 */
void board_init(void)
{
    // Set to 0V = hold camera in reset.
    nrf_gpio_pin_write(IO_N_CAM_RESET, 0);
    nrf_gpio_cfg_output(IO_N_CAM_RESET);

    // Set to 0V = not asserted.
    nrf_gpio_pin_write(IO_CAM_PWDN, 0);
    nrf_gpio_cfg_output(IO_CAM_PWDN);

    // Set to 0V on boot (datasheet p.11)
    nrf_gpio_pin_write(IO_DISP_XCLR, 0);
    nrf_gpio_cfg_output(IO_DISP_XCLR);

    // spi_fpga_CS = MODE1, for now set LOW for AUTO BOOT
    // from FPGA internal flash
    nrf_gpio_pin_clear(SPIM0_SS1_PIN);
    nrf_gpio_cfg_output(SPIM0_SS1_PIN);
}

void board_uninit(void)
{
    // return all pins configured by board_init() to default (input/hi-Z)
    nrf_gpio_cfg_default(IO_N_CAM_RESET);
    nrf_gpio_cfg_default(IO_CAM_PWDN);
    nrf_gpio_cfg_default(IO_DISP_XCLR);
    nrf_gpio_cfg_default(SPIM0_SS1_PIN);
}
