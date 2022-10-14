/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

/**
 * @file monocle_iqs620.c
 * @author Nathan Ashelman
 * @author Shreyas Hemachandra
 * @author Georgi Beloev (2021-07-20)
 */

#include "monocle_iqs620.h"
#include "monocle_i2c.h" // for existing I2C interface

#include "nrfx_gpiote.h"
#include "nrfx_systick.h"
#include "nrfx_log.h"

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

// parameters

#define IQS620_RESET_TIMEOUT_MS                 50
#define IQS620_RESET_RETRY_MS                   20

#define LOG_DEBUG(...) NRFX_LOG_DEBUG(__VA_ARGS__)
#define CHECK(err) check(__func__, err)

/**
 * Workaround the fact taht nordic returns an ENUM instead of a simple integer.
 */
static inline bool check(char const *func, nrfx_err_t err)
{
    if (err != NRFX_SUCCESS)
        NRFX_LOG_ERROR("%s: %s", func, NRFX_LOG_ERROR_STRING_GET(err));
    return err == NRFX_SUCCESS;
}

/**
 * Configure a register with given value.
 * @param sensor Instance to work upon.
 * @param reg Address of the register.
 * @param data Value to write.
 * @return True if I2C succeeds.
 */
static bool iqs620_wreg(iqs620_t *sensor, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = { reg, data };
    assert(sensor != NULL);

    LOG_DEBUG("IQS620 write 0x%x to register 0x%x.", data, reg);
    return i2c_write(sensor->addr, buf, sizeof(buf));
}

/**
 * Read multiple bytes from a register.
 * @param sensor Instance to work upon.
 * @param reg Address of the register.
 * @param data Destination buffer.
 * @param count Size of this buffer, number of bytes to read.
 * @return True if I2C succeeds.
 */
static bool iqs620_rregs(iqs620_t *sensor, uint8_t reg, uint8_t *data, unsigned count)
{
    assert(sensor != NULL);

    // I2C write for the register address (without stop)
    //ret_code_t r = nrfx_twi_tx(&sensor->twi_drv, sensor->addr, &reg, sizeof(reg), true);
    //if (r != NRF_SUCCESS) return false;
    if (!i2c_write_no_stop(sensor->addr, &reg, sizeof(reg))) return false;

    // I2C read for the data
    //r = nrfx_twi_rx(&sensor->twi_drv, sensor->addr, data, count);
    //return (r == NRF_SUCCESS);
    return i2c_read(sensor->addr, data, count);
}

/**
 * Configure the IQS620 to get it ready to work.
 * @param sensor Instance to work upon.
 * @return True if I2C succeeds.
 */
