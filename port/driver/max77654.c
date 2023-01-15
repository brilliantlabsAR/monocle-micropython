/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Nathan Ashelman <nathan@itsbrilliant.co>
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nrfx_log.h"
#include "nrfx_twi.h"
#include "nrfx_systick.h"

#include "driver/config.h"
#include "driver/i2c.h"
#include "driver/max77654.h"
#include "driver/nrfx.h"
#include "driver/timer.h"

#define ASSERT  NRFX_ASSERT
#define LEN(x)  (sizeof x / sizeof *x)

/** Allowable charge current in mA; to protect battery, disallow any higher setting (even if configurable). */
#define MAX77654_CHG_CC_MAX             140  

/** Allowable charge voltage in mV; to protect battery, disallow any higher setting (even if configurable). */
#define MAX77654_CHG_CV_MIN             3600
#define MAX77654_CHG_CV_MAX             4300 

#define MAX77654_CID_EXPECTED           0x02

// Access permissions:
//   RW read write
//   RC read clears all
//   RO read only
//   WO write only (write 1, auto-resets to 0, will be read as 0)

// Register Definitions

// Global Registers

#define MAX77654_INT_GLBL0              0x00 // RC
#define MAX77654_DOD0_R                 (0x01 << 7) // RC LDO Dropout Detector Rising Interrupt
#define MAX77654_DOD1_R                 (0x01 << 6) // RC LDO Dropout Detector Rising Interrupt
#define MAX77654_TJAL2_R                (0x01 << 5) // RC Thermal Alarm 2 Rising Interrupt
#define MAX77654_TJAL1_R                (0x01 << 4) // RC Thermal Alarm 1 Rising Interrupt
#define MAX77654_nEN_R                  (0x01 << 3) // RC nEN Rising Interrupt
#define MAX77654_nEN_F                  (0x01 << 2) // RC nEN Falling Interrupt
#define MAX77654_GPI_R                  (0x01 << 1) // RC GPI Rising Interrupt
#define MAX77654_GPI_F                  (0x01 << 0) // RC GPI Falling Interrupt

#define MAX77654_INT_GLBL1              0x04 // RC
#define MAX77654_LDO1_F                 (0x01 << 6) // LDO1 Fault Interrupt
#define MAX77654_LDO0_F                 (0x01 << 5) // LDO0 Fault Interrupt
#define MAX77654_SBB_TO                 (0x01 << 4) // SBB Timeout
#define MAX77654_GPI2_R                 (0x01 << 3) // GPI Rising Interrupt
#define MAX77654_GPI2_F                 (0x01 << 2) // GPI Fallint Interrupt
#define MAX77654_GPI1_R                 (0x01 << 1) // GPI Rising Interrupt
#define MAX77654_GPI1_F                 (0x01 << 0) // GPI Fallint Interrupt

#define MAX77654_ERCFLAG                0x05 // RC
#define MAX77654_WDT_RST                (0x01 << 7) // Watchdog Timer Reset Flag, watchdog timer expired & caused power-reset
#define MAX77654_WDT_OFF                (0x01 << 6) // Watchdog Timer OFF Flag, watchdog timer expired & caused power-off
#define MAX77654_SFT_CRST_F             (0x01 << 5) // Software Cold Reset Flag
#define MAX77654_SFT_OFF_F              (0x01 << 4) // Software OFF Flag
#define MAX77654_MRST                   (0x01 << 3) // Manual Reset Timer, a manual reset has occurred
#define MAX77654_SYSUVLO                (0x01 << 2) // SYS Domain Undervoltage lockout
#define MAX77654_SYSOVLO                (0x01 << 1) // SYS Domain Overvoltage lockout
#define MAX77654_TOVLD                  (0x01 << 0) // Thermal overload, temperature > 165C

#define MAX77654_STAT_GLBL              0x06
#define MAX77654_INTM_GLBL0             0x09
#define MAX77654_INTM_GLBL1             0x08
#define MAX77654_CNFG_GLBL              0x10

#define MAX77654_CNFG_GPIO0             0x11 // RW (except bit 1 is RO)
#define MAX77654_CNFG_GPIO1             0x12 // RW (except bit 1 is RO)
#define MAX77654_CNFG_GPIO2             0x13 // RW (except bit 1 is RO)
#define MAX77654_ALT_GPIO               (0x01 << 5) // RW Alternate mode (meaning depends on GPIO #)
#define MAX77654_DBEN_GPIO              (0x01 << 4) // RW GPI Debounce Timer Enable: 0=no debouce, 1=30ms debounce
#define MAX77654_DO                     (0x01 << 3) // RW GPO Data Output: 0=logic low, 1=logic high (DRV=1) or hiZ (DRV=0) (if DIR=1, has no effect)
#define MAX77654_DRV                    (0x01 << 2) // RW GPO Driver Type: 0=open-drain, 1=push-pull
#define MAX77654_DI                     (0x01 << 1) // RO GPIO Digital Input Value: reflects state of the GPIO (irrespective of GPI or GPO)
#define MAX77654_DIR                    (0x01 << 0) // RW GPIO Direction: 0=outupt (GPO), 1=input (GPI)

