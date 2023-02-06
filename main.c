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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/repl.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/builtin.h"
#include "mphalport.h"

#include "shared/readline/readline.h"
#include "shared/runtime/pyexec.h"
#include "genhdr/mpversion.h"

#include "monocle.h"
#include "touch.h"
#include "nrfx.h"

#include "nrfx_log.h"
#include "nrf_sdm.h"
#include "nrf_power.h"
#include "nrfx_twi.h"
#include "nrfx_spim.h"
#include "nrfx_systick.h"
#include "nrfx_gpiote.h"
#include "nrf_nvic.h"
#include "nrfx_saadc.h"
#include "nrfx_timer.h"
#include "nrfx_glue.h"
#include "nrf_soc.h"
#include "ble_gatts.h"
#include "ble_gattc.h"
#include "ble.h"

#include "bluetooth.h"

nrf_nvic_state_t nrf_nvic_state = {{0}, 0};

extern uint32_t _stack_top;
extern uint32_t _stack_bot;
extern uint32_t _heap_start;
extern uint32_t _heap_end;
extern uint32_t _ram_start;
static uint32_t ram_start = (uint32_t)&_ram_start;

static const nrfx_spim_t spi_bus_2 = NRFX_SPIM_INSTANCE(2);

// Reverse the byte order to be easier to declare.
#define UUID128(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
    {                                                           \
        .uuid128 = {                                            \
            0x##p,                                              \
            0x##o,                                              \
            0x##n,                                              \
            0x##m,                                              \
            0x##l,                                              \
            0x##k,                                              \
            0x##j,                                              \
            0x##i,                                              \
            0x##h,                                              \
            0x##g,                                              \
            0x##f,                                              \
            0x##e,                                              \
            0x##d,                                              \
            0x##c,                                              \
            0x##b,                                              \
            0x##a,                                              \
        }                                                       \
    }

// Advertising data which needs to stay in scope between connections.
static uint8_t ble_adv_len;
static uint8_t ble_adv_buf[31];
static uint8_t ble_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
static ble_uuid128_t ble_nus_uuid128 = UUID128(6E, 40, 00, 00, B5, A3, F3, 93, E0, A9, E5, 0E, 24, DC, CA, 9E);
static ble_uuid128_t ble_raw_uuid128 = UUID128(E5, 70, 00, 00, 7B, AC, 42, 9A, B4, CE, 57, FF, 90, 0F, 47, 9D);
static uint16_t ble_nus_service_handle;
static uint16_t ble_raw_service_handle;
ble_gatts_char_handles_t ble_nus_tx_char;
ble_gatts_char_handles_t ble_nus_rx_char;
ble_gatts_char_handles_t ble_raw_tx_char;
ble_gatts_char_handles_t ble_raw_rx_char;
uint16_t ble_conn_handle = BLE_CONN_HANDLE_INVALID;
uint16_t ble_negotiated_mtu;
ring_buf_t nus_rx;
ring_buf_t nus_tx;