static bool iqs620_configure(iqs620_t *sensor)
{
    bool ok;

    // acknowledge any pending resets, switch to event mode, comms enabled in ATI
    ok = iqs620_wreg(sensor, IQS620_SYS_SETTINGS, IQS620_SYS_SETTINGS_ACK_RESET |
        IQS620_SYS_SETTINGS_EVENT_MODE | IQS620_SYS_SETTINGS_COMMS_ATI);
    if (!ok) return false;

    // enable channels 0 and 1 for capacitive prox/touch sensing
    ok = iqs620_wreg(sensor, IQS620_ACTIVE_CHANNELS, (1 << 1) | (1 << 0));
    if (!ok) return false;

    // auto power mode, ULP disabled, 1/16 normal power update rate
    ok = iqs620_wreg(sensor, IQS620_POWER_MODE,
        IQS620_POWER_MODE_AUTO | IQS620_POWER_MODE_NP_RATE_1_16);
    if (!ok) return false;

    // set up channel 0 to process RX 0
    ok = iqs620_wreg(sensor, IQS620_PROX_FUSION_0_0,
        IQS620_PROX_FUSION_0_CS_MODE | IQS620_PROX_FUSION_0_CS_RX_0);
    if (!ok) return false;

    // set up channel 1 to process RX 1
    ok = iqs620_wreg(sensor, IQS620_PROX_FUSION_0_1,
        IQS620_PROX_FUSION_0_CS_MODE | IQS620_PROX_FUSION_0_CS_RX_1);
    if (!ok) return false;

    // channel 0 cap size 15 pF, full-ATI mode
    ok = iqs620_wreg(sensor, IQS620_PROX_FUSION_1_0,
        // NOTE: testing shows better sensitivity with 15pF than with 60pF
        IQS620_PROX_FUSION_1_CAP_15PF | IQS620_PROX_FUSION_1_CHG_FREQ_DIV_1_8 | IQS620_PROX_FUSION_1_ATI_FULL);
        //IQS620_PROX_FUSION_1_CAP_60PF | IQS620_PROX_FUSION_1_CHG_FREQ_DIV_1_8 | IQS620_PROX_FUSION_1_ATI_FULL); // 60 pF
    if (!ok) return false;

    // channel 1 cap size 15 pF, full-ATI mode
    ok = iqs620_wreg(sensor, IQS620_PROX_FUSION_1_1,
        // NOTE: testing shows better sensitivity with 15pF than with 60pF
        IQS620_PROX_FUSION_1_CAP_15PF | IQS620_PROX_FUSION_1_CHG_FREQ_DIV_1_8 | IQS620_PROX_FUSION_1_ATI_FULL);
        //IQS620_PROX_FUSION_1_CAP_60PF | IQS620_PROX_FUSION_1_CHG_FREQ_DIV_1_8 | IQS620_PROX_FUSION_1_ATI_FULL); // 60 pF
    if (!ok) return false;

    // channel 0 cap sensing ATI base & target (default 0xD0: base=200, target=512 is not sensitive enough)
    ok = iqs620_wreg(sensor, IQS620_PROX_FUSION_2_0,
        IQS620_PROX_FUSION_2_ATI_BASE_75 | sensor->ati_target); // base=75, target as configured
    if (!ok) return false;

    // channel 1 cap sensing ATI base & target (default 0xD0: base=200, target=512 is not sensitive enough)
    ok = iqs620_wreg(sensor, IQS620_PROX_FUSION_2_1,
        IQS620_PROX_FUSION_2_ATI_BASE_75 | sensor->ati_target); // base=75, target as configured
    if (!ok) return false;

    if (sensor->prox_threshold != 0)
    {
        // set prox detection threshold for channels 0 and 1
        ok = iqs620_wreg(sensor, IQS620_PROX_THRESHOLD_0, sensor->prox_threshold);
        if (!ok) return false;
        ok = iqs620_wreg(sensor, IQS620_PROX_THRESHOLD_1, sensor->prox_threshold);
        if (!ok) return false;
    }

    if (sensor->touch_threshold != 0)
    {
        // set touch detection threshold for channels 0 and 1
        ok = iqs620_wreg(sensor, IQS620_TOUCH_THRESHOLD_0, sensor->touch_threshold);
        if (!ok) return false;
        ok = iqs620_wreg(sensor, IQS620_TOUCH_THRESHOLD_1, sensor->touch_threshold);
        if (!ok) return false;
    }

    // event mode, comms enabled in ATI, redo ATI
    ok = iqs620_wreg(sensor, IQS620_SYS_SETTINGS, IQS620_SYS_SETTINGS_EVENT_MODE |
        IQS620_SYS_SETTINGS_COMMS_ATI | IQS620_SYS_SETTINGS_REDO_ATI);
    if (!ok) return false;

    return true;
}

// OLD  NEW
  //TP   TP
  //01   00     RELEASE             (A)
  //00   01     PROX-IN             (B)
  //0X   1X     TOUCH               (C)
  //1X   01     PROX-OUT            (D)
  //1X   00     RELEASE             (E)

typedef struct
{
    bool prox;
    bool touch;
} tp_t;

#define TP(x, p, t)         { .prox = (x) & (p), .touch = (x) & (t) }

// helper function for iqs620_prox_touch
static void iqs620_prox_touch_1(iqs620_t *sensor, iqs620_button_t button, tp_t oldstate, tp_t newstate)
{
    if (sensor->callback != NULL)
    {
        if (!oldstate.touch && newstate.touch)
        {
            // event C (touch)
            // update button_status: set button bit
            sensor->button_status = sensor->button_status | (1 << button);
            LOG_DEBUG("touch: button_status = 0x%x.", sensor->button_status);

            (*sensor->callback)(sensor, button, IQS620_BUTTON_DOWN);
        }
        else if (oldstate.touch && !newstate.touch)
        {
            if (newstate.prox)
            {
                // event D (prox-out)
                (*sensor->callback)(sensor, button, IQS620_BUTTON_PROX);
            }
            else
            {
                // event E (release)
                // update button_status: clear button bit
                sensor->button_status = sensor->button_status & ~(1 << button);
                LOG_DEBUG("release: button_status = 0x%x.", sensor->button_status);

                (*sensor->callback)(sensor, button, IQS620_BUTTON_UP);
            }
        }
        else if (!oldstate.touch && !newstate.touch)
        {
            if (!oldstate.prox && newstate.prox)
            {
                // event B (prox-in)
                (*sensor->callback)(sensor, button, IQS620_BUTTON_PROX);
            }
            if (oldstate.prox && !newstate.prox)
            {
                // event A (release)
                // update button_status: clear button bit
                sensor->button_status = sensor->button_status & ~(1 << button);
                LOG_DEBUG("release: button_status = 0x%x.", sensor->button_status);

                (*sensor->callback)(sensor, button, IQS620_BUTTON_UP);
            }
        }
    }
}