#define MAX77654_CID                    0x14 // RO
#define MAX77654_CID4                   (0x01 << 7) // RO Chip Identification Code, bit 4
#define MAX77654_CID_Msk                (0x0F << 0)

// Charger Registers

#define MAX77654_INT_CHG                0x01 // RC
#define MAX77654_STAT_CHG_A             0x02 // RO

#define MAX77654_STAT_CHG_B             0x03 // RO
#define MAX77654_CHG_DTLS_Msk           (0x0F << 4)
#define MAX77654_CHG_DTLS_OFF           (0x00 << 4) // Off
#define MAX77654_CHG_DTLS_PRE_Q         (0x01 << 4) // Prequalification mode
#define MAX77654_CHG_DTLS_FAST_CC       (0x02 << 4) // Fast-charge constant-current (CC) mode
#define MAX77654_CHG_DTLS_FAST_CC_J     (0x03 << 4) // JEITA modified fast-charge constant-current (CC) mode
#define MAX77654_CHG_DTLS_FAST_CV       (0x04 << 4) // Fast-charge constant-voltage (CV) mode
#define MAX77654_CHG_DTLS_FAST_CV_J     (0x05 << 4) // JEITA modified fast-charge constant-voltage (CV) mode
#define MAX77654_CHG_DTLS_TOP_OFF       (0x06 << 4) // Top-off mode
#define MAX77654_CHG_DTLS_TOP_OFF_J     (0x07 << 4) // JEITA modified top-off mode
#define MAX77654_CHG_DTLS_DONE          (0x08 << 4) // Done
#define MAX77654_CHG_DTLS_DONE_J        (0x09 << 4) // JEITA modified done
#define MAX77654_CHG_DTLS_FAULT_PRE_Q   (0x0A << 4) // Prequalification timer fault
#define MAX77654_CHG_DTLS_FAULT_TIME    (0x0B << 4) // Fast-charge timer fault
#define MAX77654_CHG_DTLS_FAULT_TEMP    (0x0C << 4) // Battery temperature fault
#define MAX77654_CHGIN_DTLS_Msk         (0x03 << 2)
#define MAX77654_CHG                    (0x01 << 1) // Quick Charger Status: 0=not charging, 1=charging
#define MAX77654_TIME_SUS               (0x01 << 0) // Time Suspend Indicator: 0=not active or not suspended, 1=suspended (for any of 3 given reasons)

#define MAX77654_INT_M_CHG              0x07 // RW

#define MAX77654_CNFG_CHG_A             0x20 // RW
#define MAX77654_THM_HOT_Msk            (0x03 << 6) // JEITA Temperature Threshold: HOT
#define MAX77654_THM_HOT_45C            (0x00 << 6)
#define MAX77654_THM_HOT_50C            (0x01 << 6)
#define MAX77654_THM_HOT_55C            (0x02 << 6)
#define MAX77654_THM_HOT_60C            (0x03 << 6)
#define MAX77654_THM_WARM_Msk           (0x02 << 4)
#define MAX77654_THM_WARM_35C           (0x00 << 4)
#define MAX77654_THM_WARM_40C           (0x01 << 4)
#define MAX77654_THM_WARM_45C           (0x02 << 4)
#define MAX77654_THM_WARM_50C           (0x03 << 4)
#define MAX77654_THM_COOL_Msk           (0x03 << 2
#define MAX77654_THM_COOL_00C           (0x00 << 2)
#define MAX77654_THM_COOL_05C           (0x01 << 2)
#define MAX77654_THM_COOL_10C           (0x02 << 2)
#define MAX77654_THM_COOL_15C           (0x03 << 2)
#define MAX77654_THM_COLD_Msk           (0x03 << 0)
#define MAX77654_THM_COLD_N10C          (0x00 << 0)
#define MAX77654_THM_COLD_N05C          (0x01 << 0)
#define MAX77654_THM_COLD_00C           (0x02 << 0)
#define MAX77654_THM_COLD_05C           (0x03 << 0)

