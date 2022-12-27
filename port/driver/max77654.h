/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Nathan Ashelman <nathan@itsbrilliant.co>
 * Authored by: Josuah Demangeon <me@josuah.net>
 *
 * ISC Licence
 *
 * Copyright © 2022 Brilliant Labs Inc.
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
 * Driver for the MAX77654 I²C driven battery charge controller (PMIC).
 * https://datasheets.maximintegrated.com/en/ds/MAX77654.pdf
 * There are at least 5 Different voltage outputs.
 * Main Power Rail:
 * - 1.8V Power Rail: MCU and the Touch IC, Always on.
 * Auxillary Power Rails, tured ON by MCU in a specific routine, in driver/board.c.
 * - 1.2V Power Rail
 * - 2.7V Power Rail
 * - 10V Power Rail
 * - LED Power Rail: Can be tured OFF.
 * @bug Right now a little underpowered: there is a 18 sec delay after turning the power for every module.
 * @bug Occassionally boot failure is also noticed right now.
 */

/** return values of max77654_charging_status() */
typedef enum MAX77654_charging_status {
    MAX77654_READ_ERROR = -1,///< I2C error on read of status register.
    MAX77654_READY = 0,      ///< Ready.
    MAX77654_CHARGING,       ///< Charging.
    MAX77654_CHARGE_DONE,    ///< Charge done.
    MAX77654_FAULT,          ///< Fault.
} max77654_status;

typedef enum MAX77654_charge_fault {
    MAX77654_NORMAL = 0,     ///< No faults.
    MAX77654_FAULT_PRE_Q,    ///< Prequalification timer fault.
    MAX77654_FAULT_TIME,     ///< Fast-charge timer fault.
    MAX77654_FAULT_TEMP,     ///< Battery temperature fault.
} max77654_fault;

void max77654_init(void);
void max77654_rail_1v2(bool on);
void max77654_rail_1v8(bool on);
void max77654_rail_2v7(bool on);
void max77654_rail_10v(bool on);
void max77654_rail_vled(bool on);
void max77654_led_red(bool on);
void max77654_led_green(bool on);
max77654_status max77654_charging_status(void);
max77654_fault max77654_faults_status(void);
void max77654_set_charge_current(uint16_t current);
void max77654_set_charge_voltage(uint16_t voltage);
void max77654_set_current_limit(uint16_t current);
void max77564_factory_ship_mode(void);
