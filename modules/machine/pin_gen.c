// nrf52_prefix.c becomes the initial portion of the generated pins file.

#include <stdio.h>

#include "py/obj.h"
#include "py/mphal.h"
#include "pin.h"

#define PIN(p_pin, p_af, p_adc_num, p_adc_channel) \
{ \
    { &pin_type }, \
    .name = MP_QSTR_P ## p_pin, \
    .pin = (p_pin), \
    .num_af = (sizeof(p_af) / sizeof(pin_af_obj_t)), \
    .af = p_af, \
    .adc_num = p_adc_num, \
    .adc_channel = p_adc_channel, \
}


const uint8_t machine_pin_num_of_board_pins = 30;

const pin_obj_t machine_board_pin_obj[30] = {
  PIN(2, NULL, PIN_ADC1, 0),
  PIN(3, NULL, PIN_ADC1, 1),
  PIN(4, NULL, PIN_ADC1, 2),
  PIN(5, NULL, PIN_ADC1, 3),
  PIN(6, NULL, 0, 0),
  PIN(7, NULL, 0, 0),
  PIN(8, NULL, 0, 0),
  PIN(9, NULL, 0, 0),
  PIN(10, NULL, 0, 0),
  PIN(11, NULL, 0, 0),
  PIN(12, NULL, 0, 0),
  PIN(13, NULL, 0, 0),
  PIN(14, NULL, 0, 0),
  PIN(15, NULL, 0, 0),
  PIN(16, NULL, 0, 0),
  PIN(17, NULL, 0, 0),
  PIN(18, NULL, 0, 0),
  PIN(19, NULL, 0, 0),
  PIN(20, NULL, 0, 0),
  PIN(21, NULL, 0, 0),
  PIN(22, NULL, 0, 0),
  PIN(23, NULL, 0, 0),
  PIN(24, NULL, 0, 0),
  PIN(25, NULL, 0, 0),
  PIN(26, NULL, 0, 0),
  PIN(27, NULL, 0, 0),
  PIN(28, NULL, PIN_ADC1, 4),
  PIN(29, NULL, PIN_ADC1, 5),
  PIN(30, NULL, PIN_ADC1, 6),
  PIN(31, NULL, PIN_ADC1, 7),
};
STATIC const mp_rom_map_elem_t pin_cpu_pins_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_P2), MP_ROM_PTR(&machine_board_pin_obj[0]) },
  { MP_ROM_QSTR(MP_QSTR_P3), MP_ROM_PTR(&machine_board_pin_obj[1]) },
  { MP_ROM_QSTR(MP_QSTR_P4), MP_ROM_PTR(&machine_board_pin_obj[2]) },
  { MP_ROM_QSTR(MP_QSTR_P5), MP_ROM_PTR(&machine_board_pin_obj[3]) },
  { MP_ROM_QSTR(MP_QSTR_P6), MP_ROM_PTR(&machine_board_pin_obj[4]) },
  { MP_ROM_QSTR(MP_QSTR_P7), MP_ROM_PTR(&machine_board_pin_obj[5]) },
  { MP_ROM_QSTR(MP_QSTR_P8), MP_ROM_PTR(&machine_board_pin_obj[6]) },
  { MP_ROM_QSTR(MP_QSTR_P9), MP_ROM_PTR(&machine_board_pin_obj[7]) },
  { MP_ROM_QSTR(MP_QSTR_P10), MP_ROM_PTR(&machine_board_pin_obj[8]) },
  { MP_ROM_QSTR(MP_QSTR_P11), MP_ROM_PTR(&machine_board_pin_obj[9]) },
  { MP_ROM_QSTR(MP_QSTR_P12), MP_ROM_PTR(&machine_board_pin_obj[10]) },
  { MP_ROM_QSTR(MP_QSTR_P13), MP_ROM_PTR(&machine_board_pin_obj[11]) },
  { MP_ROM_QSTR(MP_QSTR_P14), MP_ROM_PTR(&machine_board_pin_obj[12]) },
  { MP_ROM_QSTR(MP_QSTR_P15), MP_ROM_PTR(&machine_board_pin_obj[13]) },
  { MP_ROM_QSTR(MP_QSTR_P16), MP_ROM_PTR(&machine_board_pin_obj[14]) },
  { MP_ROM_QSTR(MP_QSTR_P17), MP_ROM_PTR(&machine_board_pin_obj[15]) },
  { MP_ROM_QSTR(MP_QSTR_P18), MP_ROM_PTR(&machine_board_pin_obj[16]) },
  { MP_ROM_QSTR(MP_QSTR_P19), MP_ROM_PTR(&machine_board_pin_obj[17]) },
  { MP_ROM_QSTR(MP_QSTR_P20), MP_ROM_PTR(&machine_board_pin_obj[18]) },
  { MP_ROM_QSTR(MP_QSTR_P21), MP_ROM_PTR(&machine_board_pin_obj[19]) },
  { MP_ROM_QSTR(MP_QSTR_P22), MP_ROM_PTR(&machine_board_pin_obj[20]) },
  { MP_ROM_QSTR(MP_QSTR_P23), MP_ROM_PTR(&machine_board_pin_obj[21]) },
  { MP_ROM_QSTR(MP_QSTR_P24), MP_ROM_PTR(&machine_board_pin_obj[22]) },
  { MP_ROM_QSTR(MP_QSTR_P25), MP_ROM_PTR(&machine_board_pin_obj[23]) },
  { MP_ROM_QSTR(MP_QSTR_P26), MP_ROM_PTR(&machine_board_pin_obj[24]) },
  { MP_ROM_QSTR(MP_QSTR_P27), MP_ROM_PTR(&machine_board_pin_obj[25]) },
  { MP_ROM_QSTR(MP_QSTR_P28), MP_ROM_PTR(&machine_board_pin_obj[26]) },
  { MP_ROM_QSTR(MP_QSTR_P29), MP_ROM_PTR(&machine_board_pin_obj[27]) },
  { MP_ROM_QSTR(MP_QSTR_P30), MP_ROM_PTR(&machine_board_pin_obj[28]) },
  { MP_ROM_QSTR(MP_QSTR_P31), MP_ROM_PTR(&machine_board_pin_obj[29]) },
};
MP_DEFINE_CONST_DICT(pin_cpu_pins_locals_dict, pin_cpu_pins_locals_dict_table);

