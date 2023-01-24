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

#include "nrfx_log.h"

static const char nrfx_error_unknown[1] = "";

static const char nrfx_error_success[] = "NRFX_SUCCESS";
static const char nrfx_error_internal[] = "NRFX_ERROR_INTERNAL";
static const char nrfx_error_no_mem[] = "NRFX_ERROR_NO_MEM";
static const char nrfx_error_not_supported[] = "NRFX_ERROR_NOT_SUPPORTED";
static const char nrfx_error_invalid_param[] = "NRFX_ERROR_INVALID_PARAM";
static const char nrfx_error_invalid_state[] = "NRFX_ERROR_INVALID_STATE";
static const char nrfx_error_invalid_length[] = "NRFX_ERROR_INVALID_LENGTH";
static const char nrfx_error_timeout[] = "NRFX_ERROR_TIMEOUT";
static const char nrfx_error_forbidden[] = "NRFX_ERROR_FORBIDDEN";
static const char nrfx_error_null[] = "NRFX_ERROR_NULL";
static const char nrfx_error_invalid_addr[] = "NRFX_ERROR_INVALID_ADDR";
static const char nrfx_error_busy[] = "NRFX_ERROR_BUSY";
static const char nrfx_error_already_initalized[] = "NRFX_ERROR_ALREADY_INITIALIZED";

static const char *nrfx_error_strings[13] = {
    nrfx_error_success,
    nrfx_error_internal,
    nrfx_error_no_mem,
    nrfx_error_not_supported,
    nrfx_error_invalid_param,
    nrfx_error_invalid_state,
    nrfx_error_invalid_length,
    nrfx_error_timeout,
    nrfx_error_forbidden,
    nrfx_error_null,
    nrfx_error_invalid_addr,
    nrfx_error_busy,
    nrfx_error_already_initalized
};

static const char nrfx_drv_error_twi_err_overrun[] = "NRFX_ERROR_DRV_TWI_ERR_OVERRUN";
static const char nrfx_drv_error_twi_err_anack[] = "NRFX_ERROR_DRV_TWI_ERR_ANACK";
static const char nrfx_drv_error_twi_err_dnack[] = "NRFX_ERROR_DRV_TWI_ERR_DNACK";

static const char *nrfx_drv_error_strings[3] = {
    nrfx_drv_error_twi_err_overrun,
    nrfx_drv_error_twi_err_anack,
    nrfx_drv_error_twi_err_dnack
};

const char *nrfx_error_code_lookup(uint32_t err_code)
{
    if (err_code >= NRFX_ERROR_BASE_NUM && err_code <= NRFX_ERROR_BASE_NUM + 13)
    {
        return nrfx_error_strings[err_code - NRFX_ERROR_BASE_NUM];
    }
    else if (err_code >= NRFX_ERROR_DRIVERS_BASE_NUM && err_code <= NRFX_ERROR_DRIVERS_BASE_NUM + 3)
    {
        return nrfx_drv_error_strings[err_code - NRFX_ERROR_DRIVERS_BASE_NUM];
    }

    return nrfx_error_unknown;
}
