/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Nathan Ashelman <nathan@itsbrilliant.co>
 * Authored by: Shreyas Hemachandra <shreyas.hemachandran@gmail.com>
 * Authored by: Georgi Beloev - 2021-07-20
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
 * IQS620 touch controller driver.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nrfx_gpiote.h"
#include "nrfx_systick.h"
#include "nrfx_log.h"
#include "nrfx_twi.h"

#include "driver/config.h"
#include "driver/i2c.h"
#include "driver/iqs620.h"
#include "driver/timer.h"

// registers

#define IQS620_ID                               0x00
#define IQS620_SYS_FLAGS                        0x10
#define IQS620_GLOBAL_EVENTS                    0x11
#define IQS620_PROX_FUSION_FLAGS                0x12

#define IQS620_CHANNEL_COUNT_0_LO               0x20
#define IQS620_CHANNEL_COUNT_0_HI               0x21
#define IQS620_CHANNEL_COUNT_1_LO               0x22
#define IQS620_CHANNEL_COUNT_1_HI               0x23

#define IQS620_PROX_FUSION_0_0                  0x40
#define IQS620_PROX_FUSION_0_1                  0x41
#define IQS620_PROX_FUSION_1_0                  0x43
#define IQS620_PROX_FUSION_1_1                  0x44
#define IQS620_PROX_FUSION_2_0                  0x46
#define IQS620_PROX_FUSION_2_1                  0x47
#define IQS620_PROX_FUSION_3_0                  0x49
#define IQS620_PROX_FUSION_3_1                  0x4A

#define IQS620_PROX_THRESHOLD_0                 0x60
#define IQS620_PROX_THRESHOLD_1                 0x62
#define IQS620_PROX_THRESHOLD_2                 0x64

#define IQS620_TOUCH_THRESHOLD_0                0x61
#define IQS620_TOUCH_THRESHOLD_1                0x63
#define IQS620_TOUCH_THRESHOLD_2                0x65

#define IQS620_SYS_SETTINGS                     0xD0
#define IQS620_ACTIVE_CHANNELS                  0xD1
#define IQS620_POWER_MODE                       0xD2
#define IQS620_NORMAL_POWER_REPORT_RATE         0xD3
#define IQS620_LOW_POWER_REPORT_RATE            0xD4
#define IQS620_ULTRA_LOW_POWER_REPORT_RATE      0xD5
#define IQS620_AUTO_SWITCH_TIMER_500MS          0xD6

// bit fields

#define IQS620_SYS_FLAGS_RESET_HAPPENED         (1 << 7)
#define IQS620_SYS_FLAGS_POWER_MODE_NP          (0 << 3)
#define IQS620_SYS_FLAGS_POWER_MODE_LP          (1 << 3)
#define IQS620_SYS_FLAGS_POWER_MODE_ULP         (2 << 3)
#define IQS620_SYS_FLAGS_POWER_MODE_HALT        (3 << 3)
#define IQS620_SYS_FLAGS_ATI_BUSY               (1 << 2)
#define IQS620_SYS_FLAGS_EVENT                  (1 << 1)
#define IQS620_SYS_FLAGS_NP_UPDATE              (1 << 0)

#define IQS620_GLOBAL_EVENTS_SAR_ACTIVE         (1 << 7)
#define IQS620_GLOBAL_EVENTS_PMU                (1 << 6)
#define IQS620_GLOBAL_EVENTS_SYS                (1 << 5)
#define IQS620_GLOBAL_EVENTS_TEMP               (1 << 4)
#define IQS620_GLOBAL_EVENTS_HYST               (1 << 3)
#define IQS620_GLOBAL_EVENTS_HALL               (1 << 2)
#define IQS620_GLOBAL_EVENTS_SAR                (1 << 1)
#define IQS620_GLOBAL_EVENTS_PROX               (1 << 0)

#define IQS620_PROX_FUSION_FLAGS_CH2_T          (1 << 6)
#define IQS620_PROX_FUSION_FLAGS_CH1_T          (1 << 5)
#define IQS620_PROX_FUSION_FLAGS_CH0_T          (1 << 4)
#define IQS620_PROX_FUSION_FLAGS_CH2_P          (1 << 2)
#define IQS620_PROX_FUSION_FLAGS_CH1_P          (1 << 1)
#define IQS620_PROX_FUSION_FLAGS_CH0_P          (1 << 0)

#define IQS620_PROX_FUSION_0_CS_MODE            (0 << 6)
#define IQS620_PROX_FUSION_0_CS_RX_NONE         (0 << 0)
#define IQS620_PROX_FUSION_0_CS_RX_0            (1 << 0)
#define IQS620_PROX_FUSION_0_CS_RX_1            (2 << 0)
#define IQS620_PROX_FUSION_0_CS_RX_01           (3 << 0)

