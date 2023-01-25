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

/**
 * @warning CHANGING THIS FILE CAN DAMAGE YOUR HARDWARE.
 *          - Read the PMIC datasheet carefully before changing PMIC settings.
 *          - Breaking the setup function may prevent the nRF from booting.
 *          - Changing I2C behaviour may send dangerous value to the PMIC.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include "critical_functions.h"
#include "nrfx.h"
#include "nrf_soc.h"
#include "nrfx_twim.h"
#include "nrfx_timer.h"
#include "pinout.h"

/**
 * @brief The PMIC address is not exposed outside of this file because it should
 *        never be accessed directly.
 */
#define PMIC_ADDRESS 0x48

static const nrfx_twim_t i2c_bus_0 = NRFX_TWIM_INSTANCE(0);
static const nrfx_twim_t i2c_bus_1 = NRFX_TWIM_INSTANCE(1);
static bool not_real_hardware = false;

void pmic_set_led(led_t led, bool enable)
{
    if (led == RED_LED)
    {
        if (enable)
        {
            app_err(i2c_write(PMIC_ADDRESS, 0x11, 0x2D, 0x00).fail);
        }
        else
        {
            app_err(i2c_write(PMIC_ADDRESS, 0x11, 0x2D, 0x08).fail);
        }
    }
    if (led == GREEN_LED)
    {
        if (enable)
        {
            app_err(i2c_write(PMIC_ADDRESS, 0x12, 0x2D, 0x00).fail);
        }
        else
        {
            app_err(i2c_write(PMIC_ADDRESS, 0x12, 0x2D, 0x08).fail);
        }
    }
}

i2c_response_t i2c_read(uint8_t device_address_7bit,
                        uint8_t register_address,
                        uint8_t register_mask)
{
    if (not_real_hardware)
    {
        return (i2c_response_t){.fail = false, .value = 0x00};
    }

    i2c_response_t i2c_response = {
        .fail = true, // Default if failure
        .value = 0x00,
    };

    // Try several times
    for (uint8_t i = 0; i < 3; i++)
    {
        nrfx_twim_t i2c_handle = i2c_bus_0;
        if (device_address_7bit == CAMERA_ADDRESS)
        {
            i2c_handle = i2c_bus_1;
        }

        nrfx_twim_xfer_desc_t i2c_tx = NRFX_TWIM_XFER_DESC_TX(device_address_7bit,
                                                              &register_address, 1);

        nrfx_twim_xfer_desc_t i2c_rx = NRFX_TWIM_XFER_DESC_RX(device_address_7bit,
                                                              &i2c_response.value, 1);

        nrfx_err_t tx_err = nrfx_twim_xfer(&i2c_handle, &i2c_tx, 0);

        if (tx_err == NRFX_ERROR_BUSY ||
            tx_err == NRFX_ERROR_NOT_SUPPORTED ||
            tx_err == NRFX_ERROR_INTERNAL ||
            tx_err == NRFX_ERROR_INVALID_ADDR ||
            tx_err == NRFX_ERROR_DRV_TWI_ERR_OVERRUN)
        {
            app_err(tx_err);
        }

        nrfx_err_t rx_err = nrfx_twim_xfer(&i2c_handle, &i2c_rx, 0);

        if (rx_err == NRFX_ERROR_BUSY ||
            rx_err == NRFX_ERROR_NOT_SUPPORTED ||
            rx_err == NRFX_ERROR_INTERNAL ||
            rx_err == NRFX_ERROR_INVALID_ADDR ||
            rx_err == NRFX_ERROR_DRV_TWI_ERR_OVERRUN)
        {
            app_err(rx_err);
        }

        if (tx_err == NRFX_SUCCESS && rx_err == NRFX_SUCCESS)
        {
            i2c_response.fail = false;
            break;
        }
    }

    i2c_response.value &= register_mask;

    return i2c_response;
}

