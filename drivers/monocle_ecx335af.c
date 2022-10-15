/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

/**
 * Sony Microdisplay driver.
 * @file monocle_ecx335af.c
 * @author Shreyas Hemachandra
 * @author Nathan Ashelman
 * @bug ecx335af_config_burst() is not working correctly.
 */

#include <stdbool.h>
#include <stdint.h>
#include "monocle_ecx335af.h"
#include "monocle_fpga.h" // for fpga_checksum()
#include "monocle_spi.h"
#include "monocle_config.h"
#include "nrfx_systick.h"
#include "nrfx_log.h"

#define ecx335af_write_byte(addr, data) spi_write_byte(addr, data)
#define ecx335af_write_burst(addr, data, length) spi_write_burst(addr, data, length)
#define ecx335af_read_byte(addr) spi_read_byte(addr)

#define LOG(...) NRFX_LOG_ERROR(__VA_ARGS__)

/** Table summarizing all configuration to send to the devbice used by ecx335af_config_burst() */
// TODO: Where are these values coming from? Nothing on the Datasheet.
const uint8_t ecx335af_config_reg[] =
//  0x_0, 0x_1, 0x_2, 0x_3, 0x_4, 0x_5, 0x_6, 0x_7, 0x_8, 0x_9, 0x_A, 0x_B, 0x_C, 0x_D, 0x_E, 0x_F  Address
{
    0x9E, 0x20, 0x00, 0x20, 0x3F, 0xC8, 0x00, 0x40, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x56, // 0x0_
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x1_
    0x01, 0x00, 0x40, 0x40, 0x40, 0x80, 0x40, 0x40, 0x40, 0x0B, 0xBE, 0x3C, 0x02, 0x7A, 0x02, 0xFA, // 0x2_
    0x26, 0x01, 0xB6, 0x00, 0x03, 0x60, 0x00, 0x76, 0x02, 0xFE, 0x02, 0x71, 0x00, 0x1B, 0x00, 0x1C, // 0x3_
    0x02, 0x4D, 0x02, 0x4E, 0x80, 0x00, 0x00, 0x2D, 0x08, 0x01, 0x7E, 0x08, 0x0A, 0x04, 0x00, 0x3A, // 0x4_
    0x01, 0x58, 0x01, 0x2D, 0x01, 0x15, 0x00, 0x2B, 0x11, 0x02, 0x11, 0x02, 0x25, 0x04, 0x0B, 0x00, // 0x5_
    0x23, 0x02, 0x1A, 0x00, 0x0A, 0x01, 0x8C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, // 0x6_
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // 0x7_
};
const uint8_t ecx335af_config_reg_length = sizeof(ecx335af_config_reg);

/**
 * Fast configuraiton of the OLED device by sending them all through a SPI burst write.
 * @bug Does not currently work correctly. ecx335af_config() is used instead.
 */
void ecx335af_config_burst(void)
{
    const uint16_t burst_length = 4; // MCU burst write is failing with anything >7 bytes
    assert(0 == ecx335af_config_reg_length % burst_length); // should be evenly divisible
    uint16_t start = 0;

    // select OLED on SPI bus
    spi_set_cs_pin(SPIM0_SS0_PIN);

    for(start=0; start < ecx335af_config_reg_length; start=start+burst_length)
    {
        ecx335af_write_burst(start, ecx335af_config_reg + start, burst_length);
    }

    ecx335af_write_byte(0x00,0x9F); // exit power saving mode, YUV

    nrfx_systick_delay_ms(1);
}

/**
 * Configure each value of the screen over SPI.
 */
