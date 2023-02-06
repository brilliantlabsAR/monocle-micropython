/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Inc (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Inc.
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

#include "py/builtin.h"
#include "py/mperrno.h"
#include "py/lexer.h"
#include "py/runtime.h"
#include "mpconfigport.h"
#include "bluetooth.h"
#include "nrfx_timer.h"
#include "nrfx_systick.h"
#include "nrf_soc.h"

const char help_text[] = {
    "Welcome to MicroPython!\n\n"
    "For full documentation, visit: https://docs.brilliantmonocle.com\n"
    "Control commands:\n"
    "  Ctrl-A - enter raw REPL mode\n"
    "  Ctrl-B - enter normal REPL mode\n"
    "  CTRL-C - interrupt a running program\n"
    "  Ctrl-D - reset the device\n"
    "  Ctrl-E - enter paste mode\n\n"
    "To list available modules, type help('modules')\n"
    "For details on a specific module, import it, and then type "
    "help(module_name)\n"};

// this overflows after 49 days without reboot.
static volatile uint32_t uptime_ms;

void mp_hal_timer_1ms_callback(nrf_timer_event_t event, void *context)
{
    (void)event;
    uptime_ms++;
}

mp_uint_t mp_hal_ticks_ms(void)
{
    return uptime_ms;
}

mp_uint_t mp_hal_ticks_cpu(void)
{
    return 0;
}

uint64_t mp_hal_time_ns(void)
{
    return 0;
}

void mp_hal_delay_ms(mp_uint_t ms)
{
    for (uint64_t step; ms > 0; ms -= step)
    {
        step = MIN(ms, UINT32_MAX);
        nrfx_systick_delay_ms(step);
    }
}

void mp_hal_delay_us(mp_uint_t us)
{
    for (uint64_t step; us > 0; us -= step)
    {
        step = MIN(us, UINT32_MAX);
        nrfx_systick_delay_us(step);
    }
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs)
{
    // File opening is currently not supported
    mp_raise_OSError(MP_EPERM);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

mp_lexer_t *mp_lexer_new_from_file(const char *filename)
{
    // File opening is currently not supported
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path)
{
    // File opening is currently not supported
    return MP_IMPORT_STAT_NO_EXIST;
}

/**
 * Send a buffer out, retrying continuously until it goes to completion (with success or failure).
 */
static void ble_tx(ble_gatts_char_handles_t *tx_char, uint8_t const *buf, uint16_t len)
{
    nrfx_err_t err;
    ble_gatts_hvx_params_t hvx_params = {
        .handle = tx_char->value_handle,
        .p_data = buf,
        .p_len = (uint16_t *)&len,
        .type = BLE_GATT_HVX_NOTIFICATION,
    };

    do
    {
        app_err(ble_conn_handle == BLE_CONN_HANDLE_INVALID);

        // Send the data
        err = sd_ble_gatts_hvx(ble_conn_handle, &hvx_params);

        // Retry if resources are unavailable.
    } while (err == NRF_ERROR_RESOURCES);

    // Ignore errors if not connected
    if (err == NRF_ERROR_INVALID_STATE || err == BLE_ERROR_INVALID_CONN_HANDLE)
        return;

    // Catch other errors
    app_err(err);
}

/**
 * Sends all buffered data in the tx ring buffer over BLE.
 */
static void ble_nus_flush_tx(void)
{
    // Local buffer for sending data
    uint8_t buf[BLE_MAX_MTU_LENGTH] = "";
    uint16_t len = 0;

    // If not connected, do not flush.
    if (ble_conn_handle == BLE_CONN_HANDLE_INVALID)
        return;

    // If there's no data to send, simply return
    if (ring_empty(&ble_nus_tx))
        return;

    // For all the remaining characters, i.e until the heads come back together
    while (!ring_empty(&ble_nus_tx))
    {
        // Copy over a character from the tail to the outgoing buffer
        buf[len++] = ring_pop(&ble_nus_tx);

        // Break if we over-run the negotiated MTU size, send the rest later
        if (len >= ble_negotiated_mtu)
            break;
    }
    ble_tx(&ble_nus_tx_char, buf, len);
}

int mp_hal_stdin_rx_chr(void)
{
    while (ring_empty(&ble_nus_rx))
    {
        // While waiting for incoming data, we can push outgoing data
        ble_nus_flush_tx();

        // If there's nothing to do
        if (ring_empty(&ble_nus_tx) && ring_empty(&ble_nus_rx))
            // Wait for events to save power
            sd_app_evt_wait();
    }

    // Return next character from the RX buffer.
    return ring_pop(&ble_nus_rx);
}

void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        while (ring_full(&ble_nus_tx))
            ble_nus_flush_tx();
        ring_push(&ble_nus_tx, str[i]);
    }
}

void mp_hal_set_interrupt_char(char c)
{
    (void)c;
}

// TODO
int mp_hal_generate_random_seed(void)
{
    return 0;
}
