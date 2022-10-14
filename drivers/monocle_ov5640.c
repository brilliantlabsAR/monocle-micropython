/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

/**
 * @file
 * @author Shreyas Hemachandra
 * @author Nathan Ashelman
 * @bug oled_config_burst() is not working correctly.
 */

#include <stdbool.h>
#include <stdint.h>
#include "monocle_ov5640.h"
#include "monocle_ov5640_data.h"
#include "monocle_i2c.h"
#include "monocle_board.h"
#include "monocle_config.h"
#include "nrfx_systick.h"

/**
 * Init the camera.
 * @return True if initialisation succeeded.
 */
bool ov5640_init(void)
{
    ov5640_ll_init();
    return true;
}

/**
 * Merge 3 configuration steps into one, to reduce high current draw of ov5640_uxga_init_reg_tbl[] and speed boot.
 * It combines
 * - ov5640_uxga_init_reg_tbl[] in ov5640_pwr_on()
 * - ov5640_rgb565_reg_tbl in ov5640_yuv422_mode()
 * - ov5640_rgb565_reg_1x_tbl in ov5640_mode_1x()
 */
void ov5640_yuv422_direct(void)
{
    uint16_t i = 0;
    uint16_t i_max = sizeof(ov5640_yuv422_direct_reg_tbl)/4;
    for (i = 0; i<i_max; i++)
        ov5640_wr_reg(ov5640_yuv422_direct_reg_tbl[i][0], ov5640_yuv422_direct_reg_tbl[i][1]);
}

/**
 * Power on the camera.
 * Precondition: ov5640_init() called earlier in main.c
 * @return True on success.
 */
bool ov5640_pwr_on(void)
{ 
    uint16_t reg;

    // Power on sequence, references: Datasheet section 2.7.1; Application Notes section 3.1.1
    // assume XCLK is on & kept on, per the current code (it could be turned off when powered down)
    // 1) PWDN (active high) is high, RESET (active low) is low
    // 2) DOVDD (1.8V) on
    // 3) >= 0ms later, AVDD (2.8V) on
    // 4) >= 5ms later, PWDN low (exit low-power standby mode)
    // 5) >= 1ms later, RESET high (come out of reset)
    // 6) >=20ms later, can begin using SCCB to access ov5640 registers

    // step (1) --though already done in ov5640_ll_init(), keep in case of re-try
    ov5640_ll_pwdn(1);
    ov5640_ll_rst(0);
    ov5640_ll_delay_ms(5);
    // step (2): 1.8V is already on
    // step (3), enable 2.8V
    ov5640_ll_2v8en(1);  // in MK9B, MK10, MK11, already on; does nothing
    // step (4)
    ov5640_ll_delay_ms(8);
    ov5640_ll_pwdn(0);
    // step (5)
    ov5640_ll_delay_ms(2);
    ov5640_ll_rst(1);
    // step (6)
    ov5640_ll_delay_ms(20);
    
    reg=ov5640_rd_reg(OV5640_CHIPIDH);
    reg <<= 8;
    reg|=ov5640_rd_reg(OV5640_CHIPIDL);
    if (reg != OV5640_ID)
        return false; // unexpected chip ID, init failed
    ov5640_wr_reg(0x3103,0X11);    //system clock from pad, bit[1]
    ov5640_wr_reg(0X3008,0X82);
    ov5640_yuv422_direct();

    return true;
} 

/**
 * Put camera into low-power mode, preserving configuration.
 */
void ov5640_pwr_sleep(void)
{
    ov5640_ll_pwdn(1);
}

/**
 * Wake camera up from low-power mode, prior configuration still valid.
 */
void ov5640_pwr_wake(void)
{
    ov5640_ll_pwdn(0);
}

/**
 * Change ov5640 config for 1x zoom.
 */
