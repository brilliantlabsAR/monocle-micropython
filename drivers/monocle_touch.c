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
#define CHECK(err) check(__func__, err)

/** Timeout for button press (ticks) = 0.5 s */
#define TOUCH_DELAY_SHORT_MS        500000

/** Timeout for long button press (ticks) = 9.5 s + PRESS_INTERVAL = 10 s */
#define TOUCH_DELAY_LONG_MS         9500000

/*
 * This state machine can distinguish between the following gestures:
 * - Tap: push & quick release
 * - Slide LR or RL +: tap on one button followed by tap on other
 * - DoubleTap: Tap, followed quickly by another Tap
 * - Press: push one for >0.5s & <10s then release
 * - LongPress: push for >10s then release
 * - LongBoth +: push both buttons for >10s then release
 *
 * Transition to new state is triggered by a timeout or push/release event.
 * Timer, of given duration, is started when entering state that has a timeout.
 * Transitions back to IDLE will generate a gesture, this happens on release
 * for most gestures, but after TAP_INTERVAL for Tap (i.e. some delay).
 */

typedef enum {
    TOUCH_STATE_INVALID,

    TOUCH_STATE_IDLE,
    TOUCH_STATE_0_ON,
    TOUCH_STATE_1_ON,
    TOUCH_STATE_0_ON_SHORT,
    TOUCH_STATE_1_ON_SHORT,
    TOUCH_STATE_BOTH_ON,
    TOUCH_STATE_BOTH_ON_SHORT,
    TOUCH_STATE_0_ON_OFF,
    TOUCH_STATE_1_ON_OFF,
    TOUCH_STATE_0_ON_OFF_1_ON,
    TOUCH_STATE_1_ON_OFF_0_ON,

    // '*' for button ON
    // ' ' for button OFF
    // 'T' for timeout

    // Button 0: [**       ] 
    // Button 1: [         ]
    TOUCH_TRIGGER_0_TAP,

    // Button 0: [         ]
    // Button 1: [**       ]
    TOUCH_TRIGGER_1_TAP,

    // Button 0: [****     ] 
    // Button 1: [         ]
    TOUCH_TRIGGER_0_PRESS,

    // Button 0: [         ]
    // Button 1: [****     ]
    TOUCH_TRIGGER_1_PRESS,

    // Button 0: [******T  ] 
    // Button 1: [         ]
    TOUCH_TRIGGER_0_LONG,

    // Button 0: [         ]
    // Button 1: [******T  ]
    TOUCH_TRIGGER_1_LONG,

    // Button 0: [***      ] or [*****    ] or [ ***     ] or [ ***     ]
    // Button 1: [ ***     ]    [ ***     ]    [***      ]    [*****    ]
    TOUCH_TRIGGER_BOTH_TAP,

    // Button 0: [*****    ] or [*******  ] or [ *****   ] or [ *****   ]
    // Button 1: [ *****   ]    [ *****   ]    [*****    ]    [*******  ]
    TOUCH_TRIGGER_BOTH_PRESS,

    // Button 0: [******T  ] or [ *****T  ]
    // Button 1: [ *****T  ]    [******T  ]
    TOUCH_TRIGGER_BOTH_LONG,

    // Button 0: [***      ]
    // Button 1: [    ***  ]
    TOUCH_TRIGGER_0_1_SLIDE,

    // Button 0: [    ***  ]
    // Button 1: [***      ]
    TOUCH_TRIGGER_1_0_SLIDE,

    TOUCH_STATE_NUM,
} touch_state_t;

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
    [TOUCH_STATE_INVALID] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_0_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_IDLE,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_1_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_STATE_IDLE,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_IDLE,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_IDLE,
    },
    [TOUCH_STATE_IDLE] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_0_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_IDLE,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_1_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_STATE_IDLE,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_IDLE,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_IDLE,
    },
    [TOUCH_STATE_0_ON] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_0_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_0_ON_OFF,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_BOTH_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_STATE_0_ON,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_0_ON_SHORT,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_INVALID,
    },
    [TOUCH_STATE_1_ON] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_BOTH_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_1_ON,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_1_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_STATE_1_ON_OFF,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_1_ON_SHORT,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_INVALID,
    },
    [TOUCH_STATE_0_ON_SHORT] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_0_ON_SHORT,
        [TOUCH_EVENT_0_OFF]     = TOUCH_TRIGGER_0_PRESS,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_BOTH_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_STATE_0_ON_SHORT,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_INVALID,
        [TOUCH_EVENT_LONG]      = TOUCH_TRIGGER_0_LONG,
    },
    [TOUCH_STATE_1_ON_SHORT] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_BOTH_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_1_ON,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_1_ON_SHORT,
        [TOUCH_EVENT_1_OFF]     = TOUCH_TRIGGER_1_PRESS,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_INVALID,
        [TOUCH_EVENT_LONG]      = TOUCH_TRIGGER_1_LONG,
    },
    [TOUCH_STATE_BOTH_ON] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_BOTH_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_TRIGGER_BOTH_TAP,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_BOTH_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_TRIGGER_BOTH_TAP,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_BOTH_ON,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_INVALID,
    },
    [TOUCH_STATE_BOTH_ON_SHORT] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_BOTH_ON_SHORT,
        [TOUCH_EVENT_0_OFF]     = TOUCH_TRIGGER_BOTH_PRESS,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_BOTH_ON_SHORT,
        [TOUCH_EVENT_1_OFF]     = TOUCH_TRIGGER_BOTH_PRESS,
        [TOUCH_EVENT_SHORT]     = TOUCH_STATE_INVALID,
        [TOUCH_EVENT_LONG]      = TOUCH_TRIGGER_BOTH_LONG,
    },
    [TOUCH_STATE_0_ON_OFF] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_0_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_0_ON_OFF,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_0_ON_OFF_1_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_STATE_0_ON_OFF,
        [TOUCH_EVENT_SHORT]     = TOUCH_TRIGGER_0_TAP,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_INVALID,
    },
    [TOUCH_STATE_1_ON_OFF] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_1_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_1_ON_OFF,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_1_ON_OFF_0_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_STATE_1_ON_OFF,
        [TOUCH_EVENT_SHORT]     = TOUCH_TRIGGER_1_TAP,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_INVALID,
    },
    [TOUCH_STATE_0_ON_OFF_1_ON] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_0_ON_OFF_1_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_0_ON_OFF_1_ON,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_0_ON_OFF_1_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_TRIGGER_0_1_SLIDE,
        [TOUCH_EVENT_SHORT]     = TOUCH_TRIGGER_0_1_SLIDE,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_INVALID,
    },
    [TOUCH_STATE_1_ON_OFF_0_ON] = {
        [TOUCH_EVENT_0_ON]      = TOUCH_STATE_1_ON_OFF_0_ON,
        [TOUCH_EVENT_0_OFF]     = TOUCH_STATE_1_ON_OFF_0_ON,
        [TOUCH_EVENT_1_ON]      = TOUCH_STATE_1_ON_OFF_0_ON,
        [TOUCH_EVENT_1_OFF]     = TOUCH_TRIGGER_1_0_SLIDE,
        [TOUCH_EVENT_SHORT]     = TOUCH_TRIGGER_1_0_SLIDE,
        [TOUCH_EVENT_LONG]      = TOUCH_STATE_INVALID,
    },
};