#define MAX77654_CNFG_CHG_B             0x21 // RW
#define MAX77654_VCHGIN_MIN_Msk         (0x07 << 5)
#define MAX77654_VCHGIN_MIN_4V0         (0x00 << 5) // 4.0V
#define MAX77654_VCHGIN_MIN_4V1         (0x01 << 5) // 4.1V
#define MAX77654_VCHGIN_MIN_4V2         (0x02 << 5) // 4.2V
#define MAX77654_VCHGIN_MIN_4V3         (0x03 << 5) // 4.3V
#define MAX77654_VCHGIN_MIN_4V4         (0x04 << 5) // 4.4V
#define MAX77654_VCHGIN_MIN_4V5         (0x05 << 5) // 4.5V
#define MAX77654_VCHGIN_MIN_4V6         (0x06 << 5) // 4.6V
#define MAX77654_VCHGIN_MIN_4V7         (0x07 << 5) // 4.7V
#define MAX77654_ICHGIN_LIM_Msk         (0x07 << 2)
#define MAX77654_ICHGIN_LIM_95MA        (0x00 << 2) // 95mA
#define MAX77654_ICHGIN_LIM_190MA       (0x01 << 2) // 190mA
#define MAX77654_ICHGIN_LIM_285MA       (0x02 << 2) // 285mA
#define MAX77654_ICHGIN_LIM_380MA       (0x03 << 2) // 380mA
#define MAX77654_ICHGIN_LIM_475MA       (0x04 << 2) // 475mA
#define MAX77654_I_PQ                   (0x01 << 1) // Prequalification charge current as % of I_FAST-CHG: 0=10%, 1=20%
#define MAX77654_CHG_EN                 (0x01 << 0) // Battery Charger Enable: 0=disable, 1=enable

#define MAX77654_CNFG_CHG_C             0x22 // RW
#define MAX77654_CHG_PQ_Msk             (0x07 << 5)
#define MAX77654_CHG_PQ_2V3             (0x00 << 5) // 2.3V
#define MAX77654_CHG_PQ_2V4             (0x01 << 5) // 2.4V
#define MAX77654_CHG_PQ_2V5             (0x02 << 5) // 2.5V
#define MAX77654_CHG_PQ_2V6             (0x03 << 5) // 2.6V
#define MAX77654_CHG_PQ_2V7             (0x04 << 5) // 2.7V
#define MAX77654_CHG_PQ_2V8             (0x05 << 5) // 2.8V
#define MAX77654_CHG_PQ_2V9             (0x06 << 5) // 2.9V
#define MAX77654_CHG_PQ_3V0             (0x07 << 5) // 3.0V
#define MAX77654_I_TERM_Msk             (0x03 << 3)
#define MAX77654_I_TERM_5P              (0x00 << 3) // 5% of I_FAST-CHG
#define MAX77654_I_TERM_7P5             (0x01 << 3) // 7.5% of I_FAST-CHG
#define MAX77654_I_TERM_10P             (0x02 << 3) // 10% of I_FAST-CHG
#define MAX77654_I_TERM_15P             (0x03 << 3) // 15% of I_FAST-CHG
#define MAX77654_T_TOPOFF_Msk           (0x07 << 0)
#define MAX77654_T_TOPOFF_0M            (0x00 << 0) // 0 minutes
#define MAX77654_T_TOPOFF_5M            (0x01 << 0) // 5 minutes
#define MAX77654_T_TOPOFF_10M           (0x02 << 0) // 10 minutes
#define MAX77654_T_TOPOFF_15M           (0x03 << 0) // 15 minutes
#define MAX77654_T_TOPOFF_20M           (0x04 << 0) // 20 minutes
#define MAX77654_T_TOPOFF_25M           (0x05 << 0) // 25 minutes
#define MAX77654_T_TOPOFF_30M           (0x06 << 0) // 30 minutes
#define MAX77654_T_TOPOFF_35M           (0x07 << 0) // 35 minutes

#define MAX77654_CNFG_CHG_D             0x23 // RW
#define MAX77654_TJ_Msk                 (0x07 << 5)
#define MAX77654_VSYS_Msk               (0x1F << 0)

#define MAX77654_CNFG_CHG_E             0x24 // RW
#define MAX77654_CHG_CC_Msk             (0x3F << 2)
#define MAX77654_T_FAST_CHG_Msk         (0x03 << 0)
#define MAX77654_T_FAST_CHG_3H          (0x01 << 0) // 3 hours
#define MAX77654_T_FAST_CHG_5H          (0x02 << 0) // 5 hours
#define MAX77654_T_FAST_CHG_7H          (0x03 << 0) // 7 hours

