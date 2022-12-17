/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Nathan Ashelman <nathan@itsbrilliant.co>
 * Authored by: Shreyas Hemachandra <shreyas.hemachandran@gmail.com>
 * Authored by: Josuah Demangeon <me@josuah.net>
 *
 * ISC Licence
 *
 * Copyright © 2022 Brilliant Labs Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * User interface for capacitove touch sensors.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nrfx_log.h"
#include "nrfx_twi.h"

#include "driver_touch.h"
#include "driver_timer.h"
#include "driver_iqs620.h"
#include "driver_i2c.h"
#include "driver_config.h"

#define LOG     NRFX_LOG
#define ASSERT  NRFX_ASSERT

/** Timeout for button press (ticks) = 0.5 s */
#define TOUCH_DELAY_SHORT_MS        500000

/** Timeout for long button press (ticks) = 9.5 s + PRESS_INTERVAL = 10 s */
#define TOUCH_DELAY_LONG_MS         9500000

/*
 * This state machine can distinguish between the various gestures.
 * Transition to new state is triggered by a timeout or push/release event.
 * Timer of various duration is started when entering state that has a timeout.
 * Trigger states will reset the state back to IDLE. This happens on release
 * for most gestures, but after TAP_INTERVAL for Tap (i.e. some delay).
 */

typedef enum {
    TOUCH_EVENT_0_ON,
    TOUCH_EVENT_0_OFF,
    TOUCH_EVENT_1_ON,
    TOUCH_EVENT_1_OFF,
    TOUCH_EVENT_SHORT,  // timer triggered after a short delay
    TOUCH_EVENT_LONG,   // timer triggered after a longer delay
    TOUCH_EVENT_NUM
} touch_event_t;

touch_state_t touch_state = TOUCH_STATE_IDLE;
const touch_state_t touch_state_machine[TOUCH_STATE_NUM][TOUCH_EVENT_NUM] = {
    // When asserts are off, go back to IDLE state on every event.
    [TOUCH_STATE_INVALID] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_IDLE,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_IDLE,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_IDLE,
        [TOUCH_EVENT_1_OFF]     = TOUCH_STATE_IDLE,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_IDLE,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_IDLE,
    },
    // Starting point, also set after a TOUCH_TRIGGER_* event.
    [TOUCH_STATE_IDLE] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_0_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_IDLE,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_1_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_STATE_IDLE,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_IDLE,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_IDLE,
    },
    // Touched button 0.
    [TOUCH_STATE_0_ON] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_0_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_0_ON_OFF,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_BOTH_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_STATE_0_ON,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_0_ON_SHORT,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_INVALID,
    },
    // Touched button 1.
    [TOUCH_STATE_1_ON] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_BOTH_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_1_ON,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_1_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_STATE_1_ON_OFF,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_1_ON_SHORT,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_INVALID,
    },
    // Touched button 0 and maintained for a short time.
    [TOUCH_STATE_0_ON_SHORT] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_0_ON_SHORT,
        [TOUCH_EVENT_0_OFF]     = TOUCH_TRIGGER_0_PRESS,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_BOTH_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_STATE_0_ON_SHORT,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_INVALID,
        [TOUCH_EVENT_LONG]      = TOUCH_TRIGGER_0_LONG,
    },
    // Touched button 1 and maintained for a short time.
    [TOUCH_STATE_1_ON_SHORT] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_BOTH_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_1_ON,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_1_ON_SHORT,
        [TOUCH_EVENT_1_OFF]     = TOUCH_TRIGGER_1_PRESS,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_INVALID,
        [TOUCH_EVENT_LONG]      = TOUCH_TRIGGER_1_LONG,
    },
    // Touched both buttons.
    [TOUCH_STATE_BOTH_ON] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_BOTH_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_TRIGGER_BOTH_TAP,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_BOTH_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_TRIGGER_BOTH_TAP,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_BOTH_ON,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_INVALID,
    },
    // Touched both buttons and maintained for a short time.
    [TOUCH_STATE_BOTH_ON_SHORT] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_BOTH_ON_SHORT,
        [TOUCH_EVENT_0_OFF]     = TOUCH_TRIGGER_BOTH_PRESS,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_BOTH_ON_SHORT,
        [TOUCH_EVENT_1_OFF]     = TOUCH_TRIGGER_BOTH_PRESS,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_INVALID,
        [TOUCH_EVENT_LONG]      = TOUCH_TRIGGER_BOTH_LONG,
    },
    // Touched then released button 0.
    [TOUCH_STATE_0_ON_OFF] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_0_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_0_ON_OFF,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_0_ON_OFF_1_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_STATE_0_ON_OFF,
        [TOUCH_EVENT_SHORT]     = TOUCH_TRIGGER_0_TAP,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_INVALID,
    },
    // Touched then released button 1.
    [TOUCH_STATE_1_ON_OFF] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_1_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_1_ON_OFF,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_1_ON_OFF_0_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_STATE_1_ON_OFF,
        [TOUCH_EVENT_SHORT]     = TOUCH_TRIGGER_1_TAP,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_INVALID,
    },
    // Touched then released button 0, then touched button 1.
    [TOUCH_STATE_0_ON_OFF_1_ON] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_0_ON_OFF_1_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_0_ON_OFF_1_ON,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_0_ON_OFF_1_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_TRIGGER_0_1_SLIDE,
        [TOUCH_EVENT_SHORT]     = TOUCH_TRIGGER_0_1_SLIDE,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_INVALID,
    },
    // Touched then released button 1, then touched button 0.
    [TOUCH_STATE_1_ON_OFF_0_ON] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_1_ON_OFF_0_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_1_ON_OFF_0_ON,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_1_ON_OFF_0_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_TRIGGER_1_0_SLIDE,
        [TOUCH_EVENT_SHORT]     = TOUCH_TRIGGER_1_0_SLIDE,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_INVALID,
    },
};