STATIC const mp_rom_map_elem_t pin_board_pins_locals_dict_table[] = {
  { MP_ROM_QSTR(MP_QSTR_P2), MP_ROM_PTR(&machine_board_pin_obj[0]) },
  { MP_ROM_QSTR(MP_QSTR_P3), MP_ROM_PTR(&machine_board_pin_obj[1]) },
  { MP_ROM_QSTR(MP_QSTR_P4), MP_ROM_PTR(&machine_board_pin_obj[2]) },
  { MP_ROM_QSTR(MP_QSTR_P5), MP_ROM_PTR(&machine_board_pin_obj[3]) },
  { MP_ROM_QSTR(MP_QSTR_P6), MP_ROM_PTR(&machine_board_pin_obj[4]) },
  { MP_ROM_QSTR(MP_QSTR_P7), MP_ROM_PTR(&machine_board_pin_obj[5]) },
  { MP_ROM_QSTR(MP_QSTR_P8), MP_ROM_PTR(&machine_board_pin_obj[6]) },
  { MP_ROM_QSTR(MP_QSTR_P9), MP_ROM_PTR(&machine_board_pin_obj[7]) },
  { MP_ROM_QSTR(MP_QSTR_P10), MP_ROM_PTR(&machine_board_pin_obj[8]) },
  { MP_ROM_QSTR(MP_QSTR_P11), MP_ROM_PTR(&machine_board_pin_obj[9]) },
  { MP_ROM_QSTR(MP_QSTR_P12), MP_ROM_PTR(&machine_board_pin_obj[10]) },
  { MP_ROM_QSTR(MP_QSTR_P13), MP_ROM_PTR(&machine_board_pin_obj[11]) },
  { MP_ROM_QSTR(MP_QSTR_P14), MP_ROM_PTR(&machine_board_pin_obj[12]) },
  { MP_ROM_QSTR(MP_QSTR_P15), MP_ROM_PTR(&machine_board_pin_obj[13]) },
  { MP_ROM_QSTR(MP_QSTR_P16), MP_ROM_PTR(&machine_board_pin_obj[14]) },
  { MP_ROM_QSTR(MP_QSTR_P17), MP_ROM_PTR(&machine_board_pin_obj[15]) },
  { MP_ROM_QSTR(MP_QSTR_P18), MP_ROM_PTR(&machine_board_pin_obj[16]) },
  { MP_ROM_QSTR(MP_QSTR_P19), MP_ROM_PTR(&machine_board_pin_obj[17]) },
  { MP_ROM_QSTR(MP_QSTR_P20), MP_ROM_PTR(&machine_board_pin_obj[18]) },
  { MP_ROM_QSTR(MP_QSTR_P21), MP_ROM_PTR(&machine_board_pin_obj[19]) },
  { MP_ROM_QSTR(MP_QSTR_P22), MP_ROM_PTR(&machine_board_pin_obj[20]) },
  { MP_ROM_QSTR(MP_QSTR_P23), MP_ROM_PTR(&machine_board_pin_obj[21]) },
  { MP_ROM_QSTR(MP_QSTR_P24), MP_ROM_PTR(&machine_board_pin_obj[22]) },
  { MP_ROM_QSTR(MP_QSTR_P25), MP_ROM_PTR(&machine_board_pin_obj[23]) },
  { MP_ROM_QSTR(MP_QSTR_P26), MP_ROM_PTR(&machine_board_pin_obj[24]) },
  { MP_ROM_QSTR(MP_QSTR_P27), MP_ROM_PTR(&machine_board_pin_obj[25]) },
  { MP_ROM_QSTR(MP_QSTR_P28), MP_ROM_PTR(&machine_board_pin_obj[26]) },
  { MP_ROM_QSTR(MP_QSTR_P29), MP_ROM_PTR(&machine_board_pin_obj[27]) },
  { MP_ROM_QSTR(MP_QSTR_P30), MP_ROM_PTR(&machine_board_pin_obj[28]) },
  { MP_ROM_QSTR(MP_QSTR_P31), MP_ROM_PTR(&machine_board_pin_obj[29]) },
};
MP_DEFINE_CONST_DICT(pin_board_pins_locals_dict, pin_board_pins_locals_dict_table);