// ECX336CN datasheet section 10.1
uint8_t const ecx336cn_config[] = {
    [0x00] = 0x9E, // [0]=0 -> enter power save mode
    [0x01] = 0x20,
    /* * */
    [0x03] = 0x20, // 1125
    [0x04] = 0x3F,
    [0x05] = 0xC8, // 1125  DITHERON, LUMINANCE=0x00=2000cd/m2=medium (Datasheet 10.8)
    /* * */
    [0x07] = 0x40,
    [0x08] = 0x80, // Luminance adjustment: OTPCALDAC_REGDIS=0 (preset mode per reg 5), white chromaticity: OTPDG_REGDIS=0 (preset mode, default)
    /* * */
    [0x0A] = 0x10,
    /* * */
    [0x0F] = 0x56,
    /* * */
    [0x20] = 0x01,
    /* * */
    [0x22] = 0x40,
    [0x23] = 0x40,
    [0x24] = 0x40,
    [0x25] = 0x80,
    [0x26] = 0x40,
    [0x27] = 0x40,
    [0x28] = 0x40,
    [0x29] = 0x0B,
    [0x2A] = 0xBE, // CALDAC=190 (ignored, since OTPCALDAC_REGDIS=0)
    [0x2B] = 0x3C,
    [0x2C] = 0x02,
    [0x2D] = 0x7A,
    [0x2E] = 0x02,
    [0x2F] = 0xFA,
    [0x30] = 0x26,
    [0x31] = 0x01,
    [0x32] = 0xB6,
    /* * */
    [0x34] = 0x03,
    [0x35] = 0x60, // 1125
    /* * */
    [0x37] = 0x76,
    [0x38] = 0x02,
    [0x39] = 0xFE,
    [0x3A] = 0x02,
    [0x3B] = 0x71, // 1125
    /* * */
    [0x3D] = 0x1B,
    /* * */
    [0x3F] = 0x1C,
    [0x40] = 0x02, // 1125
    [0x41] = 0x4D, // 1125
    [0x42] = 0x02, // 1125
    [0x43] = 0x4E, // 1125
    [0x44] = 0x80,
    /* * */
    [0x47] = 0x2D, // 1125
    [0x48] = 0x08,
    [0x49] = 0x01, // 1125
    [0x4A] = 0x7E, // 1125
    [0x4B] = 0x08,
    [0x4C] = 0x0A, // 1125
    [0x4D] = 0x04, // 1125
    /* * */
    [0x4F] = 0x3A, // 1125
    [0x50] = 0x01, // 1125
    [0x51] = 0x58, // 1125
    [0x52] = 0x01,
    [0x53] = 0x2D,
    [0x54] = 0x01,
    [0x55] = 0x15, // 1125
    /* * */
    [0x57] = 0x2B,
    [0x58] = 0x11, // 1125
    [0x59] = 0x02,
    [0x5A] = 0x11, // 1125
    [0x5B] = 0x02,
    [0x5C] = 0x25,
    [0x5D] = 0x04, // 1125
    [0x5E] = 0x0B, // 1125
    /* * */
    [0x60] = 0x23,
    [0x61] = 0x02,
    [0x62] = 0x1A, // 1125
    /* * */
    [0x64] = 0x0A, // 1125
    [0x65] = 0x01, // 1125
    [0x66] = 0x8C, // 1125
    [0x67] = 0x30, // 1125
    /* * */
    [0x69] = 0x00, // 1125
    /* * */
    [0x6D] = 0x00, // 1125
    /* * */
    [0x6F] = 0x60,
    /* * */
    [0x79] = 0x68,
};

// TODO clean up all these SPI functions into a simple driver
void fpga_cmd_read(uint16_t cmd, uint8_t *buf, size_t len);
void fpga_cmd_write(uint16_t cmd, const uint8_t *buf, size_t len);

void spi_chip_select(uint8_t cs_pin)
{
    nrf_gpio_pin_clear(cs_pin);
    nrfx_systick_delay_us(100);
}

void spi_chip_deselect(uint8_t cs_pin)
{
    nrf_gpio_pin_set(cs_pin);
    nrfx_systick_delay_us(100);
}

void spi_read(uint8_t *buf, size_t len)
{
    for (size_t n = 0; len > 0; len -= n, buf += n)
    {
        n = MIN(255, len);
        nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_RX(buf, n);
        app_err(nrfx_spim_xfer(&spi_bus_2, &xfer, 0));
    }
}

void spi_write(uint8_t const *buf, size_t len)
{
    for (size_t n = 0; len > 0; len -= n, buf += n)
    {
        n = MIN(255, len);
        nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(buf, n);
        app_err(nrfx_spim_xfer(&spi_bus_2, &xfer, 0));
    }
}

static inline const void ecx336cn_write_byte(uint8_t addr, uint8_t data)
{
    spi_chip_select(DISPLAY_CS_PIN);
    spi_write(&addr, 1);
    spi_write(&data, 1);
    spi_chip_deselect(DISPLAY_CS_PIN);
}

