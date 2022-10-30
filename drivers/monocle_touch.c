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

/*
 * This state machine can distinguish between the following gestures: 
 * Tap: push & quick release
 * Slide LR or RL +: tap on one button followed by tap on other
 * DoubleTap: Tap, followed quickly by another Tap
 * Press: push one for >0.5s & <10s then release
 * LongPress: push for >10s then release
 * LongBoth +: push both buttons for >10s then release
 * (+ these 3 gestures require TOUCH_CAPSENSE to be enabled)
 *
 * Transition to new state is triggered by a timeout or push/release event.
 * Timer, of given duration, is started when entering state that has a timeout.
 * Transitions back to IDLE will generate a gesture, this happens on release
 * for most gestures, but after TAP_INTERVAL for Tap (i.e. some delay).
 *
 * State machine transitions:
 *
 * -----------------------------------------------------------------------------
 * | current state   | event/timeout | new state      | gesture    | _CAPSENSE |
 * |---------------------------------------------------------------------------|
 * | IDLE            | push[one]     | TRIGGERED      |            |           |
 * | IDLE            | push[both]    | BOTH_PRESSED   |            | *         |
 * | TRIGGERED       | 0.5s          | PRESSED        |            |           |
 * | TRIGGERED       | release       | TAPPED         |            |           |
 * | TRIGGERED       | push[other]   | BOTH_PRESSED   |            | *         |
 * | TAPPED          | 0.25s         | IDLE           | Tap        |           |
 * | TAPPED          | push[same]    | TAPPED2        |            |           |
 * | TAPPED          | push[other]   | SLID           |            | *         |
 * | TAPPED2         | (infinite)    | (no change)    |            |           |
 * | TAPPED2         | release       | IDLE           | DoubleTap  |           |
 * | SLID            | (infinite)    | (no change)    |            | *         |
 * | SLID            | release[LR]   | IDLE           | SlideLR    | *         |
 * | SLID            | release[RL]   | IDLE           | SlideRL    | *         |
 * | PRESSED         | 9.5s          | LONG           |            |           |
 * | PRESSED         | release       | IDLE           | Press      |           |
 * | PRESSED         | push[other]   | BOTH_PRESSED   |            | *         |
 * | LONG            | (infinite)    | (no change)    |            |           |
 * | LONG            | release       | IDLE           | LongPress  |           |
 * | BOTH_PRESSED    | 9.5s          | BOTH_LONG      |            | *         |
 * | BOTH_PRESSED    | release[one]  | (no change)+   |            | *         | + and stop timer
 * | BOTH_PRESSED    | release[all]  | IDLE           | PressBoth  | *         |
 * | BOTH_LONG       | (infinite)    | (no change)    |            | *         |
 * | BOTH_LONG       | release[one]  | (no change)    |            | *         |
 * | BOTH_LONG       | release[all]  | IDLE           | LongBoth   | *         |
 * -----------------------------------------------------------------------------
 *
 * See diagram at: https://drive.google.com/file/d/1nErJZ_vvQBIfS90sQ9oX6CoDodD6m3_0/view?usp=sharing
 */

#define TOUCH_TAP_INTERVAL     APP_TIMER_TICKS(250)   /**< Timeout for button tap (ticks) = 0.25 second */
#define TOUCH_PRESS_INTERVAL   APP_TIMER_TICKS(500)   /**< Timeout for button press (ticks) = 0.5 second */
#define TOUCH_LONG_INTERVAL    APP_TIMER_TICKS(9500)  /**< Timeout for long button press (ticks) = 9.5 seconds + PRESS_INTERVAL = 10 sec */

#define LOG(...) NRFX_LOG_WARNING(__VA_ARGS__)
#define CHECK(err) check(__func__, err)

typedef enum {
    TOUCH_SLID,
    TOUCH_BOTH_PRESSED,
    TOUCH_BOTH_LONG,
    TOUCH_IDLE,
    TOUCH_TRIGGERED,
    TOUCH_TAPPED,
    TOUCH_TAPPED2,
    TOUCH_PRESSED,
    TOUCH_LONG
} touch_state_t;

touch_state_t touch_state_next[] = {
    []
};

static void touch_pin_handler(void *iqs620, iqs620_button_t button, iqs620_event_t event);
static void touch_event_handler(bool istimer);

/** Current state machine state. */
static touch_state_t touch_state = TOUCH_IDLE;