void ov5640_mode_1x(void) 
{
    uint16_t i = 0;
    /* use group write to update the group of registers in the same frame
     * (guaranteed to be written prior to the internal latch at the frame boundary).
     * see Datasheet section 2.6
     */
    ov5640_wr_reg(0x3212, 0x03); // start group 3 -- for some reason this makes transition worse!
    for (i = 0;i<(sizeof(ov5640_rgb565_reg_1x_tbl)/4);i++)
    {
        ov5640_wr_reg(ov5640_rgb565_reg_1x_tbl[i][0],ov5640_rgb565_reg_1x_tbl[i][1]); 
    }
    ov5640_wr_reg(0x3212, 0x13); // end group 3
    ov5640_wr_reg(0x3212, 0xa3); // launch group 3
} 

/**
 * NWA: change ov5640 config for 2x zoom, also used for 4x.
 */
void ov5640_mode_2x(void) 
{
    uint16_t i = 0;
    ov5640_wr_reg(0x3212, 0x03); // start group 3
    for (i = 0;i<(sizeof(ov5640_rgb565_reg_2x_tbl)/4);i++)
    {
        ov5640_wr_reg(ov5640_rgb565_reg_2x_tbl[i][0],ov5640_rgb565_reg_2x_tbl[i][1]); 
    }
    ov5640_wr_reg(0x3212, 0x13); // end group 3
    ov5640_wr_reg(0x3212, 0xa3); // launch group 3
}

/**
 * Reduce camera output size.
 * @pre ov5640_rgb565_mode_1x() or _2x() should have already been called
 * @param Hpixels desired horizontal output resolution (should be <=640)
 * @param Vpixels desired vertical output resolution (should be <=400)
 * @todo This function has not been tested, but uses code from >
 * @todo Implement error checking on inputs, return a success/failure code>
 */
void ov5640_reduce_size(uint16_t Hpixels, uint16_t Vpixels)
{
    ov5640_wr_reg(0x3212, 0x03); // start group 3
    
    ov5640_wr_reg(0x3808,Hpixels>>8);      //DVPHO, upper byte
    ov5640_wr_reg(0x3809,Hpixels&0xff); //DVPHO, lower byte
    ov5640_wr_reg(0x380a,Vpixels>>8);   //DVPVO, upper byte
    ov5640_wr_reg(0x380b,Vpixels&0xff); //DVPVO, lower byte
    
    ov5640_wr_reg(0x3212, 0x13); // end group 3
    ov5640_wr_reg(0x3212, 0xa3); // launch group 3
}

/** AWB Light mode config [0..4] [Auto, Sunny, Office, Cloudy, Home]. */
const static uint8_t OV5640_LIGHTMODE_TBL[5][7] =
{ 
    { 0x04, 0X00, 0X04, 0X00, 0X04, 0X00, 0X00 },
    { 0x06, 0X1C, 0X04, 0X00, 0X04, 0XF3, 0X01 },
    { 0x05, 0X48, 0X04, 0X00, 0X07, 0XCF, 0X01 },
    { 0x06, 0X48, 0X04, 0X00, 0X04, 0XD3, 0X01 },
    { 0x04, 0X10, 0X04, 0X00, 0X08, 0X40, 0X01 },
}; 

/**
 * Configures AWB gain control.
 * @param mode Refer `AWB Light mode config`
 */
void ov5640_light_mode(uint8_t mode)
{
    uint8_t i;
    ov5640_wr_reg(0x3212,0x03);    //start group 3
    for (i = 0; i<7; i++)
    {
        ov5640_wr_reg(0x3400+i,OV5640_LIGHTMODE_TBL[mode][i]);
    }
    ov5640_wr_reg(0x3212,0x13); //end group 3
    ov5640_wr_reg(0x3212,0xa3); //launch group 3    
}

