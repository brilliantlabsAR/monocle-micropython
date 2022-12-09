/*
 * Copyright (2022), Brilliant Labs Limited (Hong Kong)
 * Licensed under the MIT License
 */

#ifndef MONOCLE_TOUCH_H
#define MONOCLE_TOUCH_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Touch interface application logics.
 * @defgroup touch
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

#endif
