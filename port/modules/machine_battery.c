#include "monocle_iqs620.h"
#include "monocle_battery.h"
#include "nrfx_log.h"
#include "py/obj.h"
#include "py/qstr.h"
#include "py/runtime.h"

#define LOG NRFX_LOG_ERROR

STATIC mp_obj_t machine_battery_level(void) {
    return MP_OBJ_NEW_SMALL_INT(battery_get_percent());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_battery_level_obj, machine_battery_level);

STATIC const mp_rom_map_elem_t machine_battery_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_level),     MP_ROM_PTR(&machine_battery_level_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_battery_locals_dict, machine_battery_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_battery_type,
    MP_QSTR_Battery,
    MP_TYPE_FLAG_NONE,
    locals_dict, &machine_battery_locals_dict
);
