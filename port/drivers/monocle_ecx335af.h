/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */
#ifndef ECX335AF_H
#define ECX335AF_H
#include <stdint.h>
#include <stdbool.h>
/**
 * Driver for the SPI Sony OLED Microdisplay.
 * No public datasheet seem to be published on the web.
 * The data path is connected directly to the data path to FPGA, the
 * MCU only has access to the SPI configuration interface.
 * After the OLED configuration is done, the luminance (brightness)
 * should be set and the display is ready to receive data.
 * @defgroup ecx335af
 * @{
 */

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
void ecx335af_set_luminance(ecx335af_luminance_t level);
void ecx335af_sleep(void);
void ecx335af_awake(void);

/** @} */
#endif
