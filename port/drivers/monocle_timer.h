/*
 * Copyright (2022), Brilliant Labs Limited (Hong Kong)
 * Licensed under the MIT License
 */

#ifndef TOUCH_H
#define TOUCH_H

#include <stdint.h>
#include <stdbool.h>

#include "monocle_config.h"

/**
 * Wrapper around NRFX timers for sharing a single periodic timer.
 * @defgroup timer
 */

typedef void timer_callback_t(void);

extern timer_callback_t timer_callbacks[TIMER_MAX_CALLBACKS];

void timer_init(void);
void timer_add_callback(timer_callback_t cb);

#endif
