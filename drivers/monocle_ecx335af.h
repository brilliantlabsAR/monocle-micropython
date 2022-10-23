#ifndef ECX335AF_H
#define ECX335AF_H
/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 **
 * Driver for the Sony OLED Microdisplay.
 * No public datasheet encountered, ask the provider.
 * @defgroup oled
 * @ingroup driver_chip
 * @{
 */

#include <stdint.h>
#include <stdbool.h>

/** See ECX336CN datasheet section 10.8; luminance values are in cd/m^2 */
typedef enum {
    ECX335AF_DIM     = 1,    // 750  cd/m2
    ECX335AF_LOW     = 2,    // 1250 cd/m2
    ECX335AF_MEDIUM  = 0,    // 2000 cd/m2, this is the default
    ECX335AF_DEFAULT = 0,
    ECX335AF_HIGH    = 3,    // 3000 cd/m2
    ECX335AF_BRIGHT  = 4,    // 4000 cd/m2
} ecx335af_luminance_t;

void ecx335af_prepare(void);
void ecx335af_init(void);
void ecx335af_deinit(void);
bool ecx335af_verify(void);
void ecx335af_set_luminance(ecx335af_luminance_t level);
void ecx335af_sleep(void);
void ecx335af_awake(void);

/** @} */
#endif