static inline uint8_t ecx336cn_read_byte(uint8_t addr)
{
    uint8_t data;

    ecx336cn_write_byte(0x80, 0x01);
    ecx336cn_write_byte(0x81, addr);

    spi_chip_select(DISPLAY_CS_PIN);
    spi_write(&addr, 1);
    spi_read(&data, 1);
    spi_chip_deselect(DISPLAY_CS_PIN);

    return data;
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

bool ring_full(ring_buf_t const *ring)
{
    uint16_t next = ring->tail + 1;

    if (next == sizeof(ring->buffer))
    {
        next = 0;
    }
    return next == ring->head;
}

bool ring_empty(ring_buf_t const *ring)
{
    return ring->head == ring->tail;
}

void ring_push(ring_buf_t *ring, uint8_t byte)
{
    ring->buffer[ring->tail++] = byte;

    if (ring->tail == sizeof(ring->buffer))
    {
        ring->tail = 0;
    }
}

uint8_t ring_pop(ring_buf_t *ring)
{
    uint8_t byte = ring->buffer[ring->head++];

    if (ring->head == sizeof(ring->buffer))
    {
        ring->head = 0;
    }
    return byte;
}

static inline void ble_adv_add_device_name(const char *name)
{
    ble_adv_buf[ble_adv_len++] = 1 + strlen(name);
    ble_adv_buf[ble_adv_len++] = BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME;
    memcpy(&ble_adv_buf[ble_adv_len], name, strlen(name));
    ble_adv_len += strlen(name);
}

static inline void ble_adv_add_discovery_mode(void)
{
    ble_adv_buf[ble_adv_len++] = 2;
    ble_adv_buf[ble_adv_len++] = BLE_GAP_AD_TYPE_FLAGS;
    ble_adv_buf[ble_adv_len++] = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
}

static inline void ble_adv_add_uuid(ble_uuid_t *uuid)
{
    uint8_t len;
    uint8_t *p_adv_size;

    p_adv_size = &ble_adv_buf[ble_adv_len];
    ble_adv_buf[ble_adv_len++] = 1;
    ble_adv_buf[ble_adv_len++] = BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE;

    app_err(sd_ble_uuid_encode(uuid, &len, &ble_adv_buf[ble_adv_len]));
    ble_adv_len += len;
    *p_adv_size += len;
}

/**
 * Add rx characteristic to the advertisement.
 */
static void ble_add_rx_characteristic(uint16_t service_handle, ble_gatts_char_handles_t *rx_char, ble_uuid_t *uuid)
{
    ble_gatts_char_md_t rx_char_md = {0};
    rx_char_md.char_props.write = 1;
    rx_char_md.char_props.write_wo_resp = 1;

    ble_gatts_attr_md_t rx_attr_md = {0};
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&rx_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&rx_attr_md.write_perm);
    rx_attr_md.vloc = BLE_GATTS_VLOC_STACK;
    rx_attr_md.vlen = 1;

    ble_gatts_attr_t rx_attr = {0};
    rx_attr.p_uuid = uuid;
    rx_attr.p_attr_md = &rx_attr_md;
    rx_attr.init_len = sizeof(uint8_t);
    rx_attr.max_len = BLE_MAX_MTU_LENGTH - 3;

    app_err(sd_ble_gatts_characteristic_add(service_handle, &rx_char_md, &rx_attr,
                                            rx_char));
}

/**
 * Add tx characteristic to the advertisement.
 */
static void ble_add_tx_characteristic(uint16_t service_handle, ble_gatts_char_handles_t *tx_char, ble_uuid_t *uuid)
{
    ble_gatts_char_md_t tx_char_md = {0};
    tx_char_md.char_props.notify = 1;

    ble_gatts_attr_md_t tx_attr_md = {0};
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&tx_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&tx_attr_md.write_perm);
    tx_attr_md.vloc = BLE_GATTS_VLOC_STACK;
    tx_attr_md.vlen = 1;

    ble_gatts_attr_t tx_attr = {0};
    tx_attr.p_uuid = uuid;
    tx_attr.p_attr_md = &tx_attr_md;
    tx_attr.init_len = sizeof(uint8_t);
    tx_attr.max_len = BLE_MAX_MTU_LENGTH - 3;

    app_err(sd_ble_gatts_characteristic_add(service_handle, &tx_char_md, &tx_attr,
                                            tx_char));
}

static void ble_configure_nus_service(ble_uuid_t *service_uuid)
{
    // Set the 16 bit UUIDs for the service and characteristics
    service_uuid->uuid = 0x0001;
    ble_uuid_t rx_uuid = {.uuid = 0x0002};
    ble_uuid_t tx_uuid = {.uuid = 0x0003};

    app_err(sd_ble_uuid_vs_add(&ble_nus_uuid128, &service_uuid->type));

    app_err(sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                     service_uuid, &ble_nus_service_handle));

    // Copy the service UUID type to both rx and tx UUID
    rx_uuid.type = service_uuid->type;
    tx_uuid.type = service_uuid->type;

    // Add tx and rx characteristics to the advertisement.
    ble_add_rx_characteristic(ble_nus_service_handle, &ble_nus_rx_char, &rx_uuid);
    ble_add_tx_characteristic(ble_nus_service_handle, &ble_nus_tx_char, &tx_uuid);
}