#define MAX77654_CNFG_CHG_F             0x25 // RW
#define MAX77654_CHG_CC_JEITA_Pos       2
#define MAX77654_CHG_CC_JEITA_Msk       (0x3F << 2)
#define MAX77654_THM_EN                 (0x01 << 1)

#define MAX77654_CNFG_CHG_G             0x26 // RW
#define MAX77654_CHG_CV_Msk             (0x3F << 2)
#define MAX77654_USBS                   (0x01 << 1) // USB suspend mode: 0=not suspended, draw current from adapter; 1=suspended, draw no current (charge off)

#define MAX77654_CNFG_CHG_H             0x27 // RW
#define MAX77654_CHG_CV_JEITA_Msk       (0x3F << 2)

#define MAX77654_CNFG_CHG_I             0x28 // RW
#define MAX77654_IMON_DISCHG_SCALE_Msk  (0x0F << 4)
#define MAX77654_MUX_SEL_Msk            (0x0F << 0)
// values for MUX_SEL
#define MAX77654_MUX_DISABLE            0x0 // disabled, AMUX is hi-Z
#define MAX77654_MUX_CHGIN_V            0x1 // CHGIN voltage monitor
#define MAX77654_MUX_CHGIN_I            0x2 // CHGIN current monitor
#define MAX77654_MUX_BATT_V             0x3 // BATT voltage monitor
#define MAX77654_MUX_BATT_I             0x4 // BATT current monitor, valid only when charging in progress (CHG=1)
#define MAX77654_MUX_BATT_DIS_I         0x5 // BATT discharge current monitor normal measurement
#define MAX77654_MUX_BATT_NUL_I         0x6 // BATT discharge current monitor nulling measurement
#define MAX77654_MUX_THM_V              0x7 // THM voltage monitor
#define MAX77654_MUX_TBIAS_V            0x8 // TBIAS voltage monitor
#define MAX77654_MUX_AGND_V             0x9 // AGND voltage monitor, through 100 ohm pull-down resistor
#define MAX77654_MUX_SYS_V              0xA // SYS voltage monitor

// SBB Registers

#define MAX77654_CNFG_SBB0_A            0x29 // RW
#define MAX77654_CNFG_SBB1_A            0x2B // RW
#define MAX77654_CNFG_SBB2_A            0x2D // RW
// directly write these values to the register to set voltage (other voltages available, see datasheet)
#define MAX77654_CNFG_SBB_A_TV_1V2      0x08   // 800 mV + 0x08 * 50 mV = 1.2 V
#define MAX77654_CNFG_SBB_A_TV_1V8      0x14   // 800 mV + 0x14 * 50 mV = 1.8 V
#define MAX77654_CNFG_SBB_A_TV_2V7      0x26   // 800 mV + 0x26 * 50 mV = 2.7 V
#define MAX77654_CNFG_SBB_A_TV_2V8      0x28   // 800 mV + 0x28 * 50 mV = 2.8 V

#define MAX77654_CNFG_SBB0_B            0x2A // RW
#define MAX77654_CNFG_SBB1_B            0x2C // RW
#define MAX77654_CNFG_SBB2_B            0x2E // RW
#define MAX77654_CNFG_SBB_B_MD          (0x01 << 6) // Operation Mode: 0=buck-boost, 1=buck
#define MAX77654_CNFG_SBB_B_IP_Msk      (0x03 << 4)
#define MAX77654_CNFG_SBB_B_IP_1000     (0x00 << 4)  // 1000mA output current limit
#define MAX77654_CNFG_SBB_B_IP_750      (0x01 << 4)   // 750mA output current limit
#define MAX77654_CNFG_SBB_B_IP_500      (0x02 << 4)   // 500mA output current limit
#define MAX77654_CNFG_SBB_B_IP_333      (0x03 << 4)   // 333mA output current limit
#define MAX77654_CNFG_SBB_B_ADE         (0x01 << 3) // Active Discharge Enable: 0=disabled, 1=enabled
#define MAX77654_CNFG_SBB_B_EN_Msk      (0x07 << 0)
#define MAX77654_CNFG_SBB_B_EN_SLOT0    (0x00 << 0)  // FPS slot 0
#define MAX77654_CNFG_SBB_B_EN_SLOT1    (0x01 << 0)  // FPS slot 1
#define MAX77654_CNFG_SBB_B_EN_SLOT2    (0x02 << 0)  // FPS slot 2
#define MAX77654_CNFG_SBB_B_EN_SLOT3    (0x03 << 0)  // FPS slot 3
#define MAX77654_CNFG_SBB_B_EN_OFF      (0x04 << 0)  // Off irrespective of FPS
#define MAX77654_CNFG_SBB_B_EN_ON       (0x06 << 0)  // On irrespective of FPS