#define IQS620_PROX_FUSION_1_CAP_15PF           (0 << 6)
#define IQS620_PROX_FUSION_1_CAP_60PF           (1 << 6)
#define IQS620_PROX_FUSION_1_CHG_FREQ_DIV_1_2   (0 << 4)
#define IQS620_PROX_FUSION_1_CHG_FREQ_DIV_1_4   (1 << 4)
#define IQS620_PROX_FUSION_1_CHG_FREQ_DIV_1_8   (2 << 4)
#define IQS620_PROX_FUSION_1_CHG_FREQ_DIV_1_16  (3 << 4)
#define IQS620_PROX_FUSION_1_ATI_DISABLED       (0 << 0)
#define IQS620_PROX_FUSION_1_ATI_PARTIAL        (1 << 0)
#define IQS620_PROX_FUSION_1_ATI_SEMI_PARTIAL   (2 << 0)
#define IQS620_PROX_FUSION_1_ATI_FULL           (3 << 0)

#define IQS620_PROX_FUSION_2_ATI_BASE_75        (0 << 6)
#define IQS620_PROX_FUSION_2_ATI_BASE_100       (1 << 6)
#define IQS620_PROX_FUSION_2_ATI_BASE_150       (2 << 6)
#define IQS620_PROX_FUSION_2_ATI_BASE_200       (3 << 6)

#define IQS620_SYS_SETTINGS_SOFT_RESET          (1 << 7)
#define IQS620_SYS_SETTINGS_ACK_RESET           (1 << 6)
#define IQS620_SYS_SETTINGS_EVENT_MODE          (1 << 5)
#define IQS620_SYS_SETTINGS_4MHZ                (1 << 4)
#define IQS620_SYS_SETTINGS_COMMS_ATI           (1 << 3)
#define IQS620_SYS_SETTINGS_ATI_BAND_1_16       (1 << 2)
#define IQS620_SYS_SETTINGS_REDO_ATI            (1 << 1)
#define IQS620_SYS_SETTINGS_RESEED              (1 << 0)

#define IQS620_POWER_MODE_PWM_OUT               (1 << 7)
#define IQS620_POWER_MODE_ULP_ENABLE            (1 << 6)
#define IQS620_POWER_MODE_AUTO                  (0 << 3)
#define IQS620_POWER_MODE_NP                    (4 << 3)
#define IQS620_POWER_MODE_LP                    (5 << 3)
#define IQS620_POWER_MODE_ULP                   (6 << 3)
#define IQS620_POWER_MODE_HALT                  (7 << 3)
#define IQS620_POWER_MODE_NP_RATE_1_2           (0 << 0)
#define IQS620_POWER_MODE_NP_RATE_1_4           (1 << 0)
#define IQS620_POWER_MODE_NP_RATE_1_8           (2 << 0)
#define IQS620_POWER_MODE_NP_RATE_1_16          (3 << 0)
#define IQS620_POWER_MODE_NP_RATE_1_32          (4 << 0)
#define IQS620_POWER_MODE_NP_RATE_1_64          (5 << 0)
#define IQS620_POWER_MODE_NP_RATE_1_128         (6 << 0)
#define IQS620_POWER_MODE_NP_RATE_1_256         (7 << 0)

// values

#define IQS620_ID_VALUE                         0x41

// default is 0x10 = target of 512.
// target = 0x1E * 32 = 960, gives good results on MK11 Flex through
// 1mm plastic (higher value slow to react)
#define IQS620_ATI_TARGET                       0x1E

// 0=default (22), 1=most sensitive, 255=least sensitive
#define IQS620_PROX_THRESHOLD                   10         

// 0=default (27), 1=most sensitive, 255=least sensitive
#define IQS620_TOUCH_THRESHOLD                  10         

// Last known state of the buttons.
static uint8_t iqs620_button_0_state;       
static uint8_t iqs620_button_1_state;       

/**
 * Workaround the fact taht nordic returns an ENUM instead of a simple integer.
 */
static inline void check(char const *func, nrfx_err_t err)
{
    if (err != NRFX_SUCCESS)
        LOG("%s: %s", func, NRFX_LOG_ERROR_STRING_GET(err));
}

/**
 * Configure a register with given value.
 * @param reg Address of the register.
 * @param data Value to write.
 */
static void iqs620_write_reg(uint8_t addr, uint8_t data)
{
    uint8_t buf[2] = { addr, data };
    bool ok;

    ok = i2c_write(IQS620_I2C, IQS620_ADDR, buf, sizeof(buf));
    assert(ok);
}

/**
 * Read multiple bytes from a register.
 * @param reg Address of the register.
 * @param buf Destination buffer.
 * @param len Size of this buffer, number of bytes to read.
 * @bug NRFX library returns ERR_ANACK, but still getting data in: bug in NRFX?
 */
