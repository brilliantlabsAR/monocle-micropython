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

#pragma once

#include "py/mpconfig.h"
#include "nrf.h"

// For every module, it might be necessary to set the IRQ priority to
// 7 so that the Nordic Uart Service (NUS) gets a higher priority.

#define NRFX_LOG_ENABLED 1
#define NRFX_LOG_UART_DISABLED 1

#define NRFX_GPIOTE_ENABLED 1
#define NRFX_GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS 1
#define NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY 7

// Used by driver/i2c.c
#define NRFX_TWI_ENABLED 1
#define NRFX_TWI0_ENABLED 1
#define NRFX_TWI1_ENABLED 1
#define NRFX_TWI2_ENABLED 0
#define NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY 7

#define NRFX_TWIM_ENABLED 1
#define NRFX_TWIM0_ENABLED 1
#define NRFX_TWIM1_ENABLED 1
#define NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY 7

// Used by driver/spi.c
#define NRFX_SPI_ENABLED 0
#define NRFX_SPIM_ENABLED 1
#define NRFX_SPIM0_ENABLED 0
#define NRFX_SPIM1_ENABLED 0
#define NRFX_SPIM2_ENABLED 1
#define NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY 7

#define NRFX_RTC_ENABLED 1
#define NRFX_RTC0_ENABLED 1
#define NRFX_RTC1_ENABLED 1
#define NRFX_RTC2_ENABLED 1
#define NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY 7

#define NRFX_SYSTICK_ENABLED 1
#define NRFX_SYSTICK_DEFAULT_CONFIG_IRQ_PRIORITY 7

#define NRFX_TIMER_ENABLED 1
#define NRFX_TIMER0_ENABLED 1 // Used by the SoftDevice
#define NRFX_TIMER1_ENABLED 1 // Used for "from machine import Timer"
#define NRFX_TIMER2_ENABLED 1 // Used by driver/touch.c
#define NRFX_TIMER4_ENABLED 1 // Used for checking battery state
#define NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY 7

#define NRFX_NVMC_ENABLED 1

// Peripheral Resource Sharing
#define NRFX_PRS_BOX_0_ENABLED (NRFX_TWI_ENABLED && NRFX_TWI0_ENABLED && NRFX_SPI_ENABLED && NRFX_SPI0_ENABLED)
#define NRFX_PRS_BOX_1_ENABLED (NRFX_TWI_ENABLED && NRFX_TWI1_ENABLED && NRFX_SPI_ENABLED && NRFX_SPI1_ENABLED)
#define NRFX_PRS_BOX_2_ENABLED (NRFX_TWI_ENABLED && NRFX_TWI1_ENABLED && NRFX_SPI_ENABLED && NRFX_SPI1_ENABLED)
#define NRFX_PRS_ENABLED (NRFX_PRS_BOX_0_ENABLED || NRFX_PRS_BOX_1_ENABLED || NRFX_PRS_BOX_2_ENABLED)

// Used by driver/battery.c
#define NRFX_SAADC_ENABLED 1
#define NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY 7