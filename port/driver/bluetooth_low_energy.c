/*
 * This file is part of the MicroPython for Monocle:
 *      https://github.com/Itsbrilliantlabs/monocle-micropython
 *
 * Authored by: Raj Nakarja - Silicon Witchery AB
 * Authored by: Josuah Demangeon - Panoramix Labs
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

/**
 * Bluetooth Low Energy (BLE) driver with Nordic UART Service console.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "ble.h"
#include "nrf_clock.h"
#include "nrf_sdm.h"
#include "nrfx_log.h"

#include "driver/bluetooth_low_energy.h"
#include "driver/config.h"

#define BLE_ADV_MAX_SIZE            31
#define BLE_UUID_COUNT              2

#define ASSERT  NRFX_ASSERT

/** Buffer sizes for REPL ring buffers; +45 allows a bytearray to be printed in one go. */
#define RING_BUFFER_LENGTH (1024 + 45)

#define UUID128(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) { .uuid128 = { \
    0x##p, 0x##o, 0x##n, 0x##m, 0x##l, 0x##k, 0x##j, 0x##i, \
    0x##h, 0x##g, 0x##f, 0x##e, 0x##d, 0x##c, 0x##b, 0x##a, \
} }

ble_uuid128_t ble_nus_uuid128 = UUID128(6E,40,00,00, B5,A3, F3,93, E0,A9, E5,0E,24,DC,CA,9E);
ble_uuid128_t ble_raw_uuid128 = UUID128(E5,70,00,00, 7B,AC, 42,9A, B4,CE, 57,FF,90,0F,47,9D);

/**
 * Holds the handles for the conenction and characteristics.
 * Convenient for use in interrupts, to get all service-specific data
 * we need to carry around.
 */
typedef struct {
    uint16_t handle;
    ble_gatts_char_handles_t rx_characteristic;
    ble_gatts_char_handles_t tx_characteristic;
} ble_service_t;

/** List of all services we might get a connection for. */
static ble_service_t ble_nus_service, ble_raw_service;

/** Identifier for the active connection with a single device. */
uint16_t ble_conn_handle = BLE_CONN_HANDLE_INVALID;

/** Advertising configured globally for all services. */
uint8_t ble_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;

/** This is the ram start pointer as set in the nrf52811.ld file. */
extern uint32_t _ram_start;

/** The `_ram_start` symbol's address often needs to be passed as an integer. */
static uint32_t ram_start = (uint32_t)&_ram_start;

/** MTU length obtained by the negotiation with the currently connected peer. */
uint16_t ble_negotiated_mtu;


// Ring buffer library

/**
 * Ring buffers for the repl rx and tx data which goes over BLE.
 */
typedef struct {
    uint8_t buffer[RING_BUFFER_LENGTH];
    uint16_t head;
    uint16_t tail;
} ring_buf_t;

ring_buf_t nus_rx, nus_tx;

static inline bool ring_full(ring_buf_t const *ring)
{
    uint16_t next = ring->tail + 1;
    if (next == sizeof(ring->buffer))
        next = 0;
    return next == ring->head;
}

static inline bool ring_empty(ring_buf_t const *ring)
{
    return ring->head == ring->tail;
}

static inline void ring_push(ring_buf_t *ring, uint8_t byte)
{
    ring->buffer[ring->tail++] = byte;
    if (ring->tail == sizeof(ring->buffer))
        ring->tail = 0;
}

static inline uint8_t ring_pop(ring_buf_t *ring)
{
    uint8_t byte = ring->buffer[ring->head++];
    if (ring->head == sizeof(ring->buffer))
            ring->head = 0;
    return byte;
}

// Nordic UART Service service functions

/**
 * Send a buffer out, retrying continuously until it goes to completion (with success or failure).
 */
static void ble_tx(ble_service_t *service, uint8_t const *buf, uint16_t len)
{
    uint32_t err;
    ble_gatts_hvx_params_t hvx_params = {
        .handle = service->tx_characteristic.value_handle,
        .p_data = buf,
        .p_len = (uint16_t *)&len,
        .type = BLE_GATT_HVX_NOTIFICATION,
    };

    do {
        ASSERT(ble_conn_handle != BLE_CONN_HANDLE_INVALID);

        // Send the data
        err = sd_ble_gatts_hvx(ble_conn_handle, &hvx_params);

    // Retry if resources are unavailable.
    } while (err == NRF_ERROR_RESOURCES);

    // Ignore errors if not connected
    if (err == NRF_ERROR_INVALID_STATE || err == BLE_ERROR_INVALID_CONN_HANDLE)
        return;

    // Catch other errors
    ASSERT(!err);
}

