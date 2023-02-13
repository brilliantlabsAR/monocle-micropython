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

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "monocle.h"
#include "touch.h"
#include "config-tables.h"

#include "genhdr/mpversion.h"
#include "mpconfigport.h"
#include "mphalport.h"
#include "py/builtin.h"
#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/repl.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "shared/readline/readline.h"
#include "shared/runtime/pyexec.h"

#include "ble_gattc.h"
#include "ble.h"
#include "nrf_gpio.h"
#include "nrf_nvic.h"
#include "nrf_sdm.h"
#include "nrfx_glue.h"
#include "nrfx_gpiote.h"
#include "nrfx_log.h"
#include "nrfx_saadc.h"
#include "nrfx_systick.h"
// #include "nrfx_timer.h"
#include "nrfx_rtc.h"
#include "nrfx.h"

nrf_nvic_state_t nrf_nvic_state = {{0}, 0};

extern uint32_t _ram_start;
static uint32_t ram_start = (uint32_t)&_ram_start;
extern uint32_t _stack_top;
extern uint32_t _stack_bot;
extern uint32_t _heap_start;
extern uint32_t _heap_end;

static struct ble_handles_t
{
    uint16_t connection;
    uint8_t advertising;
    ble_gatts_char_handles_t repl_rx_unused;
    ble_gatts_char_handles_t repl_tx_notification;
    ble_gatts_char_handles_t data_rx_unused;
    ble_gatts_char_handles_t data_tx_notification;
} ble_handles = {
    .connection = BLE_CONN_HANDLE_INVALID,
    .advertising = BLE_GAP_ADV_SET_HANDLE_NOT_SET,
};

static struct advertising_data_t
{
    uint8_t length;
    uint8_t payload[31];
} adv = {
    .length = 0,
    .payload = {0},
};

#define BLE_PREFERRED_MAX_MTU 128
uint16_t ble_negotiated_mtu;

static struct ble_ring_buffer_t
{
    uint8_t buffer[1024];
    uint16_t head;
    uint16_t tail;
} repl_rx = {
    .buffer = "",
    .head = 0,
    .tail = 0,
},
  repl_tx = {
      .buffer = "",
      .head = 0,
      .tail = 0,
},
  data_tx = {
      .buffer = "",
      .head = 0,
      .tail = 0,
};

bool ble_are_tx_notifications_enabled(ble_tx_channel_t channel)
{
    uint8_t value_buffer[2] = {0};

    ble_gatts_value_t value = {.len = sizeof(value_buffer),
                               .offset = 0,
                               .p_value = &(value_buffer[0])};

    // Read the CCCD attribute value for one of the tx characteristics
    switch (channel)
    {
    case REPL_TX:
    {
        app_err(sd_ble_gatts_value_get(ble_handles.connection,
                                       ble_handles.repl_tx_notification.cccd_handle,
                                       &value));
        break;
    }

    case DATA_TX:
    {
        app_err(sd_ble_gatts_value_get(ble_handles.connection,
                                       ble_handles.data_tx_notification.cccd_handle,
                                       &value));
        break;
    }
    }

    // Value of 0x0001 means that notifications are enabled
    if (value_buffer[1] == 0x00 && value_buffer[0] == 0x01)
    {
        return true;
    }

    return false;
}

static bool ble_send_repl_data(void)
{
    if (ble_handles.connection == BLE_CONN_HANDLE_INVALID)
    {
        return true;
    }

    if (!ble_are_tx_notifications_enabled(REPL_TX))
    {
        return true;
    }

    if (repl_tx.head == repl_tx.tail)
    {
        return true;
    }

    uint8_t tx_buffer[BLE_PREFERRED_MAX_MTU] = "";
    uint16_t tx_length = 0;

    uint16_t buffered_tail = repl_tx.tail;

    while (buffered_tail != repl_tx.head)
    {
        tx_buffer[tx_length++] = repl_tx.buffer[buffered_tail++];

        if (buffered_tail == sizeof(repl_tx.buffer))
        {
            buffered_tail = 0;
        }

        if (tx_length == ble_negotiated_mtu)
        {
            break;
        }
    }

    // Initialise the handle value parameters
    ble_gatts_hvx_params_t hvx_params = {0};
    hvx_params.handle = ble_handles.repl_tx_notification.value_handle;
    hvx_params.p_data = tx_buffer;
    hvx_params.p_len = (uint16_t *)&tx_length;
    hvx_params.type = BLE_GATT_HVX_NOTIFICATION;

    uint32_t status = sd_ble_gatts_hvx(ble_handles.connection, &hvx_params);

    if (status == NRF_SUCCESS)
    {
        repl_tx.tail = buffered_tail;
    }

    return false;
}