/** Gesture handler registered by init(). */
static touch_gesture_handler_t touch_gesture_handler = NULL;

/** State machine timer id. */
nrfx_timer_t delay_timer = NRFX_TIMER_INSTANCE(0);

static uint16_t first_push_status = 0;
static uint16_t second_push_status = 0;
static uint16_t release_status = 0;

/**
 * Workaround the fact taht nordic returns an ENUM instead of a simple integer.
 */
static inline void check(char const *func, nrfx_err_t err)
{
    if (err != NRFX_SUCCESS)
        LOG("%s: %s", func, NRFX_LOG_ERROR_STRING_GET(err));
}

static iqs620_t sensor = {
    .rdy_pin         = IO_TOUCHED_PIN,
    .addr            = IQS620_ADDR,
    .callback        = touch_pin_handler,
    .prox_threshold  = 10, // 0=most sensitive, 255=least sensitive
    .touch_threshold = 10,
    .ati_target      = 0x1E // target = 0x1E * 32 = 960, gives good results on MK11 Flex through 1mm plastic (higher value slow to react)
};

static const char *button_names[] = {
    [IQS620_BUTTON_B0]      = "B0",
    [IQS620_BUTTON_B1]      = "B1"
};

static const char *event_names[] = {
    [IQS620_BUTTON_UP]      = "UP",
    [IQS620_BUTTON_PROX]    = "PROX",
    [IQS620_BUTTON_DOWN]    = "DOWN",
};

// send gesture to the handler (if any) registered by touch_init()
static void generate_gesture(touch_gesture_t gesture)
{
    switch(gesture)
    {
    case TOUCH_GESTURE_TAP:
        LOG("Touch gesture Tap.");
        break;
    case TOUCH_GESTURE_TAP_BOTH:
        LOG("Touch gesture DoubleTap.");
        break;
    case TOUCH_GESTURE_PRESS:
        LOG("Touch gesture Press.");
        break;
    case TOUCH_GESTURE_LONG_PRESS:
        LOG("Touch gesture LongPress.");
        break;
    case TOUCH_GESTURE_PRESSBOTH:
        LOG("Touch gesture PressBoth.");
        break;
    case TOUCH_GESTURE_LONGBOTH:
        LOG("Touch gesture LongBoth.");
        break;
    case TOUCH_GESTURE_SLIDELR:
        LOG("Touch gesture SlideLR.");
        break;
    case TOUCH_GESTURE_SLIDERL:
        LOG("Touch gesture SlideRL.");
        break;
    }

    if (touch_gesture_handler)
    {
        touch_gesture_handler(gesture);
    }
}

uint8_t bit_count (uint16_t value) {
    uint8_t count = 0;
    while (value > 0) {              // until all bits are zero
        if ((value & 0x0001) == 1)   // check lower bit
            count++;
        value >>= 1;                 // shift bits, removing lower bit
    }
    return count;
}

/**
 * Start the main touch timer with a custom duration.
 * @param ticks The number of ticks of the RTC1 clock (including prescaling).
 */
static void touch_timer_start(uint32_t ticks)
{
    timer_config_t config = NRFX_TIMER_DEFAULT_CONFIG;

    // Start application timer
    CHECK(nrfx_timer_init(&delay_timer, &config, delay_timeout_handler));
    CHECK(nrfx_timer_start(delay_timer, ticks, NULL));
}

/**
 * Get the current status of the touch button.
 * @param status Pointer filled with the button status.
 */
static void touch_store_button_status(uint16_t *status)
{
    iqs620_get_button_status(&sensor, status);
}

// public function implementations

/**
 * Init the touch peripheral using GPIO instead of the capsense.
 * Used in early init, for wakeup if put to sleep immediately due to low or charging battery
 */
void touch_quick_init(void)
{
    nrf_gpio_cfg(
        IO_TOUCHED_PIN,
        NRF_GPIO_PIN_DIR_INPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        IO_TOUCHED_PIN_PULL,
        NRF_GPIO_PIN_S0S1,
        // Sense is set in shutdown_handler().
        NRF_GPIO_PIN_NOSENSE);
}

/**
 * Reset the capsense chip, resending all its configuration.
 * @return True on success.
 */
bool touch_reprogram(void)
{
    return iqs620_reset(&sensor);
}

/**
 * Print the raw count of characters read insofar.
 * @todo Does not currently print anything.
 * @return True on success.
 */