/** Color saturation config [0..6] [-3, -2, -1, 0, 1, 2, 3]> */
const static uint8_t OV5640_SATURATION_TBL[7][6]=
{ 
    { 0X0C, 0x30, 0X3D, 0X3E, 0X3D, 0X01 },
    { 0X10, 0x3D, 0X4D, 0X4E, 0X4D, 0X01 },
    { 0X15, 0x52, 0X66, 0X68, 0X66, 0X02 },
    { 0X1A, 0x66, 0X80, 0X82, 0X80, 0X02 },
    { 0X1F, 0x7A, 0X9A, 0X9C, 0X9A, 0X02 },
    { 0X24, 0x8F, 0XB3, 0XB6, 0XB3, 0X03 },
    { 0X2B, 0xAB, 0XD6, 0XDA, 0XD6, 0X04 },
}; 

/**
 * Configures color saturation.
 * @param sat Refer Color saturation config
 */
void ov5640_color_saturation(uint8_t sat)
{ 
    ov5640_wr_reg(0x3212,0x03); // start group 3
    ov5640_wr_reg(0x5381,0x1c);
    ov5640_wr_reg(0x5382,0x5a);
    ov5640_wr_reg(0x5383,0x06);
    for (int i = 0; i < 6; i++)
        ov5640_wr_reg(0x5384+i,OV5640_SATURATION_TBL[sat][i]);
    ov5640_wr_reg(0x538b, 0x98);
    ov5640_wr_reg(0x538a, 0x01);
    ov5640_wr_reg(0x3212, 0x13); // end group 3
    ov5640_wr_reg(0x3212, 0xa3); // launch group 3    
}

/**
 * Configures brightness
 * @param bright Range 0 - 8
 */
void ov5640_brightness(uint8_t bright)
{
    uint8_t brtval;
    brtval = (bright < 4) ? 4-bright : bright-4;
    
    ov5640_wr_reg(0x3212,0x03);    //start group 3
    ov5640_wr_reg(0x5587,brtval<<4);
    if (bright<4)ov5640_wr_reg(0x5588,0x09);
    else ov5640_wr_reg(0x5588,0x01);
    ov5640_wr_reg(0x3212,0x13); //end group 3
    ov5640_wr_reg(0x3212,0xa3); //launch group 3
}

/**
 * Configures contrast.
 * @param contrast Range 0 - 6.
 */
void ov5640_contrast(uint8_t contrast)
{
    uint8_t reg0val=0X00;//contrast=3
    uint8_t reg1val=0X20;
    switch(contrast)
    {
        case 0://-3
            reg1val=reg0val=0X14;          
            break;    
        case 1://-2
            reg1val=reg0val=0X18;      
            break;    
        case 2://-1
            reg1val=reg0val=0X1C;     
            break;    
        case 4://1
            reg0val=0X10;          
            reg1val=0X24;          
            break;    
        case 5://2
            reg0val=0X18;          
            reg1val=0X28;          
            break;    
        case 6://3
            reg0val=0X1C;          
            reg1val=0X2C;          
            break;    
    } 
    ov5640_wr_reg(0x3212, 0x03); // start group 3
    ov5640_wr_reg(0x5585, reg0val); 
    ov5640_wr_reg(0x5586, reg1val);  
    ov5640_wr_reg(0x3212, 0x13); // end group 3
    ov5640_wr_reg(0x3212, 0xa3); // launch group 3
}

/**
 * Configures sharpness.
 * @param sharp Range 0 to 33, 0: off 33: auto.
 */
void ov5640_sharpness(uint8_t sharp)
{
    if (sharp<33) {
        ov5640_wr_reg(0x5308, 0x65);
        ov5640_wr_reg(0x5302, sharp);
    } else {
        ov5640_wr_reg(0x5308, 0x25);
        ov5640_wr_reg(0x5300, 0x08);
        ov5640_wr_reg(0x5301, 0x30);
        ov5640_wr_reg(0x5302, 0x10);
        ov5640_wr_reg(0x5303, 0x00);
        ov5640_wr_reg(0x5309, 0x08);
        ov5640_wr_reg(0x530a, 0x30);
        ov5640_wr_reg(0x530b, 0x04);
        ov5640_wr_reg(0x530c, 0x06);
    }
    
}
/** Effect configs [0..6] [Normal (off), Blueish (cool light), Redish (warm), Black & White, Sepia, Negative, Greenish] */
const static uint8_t OV5640_EFFECTS_TBL[7][3] =
{ 
    { 0X06, 0x40, 0X10 },
    { 0X1E, 0xA0, 0X40 },
    { 0X1E, 0x80, 0XC0 },
    { 0X1E, 0x80, 0X80 },
    { 0X1E, 0x40, 0XA0 }, 
    { 0X40, 0x40, 0X10 },
    { 0X1E, 0x60, 0X60 },
};

