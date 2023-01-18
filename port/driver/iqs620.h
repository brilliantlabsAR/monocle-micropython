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

/**
 * Driver for the IQS620 hall effect, proximity sensor.
 * https://www.azoteq.com/images/stories/pdf/iqs620_datasheet.pdf
 * The interrupt line indicates that there is an event pending, that the MCU queries over I²C.
 * There are 2 touch buttons on the Monocle, each triggering these events:
 * - TOUCH_GESTURE_TAP        : 0.25s
 * - TOUCH_GESTURE_DOUBLETAP  : 0.25s (Both Tapped)
 * - TOUCH_GESTURE_PRESS      : 0.5s
 * - TOUCH_GESTURE_LONGPRESS  : 9.5s
 * - TOUCH_GESTURE_LONGBOTH   : 9.5s (Both Pressed)
 */

void iqs620_init(void);
void iqs620_enable(void);
void iqs620_reset(void);
void iqs620_callback_button_pressed(uint8_t button);
void iqs620_callback_button_released(uint8_t button);
uint32_t iqs620_get_id(void);
uint16_t iqs620_get_count(uint8_t channel);