/**
 * Sends all buffered data in the tx ring buffer over BLE.
 */
static void ble_nus_flush_tx(void)
{
    // Local buffer for sending data
    uint8_t buf[BLE_MAX_MTU_LENGTH] = "";
    uint16_t len = 0;

    // If not connected, do not flush.
    if (ble_conn_handle == BLE_CONN_HANDLE_INVALID)
        return;

    // If there's no data to send, simply return
    if (ring_empty(&nus_tx))
        return;

    // For all the remaining characters, i.e until the heads come back together
    while (!ring_empty(&nus_tx))
    {
        // Copy over a character from the tail to the outgoing buffer
        buf[len++] = ring_pop(&nus_tx);

        // Break if we over-run the negotiated MTU size, send the rest later
        if (len >= ble_negotiated_mtu)
            break;
    }
    ble_tx(&ble_nus_service, buf, len);
}

int ble_nus_rx(void)
{
    while (ring_empty(&nus_rx))
    {
        // While waiting for incoming data, we can push outgoing data
        ble_nus_flush_tx();

        // If there's nothing to do
        if (ring_empty(&nus_tx) && ring_empty(&nus_rx))
            // Wait for events to save power
            sd_app_evt_wait();
    }

    // Return next character from the RX buffer.
    return ring_pop(&nus_rx);
}

void ble_nus_tx(char const *buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        while (ring_full(&nus_tx))
            ble_nus_flush_tx();
        ring_push(&nus_tx, buf[i]);
    }
}

bool ble_nus_is_rx_pending(void)
{
    return ring_empty(&nus_rx);
}

// Global Bluetooth Low Energy setup

// Advertising data which needs to stay in scope between connections.
uint8_t ble_adv_len;
uint8_t ble_adv_buf[BLE_ADV_MAX_SIZE];

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
    uint32_t err;
    uint8_t len;
    uint8_t *p_adv_size;

    p_adv_size = &ble_adv_buf[ble_adv_len];
    ble_adv_buf[ble_adv_len++] = 1;
    ble_adv_buf[ble_adv_len++] = BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE;

    err = sd_ble_uuid_encode(uuid, &len, &ble_adv_buf[ble_adv_len]);
    ASSERT(!err);
    ble_adv_len += len;
    *p_adv_size += len;
}

static inline void ble_adv_start(void)
{
    uint32_t err;

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
    err = sd_ble_gap_adv_set_configure(&ble_adv_handle, &adv_data, &adv_params);
    ASSERT(!err);

    // Start the configured BLE advertisement
    err = sd_ble_gap_adv_start(ble_adv_handle, 1);
    ASSERT(!err);
}

/**
 * Add rx characteristic to the advertisement.
 */
static void ble_service_add_characteristic_rx(ble_service_t *service, ble_uuid_t *uuid)
{
    uint32_t err;

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

    err = sd_ble_gatts_characteristic_add(service->handle, &rx_char_md, &rx_attr,
        &service->rx_characteristic);
    ASSERT(!err);
}

/**
 * Add tx characteristic to the advertisement.
 */
static void ble_service_add_characteristic_tx(ble_service_t *service, ble_uuid_t *uuid)
{
    uint32_t err;

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

    err = sd_ble_gatts_characteristic_add(service->handle, &tx_char_md, &tx_attr,
        &service->tx_characteristic);
    ASSERT(!err);
}

static void ble_configure_nus_service(ble_uuid_t *service_uuid)
{
    uint32_t err;
    ble_service_t *service = &ble_nus_service;

    // Set the 16 bit UUIDs for the service and characteristics
    service_uuid->uuid = 0x0001;
    ble_uuid_t rx_uuid = { .uuid = 0x0002 };
    ble_uuid_t tx_uuid = { .uuid = 0x0003 };

    err = sd_ble_uuid_vs_add(&ble_nus_uuid128, &service_uuid->type);
    ASSERT(!err);

    err = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
        service_uuid, &service->handle);

    // Copy the service UUID type to both rx and tx UUID
    rx_uuid.type = service_uuid->type;
    tx_uuid.type = service_uuid->type;

    // Add tx and rx characteristics to the advertisement.
    ble_service_add_characteristic_rx(service, &rx_uuid);
    ble_service_add_characteristic_tx(service, &tx_uuid);
}

