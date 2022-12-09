/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

/**
 * @file driver_battery.c
 * @author Nathan Ashelman
 * @author Josuah Demangeon
 * @todo Not enabled yet.
 * @bug A timer might need to be configured to periodically trigger the sampling.
 */

#include <math.h>

#include "driver_board.h"
#include "driver_battery.h"
#include "driver_timer.h"
#include "driver_config.h"

#include "nrfx_saadc.h"
#include "nrfx_systick.h"
#include "nrfx_log.h"

#define LOG NRFX_LOG_ERROR
#define ASSERT BOARD_ASSERT

/*
 * Lithium battery discharge curve, modeled from Grepow data for 1C discharge rate
 * Requirement: x-values (i.e. voltage) must be stricly increasing
 */

/** Table of voltages to match with percentages below. */
static float battery_voltage_table[] = {
    3.0, 3.3, 3.35,  3.4, 3.43, 3.48, 3.54, 3.64, 3.76, 3.90, 4.02, 4.13,  4.25
};

/** Tables of percentages to match with voltages above. */
static float battery_percent_table[] = {
    0.0, 4.0,  6.0, 12.0, 17.0, 28.0, 39.0, 51.0, 62.0, 74.0, 85.0, 96.0, 100.0
};

/** Number of points derived from the battery_voltage_table and battery_percent_table */
const uint8_t battery_points = sizeof battery_voltage_table / sizeof *battery_voltage_table;

/** Input resistor divider, high value resistance in kOhm, from MK9B R2 */
#define R_HI (4.8 - 1.25)

/** Input resistor divider, low value resistance in kOhm, from MK9B R3 */
#define R_LO 1.25

// Depends on R_HI
// https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.nrf52832.ps.v1.1/saadc.html?cp=4_2_0_36_8#concept_qh3_spp_qr

/** VDD=1.8V divided by 4 as reference. */
#define BATTERY_ADC_REFERENCE NRF_SAADC_REFERENCE_VDD4
#define BATTERY_REFERENCE (1.8 / 4.0)

/** Gain 1/4, so input range = VDD (full range). */
#define BATTERY_ADC_GAIN NRF_SAADC_GAIN1_4
#define BATTERY_GAIN (1.0 / 4.0)

/** Resolution of the ADC: for a 10-bit ADC, the resolution is 1 << 10 = 1024. */
#define BATTERY_ADC_RESOLUTION NRF_SAADC_RESOLUTION_10BIT

/*
 * These values are averages, taken over time determined by sampling time
 * (in main code) and p_event->data.done.size. They will only be valid after an
 * initial time (currently 5 seconds).
 */

/* stores battery voltage, expressed in Volts */
static float battery_voltage = 0;

/* Stores battery state-of-charge, expressed in percent (0-100) */
static uint8_t battery_percent = 0;

/** Used by the sampling callback. */
nrf_saadc_value_t adc_buffer;

typedef struct {
    uint8_t x_length;
    float *x_values;
    float *y_values;
} table_1d_t;

/*
 * Declare variable using above structure and the battery discharge function datapoints
 */
static table_1d_t battery_table = {
    battery_points,       /* Number of data points */
    battery_voltage_table,  /* Array of x-coordinates */
    battery_percent_table   /* Array of y-coordinates */
};

/**
 * Interpolate a value at the middle of a segment.
 * For a segment [(x0, y0), (x1, y1)], this will compute the Y value for a queried X position on that segment.
 * @param x0 Coordinate X for the first point of the segment.
 * @param y0 Coordinate Y for the first point of the segment.
 * @param x1 Coordinate X for the second point of the segment.
 * @param y1 Coordinate Y for the second point of the segment.
 * @param x Coordinate X for the position to query.
 * @return The Y position associated with x, or y0 or y1 if out of bound.
 */
static float interpolate_segment(float x0, float y0, float x1, float y1, float x)
{
    float t = 0;

    if (x <= x0)
        return y0;
    if (x >= x1)
        return y1;
    t = (x - x0) / (x1 - x0);
    return y0 + t * (y1 - y0);
}

/*
 * 1D Table lookup with interpolation
 */