#define MAX77654_CNFG_SBB_TOP           0x2F // RW
#define MAX77654_CNFG_SBB_TOP_DRV       (0x03 << 0)

// LDO Registers

#define MAX77654_CNFG_LDO0_A            0x38 // RW
#define MAX77654_CNFG_LDO1_A            0x3A // RW
// directly write these values to the register to set voltage (other voltages available, see datasheet)
#define MAX77654_CNFG_LDO_A_TV_1V2      0x10   // 800 mV + 0x10 * 25 mV = 1.2V
#define MAX77654_CNFG_LDO_A_TV_1V8      0x28   // 800 mV + 0x28 * 25 mV = 1.8V
#define MAX77654_CNFG_LDO_A_TV_2V7      0x4C   // 800 mV + 0x4C * 25 mV = 2.7V
#define MAX77654_CNFG_LDO_A_TV_2V8      0x50   // 800 mV + 0x50 * 25 mV = 2.8V

#define MAX77654_CNFG_LDO0_B            0x39 // RW
#define MAX77654_CNFG_LDO1_B            0x3B // RW
#define MAX77654_CNFG_LDO_B_MD          (0x01 << 4) // RW Mode: 0=LDO, 1=Load Switch
#define MAX77654_CNFG_LDO_B_ADE         (0x01 << 3) // RW Active Discharge Enable: 0=disabled, 1=enabled
#define MAX77654_CNFG_LDO_B_EN_Msk      (0x07 << 0)
#define MAX77654_CNFG_LDO_B_EN_SLOT0    (0x00 << 0)
#define MAX77654_CNFG_LDO_B_EN_SLOT1    (0x01 << 0)
#define MAX77654_CNFG_LDO_B_EN_SLOT2    (0x02 << 0)
#define MAX77654_CNFG_LDO_B_EN_SLOT3    (0x03 << 0)
#define MAX77654_CNFG_LDO_B_EN_OFF      (0x04 << 0)
#define MAX77654_CNFG_LDO_B_EN_ON       (0x06 << 0)

/**
 * Configure a register value over I2C.
 * @param addr Address of the register.
 * @param data Value to write.
 */
void max77654_write(uint8_t addr, uint8_t data)
{
    uint8_t buf[] = { addr, data };

    if (!i2c_write(MAX77654_I2C, MAX77654_ADDR, buf, sizeof buf))
        ASSERT(!"I2C write failed");
}

/**
 * Read a register value over I2C.
 * @param addr Address of the register.
 * @return The value returned.
 */
uint8_t max77654_read(uint8_t addr)
{
    uint8_t val;

    if (!i2c_write(MAX77654_I2C, MAX77654_ADDR, &addr, 1))
        ASSERT(!"I2C write failed");
    if (!i2c_read(MAX77654_I2C, MAX77654_ADDR, &val, 1))
        ASSERT(!"I2C read failed");
    return val;
}

/**
 * Read the Chip ID (CID).
 * @return 0 on read failure or CID value (6-bits) on read success
 */
uint8_t max77654_get_cid(void)
{
    uint8_t reg = 0;
    uint8_t bit4 = 0;
    uint8_t cid = 0;

    reg = max77654_read(MAX77654_CID);
    bit4 = (reg & MAX77654_CID4) >> 3;
    cid = bit4 | (reg & MAX77654_CID_Msk);
    LOG("MAX77654 CID = 0x%02X.", cid);
    return cid;
}

/**
 * Convert charge current to register value.
 * @param mA Value read from the register in milliamps.
 * @return The value to write in the register.
 */
static inline uint8_t cc_to_hw(uint32_t mA)
{
    // Datasheet:
    // This 6-bit configuration is a linear transfer function that
    // starts at 7.5mA and ends at 300mA, with 7.5mA increments.

    if (mA >= 300)
        return 0x27 << 2;
    if (mA <= 7)
        return 0x00 << 2;
    return (mA * 2 / 15 - 1) << 2;
}

/**
 * Convert charge voltage to register value.
 * @param mV Voltage in mV.
 * @return Value to write into the register.
 */
static uint8_t cv_to_hw(uint32_t mV)
{
    if (mV >= 4600)
        return 0x28 << 2;
    if (mV <= 3600)
        return 0x00 << 2;
    return ((mV - 3600) / 25) << 2;
}