/**
 * Configures effects.
 * @param eft refer `Effect configs`
 */
void ov5640_special_effects(uint8_t eft)
{ 
    ov5640_wr_reg(0x3212, 0x03); //start group 3
    ov5640_wr_reg(0x5580, OV5640_EFFECTS_TBL[eft][0]);
    ov5640_wr_reg(0x5583, OV5640_EFFECTS_TBL[eft][1]); // sat U
    ov5640_wr_reg(0x5584, OV5640_EFFECTS_TBL[eft][2]); // sat V
    ov5640_wr_reg(0x5003, 0x08);
    ov5640_wr_reg(0x3212, 0x13); //end group 3
    ov5640_wr_reg(0x3212, 0xa3); //launch group 3
}

/**
 * Controls flash.
 * @param sw 0: off 1: on
 */
void ov5640_flash_ctrl(uint8_t sw)
{
    ov5640_wr_reg(0x3016, 0X02);
    ov5640_wr_reg(0x301C, 0X02); 
    if (sw)ov5640_wr_reg(0X3019, 0X02); 
    else ov5640_wr_reg(0X3019, 0X00);
} 

/**
 * Mirrors camera output (horizontal flip).
 * @param on 1 to turn on, 0 to turn off
 */
void ov5640_mirror(uint8_t on)
{
    uint8_t reg3821;
    reg3821 = ov5640_rd_reg(0x3821);
    if (on)        // turn mirror mode on
    {
        reg3821 = reg3821 | 0x06;
    } else {    // turn mirror mode off
        reg3821 = reg3821 & 0xF9;
    }
    ov5640_wr_reg(0x3821, reg3821);
}

/** 
 * Flips camera output (vertical flip).
 * @param on 1 to turn on, 0 to turn off
 */
void ov5640_flip(uint8_t on)
{
    uint8_t reg3820;
    reg3820 = ov5640_rd_reg(0x3820);
    if (on)        // turn flip mode on
    {
        reg3820 = reg3820 | 0x06;
    } else {    // turn flip mode off
        reg3820 = reg3820 & 0xF9;
    }
    ov5640_wr_reg(0x3820, reg3820);
}

/**
 * Configures window size.
 * @param offx Offset X 
 * @param offy Offset Y
 * @param width Prescale width
 * @param height Presclae height
 * @return true Once set 
 */
bool ov5640_outsize_set(uint16_t offx, uint16_t offy, uint16_t width, uint16_t height)
{ 
    ov5640_wr_reg(0X3212,0X03);

    // Set pre-scaling size
    ov5640_wr_reg(0x3808, width>>8);
    ov5640_wr_reg(0x3809, width&0xff);
    ov5640_wr_reg(0x380a, height>>8);
    ov5640_wr_reg(0x380b, height&0xff);

    // Set ofsset
    ov5640_wr_reg(0x3810, offx>>8);
    ov5640_wr_reg(0x3811, offx&0xff);
    
    ov5640_wr_reg(0x3812, offy>>8);
    ov5640_wr_reg(0x3813, offy&0xff);
    
    ov5640_wr_reg(0X3212, 0X13);
    ov5640_wr_reg(0X3212, 0Xa3);
    return true; 
}

/**
 * Focus init transfer camera module firmware.
 * @return true on success
 */
