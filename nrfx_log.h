/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Glenn Ruben Bakke
 * Copyright (c) 2022-2023 Brilliant Labs Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "mphalport.h"
#include "nrfx_config.h"
#include "SEGGER_RTT.h"
#include <string.h>

#define TEST_MODULE_IMPL(x, y) LOG_TEST_##x == LOG_TEST_##y
#define TEST_MODULE(x, y) TEST_MODULE_IMPL(x, y)

#define VALUE_TO_STR(x) #x
#define VALUE(x) VALUE_TO_STR(x)

static inline void LOG_NONE(void *v, ...) { (void)v; }

#define log_clear() SEGGER_RTT_printf(0, RTT_CTRL_CLEAR "\r");
#define log(format, ...) SEGGER_RTT_printf(0, "\r\n" format, ##__VA_ARGS__)

#define NRFX_LOG_DEBUG LOG_NONE
#define NRFX_LOG_INFO LOG_NONE
#define NRFX_LOG_WARNING LOG_NONE
#define NRFX_LOG_ERROR log

#define NRFX_LOG_ERROR_STRING_GET(error_code) nrfx_error_code_lookup(error_code)
#define NRFX_LOG_HEXDUMP_ERROR(p_memory, length)
#define NRFX_LOG_HEXDUMP_WARNING(p_memory, length)
#define NRFX_LOG_HEXDUMP_INFO(p_memory, length)
#define NRFX_LOG_HEXDUMP_DEBUG(p_memory, length)

const char *nrfx_error_code_lookup(uint32_t err_code);