/**
 * @brief setup the service UUID for the raw service used for media transfer.
 */
void ble_configure_raw_service(ble_uuid_t *service_uuid)
{
    // Set the 16 bit UUIDs for the service and characteristics
    service_uuid->uuid = 0x0001;
    ble_uuid_t rx_uuid = {.uuid = 0x0002};
    ble_uuid_t tx_uuid = {.uuid = 0x0003};

    app_err(sd_ble_uuid_vs_add(&ble_raw_uuid128, &service_uuid->type));

    app_err(sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                     service_uuid, &ble_raw_service_handle));

    // Copy the service UUID type to both rx and tx UUID
    rx_uuid.type = service_uuid->type;
    tx_uuid.type = service_uuid->type;

    // Add tx and rx characteristics to the advertisement.
    ble_add_rx_characteristic(ble_raw_service_handle, &ble_raw_rx_char, &rx_uuid);
    ble_add_tx_characteristic(ble_raw_service_handle, &ble_raw_tx_char, &tx_uuid);
}

/**
 * @brief Softdevice // assert handler. Called whenever softdevice crashes.
 */
static void softdevice_assert_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    app_err(0x5D000000 & id);
}

void SWI2_IRQHandler(void)
{
    uint32_t evt_id;
    uint8_t ble_evt_buffer[sizeof(ble_evt_t) + BLE_MAX_MTU_LENGTH];

    // While any softdevice events are pending, service flash operations
    while (sd_evt_get(&evt_id) != NRF_ERROR_NOT_FOUND)
    {
        switch (evt_id)
        {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
            {
                // TODO In case we add a filesystem in the future
                break;
            }

        case NRF_EVT_FLASH_OPERATION_ERROR:
            {
                // TODO In case we add a filesystem in the future
                break;
            }

        default:
        {
            break;
        }
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
            break;

        // Check for other errors
        app_err(status);

        // Make a pointer from the buffer which we can use to find the event
        ble_evt_t *ble_evt = (ble_evt_t *)ble_evt_buffer;

        // Otherwise on NRF_SUCCESS, we service the new event
        volatile uint16_t ble_evt_id = ble_evt->header.evt_id;
        switch (ble_evt_id)
        {

        // When connected
        case BLE_GAP_EVT_CONNECTED:
            {
                // Set the connection service
                ble_conn_handle = ble_evt->evt.gap_evt.conn_handle;

                // Update connection parameters
                ble_gap_conn_params_t conn_params;

                app_err(sd_ble_gap_ppcp_get(&conn_params));

                app_err(sd_ble_gap_conn_param_update(ble_conn_handle, &conn_params));

                app_err(sd_ble_gatts_sys_attr_set(ble_conn_handle, NULL, 0, 0));

                break;
            }

        // When disconnected
        case BLE_GAP_EVT_DISCONNECTED:
            {
                // // assert(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);

                // Clear the connection service
                ble_conn_handle = BLE_CONN_HANDLE_INVALID;

                // Start advertising
                app_err(sd_ble_gap_adv_start(ble_adv_handle, 1));
                break;
            }

        // On a phy update request, set the phy speed automatically
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
            {
                // // assert(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);

                ble_gap_phys_t const phys = {
                    .rx_phys = BLE_GAP_PHY_AUTO,
                    .tx_phys = BLE_GAP_PHY_AUTO,
                };
                app_err(sd_ble_gap_phy_update(ble_evt->evt.gap_evt.conn_handle, &phys));
                break;
            }

        // Handle requests for changing MTU length
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
            {
                // // assert(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);

                // The client's desired MTU size
                uint16_t client_mtu =
                    ble_evt->evt.gatts_evt.params.exchange_mtu_request.client_rx_mtu;

                // Respond with our max MTU size
                sd_ble_gatts_exchange_mtu_reply(ble_conn_handle, BLE_MAX_MTU_LENGTH);

                // Choose the smaller MTU as the final length we'll use
                // -3 bytes to accommodate for Op-code and attribute service
                ble_negotiated_mtu = BLE_MAX_MTU_LENGTH < client_mtu
                                         ? BLE_MAX_MTU_LENGTH - 3
                                         : client_mtu - 3;
                break;
            }

        // When data arrives, we can write it to the buffer
        case BLE_GATTS_EVT_WRITE:
            {
                // // assert(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
                // For the entire incoming string
                for (uint16_t length = 0;
                     length < ble_evt->evt.gatts_evt.params.write.len;
                     length++)
                {
                    // Break if the ring buffer is full, we can't write more
                    if (ring_full(&nus_rx))
                        break;

                    // Copy a character into the ring buffer
                    ring_push(&nus_rx, ble_evt->evt.gatts_evt.params.write.data[length]);
                }
                break;
            }

        // Disconnect on GATT Client timeout
        case BLE_GATTC_EVT_TIMEOUT:
            {
                // assert(!"not reached");
                break;
            }

        // Disconnect on GATT Server timeout
        case BLE_GATTS_EVT_TIMEOUT:
            {
                // assert(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
                app_err(sd_ble_gap_disconnect(ble_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION));
                break;
            }

        // Updates system attributes after a new connection event
        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            {
                // assert(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
                app_err(sd_ble_gatts_sys_attr_set(ble_conn_handle, NULL, 0, 0));
                break;
            }

        // We don't support pairing, so reply with that message
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            {
                // assert(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
                app_err(sd_ble_gap_sec_params_reply(ble_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL));
                break;
            }

        case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST:
            {
                // assert(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
                app_err(sd_ble_gap_data_length_update(ble_conn_handle, NULL, NULL));
                break;
            }

        case BLE_GAP_EVT_SEC_INFO_REQUEST:
            {
                // assert(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
                app_err(sd_ble_gap_sec_info_reply(ble_conn_handle, NULL, NULL, NULL));
                break;
            }

        case BLE_GAP_EVT_SEC_REQUEST:
            {
                // assert(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
                app_err(sd_ble_gap_authenticate(ble_conn_handle, NULL));
                break;
            }

        case BLE_GAP_EVT_AUTH_KEY_REQUEST:
            {
                // assert(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
                app_err(sd_ble_gap_auth_key_reply(ble_conn_handle, BLE_GAP_AUTH_KEY_TYPE_NONE, NULL));
                break;
            }

        case BLE_EVT_USER_MEM_REQUEST:
        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
        {
            // assert(!"only expected on Bluetooth Centrals, not on Peripherals");
            break;
        }

        default:
        {
            // ignore unused events
            break;
        }
        }
    }
}

/**
 * @brief Main application called from Reset_Handler().
 */
int main(void)
{
    NRFX_LOG_ERROR(RTT_CTRL_CLEAR "\rMicroPython on Monocle - " BUILD_VERSION
                                  " (" MICROPY_GIT_HASH ").");

    // Set up the PMIC and go to sleep if on charge
    monocle_critical_startup();

    // Setup touch interrupt
    {
        app_err(nrfx_gpiote_init(NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY));
        nrfx_gpiote_in_config_t config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
        app_err(nrfx_gpiote_in_init(TOUCH_INTERRUPT_PIN, &config, touch_interrupt_handler));
        nrfx_gpiote_in_event_enable(TOUCH_INTERRUPT_PIN, true);
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
    // TODO move these to the monocle folder
    nrf_gpio_cfg_output(FPGA_INTERRUPT_PIN);
    nrf_gpio_cfg_output(FPGA_CS_PIN);
    nrf_gpio_cfg_output(FLASH_CS_PIN);

    // Setup an RTC counting milliseconds since now
    {
        static nrfx_timer_config_t config = NRFX_TIMER_DEFAULT_CONFIG;
        nrfx_timer_t timer = NRFX_TIMER_INSTANCE(3);

        // Prepare the configuration structure.
        config.frequency = NRF_TIMER_FREQ_125kHz;
        config.mode = NRF_TIMER_MODE_TIMER;
        config.bit_width = NRF_TIMER_BIT_WIDTH_8;

        app_err(nrfx_timer_init(&timer, &config, &mp_hal_timer_1ms_callback));

        // Raise an interrupt every 1ms: 125 kHz / 125
        nrfx_timer_extended_compare(&timer, NRF_TIMER_CC_CHANNEL0, 125,
                                    NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

        // Start the timer, letting timer_add_task() append more of them while running.
        nrfx_timer_enable(&timer);
    }

    // Setup the SoftDevice
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

        // Enable the DC-DC convertor
        app_err(sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE));

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
        cfg.conn_cfg.params.gatt_conn_cfg.att_mtu = BLE_MAX_MTU_LENGTH;
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
    }

    // Setup BLE
    {
        // Start bluetooth. `ram_start` is the address of a variable containing
        // an address, defined in the linker script. It updates that address
        // with another one planning ahead the RAM needed by the softdevice.
        app_err(sd_ble_enable(&ram_start));

        // Set security to open
        ble_gap_conn_sec_mode_t sec_mode;
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

        // Set device name. Last four characters are taken from MAC address
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

        // Add name to advertising payload
        ble_adv_add_device_name(device_name);

        // Set discovery mode flag
        ble_adv_add_discovery_mode();

        ble_uuid_t nus_service_uuid, raw_service_uuid;

        // Configure the Nordic UART Service (NUS) and custom "raw" service.
        ble_configure_nus_service(&nus_service_uuid);
        ble_configure_raw_service(&raw_service_uuid);

        // Add only the Nordic UART Service to the advertisement.
        ble_adv_add_uuid(&nus_service_uuid);

        // Submit the adv now that it is complete.
        ble_gap_adv_data_t adv_data = {
            .adv_data.p_data = ble_adv_buf,
            .adv_data.len = ble_adv_len,
        };

        // Set up advertising parameters
        ble_gap_adv_params_t adv_params = {0};
        adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
        adv_params.primary_phy = BLE_GAP_PHY_AUTO;
        adv_params.secondary_phy = BLE_GAP_PHY_AUTO;
        adv_params.interval = (20 * 1000) / 625;

        // Configure the advertising set
        app_err(sd_ble_gap_adv_set_configure(&ble_adv_handle, &adv_data, &adv_params));

        // Start the configured BLE advertisement
        app_err(sd_ble_gap_adv_start(ble_adv_handle, 1));
    }

    // Check if external flash has an FPGA image and boot it
    {
        // nrf_gpio_pin_set(FLASH_CS_PIN);

        // Let the FPGA start as soon as it has the power on.
        nrf_gpio_pin_set(FPGA_INTERRUPT_PIN);
    }

    // Setup display
    {
        // configure CS pin for the Display (for active low)
        nrf_gpio_cfg_output(DISPLAY_CS_PIN);
        nrf_gpio_pin_set(DISPLAY_CS_PIN);

        ecx336cn_write_byte(0x00, 0x9E); // enter power saving mode (YUV)
        // it is now possible to turn off the 10V rail

        for (size_t i = 0; i < sizeof(ecx336cn_config) / sizeof(*ecx336cn_config); i++)
        {
            ecx336cn_write_byte(i, ecx336cn_config[i]);
        }

        // the 10V power rail needs to be turned back on first
        ecx336cn_write_byte(0x00, 0x9F); // exit power saving mode (YUV)

        // check that 0x29 changed from default 0x0A to 0x0B
        app_err(ecx336cn_read_byte(0x29) != 0x0B);
    }

    // Setup camera
    {
        nrfx_systick_delay_ms(750); // TODO optimize the FPGA to not need this delay
        fpga_cmd_write(0x1009, NULL, 0);
        nrf_gpio_pin_clear(CAMERA_SLEEP_PIN);
        nrf_gpio_pin_clear(CAMERA_RESET_PIN);

        // Read the camera CID (one of them)
        i2c_response_t resp = i2c_read(CAMERA_I2C_ADDRESS, 0x300A, 0xFF);

        if (resp.fail || resp.value != 0x56)
        {
            NRFX_LOG_ERROR("Error: Camera not found.");
            monocle_set_led(RED_LED, true);
        }

        // TODO camera configuration (don't error check)

        // Put the camera to sleep
        nrf_gpio_pin_set(CAMERA_SLEEP_PIN);
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

/**
 * @brief Garbage collection route for nRF.
 */
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

/**
 * @brief Called if an exception is raised outside all C exception-catching handlers.
 */
void nlr_jump_fail(void *val)
{
    app_err((uint32_t)val);
    NVIC_SystemReset();
}
