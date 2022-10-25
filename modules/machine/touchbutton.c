#include "monocle_iqs620.h"
#include "nrfx_log.h"
#include "py/obj.h"
#include "py/qstr.h"
#include "py/runtime.h"

#define LOG NRFX_LOG_ERROR

struct {
    mp_obj_t callback;
} machine_touchbutton;

/**
 * Overriding the default callback implemented in monocle_iqs620.c
 * @param button The button related to this event.
 * @param event The event triggered for that button.
 */
void iqs620_callback(iqs620_button_t button, iqs620_event_t event)
{
    LOG("button=0x%02X event=0x%02X", button, event);

    // Issue the callback.
    mp_call_function_2(machine_touchbutton.callback, MP_OBJ_NEW_SMALL_INT(button), MP_OBJ_NEW_SMALL_INT(event));
}

void machine_touchbutton_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)self_in;
    (void)kind;

    mp_printf(print, "TouchButton()");
}

STATIC mp_obj_t machine_touchbutton_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    // Allowed arguments table.
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_callback, MP_ARG_REQUIRED | MP_ARG_OBJ},
    };

    // Parse args.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Extract the function callback from the arguments.
    machine_touchbutton.callback = args[0].u_obj;

    // Return the newly created TouchButton object.
    return MP_OBJ_FROM_PTR(&machine_touchbutton);
}

MP_DEFINE_CONST_OBJ_TYPE(
    machine_touchbutton_type,
    MP_QSTR_TouchButton,
    MP_TYPE_FLAG_NONE,
    make_new, machine_touchbutton_make_new,
    print, machine_touchbutton_print
);
