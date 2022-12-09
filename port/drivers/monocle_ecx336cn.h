/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#ifndef MONOCLE_ECX336CN_H
#define MONOCLE_ECX336CN_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Driver for the SPI Sony OLED Microdisplay.
 * No public datasheet seem to be published on the web.
 * The data path is connected directly to the data path to FPGA, the
 * MCU only has access to the SPI configuration interface.
 * After the OLED configuration is done, the luminance (brightness)
 * should be set and the display is ready to receive data.
 * @defgroup ecx336cn
 */

/** See ECX336CN datasheet section 10.8; luminance values are in cd/m^2 */
typedef enum {
    ECX336CN_DIM     = 1,    // 750  cd/m2
    ECX336CN_LOW     = 2,    // 1250 cd/m2
    ECX336CN_MEDIUM  = 0,    // 2000 cd/m2, this is the default
    ECX336CN_DEFAULT = 0,
    ECX336CN_HIGH    = 3,    // 3000 cd/m2
    ECX336CN_BRIGHT  = 4,    // 4000 cd/m2
} ecx336cn_luminance_t;

void ecx336cn_prepare(void);
void ecx336cn_init(void);
void ecx336cn_deinit(void);
void ecx336cn_set_luminance(ecx336cn_luminance_t level);
void ecx336cn_sleep(void);
void ecx336cn_awake(void);

#endif
