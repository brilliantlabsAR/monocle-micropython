/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Josuah Demangeon (name@email.com)
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
 * Touch interface application logics.
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

void touch_callback(touch_state_t trigger);