static bool ble_send_raw_data(void)
{
    if (ble_handles.connection == BLE_CONN_HANDLE_INVALID)
    {
        return true;
    }

    if (!ble_are_tx_notifications_enabled(DATA_TX))
    {
        return true;
    }

    if (data_tx.head == data_tx.tail)
    {
        return true;
    }

    uint8_t tx_buffer[BLE_PREFERRED_MAX_MTU] = "";
    uint16_t tx_length = 0;

    uint16_t buffered_tail = data_tx.tail;

    while (buffered_tail != data_tx.head)
    {
        tx_buffer[tx_length++] = data_tx.buffer[buffered_tail++];

        if (buffered_tail == sizeof(data_tx.buffer))
        {
            buffered_tail = 0;
        }

        if (tx_length == ble_negotiated_mtu)
        {
            break;
        }
    }

    // Initialise the handle value parameters
    ble_gatts_hvx_params_t hvx_params = {0};
    hvx_params.handle = ble_handles.data_tx_notification.value_handle;
    hvx_params.p_data = tx_buffer;
    hvx_params.p_len = (uint16_t *)&tx_length;
    hvx_params.type = BLE_GATT_HVX_NOTIFICATION;

    uint32_t status = sd_ble_gatts_hvx(ble_handles.connection, &hvx_params);

    if (status == NRF_SUCCESS)
    {
        data_tx.tail = buffered_tail;
    }

    return false;
}

void ble_buffer_raw_tx_data(const uint8_t *bytes, size_t len)
{
    for (uint16_t position = 0; position < len; position++)
    {
        while (data_tx.head == data_tx.tail - 1)
        {
            MICROPY_EVENT_POLL_HOOK;
        }

        data_tx.buffer[data_tx.head++] = bytes[position];

        if (data_tx.head == sizeof(data_tx.buffer))
        {
            data_tx.head = 0;
        }
    }
}

void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len)
{
    for (uint16_t position = 0; position < len; position++)
    {
        while (repl_tx.head == repl_tx.tail - 1)
        {
            MICROPY_EVENT_POLL_HOOK;
        }

        repl_tx.buffer[repl_tx.head++] = str[position];

        if (repl_tx.head == sizeof(repl_tx.buffer))
        {
            repl_tx.head = 0;
        }
    }
}

int mp_hal_stdin_rx_chr(void)
{
    if (repl_rx.head == repl_rx.tail)
    {
        MICROPY_EVENT_POLL_HOOK;
        return 0;
    }

    uint16_t next = repl_rx.tail + 1;

    if (next == sizeof(repl_rx.buffer))
    {
        next = 0;
    }

    int character = repl_rx.buffer[repl_rx.tail];

    repl_rx.tail = next;

    return character;
}

static void touch_interrupt_handler(nrfx_gpiote_pin_t pin,
                                    nrf_gpiote_polarity_t polarity)
{
    (void)pin;
    (void)polarity;

    /*
    // Read the interrupt registers
    i2c_response_t global_reg_0x11 = i2c_read(TOUCH_I2C_ADDRESS, 0x11, 0xFF);
    i2c_response_t sar_ui_reg_0x12 = i2c_read(TOUCH_I2C_ADDRESS, 0x12, 0xFF);
    i2c_response_t sar_ui_reg_0x13 = i2c_read(TOUCH_I2C_ADDRESS, 0x13, 0xFF);
    */
    touch_action_t touch_action = A_TOUCH; // TODO this should be decoded from the I2C responses
    touch_event_handler(touch_action);
}

static void softdevice_assert_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    app_err(0x5D000000 & id);
}

