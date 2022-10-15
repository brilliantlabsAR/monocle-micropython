/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#ifndef ECX335AF_H
#define ECX335AF_H

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
enum ecx335af_luminance_t {
    ECX335AF_DIM     = 1,    //  750 cd/m2
    ECX335AF_LOW     = 2,    // 1250 cd/m2
    ECX335AF_MEDIUM  = 0,    // 2000 cd/m2, this is the default
    ECX335AF_DEFAULT = 0,
    ECX335AF_HIGH    = 3,    // 3000 cd/m2
    ECX335AF_BRIGHT  = 4     // 4000 cd/m2
};

void ecx335af_config(void); // known to work, uses single-byte writes
void ecx335af_config_burst(void); // faster, but validation failed (doesn't seem to work)
bool ecx335af_verify_config(void);
bool ecx335af_verify_config_full(void);
void ecx335af_set_luminance(enum ecx335af_luminance_t level);
void ecx335af_pwr_sleep(void);
void ecx335af_pwr_wake(void);

// for unit testing
bool ecx335af_spi_exercise_register(void);

/** @} */
#endif