/**
 * Configure only a few bits out of a register of the chip.
 *  Done by reading the old value, masking, writing back over I2C.
 * @param addr Address of the register.
 * @param newbits Value to put, not yet shifted.
 * @param mask Bitmask with all bits to be modified set to 1.
 */
static void max77654_update(uint8_t addr, uint8_t newbits, uint8_t mask)
{
    max77654_write(addr, (max77654_read(addr) & ~mask) | newbits);
}

struct { uint8_t addr, data; } max77654_conf[] = {
    // Power Rail Configuration

    // Power Rail: 2.7V
    // set SBB0 to 2.7V, buck, 333mA, active discharge, OFF
    { MAX77654_CNFG_SBB0_A, MAX77654_CNFG_SBB_A_TV_2V7 },
    { MAX77654_CNFG_SBB0_B, MAX77654_CNFG_SBB_B_MD | MAX77654_CNFG_SBB_B_IP_333 | MAX77654_CNFG_SBB_B_ADE | MAX77654_CNFG_SBB_B_EN_OFF },

    // Power Rail: 1.8V always on
    // set SBB1 to 1.8V, buck, 333mA, active discharge, ON
    { MAX77654_CNFG_SBB1_B, MAX77654_CNFG_SBB_B_MD | MAX77654_CNFG_SBB_B_IP_333 | MAX77654_CNFG_SBB_B_ADE | MAX77654_CNFG_SBB_B_EN_ON },

    // Power Rail: 1.2V
    // set SBB2 to 1.2V, buck, 333mA, active discharge, OFF
    { MAX77654_CNFG_SBB2_A, MAX77654_CNFG_SBB_A_TV_1V2 },
    { MAX77654_CNFG_SBB2_B, MAX77654_CNFG_SBB_B_MD | MAX77654_CNFG_SBB_B_IP_333 | MAX77654_CNFG_SBB_B_ADE | MAX77654_CNFG_SBB_B_EN_OFF },

    // Power Rail: 1.8VDC_SW, 1.8V switched set LDO0 as load switch,
    // Active Discharge, OFF
    // Not needed; just in case mode set to LDO by mistake.
    { MAX77654_CNFG_LDO0_A, MAX77654_CNFG_LDO_A_TV_1V8 },
    { MAX77654_CNFG_LDO0_B, MAX77654_CNFG_LDO_B_MD | MAX77654_CNFG_LDO_B_ADE | MAX77654_CNFG_LDO_B_EN_OFF },

    // Power Rail: VLED, 2.7V
    // set LDO1 to LDO at 2.7V, Active Discharge, OFF
    { MAX77654_CNFG_LDO1_A, MAX77654_CNFG_LDO_A_TV_2V7 },
    { MAX77654_CNFG_LDO1_B, MAX77654_CNFG_LDO_B_ADE | MAX77654_CNFG_LDO_B_EN_OFF },

    // ICHGIN_LIM_DEF=0: clear this bit so dev brd "M" OTP matches "B" OTP of 0, that is ICHGIN_LIM scale starts at 95mA
    // ICHGIN_LIM assumes CNFG_SBB_TOP.ICHGIN_LIM_DEF = 0
    // Drive strength, slow down to reduce EMI (but reduces efficiency)
    { MAX77654_CNFG_SBB_TOP, 0x02 },

    // GPIO configuration

    // Note: OTP "B" version defaults to Alternate functions, not GPO, so must be reconfigured
    // GPIO0 (Red LED) : GPO, open-drain, logic low
    // set to hi-Z, LED off
    { MAX77654_CNFG_GPIO0, MAX77654_DO },

    // GPIO1 (Green LED) : GPO, open-drain, logic low
    // set to hi-Z, LED off
    { MAX77654_CNFG_GPIO1, MAX77654_DO },

    // GPIO2 (DISP_PWR_EN): GPO, push-pull, logic low -> 10V off
    { MAX77654_CNFG_GPIO2, MAX77654_DRV },

    // Charging configuration

    // Battery charging parameters from datasheets
    // for Varta CP1254 A4 CoinPower battery * 1
    // Charging:
    // - Voltage: 4.3V (4.0V for rapid)
    // - Current: 35mA (standard), 70mA (fast), 140mA (rapid)
    // - Operating temperature (standard & fast) 0 to 45C
    // - Operating temperature (rapid) 20 to 45C
    // - Cut-off time: 5hr (standard), 3h (fast & rapid)
    // - Cut-off current: 1.4mA
    // - Pre-qualification charging not specified; assume 14mA to 2.5V
    // Discharge
    // - Cut-off voltage: 3.0V
    // - Max current (continuous): 140mA
    // - Operating temperature: -20 to 60C