static void (*touch_trigger_fn[TOUCH_STATE_NUM])(void) = {
    [TOUCH_TRIGGER_0_TAP]      = touch_callback_trigger_0_tap,
    [TOUCH_TRIGGER_1_TAP]      = touch_callback_trigger_1_tap,
    [TOUCH_TRIGGER_0_PRESS]    = touch_callback_trigger_0_press,
    [TOUCH_TRIGGER_1_PRESS]    = touch_callback_trigger_1_press,
    [TOUCH_TRIGGER_0_LONG]     = touch_callback_trigger_0_long,
    [TOUCH_TRIGGER_1_LONG]     = touch_callback_trigger_1_long,
    [TOUCH_TRIGGER_BOTH_TAP]   = touch_callback_trigger_both_tap,
    [TOUCH_TRIGGER_BOTH_PRESS] = touch_callback_trigger_both_press,
    [TOUCH_TRIGGER_BOTH_LONG]  = touch_callback_trigger_both_long,
    [TOUCH_TRIGGER_0_1_SLIDE]  = touch_callback_trigger_0_1_slide,
    [TOUCH_TRIGGER_1_0_SLIDE]  = touch_callback_trigger_1_0_slide,
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
    CHECK(nrfx_timer_init(&touch_timer, &touch_timer_config, touch_timer_handler));
    init = 1;

    // Do not raise an interrupt on every MHz, but on every 100 MHz.
    nrfx_timer_extended_compare(&touch_timer, NRF_TIMER_CC_CHANNEL0, 100, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

    // Start the timer.
    nrfx_timer_enable(&touch_timer);
}

static void touch_next_state(touch_event_t event)
{
    // Update the state using the state machine encoded above.
    touch_state = touch_state_machine[touch_state][event];
    assert(touch_state != TOUCH_STATE_INVALID);

    // Handle the multiple states.
    if (touch_trigger_fn[touch_state] != NULL)
    {
        // When there is a callback associated with the state, run it.
        touch_trigger_fn[touch_state]();

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

void touch_callback_trigger_0_tap(void)
{
    LOG("");
}

void touch_callback_trigger_1_tap(void)
{
    LOG("");
}

void touch_callback_trigger_0_press(void)
{
    LOG("");
}

void touch_callback_trigger_1_press(void)
{
    LOG("");
}

void touch_callback_trigger_0_long(void)
{
    LOG("");
}

void touch_callback_trigger_1_long(void)
{
    LOG("");
}

void touch_callback_trigger_both_tap(void)
{
    LOG("");
}

void touch_callback_trigger_both_press(void)
{
    LOG("");
}

void touch_callback_trigger_both_long(void)
{
    LOG("");
}

void touch_callback_trigger_0_1_slide(void)
{
    LOG("");
}

void touch_callback_trigger_1_0_slide(void)
{
    LOG("");
}