static void iqs620_read_reg(uint8_t addr, uint8_t *buf, unsigned len)
{
    bool ok;

    // I2C write for the register address (without stop)
    ok = i2c_write_no_stop(IQS620_I2C, IQS620_ADDR, &addr, 1);
    assert(ok);

    // I2C read the data after the write.
    ok = i2c_read(IQS620_I2C, IQS620_ADDR, buf, len);
    assert(ok);
}

static uint8_t iqs620_read_u8(uint8_t addr)
{
    uint8_t val;

    iqs620_read_reg(addr, &val, 1);
    return val;
}

/**
 * Configure the IQS620 to get it ready to work.
 */
static void iqs620_configure(void)
{
    // acknowledge any pending resets, switch to event mode, comms enabled in ATI
    iqs620_write_reg(IQS620_SYS_SETTINGS, IQS620_SYS_SETTINGS_ACK_RESET |
        IQS620_SYS_SETTINGS_EVENT_MODE | IQS620_SYS_SETTINGS_COMMS_ATI);

    // enable channels 0 and 1 for capacitive prox/touch sensing
    iqs620_write_reg(IQS620_ACTIVE_CHANNELS, (1 << 1) | (1 << 0));

    // auto power mode, ULP disabled, 1/16 normal power update rate
    iqs620_write_reg(IQS620_POWER_MODE,
        IQS620_POWER_MODE_AUTO | IQS620_POWER_MODE_NP_RATE_1_16);

    // set up channel 0 to process RX 0
    iqs620_write_reg(IQS620_PROX_FUSION_0_0,
        IQS620_PROX_FUSION_0_CS_MODE | IQS620_PROX_FUSION_0_CS_RX_0);

    // set up channel 1 to process RX 1
    iqs620_write_reg(IQS620_PROX_FUSION_0_1,
        IQS620_PROX_FUSION_0_CS_MODE | IQS620_PROX_FUSION_0_CS_RX_1);

    // channel 0 cap size 15 pF, full-ATI mode
    iqs620_write_reg(IQS620_PROX_FUSION_1_0,
        IQS620_PROX_FUSION_1_CAP_15PF | IQS620_PROX_FUSION_1_CHG_FREQ_DIV_1_8 | IQS620_PROX_FUSION_1_ATI_FULL);

    // channel 1 cap size 15 pF, full-ATI mode
    iqs620_write_reg(IQS620_PROX_FUSION_1_1,
        IQS620_PROX_FUSION_1_CAP_15PF | IQS620_PROX_FUSION_1_CHG_FREQ_DIV_1_8 | IQS620_PROX_FUSION_1_ATI_FULL);

    // channel 0 cap sensing ATI base & target (default 0xD0: base=200, target=512 is not sensitive enough)
    iqs620_write_reg(IQS620_PROX_FUSION_2_0,
        // base=75, target as configured
        IQS620_PROX_FUSION_2_ATI_BASE_75 | IQS620_ATI_TARGET);

    // channel 1 cap sensing ATI base & target (default 0xD0: base=200, target=512 is not sensitive enough)
    iqs620_write_reg(IQS620_PROX_FUSION_2_1,
        // base=75, target as configured
        IQS620_PROX_FUSION_2_ATI_BASE_75 | IQS620_ATI_TARGET);

    if (IQS620_PROX_THRESHOLD != 0)
    {
        // set prox detection threshold for channels 0 and 1
        iqs620_write_reg(IQS620_PROX_THRESHOLD_0, IQS620_PROX_THRESHOLD);
        iqs620_write_reg(IQS620_PROX_THRESHOLD_1, IQS620_PROX_THRESHOLD);
    }

    if (IQS620_TOUCH_THRESHOLD != 0)
    {
        // set touch detection threshold for channels 0 and 1
        iqs620_write_reg(IQS620_TOUCH_THRESHOLD_0, IQS620_TOUCH_THRESHOLD);
        iqs620_write_reg(IQS620_TOUCH_THRESHOLD_1, IQS620_TOUCH_THRESHOLD);
    }

    // event mode, comms enabled in ATI, redo ATI
    iqs620_write_reg(IQS620_SYS_SETTINGS, IQS620_SYS_SETTINGS_EVENT_MODE |
        IQS620_SYS_SETTINGS_COMMS_ATI | IQS620_SYS_SETTINGS_REDO_ATI);
}

/**
 * State of one button.
 *
 * In the IQS620, the state is encoded as two boolean flags:
 * Proximity state and Touch state.
 *
 * Here we assume that when Touch is set, Proximity is also implicitly
 * set, as it is not possible to touch the button without getting close
 * to it.
 */
typedef enum
{
    IQS620_STATE_NONE,
    IQS620_STATE_PROX,
    IQS620_STATE_TOUCH,
} iqs620_state_t;

/**
 * Callback to override to react to button 0 or 1 press event.
 * @param button The button number.
 */
