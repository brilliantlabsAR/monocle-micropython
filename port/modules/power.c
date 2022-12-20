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

#include <stdint.h>

#include "py/obj.h"
#include "py/objarray.h"
#include "py/runtime.h"
#include "shared/runtime/pyexec.h"

#include "nrfx_log.h"
#include "nrf_soc.h"
#include "nrfx_reset_reason.h"

#include "driver_board.h"
#include "driver_ov5640.h"
#include "driver_ecx336cn.h"
#include "driver_max77654.h"
#include "modules.h"

#define PYB_RESET_HARD      (0)
#define PYB_RESET_WDT       (1)
#define PYB_RESET_SOFT      (2)
#define PYB_RESET_LOCKUP    (3)
#define PYB_RESET_POWER_ON  (16)
#define PYB_RESET_LPCOMP    (17)
#define PYB_RESET_DIF       (18)
#define PYB_RESET_NFC       (19)

enum {
    RESET_BOOTUP,
    RESET_SOFTWARE,
    RESET_OTHER
};

STATIC mp_obj_t power_hibernate(void)
{
    return mp_const_notimplemented;
}
MP_DEFINE_CONST_FUN_OBJ_0(power_hibernate_obj, &power_hibernate);

NORETURN STATIC mp_obj_t power_reset(void)
{
    board_deinit();
    NVIC_SystemReset();
}
MP_DEFINE_CONST_FUN_OBJ_0(power_reset_obj, &power_reset);

STATIC uint32_t reset_cause;

void power_init(void)
{
    uint32_t state = NRF_POWER->RESETREAS;
    if (state & POWER_RESETREAS_RESETPIN_Msk) {
        reset_cause = PYB_RESET_HARD;
    } else if (state & POWER_RESETREAS_DOG_Msk) {
        reset_cause = PYB_RESET_WDT;
    } else if (state & POWER_RESETREAS_SREQ_Msk) {
        reset_cause = PYB_RESET_SOFT;
    } else if (state & POWER_RESETREAS_LOCKUP_Msk) {
        reset_cause = PYB_RESET_LOCKUP;
    } else if (state & POWER_RESETREAS_OFF_Msk) {
        reset_cause = PYB_RESET_POWER_ON;
    } else if (state & POWER_RESETREAS_DIF_Msk) {
        reset_cause = PYB_RESET_DIF;
    }

    // clear reset reason
    NRF_POWER->RESETREAS = (1 << reset_cause);
}

STATIC mp_obj_t power_reset_cause(void)
{
    return MP_OBJ_NEW_SMALL_INT(reset_cause);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(power_reset_cause_obj, power_reset_cause);

STATIC mp_obj_t power_shutdown(mp_obj_t timeout)
{
    (void)timeout;
    return mp_const_notimplemented;
}
MP_DEFINE_CONST_FUN_OBJ_1(power_shutdown_obj, &power_shutdown);

STATIC mp_obj_t power_soft_reset(void)
{
    pyexec_system_exit = PYEXEC_FORCED_EXIT;
    mp_raise_type(&mp_type_SystemExit);
}
MP_DEFINE_CONST_FUN_OBJ_0(power_soft_reset_obj, power_soft_reset);

STATIC mp_obj_t power_lightsleep(void)
{
    __WFE();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(power_lightsleep_obj, power_lightsleep);

STATIC mp_obj_t power_deepsleep(void)
{
    __WFI();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(power_deepsleep_obj, power_deepsleep);

STATIC mp_obj_t power_enable_irq(void)
{
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(power_enable_irq_obj, power_enable_irq);

// Resets the board in a manner similar to pushing the external RESET button.
STATIC mp_obj_t power_disable_irq(void)
{
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(power_disable_irq_obj, power_disable_irq);

STATIC const mp_rom_map_elem_t power_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_hibernate),           MP_ROM_PTR(&power_hibernate_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset),               MP_ROM_PTR(&power_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_cause),         MP_ROM_PTR(&power_reset_cause_obj) },
    { MP_ROM_QSTR(MP_QSTR_shutdown),            MP_ROM_PTR(&power_shutdown_obj) },

    // tmp
    { MP_ROM_QSTR(MP_QSTR_reset),               MP_ROM_PTR(&power_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_soft_reset),          MP_ROM_PTR(&power_soft_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_irq),          MP_ROM_PTR(&power_enable_irq_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable_irq),         MP_ROM_PTR(&power_disable_irq_obj) },
    { MP_ROM_QSTR(MP_QSTR_idle),                MP_ROM_PTR(&power_lightsleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep),               MP_ROM_PTR(&power_lightsleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_lightsleep),          MP_ROM_PTR(&power_lightsleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_deepsleep),           MP_ROM_PTR(&power_deepsleep_obj) },

    // reset causes
    { MP_ROM_QSTR(MP_QSTR_HARD_RESET),          MP_ROM_INT(PYB_RESET_HARD) },
    { MP_ROM_QSTR(MP_QSTR_WDT_RESET),           MP_ROM_INT(PYB_RESET_WDT) },
    { MP_ROM_QSTR(MP_QSTR_SOFT_RESET),          MP_ROM_INT(PYB_RESET_SOFT) },
    { MP_ROM_QSTR(MP_QSTR_LOCKUP_RESET),        MP_ROM_INT(PYB_RESET_LOCKUP) },
    { MP_ROM_QSTR(MP_QSTR_PWRON_RESET),         MP_ROM_INT(PYB_RESET_POWER_ON) },
    { MP_ROM_QSTR(MP_QSTR_LPCOMP_RESET),        MP_ROM_INT(PYB_RESET_LPCOMP) },
    { MP_ROM_QSTR(MP_QSTR_DEBUG_IF_RESET),      MP_ROM_INT(PYB_RESET_DIF) },

    { MP_ROM_QSTR(MP_QSTR_RESET_BOOTUP),        MP_OBJ_NEW_SMALL_INT(RESET_BOOTUP) },
    { MP_ROM_QSTR(MP_QSTR_RESET_SOFTWARE),      MP_OBJ_NEW_SMALL_INT(RESET_SOFTWARE) },
    { MP_ROM_QSTR(MP_QSTR_RESET_OTHER),         MP_OBJ_NEW_SMALL_INT(RESET_OTHER) },
};
STATIC MP_DEFINE_CONST_DICT(power_locals_dict, power_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    power_type,
    MP_QSTR_Power,
    MP_TYPE_FLAG_NONE,
    locals_dict, &power_locals_dict
);
