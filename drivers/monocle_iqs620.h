/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */
#ifndef IQS620_H
#define IQS620_H
#include <stdint.h>
#include <stdbool.h>
#include "nrfx_twi.h"
/**
 * Driver for the IQS620 hall effect, proximity sensor.
 * https://www.azoteq.com/images/stories/pdf/iqs620_datasheet.pdf
 *
 * The interrupt line indicates that there is an event pending, that the MCU queries over I²C.
 *
 * There are 2 touch buttons on the Monocle, each triggering these events:
 *
 * - TOUCH_GESTURE_TAP        : 0.25s
 * - TOUCH_GESTURE_DOUBLETAP  : 0.25s (Both Tapped)
 * - TOUCH_GESTURE_PRESS      : 0.5s
 * - TOUCH_GESTURE_LONGPRESS  : 9.5s
 * - TOUCH_GESTURE_LONGBOTH   : 9.5s (Both Pressed)
 * 
 * @defgroup IQS620
 * @{
 */

typedef unsigned pin_t;

typedef enum {
    IQS620_BUTTON_B0,
    IQS620_BUTTON_B1
} iqs620_button_t;

typedef enum {
    IQS620_BUTTON_UP,
    IQS620_BUTTON_PROX,
    IQS620_BUTTON_DOWN,
} iqs620_event_t;

typedef void (*iqs620_callback_t)(iqs620_button_t button, iqs620_event_t event);

void iqs620_init(void);
void iqs620_reset(void);
void iqs620_callback(iqs620_button_t, iqs620_event_t);
uint32_t iqs620_get_id(void);
uint16_t iqs620_get_button_status(void); // added to mimic cy8cmbr3 interfece
uint16_t iqs620_get_count(uint8_t channel);

/** @} */
#endif

