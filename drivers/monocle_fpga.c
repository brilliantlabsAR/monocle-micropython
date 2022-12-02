/*
 * Copyright (c) 2021 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#include <assert.h>
#include "monocle_fpga.h"
#include "monocle_ov5640.h"
#include "monocle_spi.h"
#include "monocle_config.h"
#include "nrfx_systick.h"
#include "nrfx_log.h"

#define LOG NRFX_LOG_ERROR

void fpga_check_pins(char const *msg)
{
    static bool first = true;

    if (first) {
        LOG("| INT   |       | MODE1 |       |       |");
        LOG("| RECFG | SCK   | CSN   | MOSI  | MISO  |");
        LOG("+-------+-------+-------+-------+-------+");
        LOG("| P0.05 | P0.07 | P0.08 | P0.09 | P0.10 |");
        LOG("+=======+=======+=======+=======+=======+");
        first = false;
    }
    LOG("|  %3d  |  %3d  |  %3d  |  %3d  |  %3d  | %s",
        nrf_gpio_pin_read(5),
        nrf_gpio_pin_read(7),
        nrf_gpio_pin_read(8),
        nrf_gpio_pin_read(9),
        nrf_gpio_pin_read(10),
        msg
    );
}

/**
 * Write a byte to the FPGA over SPI using a bridge protocol.
 * @param addr The address of the FPGA to write to.
 * @param byte The byte to write on that address.
 */
void fpga_set_register(uint8_t addr, uint8_t byte)
{
    assert(FPGA_REGISTER_IS_WRITABLE(addr));
    spi_write_register(SPIM0_FPGA_CS_PIN, addr, byte);
}

/**
 * Read a byte to the FPGA over SPI using a bridge protocol.
 * @param addr The address of the FPGA to read from.
 * @return The value read at that address.
 */
uint8_t fpga_get_register(uint8_t addr)
{
    assert(FPGA_REGISTER_EXISTS(addr));
    return spi_read_register(SPIM0_FPGA_CS_PIN, addr);
}

/**
 * Write a multi-byte burst of byte to the FPGA over SPI.
 * The address is implicit.
 * @param buf The buffer containing the data.
 * @param len The size of that buffer.
 */
void fpga_write_burst(uint8_t *buf, uint16_t len)
{
    // prepare FPGA to receive the byte stream
    fpga_set_register(FPGA_WR_BURST_SIZE_LO, (len & 0x00FF) >> 0);
    fpga_set_register(FPGA_WR_BURST_SIZE_HI, (len & 0xFF00) >> 8);

    // send the buffer to the FPGA
    spi_write_buffer(SPIM0_FPGA_CS_PIN, FPGA_BURST_WR_DATA, buf, len);
}

/**
 * Read a burst of data from the FPGA.
 * The address is implicit.
 * @param data The buffer that will be written to.
 * @param len The number of bytes to read onto that buffer.
 */
void fpga_read_burst_ref(uint8_t *buf, uint16_t len)
{
     do {
        size_t chunk_len = MIN(SPI_MAX_XFER_LEN, len);

        // prepare FPGA for burst read
        fpga_set_register(FPGA_RD_BURST_SIZE_LO, (chunk_len & 0x00FF) >> 0);
        fpga_set_register(FPGA_RD_BURST_SIZE_HI, (chunk_len & 0xFF00) >> 8);

        spi_read_buffer(SPIM0_FPGA_CS_PIN, FPGA_BURST_RD_DATA, buf, len);
        buf += chunk_len;
        len -= chunk_len;
    } while (len > 0);
}

void fpga_read_burst(uint8_t *buf, uint16_t len)
{
    // prepare FPGA for burst read
    fpga_set_register(FPGA_RD_BURST_SIZE_LO, (len & 0x00FF) >> 0);
    fpga_set_register(FPGA_RD_BURST_SIZE_HI, (len & 0xFF00) >> 8);
    spi_read_buffer(SPIM0_FPGA_CS_PIN, FPGA_BURST_RD_DATA, buf, len);
}

uint32_t fpga_get_capture_size(void)
{
    assert(fpga_capture_done());
    return (fpga_get_register(FPGA_CAPTURE_SIZE_0) << 24
          | fpga_get_register(FPGA_CAPTURE_SIZE_1) << 16
          | fpga_get_register(FPGA_CAPTURE_SIZE_2) << 8
          | fpga_get_register(FPGA_CAPTURE_SIZE_3) << 0);
}

