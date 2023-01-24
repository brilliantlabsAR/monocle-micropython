/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
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


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "py/obj.h"
#include "py/qstr.h"
#include "py/runtime.h"

#include "nrfx_log.h"
#include "nrfx_twi.h"

#include "driver/config.h"
#include "driver/i2c.h"
#include "driver/iqs620.h"
#include "driver/timer.h"

#define ASSERT  NRFX_ASSERT
#define LEN(x) (sizeof(x) / sizeof*(x))

enum {
    TOUCH_ACTION_LONG,
    TOUCH_ACTION_PRESS,
    TOUCH_ACTION_SLIDE,
    TOUCH_ACTION_TAP,

    TOUCH_ACTION_NUM,
};

/** Number of touch buttons supported by that board */
#define TOUCH_BUTTON_NUM 2

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

static void touch_timer_task(void);

uint32_t touch_timer_ticks;
touch_event_t touch_timer_event;

static void touch_set_timer(touch_event_t event)
{
    // Choose the apropriate duration depending on the event triggered.
    switch (event)
    {

    case TOUCH_EVENT_LONG:
    {
        LOG("TOUCH_EVENT_LONG");
        // No timer to configure.
        timer_del_task(timer_1ms, touch_timer_task);
        return;
    }

    case TOUCH_EVENT_SHORT:
    {
        LOG("TOUCH_EVENT_SHORT");
        // After a short timer, extend to a long timer.
        touch_timer_ticks = TOUCH_DELAY_LONG_MS;
        touch_timer_event = TOUCH_EVENT_LONG;
        break;
    }

    default:
    {
        LOG("default event");
        // After a button event, setup a short timer.
        touch_timer_ticks = TOUCH_DELAY_SHORT_MS;
        touch_timer_event = TOUCH_EVENT_SHORT;
        break;
    }

    }

    // Submit the configuration.
    LOG("timer_add_task");
    timer_add_task(timer_1ms, touch_timer_task);
}

void touch_callback(touch_state_t trigger);

static void touch_next_state(touch_event_t event)
{
    // Update the state using the state machine encoded above.
    touch_state = touch_state_machine[touch_state][event];
    ASSERT(touch_state != TOUCH_STATE_INVALID);

    // Handle the multiple states.
    if (touch_trigger_is_on[touch_state])
    {
        // When there is a task associated with the state, run it.
        touch_callback(touch_state);

        // If something was triggered, come back to the "IDLE" state.
        touch_state = TOUCH_STATE_IDLE;

        // And then disable the timer.
        timer_del_task(timer_1ms, touch_timer_task);
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

static void touch_timer_task(void)
{ 
    // If the timer's counter reaches 0
    if (touch_timer_ticks == 0)
    {
        // Disable the timer for now.
        timer_del_task(timer_1ms, touch_timer_task);

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

/** ******************************************
 *
 * IQS620 bindings
 *
 ****************************************** */

void iqs620_callback_button_pressed(uint8_t button)
{
    LOG("button=%d", button);

    if (button == 0)
        touch_next_state(TOUCH_EVENT_0_ON);
    if (button == 1)
        touch_next_state(TOUCH_EVENT_1_ON);
}

void iqs620_callback_button_released(uint8_t button)
{
    LOG("button=%d", button);

    if (button == 0)
        touch_next_state(TOUCH_EVENT_0_OFF);
    if (button == 1)
        touch_next_state(TOUCH_EVENT_1_OFF);
}

/** ******************************************
 *
 * micropython bindings
 *
 ****************************************** */

STATIC mp_obj_t callback_list[TOUCH_BUTTON_NUM][TOUCH_ACTION_NUM];

STATIC mp_obj_t mod_touch___init__(void)
{
    for (size_t i = 0; i < LEN(callback_list); i++)
        callback_list[0][i] = callback_list[1][i] = mp_const_none;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_touch___init___obj, mod_touch___init__);

static inline mp_obj_t touch_get_callback(touch_state_t trigger)
{
    switch (trigger) {
    case TOUCH_TRIGGER_0_TAP:       return callback_list[0][TOUCH_ACTION_TAP];
    case TOUCH_TRIGGER_1_TAP:       return callback_list[1][TOUCH_ACTION_TAP];
    case TOUCH_TRIGGER_0_PRESS:     return callback_list[0][TOUCH_ACTION_PRESS];
    case TOUCH_TRIGGER_1_PRESS:     return callback_list[1][TOUCH_ACTION_PRESS];
    case TOUCH_TRIGGER_0_LONG:      return callback_list[0][TOUCH_ACTION_LONG];
    case TOUCH_TRIGGER_1_LONG:      return callback_list[1][TOUCH_ACTION_LONG];
    case TOUCH_TRIGGER_0_1_SLIDE:   return callback_list[0][TOUCH_ACTION_SLIDE];
    case TOUCH_TRIGGER_1_0_SLIDE:   return callback_list[1][TOUCH_ACTION_SLIDE];
    default:                        return mp_const_none;
    }
}

/**
 * Overriding the default callback implemented in driver/iqs620.c
 * @param trigger The trigger that ran the callback.
 */
void touch_callback(touch_state_t trigger)
{
    mp_obj_t callback = touch_get_callback(trigger);

    if (callback == mp_const_none)
    {
        LOG("trigger=0x%02X no callback set", trigger);
    }
    else
    {
        LOG("trigger=0x%02X scheduling trigger", trigger);
        mp_sched_schedule(callback, MP_OBJ_NEW_SMALL_INT(trigger));
    }
}

/**
 * Setup a python function as a callback for a given action.
 * @param button_in Button (0 or 1) to use.
 * @param action_in Action to react to.
 * @param callback Python function to be called.
 */
STATIC mp_obj_t touch_bind(mp_obj_t button_in, mp_obj_t action_in, mp_obj_t callback)
{
    uint8_t button = mp_obj_get_int(button_in);
    uint8_t action = mp_obj_get_int(action_in);

    if (button >= TOUCH_BUTTON_NUM)
        mp_raise_ValueError(MP_ERROR_TEXT("button must be touch.A or touch.B"));
    if (action >= TOUCH_ACTION_NUM)
        mp_raise_ValueError(MP_ERROR_TEXT("action must be any of touch.TOUCH_*"));

    callback_list[button][action] = callback;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(touch_bind_obj, touch_bind);

STATIC const mp_rom_map_elem_t touch_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_touch) },
    { MP_ROM_QSTR(MP_QSTR___init__),            MP_ROM_PTR(&mod_touch___init___obj) },

    // methods
    { MP_ROM_QSTR(MP_QSTR_bind),                MP_ROM_PTR(&touch_bind_obj) },

    // constants
    { MP_ROM_QSTR(MP_QSTR_A),                   MP_OBJ_NEW_SMALL_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_B),                   MP_OBJ_NEW_SMALL_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_LONG),                MP_OBJ_NEW_SMALL_INT(TOUCH_ACTION_LONG) },
    { MP_ROM_QSTR(MP_QSTR_PRESS),               MP_OBJ_NEW_SMALL_INT(TOUCH_ACTION_PRESS) },
    { MP_ROM_QSTR(MP_QSTR_SLIDE),               MP_OBJ_NEW_SMALL_INT(TOUCH_ACTION_SLIDE) },
    { MP_ROM_QSTR(MP_QSTR_TAP),                 MP_OBJ_NEW_SMALL_INT(TOUCH_ACTION_TAP) },
};
STATIC MP_DEFINE_CONST_DICT(touch_module_globals, touch_module_globals_table);

const mp_obj_module_t touch_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&touch_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_touch, touch_module);