/**
 * Process the touch events received from the chip.
 * @param sensor Instance to work upon.
 * @param proxflags Binary flags describing the state of touch/proximity.
 */
static void iqs620_prox_touch(iqs620_t *sensor, uint8_t proxflags)
{
    // extract B0 prox/touch flags
    tp_t b0old = TP(sensor->prox_touch_state,
        IQS620_PROX_FUSION_FLAGS_CH0_P, IQS620_PROX_FUSION_FLAGS_CH0_T);
    tp_t b0new = TP(proxflags,
        IQS620_PROX_FUSION_FLAGS_CH0_P, IQS620_PROX_FUSION_FLAGS_CH0_T);

    // process B0 events
    iqs620_prox_touch_1(sensor, IQS620_BUTTON_B0, b0old, b0new);

    // extract B1 prox/touch flags
    tp_t b1old = TP(sensor->prox_touch_state,
        IQS620_PROX_FUSION_FLAGS_CH1_P, IQS620_PROX_FUSION_FLAGS_CH1_T);
    tp_t b1new = TP(proxflags,
        IQS620_PROX_FUSION_FLAGS_CH1_P, IQS620_PROX_FUSION_FLAGS_CH1_T);

    // process B1 events
    iqs620_prox_touch_1(sensor, IQS620_BUTTON_B1, b1old, b1new);

    // udpate state
    sensor->prox_touch_state = proxflags;
}

// RDY pin high-to-low state change handler
  
// workaround for lack of context passed in gpiote callbacks
// could be extended to handle multiple iqs620 driver instances if needed

static iqs620_t *g_sensor           = NULL;

/**
 * Handler for an event on a GPIO pin, notifying that the IQS620 is ready.
 * @param pin The pin triggering the event.
 * @param action The event triggered.
 */
static void iqs620_rdy_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if ((g_sensor != NULL) && (g_sensor->rdy_pin == pin))
    {
        bool ok;
        iqs620_t *sensor = g_sensor;

        // iqs620 event detected; see what type

        uint8_t events = 0;
        ok = iqs620_rregs(sensor, IQS620_GLOBAL_EVENTS, &events, sizeof(events));
        if (!ok) NRFX_LOG_ERROR("IQS620 communication error!");

        LOG_DEBUG("IQS620 global events: %02x", events);

        if (events & IQS620_GLOBAL_EVENTS_PROX)
        {
            // prox/touch event detected
            LOG_DEBUG("IQS620 prox event detected!");

            // read prox/touch UI status
            uint8_t proxflags = 0;
            ok = iqs620_rregs(sensor, IQS620_PROX_FUSION_FLAGS, &proxflags, sizeof(proxflags));
            if (!ok) NRFX_LOG_ERROR("IQS620 communication error!");
            LOG_DEBUG("IQS620 prox/touch flags: %02X", proxflags);

            // process prox/touch events
            iqs620_prox_touch(sensor, proxflags);
        }

        if (events & IQS620_GLOBAL_EVENTS_SYS) {
            uint8_t sysflags = 0;
            ok = iqs620_rregs(sensor, IQS620_SYS_FLAGS, &sysflags, sizeof(sysflags));
            if (!ok) NRFX_LOG_ERROR("IQS620 communication error!");
            LOG_DEBUG("IQS620 system flags: %02x", sysflags);

            if (sysflags & IQS620_SYS_FLAGS_RESET_HAPPENED) {
                // sensor reset detected, reconfigure the sensor
                LOG_DEBUG("IQS620 reset detected!");
                if (!iqs620_configure(sensor))
                    NRFX_LOG_ERROR("IQS620 configuration failed!");
                LOG_DEBUG("IQS620 reconfigured");
            }
        }
    }
}

/**
 * Setup an event handler tha will let us know that the IQS620 is ready.
 * @param sensor Instance to work upon.
 */
static void iqs620_init_rdy_handler(iqs620_t *sensor)
{
    // workaround for lack of context passed in gpiote callbacks
    g_sensor = sensor;

    // initialize GPIOTE
    CHECK(nrfx_gpiote_init(NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY));

    // configure the RDY pin for high-to-low edge GPIOTE event
    nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;

    CHECK(nrfx_gpiote_in_init(sensor->rdy_pin, &in_config, iqs620_rdy_handler));
}

/**
 * Enable (or disable) the event telling that IQS620 is ready.
 * @param sensor Instance to work upon.
 */
static void iqs620_enable_rdy_handler(iqs620_t *sensor, bool enable)
{
    if (enable)
    {
        // enable the GPIOTE event
        nrfx_gpiote_in_event_enable(sensor->rdy_pin, true);
    }
    else
    {
        // disable the GPIOTE event
        nrfx_gpiote_in_event_disable(sensor->rdy_pin);
    }
}

