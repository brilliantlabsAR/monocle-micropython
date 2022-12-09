/*
 * Copyright (2022), Brilliant Labs Limited (Hong Kong)
 * Licensed under the MIT License
 */

#ifndef MONOCLE_TIMER_H
#define MONOCLE_TIMER_H

#include <stdint.h>
#include <stdbool.h>

#include "monocle_config.h"

/**
 * Wrapper around NRFX timers for sharing a single periodic timer.
 * @defgroup timer
 */

typedef void timer_handler_t(void);

void timer_init(void);
void timer_add_handler(timer_handler_t *handler);
void timer_del_handler(timer_handler_t *handler);

#endif
