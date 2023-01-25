#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "nrfx_log.h"

/**
 * @brief Sets up the PMIC and low power mode of the Monocle. Won't return if
 *        the device is charging, and will put everything to sleep. Should only
 *        be called once during startup.
 */
void setup_pmic_and_sleep_mode(void);

/**
 * @brief PMIC helper functions.
 */

/**
 * @brief I2C helper functions
 */

#define IQS620_ADDRESS 0x44
#define OV5640_ADDRRESS 0x3C

typedef struct i2c_response_t
{
    bool fail;
    uint8_t value;
} i2c_response_t;

void i2c_init(void);

i2c_response_t i2c_read(uint8_t device_address_7bit,
                        uint8_t register_address,
                        uint8_t register_mask);

i2c_response_t i2c_write(uint8_t device_address_7bit,
                         uint8_t register_address,
                         uint8_t register_mask,
                         uint8_t set_value);

/**
 * @brief PMIC helpers
 */

typedef enum led_t
{
    GREEN_LED,
    RED_LED
} led_t;

void pmic_set_led(led_t led, bool enable);

/**
 * @brief Error and logging macro
 */

#define app_err(eval)                                                          \
    do                                                                         \
    {                                                                          \
        nrfx_err_t err = (eval);                                               \
        if (0x0000FFFF & err)                                                  \
        {                                                                      \
            log("App error code: 0x%x at %s:%u\r\n", err, __FILE__, __LINE__); \
            if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)              \
            {                                                                  \
                __BKPT();                                                      \
            }                                                                  \
            NVIC_SystemReset();                                                \
        }                                                                      \
    } while (0)