void ble_raw_tx(uint8_t const *buf, uint16_t len)
{
    ble_tx(&ble_raw_service, buf, len);
}

void ble_configure_raw_service(ble_uuid_t *service_uuid)
{
    uint32_t err;
    ble_service_t *service = &ble_raw_service;

    // Set the 16 bit UUIDs for the service and characteristics
    service_uuid->uuid = 0x0001;
    ble_uuid_t rx_uuid = { .uuid = 0x0002 };
    ble_uuid_t tx_uuid = { .uuid = 0x0003 };

    err = sd_ble_uuid_vs_add(&ble_raw_uuid128, &service_uuid->type);
    ASSERT(!err);

    err = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
        service_uuid, &service->handle);

    // Copy the service UUID type to both rx and tx UUID
    rx_uuid.type = service_uuid->type;
    tx_uuid.type = service_uuid->type;

    // Add tx and rx characteristics to the advertisement.
    ble_service_add_characteristic_rx(service, &rx_uuid);
    ble_service_add_characteristic_tx(service, &tx_uuid);
}

/**
 * Setup BLE parameters adapted to this driver.
 */
void ble_configure_softdevice(void)
{
    uint32_t err;

    // Add GAP configuration to the BLE stack
    ble_cfg_t cfg;
    cfg.conn_cfg.conn_cfg_tag = 1;
    cfg.conn_cfg.params.gap_conn_cfg.conn_count = 1;
    cfg.conn_cfg.params.gap_conn_cfg.event_length = 3;
    err = sd_ble_cfg_set(BLE_CONN_CFG_GAP, &cfg, ram_start);
    ASSERT(!err);

    // Set BLE role to peripheral only
    memset(&cfg, 0, sizeof(cfg));
    cfg.gap_cfg.role_count_cfg.periph_role_count = 1;
    err = sd_ble_cfg_set(BLE_GAP_CFG_ROLE_COUNT, &cfg, ram_start);
    ASSERT(!err);

    // Set max MTU size
    memset(&cfg, 0, sizeof(cfg));
    cfg.conn_cfg.conn_cfg_tag = 1;
    cfg.conn_cfg.params.gatt_conn_cfg.att_mtu = BLE_MAX_MTU_LENGTH;
    err = sd_ble_cfg_set(BLE_CONN_CFG_GATT, &cfg, ram_start);
    ASSERT(!err);

    // Configure a single queued transfer
    memset(&cfg, 0, sizeof(cfg));
    cfg.conn_cfg.conn_cfg_tag = 1;
    cfg.conn_cfg.params.gatts_conn_cfg.hvn_tx_queue_size = 1;
    err = sd_ble_cfg_set(BLE_CONN_CFG_GATTS, &cfg, ram_start);
    ASSERT(!err);

    // Configure number of custom UUIDs
    memset(&cfg, 0, sizeof(cfg));
    cfg.common_cfg.vs_uuid_cfg.vs_uuid_count = 2;
    err = sd_ble_cfg_set(BLE_COMMON_CFG_VS_UUID, &cfg, ram_start);
    ASSERT(!err);

    // Configure GATTS attribute table
    memset(&cfg, 0, sizeof(cfg));
    cfg.gatts_cfg.attr_tab_size.attr_tab_size = 1408;
    err = sd_ble_cfg_set(BLE_GATTS_CFG_ATTR_TAB_SIZE, &cfg, ram_start);
    ASSERT(!err);

    // No service changed attribute needed
    memset(&cfg, 0, sizeof(cfg));
    cfg.gatts_cfg.service_changed.service_changed = 0;
    err = sd_ble_cfg_set(BLE_GATTS_CFG_SERVICE_CHANGED, &cfg, ram_start);
    ASSERT(!err);
}

/**
 * Softdevice assert handler. Called whenever softdevice crashes.
 */
static void softdevice_assert_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    assert(id == 0);
}

/**
 * BLE event handler.
 */
