/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#ifndef DRIVER_IQS620_H
#define DRIVER_IQS620_H

#include <stdint.h>
#include <stdbool.h>

#include "nrfx_twi.h"

/**
 * Driver for the IQS620 hall effect, proximity sensor.
 * https://www.azoteq.com/images/stories/pdf/iqs620_datasheet.pdf
 * The interrupt line indicates that there is an event pending, that the MCU queries over I²C.
 * There are 2 touch buttons on the Monocle, each triggering these events:
 * - TOUCH_GESTURE_TAP        : 0.25s
 * - TOUCH_GESTURE_DOUBLETAP  : 0.25s (Both Tapped)
 * - TOUCH_GESTURE_PRESS      : 0.5s
 * - TOUCH_GESTURE_LONGPRESS  : 9.5s
 * - TOUCH_GESTURE_LONGBOTH   : 9.5s (Both Pressed)
 * @defgroup iqs620
 */

void iqs620_init(void);
void iqs620_reset(void);
void iqs620_callback_button_pressed(uint8_t button);
void iqs620_callback_button_released(uint8_t button);
uint32_t iqs620_get_id(void);
uint16_t iqs620_get_count(uint8_t channel);

#endif