static bool touch_trigger_is_on[TOUCH_STATE_NUM] = {
    // Push and quick release.
    [TOUCH_TRIGGER_0_TAP]      = true,
    [TOUCH_TRIGGER_1_TAP]      = true,
    [TOUCH_TRIGGER_BOTH_TAP]   = true,
    // Push one for >0.5s and <10s then release.
    [TOUCH_TRIGGER_0_PRESS]    = true,
    [TOUCH_TRIGGER_1_PRESS]    = true,
    [TOUCH_TRIGGER_BOTH_PRESS] = true,
    // Push for >10s then release.
    [TOUCH_TRIGGER_0_LONG]     = true,
    [TOUCH_TRIGGER_1_LONG]     = true,
    [TOUCH_TRIGGER_BOTH_LONG]  = true,
    // Tap on one button followed by tap on other.
    [TOUCH_TRIGGER_0_1_SLIDE]  = true,
    [TOUCH_TRIGGER_1_0_SLIDE]  = true,
    // Tap, followed quickly by another Tap.
    // TODO
};

static void touch_timer_handler(void);

uint32_t touch_timer_ticks;
touch_event_t touch_timer_event;

static void touch_set_timer(touch_event_t event)
{

    // Choose the apropriate duration depending on the event triggered.
    switch (event) {
        case TOUCH_EVENT_LONG:
            LOG("TOUCH_EVENT_LONG");
            // No timer to configure.
            timer_del_handler(&touch_timer_handler);
            return;
        case TOUCH_EVENT_SHORT:
            LOG("TOUCH_EVENT_SHORT");
            // After a short timer, extend to a long timer.
            touch_timer_ticks = TOUCH_DELAY_LONG_MS;
            touch_timer_event = TOUCH_EVENT_LONG;
            break;
        default:
            LOG("default event");
            // After a button event, setup a short timer.
            touch_timer_ticks = TOUCH_DELAY_SHORT_MS;
            touch_timer_event = TOUCH_EVENT_SHORT;
            break;
    }

    // Submit the configuration.
    LOG("timer_add_handler");
    timer_add_handler(&touch_timer_handler);
}

static void touch_next_state(touch_event_t event)
{
    // Update the state using the state machine encoded above.
    touch_state = touch_state_machine[touch_state][event];
    ASSERT(touch_state != TOUCH_STATE_INVALID);

    // Handle the multiple states.
    if (touch_trigger_is_on[touch_state]) {
        // When there is a handler associated with the state, run it.
        touch_callback(touch_state);

        // If something was triggered, come back to the "IDLE" state.
        touch_state = TOUCH_STATE_IDLE;

        // And then disable the timer.
        timer_del_handler(&touch_timer_handler);
    } else if (touch_state == TOUCH_STATE_IDLE) {
            // Idle, do not setup a timer.
    } else {
            // Intermediate states, do not go back to TOUCH_STATE_IDLE.
            touch_set_timer(event);
    }
}

static void touch_timer_handler(void)
{ 
    // If the timer's counter reaches 0
    if (touch_timer_ticks == 0) {
        // Disable the timer for now.
        timer_del_handler(&touch_timer_handler);

        // Submit the event to the state machine.
        LOG("touch_timer_event=%s",
                touch_timer_event == TOUCH_EVENT_SHORT ? "SHORT" :
                touch_timer_event == TOUCH_EVENT_LONG ? "LONG" :
                "?");
        touch_next_state(touch_timer_event);
    } else {
        // Not triggering yet.
        touch_timer_ticks -= 100;
    }
}

void iqs620_callback_button_pressed(uint8_t button)
{
    LOG("button=%d", button);
    switch (button) {
        case 0:
            touch_next_state(TOUCH_EVENT_0_ON);
            break;
        case 1:
            touch_next_state(TOUCH_EVENT_1_ON);
            break;
    }
}

void iqs620_callback_button_released(uint8_t button)
{
    LOG("button=%d", button);
    switch (button) {
        case 0:
            touch_next_state(TOUCH_EVENT_0_OFF);
            break;
        case 1:
            touch_next_state(TOUCH_EVENT_1_OFF);
            break;
    }
}

__attribute__((weak))
void touch_callback(touch_state_t trigger)
{
    LOG("trigger=%d", trigger);
}