uint32_t fpga_get_bytes_read(void)
{
    return (fpga_get_register(FPGA_CAPT_BYTE_COUNT_0) << 24
          | fpga_get_register(FPGA_CAPT_BYTE_COUNT_1) << 16
          | fpga_get_register(FPGA_CAPT_BYTE_COUNT_2) << 8
          | fpga_get_register(FPGA_CAPT_BYTE_COUNT_3) << 0);
}

uint16_t fpga_get_checksum(void)
{
    return (fpga_get_register(FPGA_CAPT_FRM_CHECKSUM_0) << 8
          | fpga_get_register(FPGA_CAPT_FRM_CHECKSUM_1) << 0);
}

bool fpga_is_buffer_at_start(void)
{
    return (fpga_get_register(FPGA_CAPTURE_STATUS) & FPGA_START_OF_CAPT);
}

bool fpga_is_buffer_read_done(void)
{
    return (fpga_get_register(FPGA_CAPTURE_STATUS)
            & (FPGA_VIDEO_CAPT_DONE | FPGA_AUDIO_CAPT_DONE | FPGA_FRAME_CAPT_DONE));
    //return (fpga_get_register(FPGA_CAPTURE_STATUS) & (FPGA_AUDIO_CAPT_DONE));
}

/**
 * Preparations for GPIO pins before to power-on the FPGA.
 */
void fpga_prepare(void)
{
    // MODE1 set low for AUTOBOOT from FPGA internal flash
    nrf_gpio_pin_write(FPGA_MODE1_PIN, false);
    nrf_gpio_cfg(
        FPGA_MODE1_PIN,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );

    // Let the FPGA start as soon as it has the power on.
    nrf_gpio_pin_write(FPGA_RECONFIG_N_PIN, true);
    nrf_gpio_cfg(
        FPGA_RECONFIG_N_PIN,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );
}

static void fpga_reset(void)
{
    // reset FPGA
    fpga_check_pins("will write 0x01 to FPGA_SYSTEM_CONTROL");
    fpga_set_register(FPGA_SYSTEM_CONTROL, 0x01);

    // TODO: not sure if we need this, but just in case...
    nrfx_systick_delay_ms(185);
    // clear the reset (needed for some FPGA projects, like OLED unit test)
    fpga_check_pins("will write 0x00 to FPGA_SYSTEM_CONTROL");
    fpga_set_register(FPGA_SYSTEM_CONTROL, 0x00);

    // from testing, 2ms seems to be the minimum delay needed for all registers to return to expected values
    // reason is unclear
    // use 5ms for extra safety margin (used to work earlier)
    // But from 2021-02-19, 5ms is no longer enough
    // TODO: why? Seems to require 170ms delay now
    nrfx_systick_delay_ms(200);
}

/**
 * Initial configuration of the registers of the FPGA.
 */
void fpga_init(void)
{
    // Set the FPGA to boot from its internal flash.
    fpga_check_pins("will set MODE1 to 0");
    nrf_gpio_pin_write(FPGA_MODE1_PIN, false);
    nrfx_systick_delay_ms(1);

    // Issue a "reconfig" pulse.
    // Datasheet UG290E: T_recfglw >= 70 us
    fpga_check_pins("will set RECFG to 0");
    nrf_gpio_pin_write(FPGA_RECONFIG_N_PIN, false);
    nrfx_systick_delay_ms(100); // 1000 times more than needed
    fpga_check_pins("will set RECFG to 1");
    nrf_gpio_pin_write(FPGA_RECONFIG_N_PIN, true);

    // Give the FPGA some time to boot.
    // Datasheet UG290E: T_recfgtdonel <= 
    nrfx_systick_delay_ms(100);

    // Reset the CSN pin, changed as it is also MODE1.
    fpga_check_pins("will set CSN to 1");
    nrf_gpio_pin_write(SPIM0_FPGA_CS_PIN, true);

    // Set all registers to a known state.
    fpga_reset();

    // Give the FPGA some further time.
    nrfx_systick_delay_ms(10);
    fpga_check_pins("init done");
}

void fpga_deinit(void)
{
    nrf_gpio_cfg_default(FPGA_MODE1_PIN);
    nrf_gpio_cfg_default(FPGA_RECONFIG_N_PIN);
}

void fpga_xclk_on(void)
{
    fpga_set_register(FPGA_CAMERA_CONTROL, FPGA_EN_XCLK);
    assert(fpga_get_register(FPGA_CAMERA_CONTROL) == FPGA_EN_XCLK);
}

/**
 * Turn on the camera.
 */