bool touch_print_ch_counts(void)
{
    uint16_t *data = NULL;
    if (!iqs620_get_ch_count(&sensor, 0, data))
        return false;
    if (!iqs620_get_ch_count(&sensor, 1, data))
        return false;
    return true;
}

/**
 * Timer handler for a touch event.
 * @param p_context Unused.
 */
static void delay_timeout_handler(void * p_context)
{
    (void)p_context;
    LOG("Touch timeout.");
    // process state machine change, from timer (so true)
    touch_event_handler(true);
}

/**
 * Process pin value change event, from the IQS620 capacitive controller.
 * @param iqs620 Pointer to the IQS620 instance structure.
 * @param button Button instance that did trigger that event.
 * @param event Type of event triggered.
 */
static void touch_pin_handler(void *iqs620, iqs620_button_t button, iqs620_event_t event)
{
    assert(iqs620 != NULL);
    LOG("Touch pin handler: IQS620 %s %s", button_names[button], event_names[event]);
    // ignore IQS620_BUTTON_PROX event
    if (event == IQS620_BUTTON_DOWN || event == IQS620_BUTTON_UP) {
        // process state machine change, from button push/release (not timer, so false)
        touch_event_handler(false);
    }
}

/**
 * Stop the main touch timer, start a new one with a custom duration.
 * @param ticks The number of ticks of the RTC1 clock (including prescaling).
 */
static void touch_timer_restart(uint32_t ticks)
{
    // Stop the timer
    CHECK(nrfx_timer_stop(delay_timer));

    // Re-initialize the timer (per nrfx_timer.h: "can be called again ... and will re-initialize ... if the timer is not running.")
    CHECK(nrfx_timer_create(&delay_timer, delay_timeout_handler));

    // Start application timer
    CHECK(nrfx_timer_start(delay_timer, ticks, NULL));
}

/**
 * Stop the main touch timer.
 */
static void touch_timer_stop(void)
{
    // Stop the timer
    CHECK(nrfx_timer_stop(delay_timer));

    // Re-initialize the timer (per nrfx_timer.h: "can be called again ... and will re-initialize ... if the timer is not running.")
    CHECK(nrfx_timer_create(&delay_timer, delay_timeout_handler));

    // Ready for next call to touch_timer_start()
}

/**
 * Handle events, which can be button push/release (coming from touch_pin_handler()) or timer timeout.
 * update state machine accordingly
 * @param istimer True if this function is called due to a timeout for this event.
 */
