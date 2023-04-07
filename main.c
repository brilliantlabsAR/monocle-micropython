/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Ltd. (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Ltd.
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
#include "bluetooth.h"
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
#include "py/stream.h"
#include "shared/readline/readline.h"
#include "shared/runtime/interrupt_char.h"
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
    ble_gatts_char_handles_t repl_rx_write;
    ble_gatts_char_handles_t repl_tx_notification;
    ble_gatts_char_handles_t data_rx_write;
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

#define BLE_PREFERRED_MAX_MTU 256
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

size_t ble_get_max_payload_size(void)
{
    return ble_negotiated_mtu;
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

bool ble_send_raw_data(const uint8_t *bytes, size_t len)
{
    if (ble_handles.connection == BLE_CONN_HANDLE_INVALID)
    {
        return true;
    }

    if (!ble_are_tx_notifications_enabled(DATA_TX))
    {
        return true;
    }

    // Initialise the handle value parameters
    ble_gatts_hvx_params_t hvx_params = {0};
    hvx_params.handle = ble_handles.data_tx_notification.value_handle;
    hvx_params.p_data = bytes;
    hvx_params.p_len = (uint16_t *)&len;
    hvx_params.type = BLE_GATT_HVX_NOTIFICATION;

    uint32_t status = sd_ble_gatts_hvx(ble_handles.connection, &hvx_params);

    if (status == NRF_SUCCESS)
    {
        return false;
    }

    return true;
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
    while (repl_rx.head == repl_rx.tail)
    {
        MICROPY_EVENT_POLL_HOOK;
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

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags)
{
    return (repl_rx.head == repl_rx.tail) ? poll_flags & MP_STREAM_POLL_RD : 0;
}

static void touch_interrupt_handler(nrfx_gpiote_pin_t pin,
                                    nrf_gpiote_polarity_t polarity)
{
    (void)pin;
    (void)polarity;

    i2c_response_t interrupt = monocle_i2c_read(TOUCH_I2C_ADDRESS, 0x12, 0xFF);
    app_err(interrupt.fail);

    if (interrupt.value & 0x10)
    {
        touch_event_handler(TOUCH_A);
    }

    if (interrupt.value & 0x20)
    {
        touch_event_handler(TOUCH_B);
    }
}

touch_action_t touch_get_state(void)
{
    i2c_response_t interrupt = monocle_i2c_read(TOUCH_I2C_ADDRESS, 0x12, 0xFF);
    app_err(interrupt.fail);

    if ((interrupt.value & 0x30) == 0x30)
    {
        return TOUCH_BOTH;
    }

    if (interrupt.value & 0x10)
    {
        return TOUCH_A;
    }

    if (interrupt.value & 0x20)
    {
        return TOUCH_B;
    }

    return TOUCH_NONE;
}

void unused_rtc_event_handler(nrfx_rtc_int_type_t int_type) {}

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
            // If REPL service
            if (ble_evt->evt.gatts_evt.params.write.handle ==
                ble_handles.repl_rx_write.value_handle)
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

                    // Catch keyboard interrupts
                    if (ble_evt->evt.gatts_evt.params.write.data[i] ==
                        mp_interrupt_char)
                    {
                        mp_sched_keyboard_interrupt();
                    }

                    // Otherwise add the character to the ring buffer
                    else
                    {
                        repl_rx.buffer[repl_rx.head] =
                            ble_evt->evt.gatts_evt.params.write.data[i];
                    }

                    repl_rx.head = next;
                }
            }

            // If data service
            if (ble_evt->evt.gatts_evt.params.write.handle ==
                ble_handles.data_rx_write.value_handle)
            {
                bluetooth_receive_callback_handler(
                    ble_evt->evt.gatts_evt.params.write.data,
                    ble_evt->evt.gatts_evt.params.write.len);
            }

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
            NRFX_LOG("Unhandled BLE event: %u", ble_evt->header.evt_id);
            break;
        }
        }
    }
}

