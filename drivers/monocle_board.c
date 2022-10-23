/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

/**
 * General board setups.
 * @file monocle_board.c
 * @author Nathan Ashelman
 */

#include "monocle_board.h"
#include "monocle_battery.h"
#include "monocle_ble.h"
#include "monocle_ecx335af.h"
#include "monocle_fpga.h"
#include "monocle_i2c.h"
#include "monocle_iqs620.h"
#include "monocle_max77654.h"
#include "monocle_ov5640.h"
#include "monocle_spi.h"
#include "monocle_touch.h"
#include "nrfx_systick.h"
#include "nrfx_gpiote.h"
#include <stdbool.h>

void board_power_rails_on(void)
{
    // Advised not to pause the debugger in this function to preserve
    // power-on timing requirements

    // FPGA requires VCC(1.2V) before VCCX(2.7V) or VCCO(1.8V).
    max77654_rail_1v2(true);

    // Camera requires 1.8V before 2.7V.
    max77654_rail_1v8sw(true);

    // 1.8V ramp rate is slower than 2.7V; without this, 1.8V reaches
    // target voltage 0.2s _after_ 2.7V
    nrfx_systick_delay_ms(1);
    max77654_rail_2v7(true);

    // Rise time measured as 1.2ms; give FPGA time to boot, while MODE1 is still held low.
    nrfx_systick_delay_ms(2);

    // Used by the display.
    max77654_rail_10v(true);

    // Used by the red and green LEDs.
    max77654_rail_vled(true);
}

void board_power_rails_off(void)
{
    max77654_rail_vled(false);
    max77654_rail_10v(false);
    max77654_rail_2v7(false);
    max77654_rail_1v8sw(false);
    max77654_rail_1v2(false);
}

/**
 * Initialises the hardware drivers and IO.
 */
void board_init(void)
{
    // Initialise SysTick ARM timer driver for nrfx_systick_delay_ms().
    nrfx_systick_init();

    // Initialise the GPIO driver used below.
    nrfx_gpiote_init(NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);

    // Custom wrapper around I2C used by the other drivers.
    i2c_init();

    // I2C-controlled PMIC, also controlling the red/green LEDs over I2C
    max77654_init();

    // I2C calls to make sure all power rails are turned off.
    board_power_rails_on();

    // Initialise GPIO before the chips are powered on.
    ecx335af_prepare();
    fpga_prepare();
    ov5640_prepare();

    // I2C calls to setup power rails of the MAX77654.
    board_power_rails_on();

    // Custom wrapper around SPI used by the other drivers.
    spi_init();

    // Initialise the FPGA: providing the clock for the display and screen.
    fpga_init();

    // Enable the XCLK signal used by the Camera module.
    fpga_xclk_on();

    // Initialise the Display: gpio pins startup sequence then I2C.
    ov5640_pwr_on();

    // Initialise the Capacitive Touch Button controller over I2C.
    //iqs620_init();
}

void board_deinit(void)
{
    // Call deinit() hook of each driver.
    ecx335af_deinit();
    fpga_deinit();
    ov5640_deinit();

    // Turn the power rails off before turning off.
}