i2c_response_t i2c_write(uint8_t device_address_7bit,
                         uint8_t register_address,
                         uint8_t register_mask,
                         uint8_t set_value)
{
    if (not_real_hardware)
    {
        return (i2c_response_t){.fail = false, .value = 0x00};
    }

    i2c_response_t resp = i2c_read(device_address_7bit, register_address, 0xFF);

    if (resp.fail)
    {
        return resp;
    }

    // Create a combined value with the existing data and the new value
    uint8_t updated_value = (resp.value & ~register_mask) |
                            (set_value & register_mask);

    uint8_t payload[] = {register_address, updated_value};

    // Try several times
    for (uint8_t i = 0; i < 3; i++)
    {
        nrfx_twim_t i2c_handle = i2c_bus_0;
        if (device_address_7bit == CAMERA_ADDRESS)
        {
            i2c_handle = i2c_bus_1;
        }

        nrfx_twim_xfer_desc_t i2c_tx = NRFX_TWIM_XFER_DESC_TX(device_address_7bit,
                                                              payload,
                                                              2);

        nrfx_err_t err = nrfx_twim_xfer(&i2c_handle, &i2c_tx, 0);

        if (err == NRFX_ERROR_BUSY ||
            err == NRFX_ERROR_NOT_SUPPORTED ||
            err == NRFX_ERROR_INTERNAL ||
            err == NRFX_ERROR_INVALID_ADDR ||
            err == NRFX_ERROR_DRV_TWI_ERR_OVERRUN)
        {
            app_err(err);
        }

        if (err == NRFX_SUCCESS)
        {
            break;
        }

        // If the last try failed. Don't continue
        if (i == 2)
        {
            resp.fail = true;
            return resp;
        }
    }

    return resp;
}

void enter_bootloader(void)
{
    // Set the persistent memory flag telling the bootloader to go into DFU mode.
    sd_power_gpregret_set(0, 0xB1);

    // Reset the CPU, giving control to the bootloader.
    NVIC_SystemReset();
}

/**
 * @brief Startup related functions.
 */

extern uint32_t _stack_top;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

typedef void (*func)(void);
extern void main(void) __attribute__((noreturn));
extern void SystemInit(void);

static void check_if_battery_charging_and_sleep(nrf_timer_event_t event_type,
                                                void *p_context)
{
    (void)event_type;
    (void)p_context;

    // Get the CHG value from STAT_CHG_B
    i2c_response_t battery_charging_resp = i2c_read(PMIC_ADDRESS, 0x03, 0x0C);
    app_err(battery_charging_resp.fail);
    if (battery_charging_resp.value)
    {
        // Turn off all the rails
        // CAUTION: READ DATASHEET CAREFULLY BEFORE CHANGING THESE
        app_err(i2c_write(PMIC_ADDRESS, 0x13, 0x2D, 0x04).fail); // Turn off 10V on PMIC GPIO2
        app_err(i2c_write(PMIC_ADDRESS, 0x3B, 0x1F, 0x0C).fail); // Turn off LDO to LEDs
        app_err(i2c_write(PMIC_ADDRESS, 0x2A, 0x0F, 0x0C).fail); // Turn off 2.7V
        app_err(i2c_write(PMIC_ADDRESS, 0x39, 0x1F, 0x1C).fail); // Turn off 1.8V on load switch
        app_err(i2c_write(PMIC_ADDRESS, 0x2E, 0x0F, 0x0C).fail); // Turn off 1.2V

        // Disconnect AMUX
        app_err(i2c_write(PMIC_ADDRESS, 0x28, 0x0F, 0x00).fail);

        // Put PMIC main bias into low power mode
        app_err(i2c_write(PMIC_ADDRESS, 0x10, 0x20, 0x20).fail);

        // Set up the touch interrupt pin

        nrf_gpio_cfg_sense_input(TOUCH_INTERRUPT_PIN,
                                 NRF_GPIO_PIN_NOPULL,
                                 NRF_GPIO_PIN_SENSE_LOW);

        NRF_POWER->SYSTEMOFF = 1;

        // Never return
        while (1)
        {
        }
    }
}