/**
 * Get the ID of the product number.
 * @param sensor Instance to work upon.
 * @param id Pointer to memory filled with the ID.
 * @return True if I2C succeeds and the ID is valid.
 */
bool iqs620_id(iqs620_t *sensor, uint32_t *id)
{
    uint8_t data[3];
    bool ok = iqs620_rregs(sensor, IQS620_ID, data, sizeof(data));
    
    // IQS620A = 0x00410882
    *id = ok ? ((data[0] << 16) | (data[1] << 8) | data[2]) : 0;
    return (ok && (data[0] == 0x41));
}

/**
 * Reset the chip and reconfigure it.
 * @param sensor Instance to work upon.
 */
bool iqs620_reset(iqs620_t *sensor)
{
    bool ok = false;
    unsigned wait_tries = IQS620_RESET_TIMEOUT_MS / 5;

    // disable the RDY event during reset
    iqs620_enable_rdy_handler(sensor, false);

    for (int retry = 0; !ok && (retry < 5); retry++)
    {
        LOG_DEBUG("IQS620 resetting sensor");

        // initiate soft reset
        ok = iqs620_wreg(sensor, IQS620_SYS_SETTINGS, (1 << 7));
        if (ok)
        {
            //nrfx_systick_state_t ticks;
            //nrfx_systick_get(&ticks);

            ok = false;

#if 0 // TODO:
            // wait for the reset, testing every 5ms to see if successful
            while (!ok && !nrfx_systick_test(&ticks, 1000L * IQS620_RESET_TIMEOUT_MS))
            {
                uint32_t id;
                // read the chip ID to verify reset is done
                if (iqs620_id(sensor, &id)) ok = true;
                else nrfx_systick_delay_ms(5);
            }
#endif
            while (!ok && wait_tries > 0)
            {
                uint32_t id;
                // read the chip ID to verify reset is done
                if (iqs620_id(sensor, &id)) {
                    ok = true;
                } else {
                    wait_tries--;
                    nrfx_systick_delay_ms(5);
                }
            }
        }

        // retry after a small delay
        //if (!ok) nrfx_systick_delay_ms(IQS620_RESET_RETRY_MS);
        if (!ok) nrfx_systick_delay_ms(IQS620_RESET_RETRY_MS);
    }

    // enable the RDY event after the reset
    iqs620_enable_rdy_handler(sensor, true);

    if (!ok) NRFX_LOG_ERROR("IQS620 reset failed!");

    return ok;
}

/**
 * Initialise the chip as well as the sensor instance.
 * @param sensor Variable that will hold the instance state.
 * @return True on success.
 */
bool iqs620_init(iqs620_t *sensor)
{
    assert(sensor != NULL);

    // initialize the internal state
    sensor->prox_touch_state = 0;
    sensor->button_status = 0;

    // initialize the SysTick timer
    //nrfx_systick_init();

    // initalize the I2C driver
    // done in i2c.c
    //iqs620_init_twi(sensor);

    // boundary check ati_target (6 bits)
    if(sensor->ati_target > 0x3F) sensor->ati_target = 0x3F;

    // initialize the RDY pin handler
    iqs620_init_rdy_handler(sensor);

    // perform soft reset
    bool ok = iqs620_reset(sensor);
    if (!ok) return false;

    return true;
}

/**
 * Wrapper that mimics the CY8CMBR3 driver for code compatibility.
 * @param sensor Instance to work upon.
 * @param status Pointer filled with the button status.
 * @return True if I2C succeeds.
 */
// TODO: if support for cy8cmbr3 is dropped, could be removed after rework of touch.c code
bool iqs620_get_button_status(iqs620_t *sensor, uint16_t *status)
{
    assert(sensor != NULL);
    *status = sensor->button_status;
    LOG_DEBUG("IQS620 button status = 0x%x.", *status);
    return true;
}

/**
 * Get the raw counts for tuning thresholds.
 * @param sensor Instance to work upon.
 * @param channel Sensor channel number to read the data from
 * @param ch_data Pointer filled with the raw sensor output.
 * @return True if I2C succeeds.
 */
bool iqs620_get_ch_count(iqs620_t *sensor, uint8_t channel, uint16_t *ch_data)
{
    uint8_t data[2];
    // Read 2 bytes from the base address of the channel count register
    bool ok = iqs620_rregs(sensor, channel*2 + IQS620_CHANNEL_COUNT_0_LO, data, sizeof(data));

    *ch_data = ok ? ((data[1] << 8) | data[0]) : 0;
    LOG_DEBUG("IQS620 Channel: %d count: %d", channel, *ch_data);

    return (ok);
}
