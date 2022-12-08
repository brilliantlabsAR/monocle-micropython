/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

/**
 * User interface for capacitove touch sensors.
 * @file monocle_touch.c
 * @author Nathan Ashelman
 * @author Shreyas Hemachandra
 */

#include <stdint.h>
#include <stdbool.h>
#include "monocle_touch.h"
#include "monocle_iqs620.h"
#include "monocle_i2c.h"
#include "monocle_config.h"
#include "nrfx_timer.h"
#include "nrfx_log.h"

#define LOG(...) NRFX_LOG_ERROR(__VA_ARGS__)
#define ASSERT NRFX_ASSERT

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

static void touch_timer_handler(nrf_timer_event_t event, void *ctx);

/**
 * Workaround the fact that nordic returns an ENUM instead of a simple integer.
 */
static inline void check(char const *func, nrfx_err_t err)
{
    if (err != NRFX_SUCCESS)
        LOG("%s: %s", func, NRFX_LOG_ERROR_STRING_GET(err));
}

// 0 is reserved for SoftDevice, 1 does not work for an unknown reason
nrfx_timer_t touch_timer = NRFX_TIMER_INSTANCE(2);
uint32_t touch_timer_ticks;
nrfx_timer_config_t touch_timer_config = NRFX_TIMER_DEFAULT_CONFIG;
touch_event_t touch_timer_event;

static void touch_set_timer(touch_event_t event)
{
    uint32_t err;
    static bool init = false;

    // Choose the apropriate duration depending on the event triggered.
    switch (event)
    {
        case TOUCH_EVENT_LONG:
            // No timer to configure.
            nrfx_timer_disable(&touch_timer);
            return;
        case TOUCH_EVENT_SHORT:
            // After a short timer, extend to a long timer.
            touch_timer_ticks = TOUCH_DELAY_LONG_MS;
            touch_timer_event = TOUCH_EVENT_LONG;
            break;
        default:
            // After a button event, setup a short timer.
            touch_timer_ticks = TOUCH_DELAY_SHORT_MS;
            touch_timer_event = TOUCH_EVENT_SHORT;
            break;
    }

    // Reset the timer if already configured.
    if (init)
        nrfx_timer_uninit(&touch_timer);

    // Prepare the configuration structure.
    touch_timer_config.mode = NRF_TIMER_MODE_TIMER;
    touch_timer_config.frequency = NRF_TIMER_FREQ_1MHz;
    touch_timer_config.bit_width = NRF_TIMER_BIT_WIDTH_8;

    // Submit the configuration.
    err = nrfx_timer_init(&touch_timer, &touch_timer_config, touch_timer_handler);
    ASSERT(err == NRFX_SUCCESS);
    init = true;

    // Do not raise an interrupt on every MHz, but on every 100 MHz.
    nrfx_timer_extended_compare(&touch_timer, NRF_TIMER_CC_CHANNEL0, 100, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

    // Start the timer.
    nrfx_timer_enable(&touch_timer);

    LOG("ready nrfx=timer dep=iqs620");
}

static void touch_next_state(touch_event_t event)
{
    // Update the state using the state machine encoded above.
    touch_state = touch_state_machine[touch_state][event];
    ASSERT(touch_state != TOUCH_STATE_INVALID);

    // Handle the multiple states.
    if (touch_trigger_is_on[touch_state])
    {
        // When there is a callback associated with the state, run it.
        touch_callback(touch_state);

        // If something was triggered, come back to the "IDLE" state.
        touch_state = TOUCH_STATE_IDLE;

        // And then disable the timer.
        nrfx_timer_disable(&touch_timer);
    }
    else if (touch_state == TOUCH_STATE_IDLE)
    {
            // Idle, do not setup a timer.
    }
    else
    {
            // Intermediate states, do not go back to TOUCH_STATE_IDLE.
            touch_set_timer(event);
    }
}

static void touch_timer_handler(nrf_timer_event_t event, void *ctx)
{ 
    (void)event;
    (void)ctx;

    // If the timer's counter reaches 0
    if (touch_timer_ticks == 0)
    {
        // Disable the timer for now.
        nrfx_timer_disable(&touch_timer);

        // Submit the event to the state machine.
        LOG("touch_timer_event=%s",
                touch_timer_event == TOUCH_EVENT_SHORT ? "SHORT" :
                touch_timer_event == TOUCH_EVENT_LONG ? "LONG" :
                "?");
        touch_next_state(touch_timer_event);
    }
    else
    {
        // Not triggering yet.
        touch_timer_ticks -= 100;
    }
}

void iqs620_callback_button_pressed(uint8_t button)
{
    LOG("button=%d", button);
    switch (button)
    {
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
    switch (button)
    {
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