void entry_and_critical_setup(void)
{
    // Initialise the RAM
    {
        uint32_t *p_src = &_sidata;
        uint32_t *p_dest = &_sdata;

        while (p_dest < &_edata)
        {
            *p_dest++ = *p_src++;
        }

        uint32_t *p_bss = &_sbss;
        uint32_t *p_bss_end = &_ebss;
        while (p_bss < p_bss_end)
        {
            *p_bss++ = 0ul;
        }
    }

    // Set up chip level errata fixes from Nordic
    SystemInit();

    // Enable the the DC DC convertor
    NRF_POWER->DCDCEN = 0x00000001;

    // Set up the I2C buses
    {
        nrfx_twim_config_t bus_0_config = NRFX_TWIM_DEFAULT_CONFIG(PMIC_TOUCH_I2C_SCL_PIN,
                                                                   PMIC_TOUCH_I2C_SDA_PIN);
        bus_0_config.frequency = NRF_TWIM_FREQ_100K;

        nrfx_twim_config_t bus_1_config = NRFX_TWIM_DEFAULT_CONFIG(CAMERA_I2C_SCL_PIN,
                                                                   CAMERA_I2C_SDA_PIN);
        bus_1_config.frequency = NRF_TWIM_FREQ_100K;

        app_err(nrfx_twim_init(&i2c_bus_0, &bus_0_config, NULL, NULL));
        app_err(nrfx_twim_init(&i2c_bus_1, &bus_1_config, NULL, NULL));

        nrfx_twim_enable(&i2c_bus_0);
        nrfx_twim_enable(&i2c_bus_1);
    }

    // Check the PMIC and initialize battery charger settings
    {
        // Read the PMIC CID
        i2c_response_t resp = i2c_read(PMIC_ADDRESS, 0x14, 0x0F);

        if (resp.fail || resp.value != 0x02)
        {
            app_err(resp.value);
        }

        // Set up battery charger voltage & current
        float voltage = 4.3f;
        float current = 70.0f;

        uint8_t voltage_setting = (uint8_t)round((voltage - 3.6f) / 0.025f) << 2;
        uint8_t current_setting = (uint8_t)round((current - 7.5f) / 7.5f) << 2;

        // TODO set temperature cutouts

        // Apply the constant voltage setting
        app_err(i2c_write(PMIC_ADDRESS, 0x26, 0xFC, voltage_setting).fail);
        // TODO set the JETIA voltage

        // Apply the constant current setting
        app_err(i2c_write(PMIC_ADDRESS, 0x24, 0xFC, current_setting).fail);
        // TODO set the JETIA current
    }

    // Configure the touch IC
    {
        // Read the touch CID
        i2c_response_t resp = i2c_read(TOUCH_ADDRESS, 0x00, 0xFF);
        if (resp.fail || resp.value != 0x41)
        {
            app_err(resp.value);
        }

        app_err(i2c_write(TOUCH_ADDRESS, 0xD0, 0x60, 0x60).fail); // Ack resets and enable event mode
        app_err(i2c_write(TOUCH_ADDRESS, 0xD1, 0xFF, 0x03).fail); // Enable ch0 and ch1
        app_err(i2c_write(TOUCH_ADDRESS, 0xD2, 0x20, 0x20).fail); // Disable auto power mode switching // TODO enable ULP mode
        app_err(i2c_write(TOUCH_ADDRESS, 0x40, 0xFF, 0x01).fail); // Enable rx0 to cap sensing
        app_err(i2c_write(TOUCH_ADDRESS, 0x41, 0xFF, 0x02).fail); // Enable rx1 to cap sensing
        app_err(i2c_write(TOUCH_ADDRESS, 0x43, 0x60, 0x20).fail); // 15pf, 1/8 divider on ch0
        app_err(i2c_write(TOUCH_ADDRESS, 0x44, 0x60, 0x20).fail); // 15pf, 1/8 divider on ch1
        app_err(i2c_write(TOUCH_ADDRESS, 0x46, 0xFF, 0x1E).fail); // ATI base 75 and target = 30 on ch0
        app_err(i2c_write(TOUCH_ADDRESS, 0x47, 0xFF, 0x1E).fail); // ATI base 75 and target = 30 on ch1
        app_err(i2c_write(TOUCH_ADDRESS, 0x60, 0xFF, 0x0A).fail); // Proximity thresholds
        app_err(i2c_write(TOUCH_ADDRESS, 0x62, 0xFF, 0x0A).fail); // Proximity thresholds
        app_err(i2c_write(TOUCH_ADDRESS, 0x61, 0xFF, 0x0A).fail); // Proximity thresholds
        app_err(i2c_write(TOUCH_ADDRESS, 0x63, 0xFF, 0x0A).fail); // Proximity thresholds
        app_err(i2c_write(TOUCH_ADDRESS, 0xD0, 0x22, 0x22).fail); // Redo ATI and enable event mode
                                                                  // TODO what interrupts are enabled?

        // Delay to complete configuration
        for (int i = 0; i < 10000000; i++)
        {
            __asm volatile("nop");
        }
    }

    check_if_battery_charging_and_sleep(0, NULL); // This won't return if charging

    // Set up a timer for checking charge state periodically
    {
        nrfx_timer_t timer = NRFX_TIMER_INSTANCE(4);

        nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG;
        timer_config.frequency = NRF_TIMER_FREQ_31250Hz;
        timer_config.bit_width = NRF_TIMER_BIT_WIDTH_24;
        app_err(nrfx_timer_init(&timer,
                                &timer_config,
                                check_if_battery_charging_and_sleep));

        nrfx_timer_extended_compare(&timer, NRF_TIMER_CC_CHANNEL0, 156250,
                                    NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

        nrfx_timer_enable(&timer);
    }

    // Power up everything for normal operation.
    // CAUTION: READ DATASHEET CAREFULLY BEFORE CHANGING THESE
    {
        // Set SBB2 to 1.2V and turn on
        app_err(i2c_write(PMIC_ADDRESS, 0x2D, 0xFF, 0x08).fail);
        app_err(i2c_write(PMIC_ADDRESS, 0x2E, 0x4F, 0x4F).fail);

        // Set LDO0 to load switch mode and turn on
        app_err(i2c_write(PMIC_ADDRESS, 0x39, 0x1F, 0x1F).fail);

        // Set SBB0 to 2.8V and turn on
        app_err(i2c_write(PMIC_ADDRESS, 0x29, 0xFF, 0x28).fail);
        app_err(i2c_write(PMIC_ADDRESS, 0x2A, 0x4F, 0x4F).fail);

        // Configure LEDs on GPIO0 and GPIO1 as open drain outputs. Set to hi-z
        app_err(i2c_write(PMIC_ADDRESS, 0x11, 0x2D, 0x08).fail);
        app_err(i2c_write(PMIC_ADDRESS, 0x12, 0x2D, 0x08).fail);

        // Set LDO1 to 3.3V and turn on
        app_err(i2c_write(PMIC_ADDRESS, 0x3A, 0xFF, 0x64).fail);
        app_err(i2c_write(PMIC_ADDRESS, 0x3B, 0x1F, 0x0F).fail);

        // Enable the 10V boost
        app_err(i2c_write(PMIC_ADDRESS, 0x13, 0x2D, 0x0C).fail);

        // Connect AMUX to battery voltage
        app_err(i2c_write(PMIC_ADDRESS, 0x28, 0x0F, 0x03).fail);
    }

    main();
}

void Default_Handler(void)
{
    // Trigger a breakpoint when debugging
    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
    {
        __BKPT();
    }

    app_err(0xDEADBEEF);
}

/**
 * @brief Specific handlers and interrupt vector table for the nRF52832
 */

void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemoryManagement_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));
void POWER_CLOCK_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RADIO_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UARTE0_UART0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void NFCT_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void GPIOTE_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SAADC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIMER0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIMER1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIMER2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TEMP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RNG_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ECB_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CCM_AAR_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void WDT_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void QDEC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void COMP_LPCOMP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SWI0_EGU0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SWI1_EGU1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SWI2_EGU2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SWI3_EGU3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SWI4_EGU4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SWI5_EGU5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIMER3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIMER4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PWM0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PDM_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void MWU_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PWM1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PWM2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPIM2_SPIS2_SPI2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2S_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FPU_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