static float interpolate_table_1d(table_1d_t *table, float x)
{
    uint8_t segment;

    // Check input bounds and saturate if out-of-bounds
    if (x > (table->x_values[table->x_length-1])) {
        // x-value too large, saturate to max y-value
        return table->y_values[table->x_length-1];
    }
    else if (x < (table->x_values[0])) {
        // x-value too small, saturate to min y-value
        return table->y_values[0];
    }

    // Find the segment that holds x
    for (segment = 0; segment<(table->x_length-1); segment++)
    {
        if ((table->x_values[segment]   <= x) &&
            (table->x_values[segment+1] >= x))
        {
            // Found the correct segment, interpolate
            return interpolate_segment(table->x_values[segment],   // x0
                                       table->y_values[segment],   // y0
                                       table->x_values[segment+1], // x1
                                       table->y_values[segment+1], // y1
                                       x);                         // x
        }
    }

    // Something with the data was wrong if we get here
    // Saturate to the max value
    return table->y_values[table->x_length-1];
}

static float battery_voltage_to_percent(float voltage)
{
    return interpolate_table_1d(&battery_table, voltage);
}

/*
 * see https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52832.ps.v1.1%2Fsaadc.html
 */
static float battery_saadc_to_voltage(nrf_saadc_value_t reading)
{
    float voltage = 0;
    float factor = 0;

    // observe -1 to -4 raw ADC readings when input grounded
    if (reading < 0)
        reading = 0;

    LOG(
        "R_HI=%d R_LO=%d R_EQ=%d ADJ=%d",
        (double)R_HI,
        (double)R_LO,
        (double)((R_HI + R_LO) / (R_LO)),
        (double)(BATTERY_REFERENCE / (BATTERY_GAIN * BATTERY_ADC_RESOLUTION))
    );
    factor = ((R_HI + R_LO) / (R_LO)) * (BATTERY_REFERENCE / (BATTERY_GAIN * BATTERY_ADC_RESOLUTION));

    voltage = reading * factor;
    return voltage;
}

static void saadc_callback(nrfx_saadc_evt_t const *p_event)
{
    uint32_t err;
    nrf_saadc_value_t average_level = 0;

    switch (NRFX_SAADC_EVT_DONE) {
        case NRFX_SAADC_EVT_BUF_REQ:
            err = nrfx_saadc_buffer_set(&adc_buffer, 1);
            ASSERT(err == NRFX_SUCCESS);
            break;
        case NRFX_SAADC_EVT_DONE:
            for (int i = 0; i < p_event->data.done.size; i++) {
                average_level += p_event->data.done.p_buffer[i];
                LOG("buffer[%d]=%ud average_level=%ud", i, p_event->data.done.p_buffer[i], average_level);
            }
            // This truncates, doesn't round up:
            average_level = average_level / p_event->data.done.size;
            battery_voltage = battery_saadc_to_voltage(average_level);
            battery_percent = round(battery_voltage_to_percent(battery_voltage));
            LOG("Batt average (ADC raw): %u", average_level);
            LOG("Batt average (voltage): %f", (double)battery_voltage);
            LOG("Batt percent: %d%%", battery_percent);

            // Enqueue another sampling
            err = nrfx_saadc_mode_trigger();
            ASSERT(err == NRFX_SUCCESS);

            break;
        default:
            ASSERT(!"unhandled event");
            break;
    }
}

/**
 * Get current, precomputed state-of-charge of the battery.
 * @return Remaining percentage of the battery.
 */
uint8_t battery_get_percent(void)
{
    // Everything is handled by the ADC callback above.
    return battery_percent;
}

void battery_timer_handler(void)
{
    uint32_t err;

    // Start the trigger chain: the callback will trigger another callback
    err = nrfx_saadc_mode_trigger();
    NRFX_ASSERT(err == NRFX_SUCCESS);
}

/**
 * Initialize the ADC.
 * This includes setting up buffering.
 */
void battery_init(void)
{
    uint32_t err;
    nrfx_saadc_channel_t channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(IO_ADC_VBATT, 0);

    channel.channel_config.reference = BATTERY_ADC_REFERENCE;
    channel.channel_config.gain = BATTERY_ADC_GAIN;

    err = nrfx_saadc_init(0);
    ASSERT(err == NRFX_SUCCESS);

    err = nrfx_saadc_channels_config(&channel, 1);
    ASSERT(err == NRFX_SUCCESS);

    // Configure first ADC channel with low setup (enough for battery sensing)
    err = nrfx_saadc_simple_mode_set(1u << 0, BATTERY_ADC_RESOLUTION,
        NRF_SAADC_OVERSAMPLE_DISABLED, saadc_callback);
    ASSERT(err == NRFX_SUCCESS);

    // Provide a buffer used internally by the NRFX driver
    err = nrfx_saadc_buffer_set(&adc_buffer, 1);
    NRFX_ASSERT(err == NRFX_SUCCESS);

    timer_add_handler(&battery_timer_handler);

    LOG("ready: nrfx=saadc dep=timer");
}
