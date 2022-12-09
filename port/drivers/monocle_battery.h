/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Copyright (c) 2022 Raj Nakarja - Silicon Witchery AB
 * Licensed under the MIT License
 */

#ifndef MONOCLE_BATTERY_H
#define MONOCLE_BATTERY_H

#include <stdint.h>
#include "nrf_saadc.h"

/**
 * ADC driver for sensing battery voltage.
 * battery_start_sample() must be called periodically within the main application
 * every 5 samples will be averaged, and the battery voltage and SoC updated.
 * @defgroup battery
 */

void battery_init(void);
uint8_t battery_get_percent(void);

#endif