void ecx335af_config(void)
{
    // TODO: implement burst write, which is supported, for faster configuration

    // select OLED on SPI bus
    spi_set_cs_pin(SPIM0_SS0_PIN);

    // SONY ECX336-CN register configuration, see Datasheet section 10.1
    // for RGB mode
    //ecx335af_write_byte(0x00,0x0E); //[0]=0 -> enter power save mode
    //ecx335af_write_byte(0x01,0x00);
    // for YUV mode
    ecx335af_write_byte(0x00,0x9E); //[0]=0 -> enter power save mode
    ecx335af_write_byte(0x01,0x20);
    // for register 0x00, also change the last line of this function

    ecx335af_write_byte(0x02,0x00);
    ecx335af_write_byte(0x03,0x20);  //1125  
    ecx335af_write_byte(0x04,0x3F);
    ecx335af_write_byte(0x05,0xC8);  //1125  DITHERON, LUMINANCE=0x00=2000cd/m2=medium (Datasheet 10.8)
    ecx335af_write_byte(0x06,0x00);
    ecx335af_write_byte(0x07,0x40);
    ecx335af_write_byte(0x08,0x80);  // Luminance adjustment: OTPCALDAC_REGDIS=0 (preset mode per reg 5), white chromaticity: OTPDG_REGDIS=0 (preset mode, default)
    ecx335af_write_byte(0x09,0x00);
    ecx335af_write_byte(0x0A,0x10);
    ecx335af_write_byte(0x0B,0x00);
    ecx335af_write_byte(0x0C,0x00);
    ecx335af_write_byte(0x0D,0x00);
    ecx335af_write_byte(0x0E,0x00);
    ecx335af_write_byte(0x0F,0x56);
    ecx335af_write_byte(0x10,0x00);
    ecx335af_write_byte(0x11,0x00);
    ecx335af_write_byte(0x12,0x00);
    ecx335af_write_byte(0x13,0x00);
    ecx335af_write_byte(0x14,0x00);
    ecx335af_write_byte(0x15,0x00);
    ecx335af_write_byte(0x16,0x00);
    ecx335af_write_byte(0x17,0x00);
    ecx335af_write_byte(0x18,0x00);
    ecx335af_write_byte(0x19,0x00);
    ecx335af_write_byte(0x1A,0x00);
    ecx335af_write_byte(0x1B,0x00);
    ecx335af_write_byte(0x1C,0x00);
    ecx335af_write_byte(0x1D,0x00);
    ecx335af_write_byte(0x1E,0x00);
    ecx335af_write_byte(0x1F,0x00);
    ecx335af_write_byte(0x20,0x01);
    ecx335af_write_byte(0x21,0x00);
    ecx335af_write_byte(0x22,0x40);
    ecx335af_write_byte(0x23,0x40);
    ecx335af_write_byte(0x24,0x40);
    ecx335af_write_byte(0x25,0x80);
    ecx335af_write_byte(0x26,0x40);
    ecx335af_write_byte(0x27,0x40);
    ecx335af_write_byte(0x28,0x40);
    ecx335af_write_byte(0x29,0x0B);
    ecx335af_write_byte(0x2A,0xBE); //CALDAC=190 (ignored, since OTPCALDAC_REGDIS=0)
    ecx335af_write_byte(0x2B,0x3C);
    ecx335af_write_byte(0x2C,0x02);
    ecx335af_write_byte(0x2D,0x7A);
    ecx335af_write_byte(0x2E,0x02);
    ecx335af_write_byte(0x2F,0xFA);
    ecx335af_write_byte(0x30,0x26);
    ecx335af_write_byte(0x31,0x01);
    ecx335af_write_byte(0x32,0xB6);
    ecx335af_write_byte(0x33,0x00);
    ecx335af_write_byte(0x34,0x03);
    ecx335af_write_byte(0x35,0x60);  //1125
    ecx335af_write_byte(0x36,0x00);
    ecx335af_write_byte(0x37,0x76);
    ecx335af_write_byte(0x38,0x02);
    ecx335af_write_byte(0x39,0xFE);
    ecx335af_write_byte(0x3A,0x02);
    ecx335af_write_byte(0x3B,0x71);  //1125
    ecx335af_write_byte(0x3C,0x00);
    ecx335af_write_byte(0x3D,0x1B);
    ecx335af_write_byte(0x3E,0x00);
    ecx335af_write_byte(0x3F,0x1C);
    ecx335af_write_byte(0x40,0x02);    //1125
    ecx335af_write_byte(0x41,0x4D);    //1125
    ecx335af_write_byte(0x42,0x02);    //1125
    ecx335af_write_byte(0x43,0x4E);    //1125
    ecx335af_write_byte(0x44,0x80);
    ecx335af_write_byte(0x45,0x00);
    ecx335af_write_byte(0x46,0x00);
    ecx335af_write_byte(0x47,0x2D);    //1125
    ecx335af_write_byte(0x48,0x08);
    ecx335af_write_byte(0x49,0x01);   //1125
    ecx335af_write_byte(0x4A,0x7E);    //1125
    ecx335af_write_byte(0x4B,0x08);
    ecx335af_write_byte(0x4C,0x0A);   //1125
    ecx335af_write_byte(0x4D,0x04);    //1125
    ecx335af_write_byte(0x4E,0x00);
    ecx335af_write_byte(0x4F,0x3A);   //1125
    ecx335af_write_byte(0x50,0x01);   //1125
    ecx335af_write_byte(0x51,0x58);   //1125
    ecx335af_write_byte(0x52,0x01);   
    ecx335af_write_byte(0x53,0x2D);
    ecx335af_write_byte(0x54,0x01);
    ecx335af_write_byte(0x55,0x15);   //1125
    ecx335af_write_byte(0x56,0x00);
    ecx335af_write_byte(0x57,0x2B);
    ecx335af_write_byte(0x58,0x11);  //1125
    ecx335af_write_byte(0x59,0x02);
    ecx335af_write_byte(0x5A,0x11);  //1125
    ecx335af_write_byte(0x5B,0x02);  
    ecx335af_write_byte(0x5C,0x25);
    ecx335af_write_byte(0x5D,0x04);  //1125
    ecx335af_write_byte(0x5E,0x0B);  //1125
    ecx335af_write_byte(0x5F,0x00);
    ecx335af_write_byte(0x60,0x23);
    ecx335af_write_byte(0x61,0x02);
    ecx335af_write_byte(0x62,0x1A);   //1125
    ecx335af_write_byte(0x63,0x00);
    ecx335af_write_byte(0x64,0x0A);   //1125
    ecx335af_write_byte(0x65,0x01);   //1125
    ecx335af_write_byte(0x66,0x8C);   //1125
    ecx335af_write_byte(0x67,0x30);   //1125
    ecx335af_write_byte(0x68,0x00);   
    ecx335af_write_byte(0x69,0x00);    //1125
    ecx335af_write_byte(0x6A,0x00);
    ecx335af_write_byte(0x6B,0x00);
    ecx335af_write_byte(0x6C,0x00);
    ecx335af_write_byte(0x6D,0x00);    //1125
    ecx335af_write_byte(0x6E,0x00);
    ecx335af_write_byte(0x6F,0x60);
    ecx335af_write_byte(0x70,0x00);
    ecx335af_write_byte(0x71,0x00);
    ecx335af_write_byte(0x72,0x00);
    ecx335af_write_byte(0x73,0x00);
    ecx335af_write_byte(0x74,0x00);
    ecx335af_write_byte(0x75,0x00);
    ecx335af_write_byte(0x76,0x00);
    ecx335af_write_byte(0x77,0x00);
    ecx335af_write_byte(0x78,0x00);
    ecx335af_write_byte(0x79,0x68);
    ecx335af_write_byte(0x7A,0x00);
    ecx335af_write_byte(0x7B,0x00);
    ecx335af_write_byte(0x7C,0x00);
    ecx335af_write_byte(0x7D,0x00);
    ecx335af_write_byte(0x7E,0x00);
    ecx335af_write_byte(0x7F,0x00);
    //ecx335af_write_byte(0x00,0x0F); // exit power saving mode, RGB
    ecx335af_write_byte(0x00,0x9F); // exit power saving mode, YUV

    nrfx_systick_delay_ms(1);
}