static void touch_event_handler(bool istimer)
{
    switch(touch_state)
    {
    case TOUCH_IDLE:
        if (istimer)
        {
            // stay in IDLE
            LOG("Touch IDLE: Timer was not stopped!");
        } else {
            // record first button touched
            touch_store_button_status(&first_push_status);
            // check whether one button pushed, or two
            if(first_push_status == 0) { // no buttons pushed, so an unexpected release event
                LOG("Touch IDLE: release event, but expected push");
                return;
            } else if (bit_count(first_push_status) == 2) { // two buttons pushed simultaneously
                touch_state = TOUCH_BOTH_PRESSED;
                LOG("Touch IDLE->BOTH_PRESSED");
                touch_timer_restart(TOUCH_LONG_INTERVAL);
            } else { // one button pushed
                touch_state = TOUCH_TRIGGERED;
                LOG("Touch IDLE->TRIGGERED");
                touch_timer_start(TOUCH_PRESS_INTERVAL);
            }
        }
        break;
    case TOUCH_TRIGGERED:
        if (istimer) {
            touch_state = TOUCH_PRESSED;
            LOG("Touch TRIGGERED->PRESSED");
            touch_timer_restart(TOUCH_LONG_INTERVAL);
        } else {
            touch_store_button_status(&second_push_status);
            if(second_push_status) { // non-zero -> another push
                touch_state = TOUCH_BOTH_PRESSED;
                LOG("Touch TRIGGERED->BOTH_PRESSED");
                touch_timer_restart(TOUCH_LONG_INTERVAL);
            } else { // all zeros -> release
                touch_state = TOUCH_TAPPED;
                LOG("Touch TRIGGERED->TAPPED");
                touch_timer_restart(TOUCH_TAP_INTERVAL);
            }
        }
        break;
    case TOUCH_TAPPED:
        if (istimer) {
            touch_state = TOUCH_IDLE;
            LOG("Touch TAPPED->IDLE");
            // timer already stopped
            generate_gesture(TOUCH_GESTURE_TAP);
        } else {
            touch_timer_stop();
            // no timer in TAPPED2 or SLID; wait indefinitely for release
            touch_store_button_status(&second_push_status);
            if(second_push_status != first_push_status) { // pushed different button
                touch_state = TOUCH_SLID;
                LOG("Touch TAPPED->SLID");
            } else { // pushed same button
                touch_state = TOUCH_TAPPED2;
                LOG("Touch TAPPED->TAPPED2");
            }
        }
        break;
    case TOUCH_TAPPED2:
        if (istimer) {
            // stay in TAPPED2
            LOG("Touch TAPPED2: should not be a timer here!");
        } else {
            touch_state = TOUCH_IDLE;
            LOG("Touch TAPPED2->IDLE");
            touch_timer_stop();
            generate_gesture(TOUCH_GESTURE_TAP_BOTH);
        }
        break;
    case TOUCH_PRESSED:
        if (istimer) {
            touch_state = TOUCH_LONG;
            LOG("Touch PRESSED->LONG");
            // timer already stopped
            // no timer in LONG; wait indefinitely for release
        } else {
            touch_store_button_status(&second_push_status);
            if(second_push_status) { // non-zero -> another push
                touch_state = TOUCH_BOTH_PRESSED;
                LOG("Touch PRESSED->BOTH_PRESSED");
                touch_timer_restart(TOUCH_LONG_INTERVAL);
            } else { // all zeros -> release
                touch_state = TOUCH_IDLE;
                LOG("Touch PRESSED->IDLE");
                touch_timer_stop();
                // no timer in IDLE
                generate_gesture(TOUCH_GESTURE_PRESS);
            }
        }
        break;
    case TOUCH_LONG:
        if (istimer) {
            // stay in LONG;
            LOG("Touch LONG: should not be a timer here!");
        } else {
            touch_state = TOUCH_IDLE;
            LOG("Touch LONG->IDLE");
            touch_timer_stop();
            generate_gesture(TOUCH_GESTURE_LONG_PRESS);
        }
        break;
    case TOUCH_SLID:
        if (istimer) {
            // stay in SLID;
            LOG("Touch SLID: should not be a timer here!");
        } else { // must be a release
            touch_state = TOUCH_IDLE;
            LOG("Touch SLID->IDLE");

            if(first_push_status < second_push_status) {
                generate_gesture(TOUCH_GESTURE_SLIDELR);
            } else {
                generate_gesture(TOUCH_GESTURE_SLIDERL);
            }
        }
        break;
    case TOUCH_BOTH_PRESSED:
        if (istimer) {
            touch_state = TOUCH_BOTH_LONG;
            LOG("Touch BOTH_PRESSED->BOTH_LONG");
            // timer already stopped
            // no timer in BOTH_LONG; wait indefinitely for release
        } else {
            touch_timer_stop();
            // no timer in either case
            // check whether both are released, or just one
            touch_store_button_status(&release_status);
            if(release_status == 0) { // both buttons released
                touch_state = TOUCH_IDLE;
                LOG("Touch BOTH_PRESSED->IDLE");
                generate_gesture(TOUCH_GESTURE_PRESSBOTH);
            } else { // one button released
                LOG("Touch BOTH_PRESSED: one button released");
            }
        }
        break;
    case TOUCH_BOTH_LONG:
        if (istimer) {
            // stay in BOTH_LONG;
            LOG("Touch BOTH_LONG: should not be a timer here!");
        } else {
            // check whether both are released, or just one
            touch_store_button_status(&release_status);
            if(release_status == 0) { // both buttons released
                touch_state = TOUCH_IDLE;
                LOG("Touch BOTH_LONG->IDLE");
                generate_gesture(TOUCH_GESTURE_LONGBOTH);
            } else { // one button released
                LOG("Touch BOTH_LONG: one button released");
            }
        }
        break;
    }
}

/**
 * Init the touch peripheral with the capsense chip support.
 * Used in full init.
 * @param handler Callback to a handler for touch gestures.
 * @return True on success.
 */
bool touch_init(touch_gesture_handler_t handler)
{
    bool success = false;

    // configure touched_N pin
    success = iqs620_init(&sensor);

    CHECK(nrfx_timer_create(&delay_timer, delay_timeout_handler));

    // resgister the handler
    touch_gesture_handler = handler;

    // start in IDLE state & wait for button push interrupt
    touch_state = TOUCH_IDLE;

    return(success);
}