    // Rapid charging is not currently implemented, because:
    // - It is incompatible with MAX design, which assumes non-JEITA
    //   charging to a _higher_ voltage, whereas Varta is to _lower_ voltage.
    // - If implemented rapid charging to 4.0V at temperatures 20-45C,
    //   battery would never fully charge (except when temp in range 0-20C).
    // - Could implement rapid charging if willing to accept
    //   less-than-full battery, or if MCU monitors voltage, increasing
    //   charge voltage & decreasing charge current when battery reaches 3.95V.

    // JEITA Temperatures: COLD=0C, HOT=45C, COOL=20C (not avail, use
    // 15C), WARM=45C (MK11 NTC B is 3380K)
    { MAX77654_CNFG_CHG_A, MAX77654_THM_HOT_45C | MAX77654_THM_WARM_45C | MAX77654_THM_COOL_15C | MAX77654_THM_COLD_00C },

    // "B" version input current limit is 95mA; increase to allow charging
    // + operation (at least during testing) V_CHGIN_MIN=4.3V,
    // I_CHGIN-LIM=190mA, I_pre-charge = 20% of I_fast, charge enable
    { MAX77654_CNFG_CHG_B, MAX77654_VCHGIN_MIN_4V3 | MAX77654_ICHGIN_LIM_190MA | MAX77654_I_PQ | MAX77654_CHG_EN },

    // pre-charge to 2.5V, termination current = 10% = 6.8mA (so CC6
    // green LED will turn off), top-off 5 mins (unknown how long before
    // reach 1.4mA, should be safe)
    { MAX77654_CNFG_CHG_C, MAX77654_CHG_PQ_2V5 | MAX77654_I_TERM_10P | MAX77654_T_TOPOFF_5M },
};

/**
 * Turn the 1.8V rail on/off powering all 1.8V components of the circuit.
 * @param on True for power on.
 */
void max77654_rail_1v8(bool on)
{
    uint8_t en = on ? MAX77654_CNFG_LDO_B_EN_ON : MAX77654_CNFG_LDO_B_EN_OFF;

    LOG("%s", on ? "on" : "off");
    max77654_write(MAX77654_CNFG_LDO0_B,
      MAX77654_CNFG_LDO_B_MD | MAX77654_CNFG_LDO_B_ADE | en);
}

/**
 * Turn the 2.7V rail on/off powering all 2.7V components of the circuit.
 * @param on True for power on.
 */
void max77654_rail_2v7(bool on)
{
    uint8_t en = on ? MAX77654_CNFG_SBB_B_EN_ON : MAX77654_CNFG_SBB_B_EN_OFF;

    LOG("%s", on ? "on" : "off");
    max77654_write(MAX77654_CNFG_SBB0_B,
      MAX77654_CNFG_SBB_B_MD | MAX77654_CNFG_SBB_B_IP_333 | MAX77654_CNFG_SBB_B_ADE | en);
}

/**
 * Turn the 1.2V rail on/off powering all 1.2V components of the circuit.
 * @param on True for power on.
 */
void max77654_rail_1v2(bool on)
{
    uint8_t en = on ? MAX77654_CNFG_SBB_B_EN_ON : MAX77654_CNFG_SBB_B_EN_OFF;

    LOG("%s", on ? "on" : "off");
    max77654_write(MAX77654_CNFG_SBB2_B,
      MAX77654_CNFG_SBB_B_MD | MAX77654_CNFG_SBB_B_IP_333 | MAX77654_CNFG_SBB_B_ADE | en);
}

/**
 * Turn the 10V rail on/off powering all 10V components of the circuit.
 * @param on True for power on.
 */
void max77654_rail_10v(bool on)
{
    uint8_t en = on ? MAX77654_DO : 0;

    LOG("%s", on ? "on" : "off");

    // wait a bit that the power stabilise before starting the 10V
    max77654_write(MAX77654_CNFG_GPIO2, MAX77654_DRV | en); 
}

/**
 * Enable the power rail used to power the LEDs.
 */
void max77654_rail_vled(bool on)
{
    uint8_t en = on ? MAX77654_CNFG_LDO_B_EN_ON : MAX77654_CNFG_LDO_B_EN_OFF;

    LOG("%s", on ? "on" : "off");
    max77654_write(MAX77654_CNFG_LDO1_B, MAX77654_CNFG_LDO_B_ADE | en);
}

/**
 * Turn all rails off starting with the highest voltage to the lowest one, then finally the LEDs.
 */
