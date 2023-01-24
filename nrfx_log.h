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

#ifndef NRFX_LOG_H
#define NRFX_LOG_H

#include <stdio.h>
#include <stdarg.h>
#include "mphalport.h"
#include "nrfx_config.h"
#include "SEGGER_RTT.h"
#include <string.h>

#define TEST_MODULE_IMPL(x, y) LOG_TEST_##x == LOG_TEST_##y
#define TEST_MODULE(x, y) TEST_MODULE_IMPL(x, y)

#define VALUE_TO_STR(x) #x
#define VALUE(x) VALUE_TO_STR(x)

static inline void LOG_NONE(void *v, ...) { (void)v; }

#define LOG_CLEAR() SEGGER_RTT_printf(0, RTT_CTRL_CLEAR "\r");

/**
 * Local implementation as there is no support for it in the micropython libc.
 */
static inline size_t _rttlen(const char *s, size_t n)
{
    const char *p = memchr(s, '\0', n);

    return p == NULL ? n : p - s;
}

#define LOG_RAW(format, ...)                                                                \
    do                                                                                      \
    {                                                                                       \
        char _debug_log_buffer[BUFFER_SIZE_UP] = "";                                        \
        snprintf(_debug_log_buffer, BUFFER_SIZE_UP, format, ##__VA_ARGS__);                 \
        SEGGER_RTT_Write(0, _debug_log_buffer, _rttlen(_debug_log_buffer, BUFFER_SIZE_UP)); \
    } while (0)

#define LOG(format, ...) LOG_RAW("\r\n" format, ##__VA_ARGS__)

#define PRINTF(fmt, ...) SEGGER_RTT_printf(0, fmt, ##__VA_ARGS__)
// #define LOG(fmt, ...)           PRINTF("%s: " fmt "\n", __func__, ## __VA_ARGS__)

#define NRFX_LOG_DEBUG LOG_NONE
#define NRFX_LOG_INFO LOG_NONE
#define NRFX_LOG_WARNING LOG_NONE
#define NRFX_LOG_ERROR LOG

#define NRFX_LOG_ERROR_STRING_GET(error_code) nrfx_error_code_lookup(error_code)
#define NRFX_LOG_HEXDUMP_ERROR(p_memory, length)
#define NRFX_LOG_HEXDUMP_WARNING(p_memory, length)
#define NRFX_LOG_HEXDUMP_INFO(p_memory, length)
#define NRFX_LOG_HEXDUMP_DEBUG(p_memory, length)

const char *nrfx_error_code_lookup(uint32_t err_code);

#endif
