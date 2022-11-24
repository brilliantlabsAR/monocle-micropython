#include "monocle_iqs620.h"
#include "monocle_battery.h"
#include "nrfx_log.h"
#include "py/obj.h"
#include "py/qstr.h"
#include "py/runtime.h"

#define LOG NRFX_LOG_ERROR

/**
 * Get the current battery voltage of the monocle.
 * @return Voltage as a float.
 */
STATIC mp_obj_t machine_battery_voltage(void) {
    return mp_obj_new_float_from_f(battery_get_voltage());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_battery_voltage_obj, machine_battery_voltage);

/**
 * Get the current battery percent of the monocle.
 * @return Percentage as an integer.
 */
STATIC mp_obj_t machine_battery_percent(void) {
    return MP_OBJ_NEW_SMALL_INT(battery_get_percent());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_battery_percent_obj, machine_battery_percent);

STATIC void machine_battery_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)self_in;
    (void)kind;

    mp_printf(print, "Battery(voltage=%.2lf percent=%d%%)",
        (double)battery_get_voltage(), battery_get_percent());
}

STATIC mp_obj_t machine_battery_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    (void)type;
    (void)all_args;

    // Parse args.
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // Return the newly created object.
    return MP_OBJ_FROM_PTR(NULL);
}

STATIC const mp_rom_map_elem_t machine_battery_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_voltage),     MP_ROM_PTR(&machine_battery_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_percent),     MP_ROM_PTR(&machine_battery_percent_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_battery_locals_dict, machine_battery_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_battery_type,
    MP_QSTR_Battery,
    MP_TYPE_FLAG_NONE,
    make_new, machine_battery_make_new,
    print, machine_battery_print,
    locals_dict, &machine_battery_locals_dict
);