void fpga_camera_on(void)
{
    // delay 4 frames to discard AWB adjustments (needed if camera was just powered up)
    nrfx_systick_delay_ms(4*(1000/OV5640_FPS) + 1);

    // enable camera interface (& keep XCLK enabled!)
    fpga_set_register(FPGA_CAMERA_CONTROL, (FPGA_EN_XCLK | FPGA_EN_CAM));

    LOG("waited 4 frames, sent FPGA_EN_XCLK, FPGA_EN_CAM");
    assert(fpga_get_register(FPGA_CAMERA_CONTROL) == (FPGA_EN_XCLK | FPGA_EN_CAM));
}

/**
 * Turn off the camera.
 */
void fpga_camera_off(void)
{
    // turn off camera interface (but keep XCLK enabled!)
    fpga_set_register(FPGA_CAMERA_CONTROL, FPGA_EN_XCLK);

    // allow last frame to finish entering video buffer to avoid split screen
    nrfx_systick_delay_ms(1*(1000/OV5640_FPS) + 1);

    LOG("sent FPGA_EN_XCLK, waited 1 frame");
    assert(fpga_get_register(FPGA_CAMERA_CONTROL) == FPGA_EN_XCLK);
}

/**
 * Turn on the microphone.
 * @return True if it has been effectively turned on.
 */
void fpga_mic_on(void)
{
    fpga_set_register(FPGA_MIC_CONTROL, FPGA_EN_MIC);
    assert(fpga_get_register(FPGA_MIC_CONTROL) == FPGA_EN_MIC);
}

/**
 * Turn off the microphone.
 * @return True if it has been effectively turned off.
 */
void fpga_mic_off(void)
{
    fpga_set_register(FPGA_MIC_CONTROL, 0x00);
    assert(fpga_get_register(FPGA_MIC_CONTROL) == 0x00);
}

/**
 * Enable the live streaming of the camera directly to the screen.
 */
void fpga_disp_live(void)
{
    fpga_set_register(FPGA_DISPLAY_CONTROL, FPGA_DISP_CAM);
}

/**
 * Enable the busy indicator: gray screen.
 */
void fpga_disp_busy(void)
{
    fpga_set_register(FPGA_DISPLAY_CONTROL, FPGA_DISP_BUSY);
}

/**
 * Enable the display of 8 vertical color bars to the screen.
 */
void fpga_disp_bars(void)
{
    fpga_set_register(FPGA_DISPLAY_CONTROL, FPGA_DISP_BARS);
}

/**
 * Disable the red-blue shift chrominace correction.
 */
void fpga_disp_off(void)
{
    fpga_set_register(FPGA_DISPLAY_CONTROL, FPGA_DISP_OFF);
}

/**
 * Resume live video feed and clear the checksum.
 */
void fpga_resume_live_video(void)
{
    //fpga_set_register(FPGA_CAPTURE_CONTROL, FPGA_RESUME_FILL);
    fpga_set_register(FPGA_CAPTURE_CONTROL, (FPGA_RESUME_FILL | FPGA_CLR_CHKSM));
}

/**
 * Enable the capture of a frame of the video.
 */
void fpga_image_capture(void)
{
    fpga_set_register(FPGA_CAPTURE_CONTROL, (FPGA_CAPT_EN | FPGA_CAPT_FRM));
}

void fpga_video_capture(void)
{
#ifdef MIC_ON
    fpga_set_register(FPGA_CAPTURE_CONTROL, (FPGA_CAPT_EN | FPGA_CAPT_VIDEO | FPGA_CAPT_AUDIO));
#else
    fpga_set_register(FPGA_CAPTURE_CONTROL, (FPGA_CAPT_EN | FPGA_CAPT_VIDEO));
#endif
}

// first clear checksum, then set bit for audio data burst read
void fpga_prep_read_audio(void)
{
#ifdef MIC_ON
    // ensure CLR_CHECKSUM gets a rising edge
    fpga_set_register(FPGA_CAPTURE_CONTROL, 0x00);

    // clear checksum (left from video transfer)
    fpga_set_register(FPGA_CAPTURE_CONTROL, FPGA_CLR_CHKSM);

    // this also sets CLR_CHKSM back to 0
    fpga_set_register(FPGA_CAPTURE_CONTROL, FPGA_RD_AUDIO);

    // this also sets CLR_CHKSM back to 0
    //fpga_set_register(FPGA_CAPTURE_CONTROL, (FPGA_CAPT_EN | FPGA_CAPT_VIDEO | FPGA_CAPT_AUDIO));
#else
    // do nothing
#endif
}

void fpga_replay_rate(uint8_t repeat)
{
    // cannot be zero, max 5 bits
    assert(repeat > 0);
    assert(repeat <= FPGA_REP_RATE_MASK);

    fpga_set_register(FPGA_REPLAY_RATE_CONTROL, repeat);
    LOG("replay rate set to %d", repeat);
}