void SD_EVT_IRQHandler(void)
{
    uint32_t evt_id;
    uint8_t ble_evt_buffer[sizeof(ble_evt_t) + BLE_PREFERRED_MAX_MTU];

    // While any softdevice events are pending, service flash operations
    while (sd_evt_get(&evt_id) != NRF_ERROR_NOT_FOUND)
    {
        switch (evt_id)
        {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
            // TODO In case we add a filesystem in the future
            break;

        case NRF_EVT_FLASH_OPERATION_ERROR:
            // TODO In case we add a filesystem in the future
            break;

        default:
            break;
        }
    }

    // While any BLE events are pending
    while (1)
    {
        // Pull an event from the queue
        uint16_t buffer_len = sizeof(ble_evt_buffer);
        uint32_t status = sd_ble_evt_get(ble_evt_buffer, &buffer_len);

        // If we get the done status, we can exit the handler
        if (status == NRF_ERROR_NOT_FOUND)
        {
            break;
        }

        // Check for other errors
        app_err(status);

        // Make a pointer from the buffer which we can use to find the event
        ble_evt_t *ble_evt = (ble_evt_t *)ble_evt_buffer;

        switch (ble_evt->header.evt_id)
        {

        case BLE_GAP_EVT_CONNECTED:
        {
            ble_handles.connection = ble_evt->evt.gap_evt.conn_handle;

            ble_gap_conn_params_t conn_params;

            app_err(sd_ble_gap_ppcp_get(&conn_params));
            app_err(sd_ble_gap_conn_param_update(ble_handles.connection,
                                                 &conn_params));
            app_err(sd_ble_gatts_sys_attr_set(ble_handles.connection,
                                              NULL,
                                              0,
                                              0));
            break;
        }

        case BLE_GAP_EVT_DISCONNECTED:
        {
            ble_handles.connection = BLE_CONN_HANDLE_INVALID;
            app_err(sd_ble_gap_adv_start(ble_handles.advertising, 1));
            break;
        }

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            ble_gap_phys_t const phys = {
                .rx_phys = BLE_GAP_PHY_1MBPS,
                .tx_phys = BLE_GAP_PHY_1MBPS,
            };
            app_err(sd_ble_gap_phy_update(ble_evt->evt.gap_evt.conn_handle,
                                          &phys));
            break;
        }

        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
        {
            // The client's desired MTU size
            uint16_t client_mtu =
                ble_evt->evt.gatts_evt.params.exchange_mtu_request.client_rx_mtu;

            // Respond with our max MTU size
            sd_ble_gatts_exchange_mtu_reply(ble_handles.connection,
                                            BLE_PREFERRED_MAX_MTU);

            // Choose the smaller MTU as the final length we'll use
            // -3 bytes to accommodate for Op-code and attribute service
            ble_negotiated_mtu = BLE_PREFERRED_MAX_MTU < client_mtu
                                     ? BLE_PREFERRED_MAX_MTU - 3
                                     : client_mtu - 3;
            break;
        }

        case BLE_GATTS_EVT_WRITE:
        {
            if (1 /** REPL service */)
            {
                for (uint16_t i = 0;
                     i < ble_evt->evt.gatts_evt.params.write.len;
                     i++)
                {
                    uint16_t next = repl_rx.head + 1;

                    if (next == sizeof(repl_rx.buffer))
                    {
                        next = 0;
                    }

                    if (next == repl_rx.tail)
                    {
                        break;
                    }

                    repl_rx.buffer[repl_rx.head] =
                        ble_evt->evt.gatts_evt.params.write.data[i];

                    repl_rx.head = next;
                }
            }

            // TODO if data service

            break;
        }

        case BLE_GATTS_EVT_TIMEOUT:
        {
            app_err(sd_ble_gap_disconnect(
                ble_handles.connection,
                BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION));
            break;
        }

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
        {
            app_err(sd_ble_gatts_sys_attr_set(ble_handles.connection,
                                              NULL,
                                              0,
                                              0));
            break;
        }

        case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST:
        {
            app_err(sd_ble_gap_data_length_update(ble_handles.connection,
                                                  NULL,
                                                  NULL));
            break;
        }

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
        {
            // TODO enabling pairing later
            app_err(sd_ble_gap_sec_params_reply(
                ble_handles.connection,
                BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP,
                NULL,
                NULL));
            break;
        }

        case BLE_GAP_EVT_CONN_PARAM_UPDATE:
        case BLE_GAP_EVT_PHY_UPDATE:
        case BLE_GAP_EVT_DATA_LENGTH_UPDATE:
        case BLE_GATTS_EVT_HVN_TX_COMPLETE:
        {
            // Unused events
            break;
        }

        default:
        {
            NRFX_LOG_ERROR("Unhandled BLE event: %u", ble_evt->header.evt_id);
            break;
        }
        }
    }
}

