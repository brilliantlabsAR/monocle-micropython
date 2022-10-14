/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#ifndef MONOCLE_OV5640_H
#define MONOCLE_OV5640_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Driver for the OV5640 camera sensor chip.
 * https://cdn.sparkfun.com/datasheets/Sensors/LightImaging/
 * @defgroup ov5640
 * @ingroup driver_chip
 * @{
 */

// The following functions must be implemented in ov5640_ll.h / .c and are called by ov5640.h / .c
// ov5640_ll_delay_ms(): pause CPU execution for given # of milliseconds
// ov5640_ll_init(): initialize three GPIO control pins. The following functions turn them on/off:
// ov5640_ll_PWDN(n): turn on/off the PWDN pin to ov5640 (connector pin 13 in MK6, MK8)
// ov5640_ll_RST(n): turn on/off the RESET pin to ov5640 (connector pin 14 in MK6, MK8)
// ov5640_ll_2V8EN(n): turn on/off the 2.8V power supply to OV5640
// ov5640_WR_Reg(): I2C write to OV5640
// ov5640_RD_Reg(): I2C read from OV5640

//#define OV5640_ADDR             0X78   ///< OV5640 address on I2C bus
//#define OV5640_ADDR             0X3C   ///< OV5640 address on I2C bus -- defined in mk9b/mk10_board.h
#define OV5640_CHIPIDH          0X300A ///< OV5640 Chip ID Register address, high byte
#define OV5640_CHIPIDL          0X300B ///< OV5640 Chip ID Register address, low byte
#define OV5640_ID               0X5640 ///< OV5640 Chip ID, expected value
#define OV5640_FPS              15     ///< frames per second, as implemented in camera configuration

#define TRANSFER_CMPLT 0x00u
#define TRANSFER_ERROR 0x01u
#define I2C_SLAVE_ADDR OV5640_ADDR

// Suggested sequence of calls from main program:
// Initialize MCU pins & I2C interface (done at start of program, in board init)
//  ov5640_init();
// Enable 24MHz pixel clock to the OV5640
//  ov5640_pwr_on(); (sets control pin states; in some MK versions turns on 2.8V)
//  ov5640_rgb565_mode(); -or- ov5640_yuv422_mode();
//  ov5640_mode_1x();
// if using other display: ov5640_reduce_size(new display resolution which should be < 640x400);
//  ov5640_focus_init(); 
//  ov5640_light_mode(0);
//  ov5640_color_saturation(3);
//  ov5640_brightness(4);
//  ov5640_contrast(3);
//  ov5640_sharpness(33);
//  ov5640_focus_constant(); // if AF lens module
//  ov5640_flip(1); // if needed
//  ov5640_mirror(1); // if needed

// Functions return true on success, false on failure

bool ov5640_init(void);
bool ov5640_pwr_on(void);
void ov5640_pwr_sleep(void);
void ov5640_pwr_wake(void);
//void ov5640_pwr_off(void);

void ov5640_mode_1x(void);
void ov5640_mode_2x(void);
void ov5640_reduce_size(uint16_t Hpixels, uint16_t Vpixels);
void ov5640_light_mode(uint8_t mode);
void ov5640_color_saturation(uint8_t sat);
void ov5640_brightness(uint8_t bright);
void ov5640_contrast(uint8_t contrast);
void ov5640_sharpness(uint8_t sharp);
void ov5640_special_effects(uint8_t eft);
void ov5640_flash_ctrl(uint8_t sw);

void ov5640_mirror(uint8_t on);
void ov5640_flip(uint8_t on);

bool ov5640_outsize_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height);
bool ov5640_focus_init(void);

void ov5640_ll_rst(uint8_t n);
void ov5640_ll_pwdn(uint8_t n);
void ov5640_ll_2v8en(uint8_t n);

void ov5640_ll_delay_ms(uint32_t ms);
uint8_t ov5640_ll_init(void);
uint8_t ov5640_wr_reg(uint16_t reg, uint8_t data);
uint8_t ov5640_rd_reg(uint16_t reg);

#endif
