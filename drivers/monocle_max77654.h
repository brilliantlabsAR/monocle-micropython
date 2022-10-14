/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

// Code written from scratch based on datasheet

#ifndef MAX77654_H
#define MAX77654_H

#include <stdbool.h>
#include <stdint.h>

#define MAX77654_LOG_INFO_ON
//#define MAX77654_LOG_DEBUG_ON

/**
 * Driver for the MAX77654 battery charge controller (PMIC).
 * https://datasheets.maximintegrated.com/en/ds/MAX77654.pdf
 * @defgroup max77654
 * @ingroup driver_chip
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
void max77654_rail_1v8sw_on(bool on);
void max77654_rail_2v7_on(bool on);
void max77654_rail_1v2_on(bool on);
void max77654_rail_10v_on(bool on);
void max77654_rail_vled_on(bool on);
void max77654_led_red_on(bool on);
void max77654_led_green_on(bool on);
max77654_status max77654_charging_status(void);
max77654_fault max77654_faults_status(void);
void max77654_set_charge_current(uint16_t current);
void max77654_set_charge_voltage(uint16_t voltage);
void max77654_set_current_limit(uint16_t current);
void max77564_factory_ship_mode(void);

/** @} */
#endif