/**
 * Verify that OLED is connected & configuration succeeded.
 * @return True if configured correctly.
 */
bool ecx335af_verify_config(void)
{
    // select OLED on SPI bus
    spi_set_cs_pin(SPIM0_SS0_PIN);

    // by checking register 0x29 changed from default of 0x0A to 0x0B
    //return ((0x0B == ecx335af_read_byte(0x29)));
    // and that 0x2A, which is used in the SPI unit test, has been restored
    return ((0x0B == ecx335af_read_byte(0x29)) && (0xBE == ecx335af_read_byte(0x2A)));
}

/**
 * Full validation of config by burst reading all registers & comparing to expected checksum.
 * @return True if the checksum matched.
 */
// TODO: Why is this required? Is there a stability issue of the SPI interface?
bool ecx335af_verify_config_full(void)
{
    uint8_t *ecx335af_config_data = NULL;
    uint16_t spi_checksum = 0;
    //uint16_t config_checksum = fpga_checksum(ecx335af_config_reg, ecx335af_config_reg_length); // 0xB358

    spi_set_cs_pin(SPIM0_SS0_PIN);
    ecx335af_config_data = spi_read_burst(0x00, ecx335af_config_reg_length);
    spi_checksum = fpga_calc_checksum(ecx335af_config_data, ecx335af_config_reg_length); // 0xB559 (byte-by-byte) or 0x7559 (burst)
                                                                                 // 2021-05-21: getting 0xbc59 (byte-by-byte & burst)
    LOG("OLED config checksum = 0x%x.", spi_checksum);
    return (spi_checksum == 0xB559);
}

