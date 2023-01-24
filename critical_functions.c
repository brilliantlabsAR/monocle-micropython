#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include "critical_functions.h"
#include "nrfx.h"
#include "nrf_soc.h"
#include "nrfx_twim.h"
#include "pinout.h"

/**
 * @warning CHANGING THIS FILE CAN DAMAGE YOUR HARDWARE.
 *          - Read the PMIC datasheet carefully before changing PMIC settings.
 *          - Breaking the setup function may prevent the nRF from booting.
 *          - Changing I2C behaviour may send dangerous value to the PMIC.
 */

/**
 * @brief I2C helper functions
 */

#define PMIC_ADDRESS 0x48

static const nrfx_twim_t i2c_bus_0 = NRFX_TWIM_INSTANCE(0);
static const nrfx_twim_t i2c_bus_1 = NRFX_TWIM_INSTANCE(1);

static bool not_real_hardware = false;

void i2c_init(void)
{
    nrfx_twim_config_t bus_0_config = NRFX_TWIM_DEFAULT_CONFIG(PMIC_TOUCH_I2C_SCL_PIN,
                                                               PMIC_TOUCH_I2C_SDA_PIN);
    bus_0_config.frequency = NRF_TWIM_FREQ_400K;

    nrfx_twim_config_t bus_1_config = NRFX_TWIM_DEFAULT_CONFIG(CAMERA_I2C_SCL_PIN,
                                                               CAMERA_I2C_SDA_PIN);
    bus_1_config.frequency = NRF_TWIM_FREQ_400K;

    app_err(nrfx_twim_init(&i2c_bus_0, &bus_0_config, NULL, NULL));
    app_err(nrfx_twim_init(&i2c_bus_1, &bus_1_config, NULL, NULL));

    nrfx_twim_enable(&i2c_bus_0);
    nrfx_twim_enable(&i2c_bus_1);
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
        if (device_address_7bit == OV5640_ADDRRESS)
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
        if (device_address_7bit == OV5640_ADDRRESS)
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
            log("Ran through all attempts");
            resp.fail = true;
            return resp;
        }
    }

    // Check that the register was correctly updated
    resp = i2c_read(device_address_7bit, register_address, 0xFF);

    if (resp.value != updated_value)
    {
        log("register didn't update");
        resp.fail = true;
    }

    return resp;
}

/**
 * @brief Startup and sleep functions.
 */
static bool startup_already_done = false;

static void check_if_battery_charging_and_sleep(void)
{
    // Get the CHG value from STAT_CHG_B
    i2c_response_t battery_charging_resp = i2c_read(PMIC_ADDRESS, 0x03, 0x02);
    app_err(battery_charging_resp.fail);
    if (battery_charging_resp.value)
    {
        // Turn off all the rails
        // CAUTION: READ DATASHEET CAREFULLY BEFORE CHANGING THESE
        app_err(i2c_write(PMIC_ADDRESS, 0x13, 0x2D, 0x25).fail); // Turn off 10V on PMIC GPIO2
        app_err(i2c_write(PMIC_ADDRESS, 0x3B, 0x1F, 0x1C).fail); // Turn off load switch to LEDs // TODO is this really set to load switch?
        app_err(i2c_write(PMIC_ADDRESS, 0x2A, 0x0F, 0x0C).fail); // Turn off 2.7V
        app_err(i2c_write(PMIC_ADDRESS, 0x39, 0x1F, 0x1C).fail); // Turn off 1.8V on load switch
        app_err(i2c_write(PMIC_ADDRESS, 0x2E, 0x0F, 0x0C).fail); // Turn off 1.2V

        // Disconnect AMUX
        app_err(i2c_write(PMIC_ADDRESS, 0x28, 0x0F, 0x00).fail);

        // Put PMIC main bias into low power mode
        // app_err(i2c_write(PMIC_ADDRESS, 0x10, 0x20, 0x20).fail);

        // Touch into low power mode

        // Set up the touch event
        nrf_gpio_cfg_sense_input(IQS620_TOUCH_RDY_PIN, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

        // Sleep
        app_err(sd_power_system_off());
    }
}

void setup_pmic_and_sleep_mode(void)
{
    app_err(startup_already_done);

    i2c_init();

    log("MAX77654 early setup");
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

        // Apply the constant voltage setting
        app_err(i2c_write(PMIC_ADDRESS, 0x26, 0xFC, voltage_setting).fail);
        // TODO set the JETIA voltage

        // Apply the constant current setting
        app_err(i2c_write(PMIC_ADDRESS, 0x24, 0xFC, current_setting).fail);
        // TODO set the JETIA current
    }

    // configure the touch
    // TODO
    // Configure the touch IC for low power mode
    // Configure and enable GPIO sense

    while (1)
    {
    }

    check_if_battery_charging_and_sleep(); // This won't return if charging
    // timer_add_task(timer_500ms, check_if_battery_charging_and_sleep);

    // Power up everything for normal operation.
    // CAUTION: READ DATASHEET CAREFULLY BEFORE CHANGING THESE
    {
        // Put PMIC main bias into normal mode
        // app_err(i2c_write(PMIC_ADDRESS, 0x10, 0x20, 0x00).fail);

        // Set SBB2 to 1.2V and turn on
        app_err(i2c_write(PMIC_ADDRESS, 0x2D, 0xFF, 0x08).fail);
        app_err(i2c_write(PMIC_ADDRESS, 0x2E, 0x4F, 0x4F).fail);

        // Set LDO0 to load switch mode and turn on
        app_err(i2c_write(PMIC_ADDRESS, 0x39, 0x1F, 0x1F).fail);

        // Set SBB0 to 2.7V and turn on
        app_err(i2c_write(PMIC_ADDRESS, 0x29, 0xFF, 0x26).fail);
        app_err(i2c_write(PMIC_ADDRESS, 0x2A, 0x4F, 0x4F).fail);

        // Set LDO1 to load switch mode and turn on
        app_err(i2c_write(PMIC_ADDRESS, 0x3B, 0x1F, 0x1F).fail);

        // Enable the 10V boost
        app_err(i2c_write(PMIC_ADDRESS, 0x13, 0x2D, 0x0C).fail);

        // Connect AMUX to battery voltage
        app_err(i2c_write(PMIC_ADDRESS, 0x28, 0x0F, 0x03).fail);
    }

    startup_already_done = true;
}

/**
 * @brief PMIC helper functions.
 */