int main(void)
{
    NRFX_LOG(RTT_CTRL_CLEAR
             "\rMicroPython on Monocle - " BUILD_VERSION
             " (" MICROPY_GIT_HASH ")");

    // Set up the PMIC and go to sleep if on charge
    monocle_critical_startup();

    // Start the FPGA
    monocle_fpga_reset(true);

    // Setup the camera
    {
        // Start the camera clock
        uint8_t command[2] = {0x10, 0x09};
        monocle_spi_write(FPGA, command, 2, false);

        // Reset sequence taken from Datasheet figure 2-3
        nrf_gpio_pin_write(CAMERA_RESET_PIN, false);
        nrf_gpio_pin_write(CAMERA_SLEEP_PIN, true);
        nrfx_systick_delay_ms(5); // t2
        nrf_gpio_pin_write(CAMERA_SLEEP_PIN, false);
        nrfx_systick_delay_ms(1); // t3
        nrf_gpio_pin_write(CAMERA_RESET_PIN, true);
        nrfx_systick_delay_ms(20); // t4

        // Read the camera CID (one of them)
        i2c_response_t resp = monocle_i2c_read(CAMERA_I2C_ADDRESS, 0x300A, 0xFF);
        if (resp.fail || resp.value != 0x56)
        {
            // TODO add entry in health monitor if camera didn't initialise
            NRFX_LOG("Camera not detected");
            monocle_set_led(RED_LED, true);
        }

        // Software reset
        monocle_i2c_write(CAMERA_I2C_ADDRESS, 0x3008, 0xFF, 0x82);
        nrfx_systick_delay_ms(5);

        // Send the default configuration
        for (size_t i = 0;
             i < sizeof(camera_config) / sizeof(camera_config_t);
             i++)
        {
            monocle_i2c_write(CAMERA_I2C_ADDRESS,
                              camera_config[i].address,
                              0xFF,
                              camera_config[i].value);
        }

        // Put the camera to sleep
        nrf_gpio_pin_write(CAMERA_SLEEP_PIN, true);
    }

    // Enable, and setup the display
    {
        nrf_gpio_pin_write(DISPLAY_RESET_PIN, true);
        nrfx_systick_delay_ms(1);

        for (size_t i = 0;
             i < sizeof(display_config) / sizeof(display_config_t);
             i++)
        {
            uint8_t command[2] = {display_config[i].address,
                                  display_config[i].value};
            monocle_spi_write(DISPLAY, command, 2, false);
        }
    }

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

    // Setup the real time clock for micropython's time functions
    {
        nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(1);
        nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;

        // 1024Hz = >1ms resolution
        config.prescaler = RTC_FREQ_TO_PRESCALER(1024);

        app_err(nrfx_rtc_init(&rtc, &config, unused_rtc_event_handler));
        nrfx_rtc_enable(&rtc);

        // Wake up the softdevice every ms so that MICROPY_EVENT_POLL_HOOK
        // doesn't block for longer than it's supposed to
        nrfx_rtc_tick_enable(&rtc, true);
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

        // Configure two queued transfers
        memset(&cfg, 0, sizeof(cfg));
        cfg.conn_cfg.conn_cfg_tag = 1;
        cfg.conn_cfg.params.gatts_conn_cfg.hvn_tx_queue_size = 2;
        app_err(sd_ble_cfg_set(BLE_CONN_CFG_GATTS, &cfg, ram_start));

        // Configure number of custom UUIDs
        memset(&cfg, 0, sizeof(cfg));
        cfg.common_cfg.vs_uuid_cfg.vs_uuid_count = 2;
        app_err(sd_ble_cfg_set(BLE_COMMON_CFG_VS_UUID, &cfg, ram_start));

        // Configure GATTS attribute table
        memset(&cfg, 0, sizeof(cfg));
        cfg.gatts_cfg.attr_tab_size.attr_tab_size = 365 * 4; // multiples of 4
        app_err(sd_ble_cfg_set(BLE_GATTS_CFG_ATTR_TAB_SIZE, &cfg, ram_start));

        // No service changed attribute needed
        memset(&cfg, 0, sizeof(cfg));
        cfg.gatts_cfg.service_changed.service_changed = 0;
        app_err(sd_ble_cfg_set(BLE_GATTS_CFG_SERVICE_CHANGED, &cfg, ram_start));

        // Start the Softdevice
        app_err(sd_ble_enable(&ram_start));

        NRFX_LOG("Softdevice using 0x%x bytes of RAM", ram_start - 0x20000000);

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
                                                &ble_handles.repl_rx_write));

        app_err(sd_ble_gatts_characteristic_add(repl_service_handle,
                                                &tx_char_md,
                                                &tx_attr,
                                                &ble_handles.repl_tx_notification));

        // The UUID were increased by the SoftDevice
        rx_uuid.type = data_service_uuid.type;
        tx_uuid.type = data_service_uuid.type;

        app_err(sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                         &data_service_uuid,
                                         &data_service_handle));

        app_err(sd_ble_gatts_characteristic_add(data_service_handle,
                                                &rx_char_md,
                                                &rx_attr,
                                                &ble_handles.data_rx_write));

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

    // Initialise the stack pointer for the main thread
    mp_stack_set_top(&_stack_top);

    // Set the stack limit as smaller than the real stack so we can recover
    mp_stack_set_limit((char *)&_stack_top - (char *)&_stack_bot - 400);

    // Start garbage collection, micropython and the REPL
    gc_init(&_heap_start, &_heap_end);
    mp_init();
    readline_init0();

    // Mount the filesystem, or format if needed
    pyexec_frozen_module("_mountfs.py");

    // Run the user's main file if it exists
    pyexec_file_if_exists("main.py");

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
    // Keep sending REPL data. Then if no more data is pending
    if (ble_send_repl_data())
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