/**
 * Configure the luminance level of the display.
 * @param level Predefined level of luminance.
 */
void ecx335af_set_luminance(enum ecx335af_luminance_t level)
{
    uint8_t prev_0x05, new_0x05, check_0x05;  // register values

    // select OLED on SPI bus
    spi_set_cs_pin(SPIM0_SS0_PIN);

    // maximum value value is 4
    if (level > 4) return; 
    // LUMINANCE is register 0x05[3:0]; preserve other bits
    prev_0x05 = ecx335af_read_byte(0x05);
    new_0x05 = prev_0x05 & 0xF8;         //clear lower 3 bits
    new_0x05 = new_0x05 | level;
    //write new value
    ecx335af_write_byte(0x05,new_0x05);
    check_0x05 = ecx335af_read_byte(0x05);
    assert(check_0x05 == new_0x05);
}

/**
 * Put the display to sleep power mode.
 */
void ecx335af_pwr_sleep(void)
{
    // select OLED on SPI bus
    spi_set_cs_pin(SPIM0_SS0_PIN);

    ecx335af_write_byte(0x00,0x9E); // enter power saving mode (YUV)
    //en_vcc10_Write(0);        // turn off 10V power, not available in MK9B
}

/**
 * Power back on the dispay from sleep mode.
 */
void ecx335af_pwr_wake(void)
{
    // select OLED on SPI bus
    spi_set_cs_pin(SPIM0_SS0_PIN);

    //en_vcc10_Write(1);        // turn on 10V power, not available in MK9B
    ecx335af_write_byte(0x00,0x9F); // exit power saving mode (YUV)
}

/**
 * 512 read/write cycles to OLED resgister 0x2A.
 * For unit testing.
 * @return True if the read match the write.
 */
bool ecx335af_spi_exercise_register(void)
{
    bool success = false;

    // select OLED on SPI bus
    spi_set_cs_pin(SPIM0_SS0_PIN);

    // register 0x2A has all 8 bits writeable
    // it is not used in our configuration, so should not have bad side effects
    //   also, this function attempts to restore the original register value before returning
    success = spi_exercise_register(0x2A);

    return(success);
}