void unused_rtc_event_handler(nrfx_rtc_int_type_t int_type) {}

int main(void)
{
    NRFX_LOG_ERROR(RTT_CTRL_CLEAR
                   "\rMicroPython on Monocle - " BUILD_VERSION
                   " (" MICROPY_GIT_HASH ").");

    // Set up the PMIC and go to sleep if on charge
    monocle_critical_startup();

    // Setup touch interrupt
    {
        app_err(nrfx_gpiote_init(NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY));

        nrfx_gpiote_in_config_t config =
            NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);

        app_err(nrfx_gpiote_in_init(TOUCH_INTERRUPT_PIN,
                                    &config,
                                    touch_interrupt_handler));

        nrfx_gpiote_in_event_enable(TOUCH_INTERRUPT_PIN,
                                    true);
    }

    // Setup battery ADC input
    {
        app_err(nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY));

        nrfx_saadc_channel_t channel =
            NRFX_SAADC_DEFAULT_CHANNEL_SE(BATTERY_LEVEL_PIN, 0);

        channel.channel_config.reference = NRF_SAADC_REFERENCE_INTERNAL;
        channel.channel_config.gain = NRF_SAADC_GAIN1_2;

        app_err(nrfx_saadc_channel_config(&channel));
    }

    // Set up the remaining GPIO
    {
        nrf_gpio_cfg_output(CAMERA_RESET_PIN);
        nrf_gpio_cfg_output(CAMERA_SLEEP_PIN);
        nrf_gpio_cfg_output(DISPLAY_CS_PIN);
        nrf_gpio_cfg_output(DISPLAY_RESET_PIN);
        nrf_gpio_cfg_output(FLASH_CS_PIN);
        nrf_gpio_cfg_output(FPGA_CS_PIN);
        nrf_gpio_cfg_output(FPGA_INTERRUPT_CONFIG_PIN);
    }

    // Setup the real time clock for micropython's time functions
    {
        nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(1);
        nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;

        // 1024Hz = >1ms resolution
        config.prescaler = RTC_FREQ_TO_PRESCALER(1024);

        app_err(nrfx_rtc_init(&rtc, &config, unused_rtc_event_handler));
        nrfx_rtc_enable(&rtc);
    }

    // Setup the Bluetooth
    {
        // Init LF clock
        nrf_clock_lf_cfg_t clock_config = {
            .source = NRF_CLOCK_LF_SRC_XTAL,
            .rc_ctiv = 0,
            .rc_temp_ctiv = 0,
            .accuracy = NRF_CLOCK_LF_ACCURACY_10_PPM};

        // Enable the softdevice
        app_err(sd_softdevice_enable(&clock_config, softdevice_assert_handler));

        // Enable softdevice interrupt
        app_err(sd_nvic_EnableIRQ((IRQn_Type)SD_EVT_IRQn));

        // Add GAP configuration to the BLE stack
        ble_cfg_t cfg;
        cfg.conn_cfg.conn_cfg_tag = 1;
        cfg.conn_cfg.params.gap_conn_cfg.conn_count = 1;
        cfg.conn_cfg.params.gap_conn_cfg.event_length = 3;
        app_err(sd_ble_cfg_set(BLE_CONN_CFG_GAP, &cfg, ram_start));

        // Set BLE role to peripheral only
        memset(&cfg, 0, sizeof(cfg));
        cfg.gap_cfg.role_count_cfg.periph_role_count = 1;
        app_err(sd_ble_cfg_set(BLE_GAP_CFG_ROLE_COUNT, &cfg, ram_start));

        // Set max MTU size
        memset(&cfg, 0, sizeof(cfg));
        cfg.conn_cfg.conn_cfg_tag = 1;
        cfg.conn_cfg.params.gatt_conn_cfg.att_mtu = BLE_PREFERRED_MAX_MTU;
        app_err(sd_ble_cfg_set(BLE_CONN_CFG_GATT, &cfg, ram_start));

        // Configure a single queued transfer
        memset(&cfg, 0, sizeof(cfg));
        cfg.conn_cfg.conn_cfg_tag = 1;
        cfg.conn_cfg.params.gatts_conn_cfg.hvn_tx_queue_size = 1;
        app_err(sd_ble_cfg_set(BLE_CONN_CFG_GATTS, &cfg, ram_start));

        // Configure number of custom UUIDs
        memset(&cfg, 0, sizeof(cfg));
        cfg.common_cfg.vs_uuid_cfg.vs_uuid_count = 2;
        app_err(sd_ble_cfg_set(BLE_COMMON_CFG_VS_UUID, &cfg, ram_start));

        // Configure GATTS attribute table
        memset(&cfg, 0, sizeof(cfg));
        cfg.gatts_cfg.attr_tab_size.attr_tab_size = 1408;
        app_err(sd_ble_cfg_set(BLE_GATTS_CFG_ATTR_TAB_SIZE, &cfg, ram_start));

        // No service changed attribute needed
        memset(&cfg, 0, sizeof(cfg));
        cfg.gatts_cfg.service_changed.service_changed = 0;
        app_err(sd_ble_cfg_set(BLE_GATTS_CFG_SERVICE_CHANGED, &cfg, ram_start));

        // Start the Softdevice
        app_err(sd_ble_enable(&ram_start));

        NRFX_LOG_ERROR("Softdevice using 0x%x bytes of RAM",
                       ram_start - 0x20000000);

        // Set security to open // TODO make this paired
        ble_gap_conn_sec_mode_t sec_mode;
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

        // Set device name
        const char device_name[] = "monocle";
        app_err(sd_ble_gap_device_name_set(&sec_mode,
                                           (const uint8_t *)device_name,
                                           sizeof(device_name) - 1));

        // Set connection parameters
        ble_gap_conn_params_t gap_conn_params = {0};
        gap_conn_params.min_conn_interval = (15 * 1000) / 1250;
        gap_conn_params.max_conn_interval = (15 * 1000) / 1250;
        gap_conn_params.slave_latency = 3;
        gap_conn_params.conn_sup_timeout = (2000 * 1000) / 10000;
        app_err(sd_ble_gap_ppcp_set(&gap_conn_params));

        // Create the service UUIDs
        ble_uuid128_t repl_service_uuid128 = {.uuid128 =
                                                  {0x9E, 0xCA, 0xDC, 0x24,
                                                   0x0E, 0xE5, 0xA9, 0xE0,
                                                   0x93, 0xF3, 0xA3, 0xB5,
                                                   0x00, 0x00, 0x40, 0x6E}};

        ble_uuid128_t data_service_uuid128 = {.uuid128 =
                                                  {0x9D, 0x47, 0x0F, 0x90,
                                                   0xFF, 0x57, 0xCE, 0xB4,
                                                   0x9A, 0x42, 0xAC, 0x7B,
                                                   0x00, 0x00, 0x70, 0xE5}};

        ble_uuid_t repl_service_uuid = {.uuid = 0x0001};
        ble_uuid_t data_service_uuid = {.uuid = 0x0001};

        app_err(sd_ble_uuid_vs_add(&repl_service_uuid128,
                                   &repl_service_uuid.type));
        app_err(sd_ble_uuid_vs_add(&data_service_uuid128,
                                   &data_service_uuid.type));

        uint16_t repl_service_handle;
        uint16_t data_service_handle;

        // Configure both RX characteristics as one because they're identical
        ble_uuid_t rx_uuid = {.uuid = 0x0002};
        rx_uuid.type = repl_service_uuid.type;

        ble_gatts_char_md_t rx_char_md = {0};
        rx_char_md.char_props.write = 1;
        rx_char_md.char_props.write_wo_resp = 1;

        ble_gatts_attr_md_t rx_attr_md = {0};
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&rx_attr_md.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&rx_attr_md.write_perm);
        rx_attr_md.vloc = BLE_GATTS_VLOC_STACK;
        rx_attr_md.vlen = 1;

        ble_gatts_attr_t rx_attr = {0};
        rx_attr.p_uuid = &rx_uuid;
        rx_attr.p_attr_md = &rx_attr_md;
        rx_attr.init_len = sizeof(uint8_t);
        rx_attr.max_len = BLE_PREFERRED_MAX_MTU - 3;

        // Configure both TX characteristics as one because they're identical
        ble_uuid_t tx_uuid = {.uuid = 0x0003};
        tx_uuid.type = repl_service_uuid.type;

        ble_gatts_char_md_t tx_char_md = {0};
        tx_char_md.char_props.notify = 1;

        ble_gatts_attr_md_t tx_attr_md = {0};
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&tx_attr_md.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&tx_attr_md.write_perm);
        tx_attr_md.vloc = BLE_GATTS_VLOC_STACK;
        tx_attr_md.vlen = 1;

        ble_gatts_attr_t tx_attr = {0};
        tx_attr.p_uuid = &tx_uuid;
        tx_attr.p_attr_md = &tx_attr_md;
        tx_attr.init_len = sizeof(uint8_t);
        tx_attr.max_len = BLE_PREFERRED_MAX_MTU - 3;

        // Characteristics must be added sequentially after each service
        app_err(sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                         &repl_service_uuid,
                                         &repl_service_handle));

        app_err(sd_ble_gatts_characteristic_add(repl_service_handle,
                                                &rx_char_md,
                                                &rx_attr,
                                                &ble_handles.repl_rx_unused));

        app_err(sd_ble_gatts_characteristic_add(repl_service_handle,
                                                &tx_char_md,
                                                &tx_attr,
                                                &ble_handles.repl_tx_notification));

        // Copy the service UUID type to both rx and tx UUID
        rx_uuid.type = data_service_uuid.type;
        tx_uuid.type = data_service_uuid.type;

        app_err(sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                         &data_service_uuid,
                                         &data_service_handle));

        app_err(sd_ble_gatts_characteristic_add(data_service_handle,
                                                &rx_char_md,
                                                &rx_attr,
                                                &ble_handles.data_rx_unused));

        app_err(sd_ble_gatts_characteristic_add(data_service_handle,
                                                &tx_char_md,
                                                &tx_attr,
                                                &ble_handles.data_tx_notification));

        // Add name to advertising payload
        adv.payload[adv.length++] = strlen((const char *)device_name) + 1;
        adv.payload[adv.length++] = BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME;
        memcpy(&adv.payload[adv.length],
               device_name,
               sizeof(device_name));
        adv.length += strlen((const char *)device_name);

        // Set discovery mode flag
        adv.payload[adv.length++] = 0x02;
        adv.payload[adv.length++] = BLE_GAP_AD_TYPE_FLAGS;
        adv.payload[adv.length++] = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

        // Add only the REPL service to the advertising data
        uint8_t encoded_uuid_length;
        app_err(sd_ble_uuid_encode(&repl_service_uuid,
                                   &encoded_uuid_length,
                                   &adv.payload[adv.length + 2]));

        adv.payload[adv.length++] = 0x01 + encoded_uuid_length;
        adv.payload[adv.length++] = BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE;
        adv.length += encoded_uuid_length;

        ble_gap_adv_data_t adv_data = {
            .adv_data.p_data = adv.payload,
            .adv_data.len = adv.length,
            .scan_rsp_data.p_data = NULL,
            .scan_rsp_data.len = 0};

        // Set up advertising parameters
        ble_gap_adv_params_t adv_params;
        memset(&adv_params, 0, sizeof(adv_params));
        adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
        adv_params.primary_phy = BLE_GAP_PHY_1MBPS;
        adv_params.secondary_phy = BLE_GAP_PHY_1MBPS;
        adv_params.interval = (20 * 1000) / 625;

        // Configure the advertising set
        app_err(sd_ble_gap_adv_set_configure(&ble_handles.advertising,
                                             &adv_data,
                                             &adv_params));

        // Start advertising
        app_err(sd_ble_gap_adv_start(ble_handles.advertising, 1));
    }

    // Check if external flash has an FPGA image and boot it
    {
        // TODO

        // Otherwise boot from the internal image of the FPGA
        nrf_gpio_pin_set(FPGA_INTERRUPT_CONFIG_PIN);
    }

    // Setup and start the display
    {
        // Each byte of the configuration must be sent in pairs
        for (size_t i = 0; i < sizeof(display_config); i += 2)
        {
            uint8_t command[2] = {display_config[i],      // Address
                                  display_config[i + 1]}; // Value
            spi_write(DISPLAY, command, 2, false);
        }
    }

    // Setup the camera
    {
        nrfx_systick_delay_ms(750); // TODO optimize the FPGA to not need this delay
        nrfx_systick_delay_ms(5000);
        NRFX_LOG_ERROR("camera setup");

        // TODO optimize this away. Ask the FPGA to start the camera clock
        uint8_t command[2] = {0x10, 0x09};
        spi_write(FPGA, command, 2, false);

        // Power on sequence, references: Datasheet section 2.7.1; Application Notes section 3.1.1
        // assume XCLK signal coming from the FPGA
        nrf_gpio_pin_write(CAMERA_SLEEP_PIN, true);
        nrf_gpio_pin_write(CAMERA_RESET_PIN, !true);
        nrfx_systick_delay_ms(5);
        nrfx_systick_delay_ms(8);
        nrf_gpio_pin_write(CAMERA_SLEEP_PIN, false);
        nrfx_systick_delay_ms(2);
        nrf_gpio_pin_write(CAMERA_RESET_PIN, !false);
        nrfx_systick_delay_ms(20);

        // Read the camera CID (one of them)
        i2c_response_t resp = i2c_read(CAMERA_I2C_ADDRESS, 0x300A, 0xFF);
        if (resp.fail || resp.value != 0x56)
        {
            NRFX_LOG_ERROR("Error: Camera not found.");
            monocle_set_led(RED_LED, true);
        }

        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3103, 0xFF, 0x11).fail); // system clock from pad
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3008, 0xFF, 0x82).fail);

        // combined configuration table for YUV422 mode
        for (size_t i = 0; i < MP_ARRAY_SIZE(ov5640_yuv422_direct_tbl); i++)
        {
            app_err(i2c_write(CAMERA_I2C_ADDRESS, ov5640_yuv422_direct_tbl[i].addr, 0xFF,
                              ov5640_yuv422_direct_tbl[i].value)
                        .fail);
        }

        // reduce camera output image size
        const uint16_t camera_reduced_width = 640;
        const uint16_t camera_reduced_height = 400;
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0x03).fail);                         // start group 3
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3808, 0xFF, camera_reduced_width >> 8).fail);    // DVPHO, upper byte
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3809, 0xFF, camera_reduced_width & 0xFF).fail);  // DVPHO, lower byte
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x380a, 0xFF, camera_reduced_height >> 8).fail);   // DVPVO, upper byte
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x380b, 0xFF, camera_reduced_height & 0xFF).fail); // DVPVO, lower byte
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0x13).fail);                         // end group 3
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0xA3).fail);                         // launch group 3

        // configure focus data
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3000, 0xFF, 0x20).fail); // reset MCU
        // program ov5640 MCU firmware
        for (size_t i = 0; i < MP_ARRAY_SIZE(ov5640_af_config_tbl); i++)
        {
            app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x8000 + i, 0xFF, ov5640_af_config_tbl[i]).fail);
        }
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3022, 0xFF, 0x00).fail); // ? undocumented
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3023, 0xFF, 0x00).fail); // ?
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3024, 0xFF, 0x00).fail); // ?
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3025, 0xFF, 0x00).fail); // ?
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3026, 0xFF, 0x00).fail); // ?
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3027, 0xFF, 0x00).fail); // ?
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3028, 0xFF, 0x00).fail); // ?
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3029, 0xFF, 0x7F).fail); // ?
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3000, 0xFF, 0x00).fail); // enable MCU

        // configure light mode
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0x03).fail); // start group 3
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3400, 0xFF, 0x04).fail); // auto AWB value 0
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3401, 0xFF, 0x00).fail); // auto AWB value 1
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3402, 0xFF, 0x04).fail); // auto AWB value 2
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3403, 0xFF, 0x00).fail); // auto AWB value 3
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3404, 0xFF, 0x04).fail); // auto AWB value 4
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3405, 0xFF, 0x00).fail); // auto AWB value 5
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3406, 0xFF, 0x00).fail); // auto AWB value 6
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0x13).fail); // end group 3
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0xA3).fail); // launch group 3

        // configure saturation
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0x03).fail); // start group 3
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5381, 0xFF, 0x1C).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5382, 0xFF, 0x5A).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5383, 0xFF, 0x06).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5384, 0xFF, 0x1A).fail); // saturation 0 value 0
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5385, 0xFF, 0x66).fail); // saturation 0 value 1
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5386, 0xFF, 0x80).fail); // saturation 0 value 2
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5387, 0xFF, 0x82).fail); // saturation 0 value 3
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5388, 0xFF, 0x80).fail); // saturation 0 value 4
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5389, 0xFF, 0x02).fail); // saturation 0 value 5
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x538a, 0xFF, 0x01).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x538b, 0xFF, 0x98).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0x13).fail); // end group 3
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0xA3).fail); // launch group 3

        // configure brightness
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0x03).fail); // start group 3
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5587, 0xFF, 0x00).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5588, 0xFF, 0x01).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0x13).fail); // end group 3
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0xA3).fail); // launch group 3

        // configure contrast
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0x03).fail); // start group 3
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5585, 0xFF, 0x1C).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5586, 0xFF, 0x2C).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0x13).fail); // end group 3
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x3212, 0xFF, 0xA3).fail); // launch group 3

        // configure sharpness
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5308, 0xFF, 0x25).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5300, 0xFF, 0x08).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5301, 0xFF, 0x30).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5302, 0xFF, 0x10).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5303, 0xFF, 0x00).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x5309, 0xFF, 0x08).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x530a, 0xFF, 0x30).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x530b, 0xFF, 0x04).fail);
        app_err(i2c_write(CAMERA_I2C_ADDRESS, 0x530c, 0xFF, 0x06).fail);

        // Put the camera to sleep
        nrf_gpio_pin_write(CAMERA_SLEEP_PIN, false);
    }

    // Initialise the stack pointer for the main thread
    mp_stack_set_top(&_stack_top);

    // Set the stack limit as smaller than the real stack so we can recover
    mp_stack_set_limit((char *)&_stack_top - (char *)&_stack_bot - 400);

    // Start garbage collection, micropython and the REPL
    gc_init(&_heap_start, &_heap_end);
    mp_init();
    readline_init0();

    // Stay in the friendly or raw REPL until a reset is called
    for (;;)
    {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL)
        {
            if (pyexec_raw_repl() != 0)
            {
                break;
            }
        }
        else
        {
            if (pyexec_friendly_repl() != 0)
            {
                break;
            }
        }
    }

    // On exit, clean up and reset
    gc_sweep_all();
    mp_deinit();
    sd_softdevice_disable();
    NVIC_SystemReset();
}

void mp_event_poll_hook(void)
{
    if (ble_send_repl_data() && ble_send_raw_data())
    {
        extern void mp_handle_pending(bool);
        mp_handle_pending(true);

        // Clear exceptions and PendingIRQ from the FPU
        __set_FPSCR(__get_FPSCR() & ~(0x0000009F));
        (void)__get_FPSCR();
        NVIC_ClearPendingIRQ(FPU_IRQn);

        app_err(sd_app_evt_wait());
    }
}

void gc_collect(void)
{
    // start the GC
    gc_collect_start();

    // Get stack pointer
    uintptr_t sp;
    __asm__("mov %0, sp\n"
            : "=r"(sp));

    // Trace the stack, including the registers
    // (since they live on the stack in this function)
    gc_collect_root((void **)sp, ((uint32_t)&_stack_top - sp) / sizeof(uint32_t));

    // end the GC
    gc_collect_end();
}

void nlr_jump_fail(void *val)
{
    app_err((uint32_t)val);
    NVIC_SystemReset();
}
