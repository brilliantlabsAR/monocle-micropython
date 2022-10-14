/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#ifndef OLED_H
#define OLED_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Driver for the Sony OLED Microdisplay.
 * No public datasheet encountered, ask the provider.
 * @defgroup oled
 * @ingroup driver_chip
 * @{
 */

/** See ECX336CN datasheet section 10.8; luminance values are in cd/m^2 */
enum oled_luminance_t {
    OLED_DIM     = 1,    //  750 cd/m2
    OLED_LOW     = 2,    // 1250 cd/m2
    OLED_MEDIUM  = 0,    // 2000 cd/m2, this is the default
    OLED_DEFAULT = 0,
    OLED_HIGH    = 3,    // 3000 cd/m2
    OLED_BRIGHT  = 4     // 4000 cd/m2
};

void oled_config(void); // known to work, uses single-byte writes
void oled_config_burst(void); // faster, but validation failed (doesn't seem to work)
bool oled_verify_config(void);
bool oled_verify_config_full(void);
void oled_set_luminance(enum oled_luminance_t level);
void oled_pwr_sleep(void);
void oled_pwr_wake(void);

// for unit testing
bool oled_spi_exercise_register(void);

/** @} */
#endif