bool ov5640_focus_init(void)
{ 
    uint16_t i; 
    uint16_t addr = 0x8000;
    uint8_t state = 0x8F;
    ov5640_wr_reg(0x3000, 0x20);    // reset MCU
    // program ov5640 MCU firmware
    for (i = 0; i < sizeof(ov5640_af_config_tbl); i++) {
        ov5640_wr_reg(addr,ov5640_af_config_tbl[i]);
        addr++;
    }  
    ov5640_wr_reg(0x3022, 0x00);        // ? undocumented
    ov5640_wr_reg(0x3023, 0x00);        // ?
    ov5640_wr_reg(0x3024, 0x00);        // ?
    ov5640_wr_reg(0x3025, 0x00);        // ?
    ov5640_wr_reg(0x3026, 0x00);        // ?
    ov5640_wr_reg(0x3027, 0x00);        // ?
    ov5640_wr_reg(0x3028, 0x00);        // ?
    ov5640_wr_reg(0x3029, 0x7f);        // ?
    ov5640_wr_reg(0x3000, 0x00);     // enable MCU
    i = 0;
    do
    {
        state=ov5640_rd_reg(0x3029);    
        ov5640_ll_delay_ms(5);
        i++;
        if (i>1000)return false;
    } while(state!=0x70); 
    return true;    
}

// Low-level, hardware-specific functions to interface ov5640 to I2C and GPIO pins.

/**
 * Swap the byte order.
 * @param in Input usinged value.
 * @return Swapped order value.
 */
uint16_t __bswap_16(uint16_t in)
{
    return (in << 8) | (in >> 8);
} 

/**
 * Sleep for some milliseconds.
 * @param milliseconds Sleep time.
 */
// TODO: replace these 4 functions with defines in .h
void ov5640_ll_delay_ms(uint32_t milliseconds)
{
    nrfx_systick_delay_ms(milliseconds);
}

/**
 * Set the chip on or off by changing its reset pin.
 * @param n True for on.
 */
void ov5640_ll_rst(uint8_t n)
{
    if (0 == n) {
        board_pin_off(IO_N_CAM_RESET);
    } else {
        board_pin_on(IO_N_CAM_RESET);
    }
}

/*
 * Control the power state on/off of the chip.
 * @param n True for on.
 */
void ov5640_ll_pwdn(uint8_t n)
{
    if (0 == n) {
        board_pin_off(IO_CAM_PWDN);
    } else {
        board_pin_on(IO_CAM_PWDN);
    }
}

/*
 * Do nothing: 2.8V cannot be controlled independently in MK9B & later.
 * @param n Unused.
 */
void ov5640_ll_2V8EN(uint8_t n)
{
    return;
}

/**
 * Trigger initialisation of the chip, controlling its reset and power pins.
 * @return True on success.
 */
uint8_t ov5640_ll_init(void)
{
    // no need to initialize pins, currently done in board initialization...
    ov5640_ll_pwdn(1);
    ov5640_ll_rst(0);
    return 0;
}

/**
 * Write a byte of data to a provided registry address.
 * @param reg Register address (16 bit)
 * @param data 1-Byte data
 * @return uint8_t Status of the i2c write (TRANSFER_ERROR / TRANSFER_CMPLT)
 */
uint8_t ov5640_wr_reg(uint16_t reg, uint8_t data)
{
    uint8_t buf[3];
    uint16_t swaped = __bswap_16(reg);

    memcpy(buf, &swaped, 2);
    memcpy(buf+2, &data, 1);

    if (i2c_write(I2C_SLAVE_ADDR, buf, 3))
        return TRANSFER_CMPLT;

    return TRANSFER_ERROR;
}

/**
 * Read a byte of data from the provided registry address.
 * @param reg Register address (16 bit)
 * @return uint8_t A byte read data
 */
uint8_t ov5640_rd_reg(uint16_t reg)
{
    uint8_t w_buf[2];
    uint8_t ret_val = 0;
    uint16_t swaped = __bswap_16(reg);

    memcpy(w_buf, &swaped, 2);
    if (!i2c_write(I2C_SLAVE_ADDR, w_buf, 2))
        return 0;
    if (!i2c_read(I2C_SLAVE_ADDR, &ret_val, 1))
        return 0;
    return ret_val;
}
