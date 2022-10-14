/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>
#include "nrf_saadc.h"

// Debug Logging for this module
//#define BATTERY_LOG_INFO_ON
//#define BATTERY_LOG_DEBUG_ON

/**
 * ADC driver for sensing battery voltage.
 * battery_start_sample() must be called periodically within the main application
 * every 5 samples will be averaged, and the battery voltage & SoC updated.
 * @defgroup adc
 * @ingroup driver_nrf52
 * @{
 */
void battery_init(void);
void battery_start_sample(void);
float battery_get_voltage(void);
uint8_t battery_get_percent(void);
/** @} */

#endif