void max77654_power_off(void)
{
    max77654_rail_10v(false);
    max77654_rail_2v7(false);
    max77654_rail_1v8(false);
    max77654_rail_1v2(false);
    max77654_rail_vled(false);
}

// open-drain, set low, LED on
#define LED_ON 0x00

// open-drain, set to hi-Z, LED off
#define LED_OFF MAX77654_DO

/**
 * Turn the red led connected to the max77654 on or off.
 * @param on Desired state of the led.
 */
void max77654_led_red(bool on)
{
    max77654_write(MAX77654_CNFG_GPIO0, on ? LED_ON : LED_OFF);
}

/**
 * Turn the green led connected to the max77654 on or off.
 * @param on Desired state of the led.
 */
void max77654_led_green(bool on)
{
    max77654_write(MAX77654_CNFG_GPIO1, on ? LED_ON : LED_OFF);
}

/**
 * Set input current upper limit (in mA).
 * @param current Range is 95 to 475 in increments of 95mA, see MAX77654_CNFG_CHG_B definitions.
 */
void max77654_set_current_limit(uint16_t current)
{
    uint8_t charge_bits = 0;

    if (current <= 95)
    {
        charge_bits = MAX77654_ICHGIN_LIM_95MA;
    }
    else if (current <= 190)
    {
        charge_bits = MAX77654_ICHGIN_LIM_190MA;
    }
    else if (current <= 285)
    {
        charge_bits = MAX77654_ICHGIN_LIM_285MA;
    }
    else if (current <= 380)
    {
        charge_bits = MAX77654_ICHGIN_LIM_380MA;
    }
    else
    {
        charge_bits = MAX77654_ICHGIN_LIM_475MA;
    }
    max77654_update(MAX77654_CNFG_CHG_B, charge_bits, MAX77654_ICHGIN_LIM_Msk);
}

/**
 * Set the device in extreme low power mode.
 *  This disconnects the battery from the system, not powered anymore.
 */
void max77564_factory_ship_mode(void) 
{
    max77654_write(MAX77654_CNFG_GLBL, 0xA3);
}

/**
 * Initialize the MAX77654 chip.
 *  Test results: (using resistors to simulate bulk & constant voltage charging)
 *  bulk charging current: 67.4mA, constant voltage: 4.28V
 */
void max77654_init(void)
{
    // verify MAX77654 on I2C bus by attempting to read Chip ID register
    ASSERT(max77654_get_cid() == MAX77654_CID_EXPECTED);

    for (size_t i = 0; i < LEN(max77654_conf); i++)
        max77654_write(max77654_conf[i].addr, max77654_conf[i].data);

    // MAX77654_CNFG_CHG_E: fast/rapid charge current = 67.5mA,
    // safety timer = 3 hours (default)
    // will result in 67.5mA, since next highest value is 75mA
    max77654_update(MAX77654_CNFG_CHG_E, cc_to_hw(70), MAX77654_CHG_CC_Msk);

    // MAX77654_CNFG_CHG_F: JEITA charge current = 70mA (=67.5mA), Thermistor enabled
    max77654_write(MAX77654_CNFG_CHG_F, MAX77654_THM_EN);
    max77654_update(MAX77654_CNFG_CHG_F, cc_to_hw(70), MAX77654_CHG_CC_JEITA_Msk);

    // MAX77654_CNFG_CHG_G: charge voltage 4.3V, not in USB suspend mode
    ASSERT(4300 >= MAX77654_CHG_CV_MIN);
    ASSERT(4300 <= MAX77654_CHG_CV_MAX);
    max77654_update(MAX77654_CNFG_CHG_G, cv_to_hw(4300), MAX77654_CHG_CV_Msk);

    // MAX77654_CNFG_CHG_H: JEITA charge voltage = 4.3V, Thermistor enabled
    max77654_update(MAX77654_CNFG_CHG_H, cv_to_hw(4300), MAX77654_CHG_CV_JEITA_Msk);

    // Turn everything off at startup
    max77654_power_off();

    // MAX77654_CNFG_CHG_I: use defaults: AMUX BATT discharge 300mA
    // scale, AMUX disabled (hi-Z)

    // AMUX configuration

    // By default AMUX is off (high-impedance), for now just turn on,
    // but this is not most power-efficient
    // TODO: implement turning on only when measurements are needed
    // monitor VSYS voltage (=input from CC when plugged in, or from
    // battery when stand-alone)
    max77654_update(MAX77654_CNFG_CHG_I, MAX77654_MUX_SYS_V, MAX77654_MUX_SEL_Msk);

    // Reset LED state
    max77654_led_red(false);
    max77654_led_green(false);
}