const func __Vectors[] __attribute__((section(".isr_vector"), used)) = {
    (func)&_stack_top,
    entry_and_critical_setup, // aka Reset_Handler
    NMI_Handler,
    HardFault_Handler,
    MemoryManagement_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0,
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,

    /* External Interrupts */
    POWER_CLOCK_IRQHandler,
    RADIO_IRQHandler,
    UARTE0_UART0_IRQHandler,
    SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler,
    SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler,
    NFCT_IRQHandler,
    GPIOTE_IRQHandler,
    SAADC_IRQHandler,
    TIMER0_IRQHandler,
    TIMER1_IRQHandler,
    TIMER2_IRQHandler,
    RTC0_IRQHandler,
    TEMP_IRQHandler,
    RNG_IRQHandler,
    ECB_IRQHandler,
    CCM_AAR_IRQHandler,
    WDT_IRQHandler,
    RTC1_IRQHandler,
    QDEC_IRQHandler,
    COMP_LPCOMP_IRQHandler,
    SWI0_EGU0_IRQHandler,
    SWI1_EGU1_IRQHandler,
    SWI2_EGU2_IRQHandler,
    SWI3_EGU3_IRQHandler,
    SWI4_EGU4_IRQHandler,
    SWI5_EGU5_IRQHandler,
    TIMER3_IRQHandler,
    TIMER4_IRQHandler,
    PWM0_IRQHandler,
    PDM_IRQHandler,
    0,
    0,
    MWU_IRQHandler,
    PWM1_IRQHandler,
    PWM2_IRQHandler,
    SPIM2_SPIS2_SPI2_IRQHandler,
    RTC2_IRQHandler,
    I2S_IRQHandler,
    FPU_IRQHandler,
};