void SWI2_IRQHandler(void)
{
    uint32_t err;
    uint32_t evt_id;
    uint8_t ble_evt_buffer[sizeof(ble_evt_t) + BLE_MAX_MTU_LENGTH];

    // While any softdevice events are pending, service flash operations
    while (sd_evt_get(&evt_id) != NRF_ERROR_NOT_FOUND)
    {
        switch (evt_id)
        {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
        LOG("NRF_EVT_FLASH_OPERATION_SUCCESS");
        {
            // TODO In case we add a filesystem in the future
            break;
        }

        case NRF_EVT_FLASH_OPERATION_ERROR:
        LOG("NRF_EVT_FLASH_OPERATION_ERROR");
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
        ASSERT(status == 0);

        // Make a pointer from the buffer which we can use to find the event
        ble_evt_t *ble_evt = (ble_evt_t *)ble_evt_buffer;

        // Otherwise on NRF_SUCCESS, we service the new event
        volatile uint16_t ble_evt_id = ble_evt->header.evt_id;
        switch (ble_evt_id)
        {

        // When connected
        case BLE_GAP_EVT_CONNECTED:
        LOG("BLE_GAP_EVT_CONNECTED");
        {
            ASSERT(ble_conn_handle == BLE_CONN_HANDLE_INVALID);

            // Set the connection service
            ble_conn_handle = ble_evt->evt.gap_evt.conn_handle;

            // Update connection parameters
            ble_gap_conn_params_t conn_params;

            err = sd_ble_gap_ppcp_get(&conn_params);
            ASSERT(!err);

            err = sd_ble_gap_conn_param_update(ble_conn_handle, &conn_params);
            ASSERT(!err);

            err = sd_ble_gatts_sys_attr_set(ble_conn_handle, NULL, 0, 0);
            ASSERT(!err);
            break;
        }

        // When disconnected
        case BLE_GAP_EVT_DISCONNECTED:
        LOG("BLE_GAP_EVT_DISCONNECTED");
        {
            ASSERT(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);

            // Clear the connection service
            ble_conn_handle = BLE_CONN_HANDLE_INVALID;

            // Start advertising
            err = sd_ble_gap_adv_start(ble_adv_handle, 1);
            ASSERT(!err);
            break;
        }

        // On a phy update request, set the phy speed automatically
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        LOG("BLE_GAP_EVT_PHY_UPDATE_REQUEST");
        {
            ASSERT(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);

            ble_gap_phys_t const phys = {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err = sd_ble_gap_phy_update(ble_evt->evt.gap_evt.conn_handle, &phys);
            ASSERT(!err);
            break;
        }

        // Handle requests for changing MTU length
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
        LOG("BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST");
        {
            ASSERT(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);

            // The client's desired MTU size
            uint16_t client_mtu =
                ble_evt->evt.gatts_evt.params.exchange_mtu_request.client_rx_mtu;

            // Respond with our max MTU size
            err = sd_ble_gatts_exchange_mtu_reply(ble_conn_handle, BLE_MAX_MTU_LENGTH);
            ASSERT(!err);

            // Choose the smaller MTU as the final length we'll use
            // -3 bytes to accommodate for Op-code and attribute service
            ble_negotiated_mtu = BLE_MAX_MTU_LENGTH < client_mtu
                                 ? BLE_MAX_MTU_LENGTH - 3
                                 : client_mtu - 3;
            break;
        }

        // When data arrives, we can write it to the buffer
        case BLE_GATTS_EVT_WRITE:
        LOG("BLE_GATTS_EVT_WRITE");
        {
            ASSERT(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
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
        LOG("BLE_GATTC_EVT_TIMEOUT");
        {
            ASSERT(!"not reached");
            break;
        }

        // Disconnect on GATT Server timeout
        case BLE_GATTS_EVT_TIMEOUT:
        LOG("BLE_GATTS_EVT_TIMEOUT");
        {
            ASSERT(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
            err = sd_ble_gap_disconnect(ble_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            ASSERT(!err);
            break;
        }

        // Updates system attributes after a new connection event
        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
        LOG("BLE_GATTS_EVT_SYS_ATTR_MISSING");
        {
            ASSERT(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
            err = sd_ble_gatts_sys_attr_set(ble_conn_handle, NULL, 0, 0);
            ASSERT(!err);
            break;
        }

        // We don't support pairing, so reply with that message
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
        LOG("BLE_GAP_EVT_SEC_PARAMS_REQUEST");
        {
            ASSERT(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
            err = sd_ble_gap_sec_params_reply(ble_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            ASSERT(!err);
            break;
        }

        case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST:
        LOG("BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST");
        {
            ASSERT(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
            err = sd_ble_gap_data_length_update(ble_conn_handle, NULL, NULL);
            ASSERT(!err);
            break;
        }

        case BLE_GAP_EVT_SEC_INFO_REQUEST:
        LOG("BLE_GAP_EVT_SEC_INFO_REQUEST");
        {
            ASSERT(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
            err = sd_ble_gap_sec_info_reply(ble_conn_handle, NULL, NULL, NULL);
            ASSERT(!err);
            break;
        }

        case BLE_GAP_EVT_SEC_REQUEST:
        LOG("BLE_GAP_EVT_SEC_REQUEST");
        {
            ASSERT(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
            err = sd_ble_gap_authenticate(ble_conn_handle, NULL);
            ASSERT(!err);
            break;
        }

        case BLE_GAP_EVT_AUTH_KEY_REQUEST:
        LOG("BLE_GAP_EVT_AUTH_KEY_REQUEST");
        {
            ASSERT(ble_evt->evt.gap_evt.conn_handle == ble_conn_handle);
            err = sd_ble_gap_auth_key_reply(ble_conn_handle, BLE_GAP_AUTH_KEY_TYPE_NONE, NULL);
            ASSERT(!err);
            break;
        }

        case BLE_EVT_USER_MEM_REQUEST:
        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
        {
            ASSERT(!"only expected on Bluetooth Centrals, not on Peripherals");
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
 * Initialise the bluetooth low energy driver.
 * Initialises the softdevice and Bluetooth functionality.
 * It features a single GATT profile for UART communication, used by the REPL.
 */
void ble_init(void)
{
    DRIVER("BLE");

    // Error code variable
    uint32_t err;

    // Init LF clock
    nrf_clock_lf_cfg_t clock_config = {
        .source = NRF_CLOCK_LF_SRC_XTAL,
        .rc_ctiv = 0,
        .rc_temp_ctiv = 0,
        .accuracy = NRF_CLOCK_LF_ACCURACY_10_PPM
    };

    // Enable the softdevice
    err = sd_softdevice_enable(&clock_config, softdevice_assert_handler);
    ASSERT(!err);

    // Enable softdevice interrupt
    err = sd_nvic_EnableIRQ((IRQn_Type)SD_EVT_IRQn);
    ASSERT(!err);

    // Enable the DC-DC convertor
    err = sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
    ASSERT(!err);

    // Set configuration parameters for the SoftDevice suitable for this code.
    ble_configure_softdevice();

    // Start bluetooth. `ram_start` is the address of a variable containing an address, defined in the linker script.
    // It updates that address with another one planning ahead the RAM needed by the softdevice.
    err = sd_ble_enable(&ram_start);
    ASSERT(!err);

    // Set security to open
    ble_gap_conn_sec_mode_t sec_mode;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    // Set device name. Last four characters are taken from MAC address ;
    err = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)BLE_DEVICE_NAME, sizeof BLE_DEVICE_NAME - 1);
    ASSERT(!err);

    // Set connection parameters
    ble_gap_conn_params_t gap_conn_params = {0};
    gap_conn_params.min_conn_interval = (15 * 1000) / 1250;
    gap_conn_params.max_conn_interval = (15 * 1000) / 1250;
    gap_conn_params.slave_latency = 3;
    gap_conn_params.conn_sup_timeout = (2000 * 1000) / 10000;
    err = sd_ble_gap_ppcp_set(&gap_conn_params);
    ASSERT(!err);

    // Add name to advertising payload
    ble_adv_add_device_name(BLE_DEVICE_NAME);

    // Set discovery mode flag
    ble_adv_add_discovery_mode();

    ble_uuid_t nus_service_uuid, raw_service_uuid;

    // Configure the Nordic UART Service (NUS) and custom "raw" service.
    ble_configure_nus_service(&nus_service_uuid);
    ble_configure_raw_service(&raw_service_uuid);

    // Add only the Nordic UART Service to the advertisement.
    ble_adv_add_uuid(&nus_service_uuid);

    // Submit the adv now that it is complete.
    ble_adv_start();
}