__attribute__((weak))
void iqs620_callback_button_pressed(uint8_t button)
{
    LOG("button=0x%02X", button);
}

/**
 * Callback to override to react to button 0 or 1 press event.
 * @param button The button number.
 */
__attribute__((weak))
void iqs620_callback_button_released(uint8_t button)
{
    LOG("button=0x%02X", button);
}

/**
 * Trigger
 *
 * Proximity is just here to debounce the touch event: switching quickly
 * between proximity and touch has no effect.
 */
static void iqs620_process_state(uint8_t button, iqs620_state_t *old, iqs620_state_t new)
{
    if (*old == new)
        return;

    // We changed from some state to either TOUCH or NONE.
    switch (new)
    {
    case IQS620_STATE_PROX:
    {
        // Not triggering anything, state just useful for debouncing.
        return;
    }
    case IQS620_STATE_NONE:
    {
        iqs620_callback_button_released(button);
        break;
    }
    case IQS620_STATE_TOUCH:
    {
        iqs620_callback_button_pressed(button);
        break;
    }
    }
    *old = new;
}

#define STATE(x, ch) (\
    (x) & (IQS620_PROX_FUSION_FLAGS_CH ## ch ## _T) ? IQS620_STATE_TOUCH : \
    (x) & (IQS620_PROX_FUSION_FLAGS_CH ## ch ## _P) ? IQS620_STATE_PROX : \
    IQS620_STATE_NONE \
)

/**
 * TOUCH_RDY pin high-to-low state change handler
 * Handler for an event on a GPIO pin, notifying that the IQS620 is ready.
 * @param pin The pin triggering the event.
 * @param action The event triggered.
 */
static void iqs620_touch_rdy_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    assert(pin == IQS620_TOUCH_RDY_PIN);

    uint8_t events;
    iqs620_read_reg(IQS620_GLOBAL_EVENTS, &events, 1);
    LOG("events=0x%02x", events);

    if (events & IQS620_GLOBAL_EVENTS_PROX)
    {
        uint8_t proxflags;
        iqs620_read_reg(IQS620_PROX_FUSION_FLAGS, &proxflags, 1);
        LOG("proxflags=0x%02x", proxflags);
        iqs620_process_state(0, &iqs620_button_0_state, STATE(proxflags, 0));
        iqs620_process_state(1, &iqs620_button_1_state, STATE(proxflags, 1));
    }
    if (events & IQS620_GLOBAL_EVENTS_SYS)
    {
        uint8_t sysflags;
        iqs620_read_reg(IQS620_SYS_FLAGS, &sysflags, 1);
        LOG("sysflags=0x%02x", sysflags);
        if (sysflags & IQS620_SYS_FLAGS_RESET_HAPPENED)
        {
            LOG("reset detected, reconfiguring");
            iqs620_configure();
        }
    }
}

#undef STATE

/**
 * Get the raw counts for tuning thresholds.
 * @param channel Sensor channel number to read the data from
 */
uint16_t iqs620_get_count(uint8_t channel)
{
    uint16_t count;
    uint8_t data[2];

    // Read 2 bytes from the base address of the channel count register
    iqs620_read_reg(channel * 2 + IQS620_CHANNEL_COUNT_0_LO, data, sizeof(data));

    count = data[1] << 8 | data[0] << 0;
    LOG("channel=%d count=%d", channel, count);
    return (count);
}

/**
 * Initialise the chip as well as the iqs620 instance.
 */
void iqs620_init(void)
{
    uint32_t err;

    // Setup the GPIO pin for touch state interrupts.
    nrf_gpio_cfg(
        IQS620_TOUCH_RDY_PIN,
        NRF_GPIO_PIN_DIR_INPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        NRF_GPIO_PIN_PULLUP,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_SENSE_LOW
    );

    // Configure the TOUCH_RDY pin for high-to-low edge GPIOTE event
    nrfx_gpiote_in_config_t config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    config.pull = NRF_GPIO_PIN_PULLUP;
    err = nrfx_gpiote_in_init(IQS620_TOUCH_RDY_PIN, &config, iqs620_touch_rdy_handler);
    assert(err == NRFX_SUCCESS);

    // Disable the TOUCH_RDY event during reset.
    nrfx_gpiote_in_event_disable(IQS620_TOUCH_RDY_PIN);

    // Initiate soft reset.
    iqs620_write_reg(IQS620_SYS_SETTINGS, 1 << 7);

    // Wait for IQS620 system reset completion.
    nrfx_systick_delay_ms(10);

    // Check that the chip responds correctly.
    assert(iqs620_read_u8(IQS620_ID) == IQS620_ID_VALUE);

    // Enable the TOUCH_RDY event after the reset.
    nrfx_gpiote_in_event_enable(IQS620_TOUCH_RDY_PIN, true);
}
