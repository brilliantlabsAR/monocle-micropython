/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */
#ifndef MAX77654_H
#define MAX77654_H
#include <stdbool.h>
#include <stdint.h>
/**
 * Driver for the MAX77654 I²C driven battery charge controller (PMIC).
 * https://datasheets.maximintegrated.com/en/ds/MAX77654.pdf
 * There are at least 5 Different voltage outputs.
 * Main Power Rail:
 * - 1.8V Power Rail: MCU and the Touch IC, Always on.
 * Auxillary Power Rails, tured ON by MCU in a specific routine, in monocle_board.c.
 * - 1.2V Power Rail
 * - 2.7V Power Rail
 * - 10V Power Rail
 * - LED Power Rail: Can be tured OFF.
 * @bug Right now a little underpowered: there is a 18 sec delay after turning the power for every module.
 * @bug Occassionally boot failure is also noticed right now.
 * @defgroup MAX77654
 * @{
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

/** @} */
#endif