bool fpga_capture_done(void)
{
    return (fpga_get_register(FPGA_CAPTURE_STATUS) & FPGA_CAPT_RD_VLD);
}

// Simple checksum calculation; matching the algorithm used on FPGA
// Sums a byte stream as 16-bit words (with first byte being least significant), adding carry back in, to return 16-bit result
// Precondition: len must be a multiple of 2 (bytes)
uint16_t fpga_calc_checksum(uint8_t *bytearray, uint32_t len)
{

    uint32_t checksum = 0;

    assert(bytearray != NULL);
    assert(len == 0);
    assert((len % 2) == 0);
    for (uint32_t i = 0; i < len; i = i + 2)
        checksum = fpga_checksum_add(checksum, ((bytearray[i + 1] << 8) + bytearray[i]));
    return ((uint16_t)checksum);
}

uint16_t fpga_checksum_add(uint16_t checksum1, uint16_t checksum2)
{
    uint32_t checksum = 0;
    uint32_t carry = 0;

    // add carry (which if it exists must be 1) back in
    checksum = checksum1 + checksum2;

    if (checksum > 0x0000FFFF)
    {
        carry = checksum & 0xFFFF0000;
        carry = carry >> 16;

        // this should always be true, if so we can simplify the above
        assert(carry == 1);

        checksum = (checksum & 0x0000FFFF) + carry;
        assert(checksum <= 0x0000FFFF);
    }
    return ((uint16_t)checksum);
}

// valid input zoom levels: 1, 2, 4, 8
void fpga_set_zoom(uint8_t level)
{
    uint8_t zoom_bits = 0;
    //NOTE For zoom to work, it should always be true that XCLK is on, EN_CAM is on
    // TODO: make en_luma_cor independently settable; for now always on
    switch(level)
    {
        case 1:
            zoom_bits = 0 << FPGA_ZOOM_SHIFT;
            fpga_set_register(FPGA_CAMERA_CONTROL, FPGA_EN_XCLK | FPGA_EN_CAM | zoom_bits);
            break;
        case 2:
        case 4:
        case 8:
            zoom_bits = (level >> 2) << FPGA_ZOOM_SHIFT;
            fpga_set_register(FPGA_CAMERA_CONTROL, FPGA_EN_XCLK | FPGA_EN_CAM | zoom_bits | FPGA_EN_ZOOM | FPGA_EN_LUMA_COR);
            break;
        default:
            assert(!"invalid zoom level");
    }
}

/**
 * Turn the luma correction of the camera on or off asking the FPGA over SPI.
 * Convenient for testing.
 * @param turn_on Whether to activate or disable the luma correction.
 */
void fpga_set_luma(bool turn_on)
{
    uint8_t reg =0;
    bool luma_on = false;

    // get current zoom & luma status
    reg = fpga_get_register(FPGA_CAMERA_CONTROL);

    // only valid when zoom is active
    if (!(reg & FPGA_EN_ZOOM))
        return;

    // already correctly set, nothing to do
    luma_on = (reg & FPGA_EN_LUMA_COR);
    if ((luma_on && turn_on) || (!luma_on && !turn_on))
        return;

    if (turn_on) {
        reg = reg | FPGA_EN_LUMA_COR;
        LOG("turn luma correction on.");
    } else {
        reg = reg & ~FPGA_EN_LUMA_COR;
        LOG("turn luma correction off.");
    }
    fpga_set_register(FPGA_CAMERA_CONTROL, reg);
}

/**
 * Set the display mode of the screen controlled by FPGA over SPI.
 * @param mode 0=off, 1=disp_cam, 2=disp_busy, 3=disp_bars.
 */
// TODO: Convert mode to enum.
void fpga_set_display(uint8_t mode)
{
    switch(mode) {
        case 0:
            fpga_disp_off();
            LOG("display mode = off.");
            break;
        case 1:
            fpga_disp_live();
            LOG("display mode = video.");
            break;
        case 2:
            fpga_disp_busy();
            LOG("display mode = busy.");
            break;
        case 3:
            fpga_disp_bars();
            LOG("display mode = color bars.");
            break;
        default:
            assert(!"display mode invalid.");
    }
}

/**
 * Read the version register of the FPGA.
 * @param major Major revision number.
 * @param minor Minor revision number.
 */
void fpga_get_version(uint8_t *major, uint8_t *minor)
{
    *major = fpga_get_register(FPGA_VERSION_MAJOR);
    *minor = fpga_get_register(FPGA_VERSION_MINOR);
    LOG("%d.%d", *major, *minor);
}
