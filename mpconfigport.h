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

#include <alloca.h>
#include "mphalport.h"

#define MICROPY_BANNER_MACHINE "Monocle on nRF52832"

#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)

#define MICROPY_NLR_SETJMP (1)
#define MICROPY_GCREGS_SETJMP (1)

// File handling isn't used so we can path strings short
#define MICROPY_ALLOC_PATH_MAX (64)

#define MICROPY_QSTR_BYTES_IN_HASH (2)

#define MICROPY_PERSISTENT_CODE_LOAD (1)

#define MICROPY_ENABLE_COMPILER (1)

#define MICROPY_ENABLE_SCHEDULER (1)

#define MICROPY_COMP_CONST_FOLDING (1)
#define MICROPY_COMP_CONST_TUPLE (1)
#define MICROPY_COMP_CONST_LITERAL (1)
#define MICROPY_COMP_MODULE_CONST (1)
#define MICROPY_COMP_CONST (1)
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN (1)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (1)
#define MICROPY_COMP_RETURN_IF_EXPR (1)

#define MICROPY_ERROR_REPORTING (MICROPY_ERROR_REPORTING_DETAILED)

#define MICROPY_FULL_CHECKS (1)

#define MICROPY_PY_ASYNC_AWAIT (1)
#define MICROPY_PY_UASYNCIO (1)

#define MICROPY_PY_FSTRINGS (1)

#define MICROPY_PY_ASSIGN_EXPR (1)

#define MICROPY_ENABLE_FINALISER (1)

#define MICROPY_USE_READLINE (1)
#define MICROPY_USE_READLINE_HISTORY (1)
#define MICROPY_READLINE_HISTORY_SIZE (10)
#define MICROPY_HELPER_REPL (1)
#define MICROPY_REPL_AUTO_INDENT (1)

#define MICROPY_PY_BUILTINS_STR_UNICODE (1)
#define MICROPY_PY_BUILTINS_STR_CENTER (1)
#define MICROPY_PY_BUILTINS_STR_COUNT (1)
#define MICROPY_PY_BUILTINS_STR_OP_MODULO (1)
#define MICROPY_PY_BUILTINS_STR_PARTITION (1)
#define MICROPY_PY_BUILTINS_STR_SPLITLINES (1)
#define MICROPY_PY_BUILTINS_BYTEARRAY (1)
#define MICROPY_PY_BUILTINS_DICT_FROMKEYS (1)
#define MICROPY_PY_BUILTINS_SET (1)
#define MICROPY_PY_BUILTINS_SLICE (1)
#define MICROPY_PY_BUILTINS_ROUND_INT (1)
#define MICROPY_PY_BUILTINS_ENUMERATE (1)
#define MICROPY_PY_BUILTINS_REVERSED (1)
#define MICROPY_PY_BUILTINS_NOTIMPLEMENTED (1)
#define MICROPY_PY_BUILTINS_MIN_MAX (1)
#define MICROPY_PY_BUILTINS_HELP (1)
#define MICROPY_PY_BUILTINS_HELP_TEXT help_text
#define MICROPY_PY_BUILTINS_HELP_MODULES (1)

#define MICROPY_ENABLE_EXTERNAL_IMPORT (1)
#define MICROPY_QSTR_EXTRA_POOL mp_qstr_frozen_const_pool
#define MICROPY_MODULE_FROZEN_MPY (1)
#define MICROPY_PY_BUILTINS_FROZENSET (1)

#define MICROPY_PY_GC (1)
#define MICROPY_ENABLE_GC (1)
#define MICROPY_GC_ALLOC_THRESHOLD (1)

#define MICROPY_PY_MATH (1)
#define MICROPY_PY_MATH_CONSTANTS (1)
#define MICROPY_PY_MATH_ISCLOSE (1)
#define MICROPY_PY_BUILTINS_COMPLEX (0)
#define MICROPY_FLOAT_IMPL (MICROPY_FLOAT_IMPL_FLOAT)
#define MICROPY_LONGINT_IMPL (MICROPY_LONGINT_IMPL_MPZ)
#define MP_NEED_LOG2 (1)

#define MICROPY_PY_UBINASCII (1)
#define MICROPY_PY_UBINASCII_CRC32 (1)

#define MICROPY_PY_SYS (1)
#define MICROPY_PY_SYS_EXIT (0)
#define MICROPY_PY_SYS_MODULES (0)

#define MICROPY_PY_UERRNO (1)

#define MICROPY_PY_UHASHLIB (1)

#define MICROPY_PY_IO (1)

#define MICROPY_PY_UJSON (1)

#define MICROPY_PY_URANDOM (1)
#define MICROPY_PY_URANDOM_EXTRA_FUNCS (1)
#define MICROPY_PY_URANDOM_SEED_INIT_FUNC (mp_hal_generate_random_seed())

#define MICROPY_PY_URE (1)
#define MICROPY_PY_URE_SUB (1)
#define MICROPY_PY_UHEAPQ (1)

#define MICROPY_PY_UTIME_MP_HAL (1)

#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void *)((mp_uint_t)(p) | 1))

#define MICROPY_EPOCH_IS_1970 (1)

#define MP_SSIZE_MAX (0x7fffffff)

#define MP_STATE_PORT MP_STATE_VM

#define MICROPY_EVENT_POLL_HOOK              \
    do                                       \
    {                                        \
        extern void mp_handle_pending(bool); \
        mp_handle_pending(true);             \
        __WFI();                             \
    } while (0);
