/*
 * Copyright (c) 2021 Brilliant Labs Limited
 * Licensed under the MIT License
 */

/**
 * Interface with the FPGA.
 * @file
 * @author Shreyas Hemachandra
 * @author Nathan Ashelman
 */

#include "monocle_fpga.h"
#include "monocle_ov5640.h" // for OV5640_FPS
#include "monocle_spi.h"
#include "monocle_pins.h"

#include "nrfx_systick.h"
#include "nrfx_log.h"

#define LOG(...) NRFX_LOG_ERROR(__VA_ARGS__)

/**
 * Write a byte to the FPGA over SPI using a bridge protocol.
 * @param addr The address of the FPGA to write to.
 * @param data The data to write on that address.
 */
void fpga_write_byte(uint8_t addr, uint8_t data)
{
    // check that it is a valid register to write to
    assert(FPGA_REGISTER_IS_WRITABLE(addr));

    // select FPGA on SPI bus
    spi_set_cs_pin(SPIM_SS1_PIN);

    spi_write_byte(addr, data);
    LOG("fpga_write_byte(addr=0x%x, data=0x%x).", addr, data);
}

/**
 * Read a byte to the FPGA over SPI using a bridge protocol.
 * @param addr The address of the FPGA to read from.
 * @return The value read at that address.
 */
uint8_t fpga_read_byte(uint8_t addr)
{
    // check that it is a valid register
    assert(FPGA_REGISTER_EXISTS(addr));

    uint8_t ReadData;

    // select FPGA on SPI bus
    spi_set_cs_pin(SPIM_SS1_PIN);

    ReadData = spi_read_byte(addr);
    LOG("fpga_read_byte(addr=0x%x) returned 0x%x.", addr, ReadData);    
    return ReadData;
}

/**
 * Write a multi-byte burst of data to the FPGA over SPI.
 * The address is implicit.
 * @param data The buffer containing the data.
 * @param length The size of that buffer.
 */
void fpga_write_burst(const uint8_t *data, uint16_t length)
{
    // prepare FPGA to receive the byte stream
    uint8_t length_lo = length & 0x00FF;
    uint8_t length_hi = (length & 0xFF00) >> 8;
    fpga_write_byte(FPGA_WR_BURST_SIZE_LO, length_lo);
    fpga_write_byte(FPGA_WR_BURST_SIZE_HI, length_hi);

    // select FPGA on SPI bus
    spi_set_cs_pin(SPIM_SS1_PIN);

    spi_write_burst(FPGA_BURST_WR_DATA, data, length);
}

/**
 * Read a burst of data from the FPGA.
 * The address is implicit.
 * @param data The buffer that will be written to.
 * @param length The number of bytes to read onto that buffer.
 */
uint8_t *fpga_read_burst_ref(uint8_t *data, uint16_t length)
{
    uint16_t offset = 0;
    uint8_t spiXferLength;
    uint8_t length_lo;
    uint8_t length_hi;
    uint8_t *spiRxBuff;

    do {
        spiXferLength = MIN(SPI_MAX_BURST_LENGTH, length);

        // prepare FPGA for burst read
        length_lo = spiXferLength & 0x00FF;
        length_hi = (spiXferLength & 0xFF00) >> 8;
        fpga_write_byte(FPGA_RD_BURST_SIZE_LO, length_lo);
        fpga_write_byte(FPGA_RD_BURST_SIZE_HI, length_hi);

        // select FPGA on SPI bus
        spi_set_cs_pin(SPIM_SS1_PIN);

        spiRxBuff = spi_read_burst(FPGA_BURST_RD_DATA, spiXferLength);
        memcpy((data+offset), spiRxBuff, spiXferLength);

        offset += spiXferLength;
        length -= spiXferLength;

    } while (length);

    return data;
}

uint8_t *fpga_read_burst(uint16_t length)
{
    // prepare FPGA for burst read
    uint8_t length_lo = length & 0x00FF;
    uint8_t length_hi = (length & 0xFF00) >> 8;
    fpga_write_byte(FPGA_RD_BURST_SIZE_LO, length_lo);
    fpga_write_byte(FPGA_RD_BURST_SIZE_HI, length_hi);

    // select FPGA on SPI bus
    spi_set_cs_pin(SPIM_SS1_PIN);

    return spi_read_burst(FPGA_BURST_RD_DATA, length);
}

uint32_t fpga_get_capture_size(void)
{
    // verify that captured frame(s) is/are available
    if(!fpga_capture_done()) return 0;

    uint8_t size_0 = fpga_read_byte(FPGA_CAPTURE_SIZE_0);
    uint8_t size_1 = fpga_read_byte(FPGA_CAPTURE_SIZE_1);
    uint8_t size_2 = fpga_read_byte(FPGA_CAPTURE_SIZE_2);
    uint8_t size_3 = fpga_read_byte(FPGA_CAPTURE_SIZE_3);
    return (size_3<<24) + (size_2<<16) + (size_1<<8) + size_0;
}

uint32_t fpga_get_bytes_read(void)
{
    uint8_t size_0 = fpga_read_byte(FPGA_CAPT_BYTE_COUNT_0);
    uint8_t size_1 = fpga_read_byte(FPGA_CAPT_BYTE_COUNT_1);
    uint8_t size_2 = fpga_read_byte(FPGA_CAPT_BYTE_COUNT_2);
    uint8_t size_3 = fpga_read_byte(FPGA_CAPT_BYTE_COUNT_3);
    return (size_3<<24) + (size_2<<16) + (size_1<<8) + size_0;
}

uint16_t fpga_get_checksum(void)
{
    uint8_t check_0 = fpga_read_byte(FPGA_CAPT_FRM_CHECKSUM_0);
    uint8_t check_1 = fpga_read_byte(FPGA_CAPT_FRM_CHECKSUM_1);
    return ((check_1<<8) + check_0);
}

bool fpga_is_buffer_at_start(void)
{
    return (fpga_read_byte(FPGA_CAPTURE_STATUS) & FPGA_START_OF_CAPT);
}

bool fpga_is_buffer_read_done(void)
{
    return (fpga_read_byte(FPGA_CAPTURE_STATUS) & (FPGA_VIDEO_CAPT_DONE | FPGA_AUDIO_CAPT_DONE | FPGA_FRAME_CAPT_DONE));
    //return (fpga_read_byte(FPGA_CAPTURE_STATUS) & (FPGA_AUDIO_CAPT_DONE));
}

// testing functions
bool fpga_test_reset(void)
{
    bool success = false;

    // currently this register does nothing, can be used for test
    fpga_write_byte(FPGA_MEMORY_CONTROL, 0xBB);
    success = (0xBB == fpga_read_byte(FPGA_MEMORY_CONTROL));
    if (!success) return (success);

    fpga_write_byte(FPGA_CAMERA_CONTROL, 0x01); // this will turn on XCLK
    success = (0x01 == fpga_read_byte(FPGA_CAMERA_CONTROL));
    if (!success) return (success);

    fpga_soft_reset();
    // check registers all have default values
    success = (FPGA_SYSTEM_CONTROL_DEFAULT == fpga_read_byte(FPGA_SYSTEM_CONTROL));
    if (!success) return (success);
    success = (FPGA_DISPLAY_CONTROL_DEFAULT == fpga_read_byte(FPGA_DISPLAY_CONTROL));
    if (!success) return (success);
    success = (FPGA_MEMORY_CONTROL_DEFAULT == fpga_read_byte(FPGA_MEMORY_CONTROL));
    if (!success) return (success);
    success = (FPGA_CAMERA_CONTROL_DEFAULT == fpga_read_byte(FPGA_CAMERA_CONTROL));
    if (!success) return (success);
    // moved this to memory self-test, since bit FPGA_MEM_INIT_DONE will not be set if memory initialization fails
    //success = (FPGA_SYSTEM_STATUS_DEFAULT == fpga_read_byte(FPGA_SYSTEM_STATUS));

    return(success);
}

bool fpga_ram_check(void)
{
    bool success = false;
    //first check that memory initialization succeeded
    // NOTE as of 2021-10-28, FPGA IP only supports one memory chip, so on MK11 only uses U10
    success = (FPGA_SYSTEM_STATUS_DEFAULT == fpga_read_byte(FPGA_SYSTEM_STATUS));
    if (!success) return (success);

    // run memory self-test
    // TODO: requires FPGA code support
    return(success);
}

bool fpga_spi_exercise_register(uint8_t addr)
{
    // check that it is a valid register
    assert(FPGA_REGISTER_EXISTS(addr));

    // select FPGA on SPI bus
    spi_set_cs_pin(SPIM_SS1_PIN);

    // run SPI exercise, return TRUE if successful
    return(spi_exercise_register(addr));
}

// externally visible functions

// TODO: init function, to configure fpga_int_N pin

void fpga_soft_reset(void)
{
    fpga_write_byte(FPGA_SYSTEM_CONTROL, 0x01); // reset FPGA
    nrfx_systick_delay_ms(185); // TODO: not sure if we need this, but just in case...
    fpga_write_byte(FPGA_SYSTEM_CONTROL, 0x00); // clear the reset (needed for some FPGA projects, like OLED unit test)
    // from testing, 2ms seems to be the minimum delay needed for all registers to return to expected values
    // reason is unclear
    //nrfx_systick_delay_ms(5); // use 5ms for extra safety margin (used to work earlier)
    // NOTE: from 2021-02-19, 5ms is no longer enough
    nrfx_systick_delay_ms(185); // TODO: why? Seems to require 170ms delay now
}

bool fpga_xclk_on(void)
{
    bool success = false;

    fpga_write_byte(FPGA_CAMERA_CONTROL, FPGA_EN_XCLK);
    nrfx_systick_delay_ms(300); // TODO: is this still needed?
    success = (fpga_read_byte(FPGA_CAMERA_CONTROL) == FPGA_EN_XCLK);
    return success;
}

/**
 * Turn on the camera.
 * @return True if it has been effectively turned on.
 */
bool fpga_camera_on(void)
{
    bool success = false;

    nrfx_systick_delay_ms(4*(1000/OV5640_FPS) + 1); // delay 4 frames to discard AWB adjustments (needed if camera was just powered up)
    fpga_write_byte(FPGA_CAMERA_CONTROL, (FPGA_EN_XCLK | FPGA_EN_CAM)); // enable camera interface (& keep XCLK enabled!)
    success = (fpga_read_byte(FPGA_CAMERA_CONTROL) == (FPGA_EN_XCLK | FPGA_EN_CAM));
    LOG("fpga_camera_on() waited 4 frames, sent FPGA_EN_XCLK, FPGA_EN_CAM");
    return success;
}

/**
 * Turn off the camera.
 * @return True if it has been effectively turned off.
 */
bool fpga_camera_off(void)
{
    bool success = false;

    fpga_write_byte(FPGA_CAMERA_CONTROL, FPGA_EN_XCLK); // turn off camera interface (but keep XCLK enabled!)
    nrfx_systick_delay_ms(1*(1000/OV5640_FPS) + 1); // allow last frame to finish entering video buffer to avoid split screen
    success = (fpga_read_byte(FPGA_CAMERA_CONTROL) == FPGA_EN_XCLK);
    LOG("fpga_camera_off() sent FPGA_EN_XCLK, waited 1 frame");
    return success;
}

/**
 * Turn on the microphone.
 * @return True if it has been effectively turned on.
 */
bool fpga_mic_on(void)
{
    bool success = false;

    fpga_write_byte(FPGA_MIC_CONTROL, FPGA_EN_MIC);
    success = (fpga_read_byte(FPGA_MIC_CONTROL) == FPGA_EN_MIC);
    return success;
}

/**
 * Turn off the microphone.
 * @return True if it has been effectively turned off.
 */
bool fpga_mic_off(void)
{
    bool success = false;

    fpga_write_byte(FPGA_MIC_CONTROL, 0x00);
    success = (fpga_read_byte(FPGA_MIC_CONTROL) == 0x00);
    return success;
}

/**
 * Enable the live streaming of the camera directly to the screen.
 */
void fpga_disp_live(void)
{
    fpga_write_byte(FPGA_DISPLAY_CONTROL, FPGA_DISP_CAM);
}

/**
 * Enable the busy indicator: gray screen.
 */
void fpga_disp_busy(void)
{
    fpga_write_byte(FPGA_DISPLAY_CONTROL, FPGA_DISP_BUSY);
}

/**
 * Enable the display of 8 vertical color bars to the screen.
 */
void fpga_disp_bars(void)
{
    fpga_write_byte(FPGA_DISPLAY_CONTROL, FPGA_DISP_BARS);
}

#ifndef FPGA_RELEASE_20210709 // TODO: Remove this?
/**
 * Enable the red-blue shift chrominance correction.
 */
void fpga_disp_RB_shift(bool enable)
{
    uint8_t display_reg = 0;
    display_reg = fpga_read_byte(FPGA_DISPLAY_CONTROL);
    if (enable) {
        fpga_write_byte(FPGA_DISPLAY_CONTROL, display_reg | FPGA_EN_RB_SHIFT);
    } else {
        fpga_write_byte(FPGA_DISPLAY_CONTROL, display_reg & ~FPGA_EN_RB_SHIFT);
    }
}
#endif

/**
 * Disable the red-blue shift chrominace correction.
 */
void fpga_disp_off(void)
{
    fpga_write_byte(FPGA_DISPLAY_CONTROL, FPGA_DISP_OFF);
}

/**
 * Resume live video feed and clear the checksum.
 */
void fpga_resume_live_video(void)
{
    //fpga_write_byte(FPGA_CAPTURE_CONTROL, FPGA_RESUME_FILL);
    fpga_write_byte(FPGA_CAPTURE_CONTROL, (FPGA_RESUME_FILL | FPGA_CLR_CHKSM));
}

/**
 * Enable the capture of a frame of the video.
 */
void fpga_image_capture(void)
{
    fpga_write_byte(FPGA_CAPTURE_CONTROL, (FPGA_CAPT_EN | FPGA_CAPT_FRM));
}

void fpga_video_capture(void)
{
#ifdef MIC_ON
    fpga_write_byte(FPGA_CAPTURE_CONTROL, (FPGA_CAPT_EN | FPGA_CAPT_VIDEO | FPGA_CAPT_AUDIO));
#else
    fpga_write_byte(FPGA_CAPTURE_CONTROL, (FPGA_CAPT_EN | FPGA_CAPT_VIDEO));
#endif
}

// first clear checksum, then set bit for audio data burst read
void fpga_prep_read_audio(void)
{
#ifdef MIC_ON
    fpga_write_byte(FPGA_CAPTURE_CONTROL, 0x00); //ensure CLR_CHECKSUM gets a rising edge
    fpga_write_byte(FPGA_CAPTURE_CONTROL, FPGA_CLR_CHKSM); // clear checksum (left from video transfer)
    fpga_write_byte(FPGA_CAPTURE_CONTROL, FPGA_RD_AUDIO);  // this also sets CLR_CHKSM back to 0
    //fpga_write_byte(FPGA_CAPTURE_CONTROL, (FPGA_CAPT_EN | FPGA_CAPT_VIDEO | FPGA_CAPT_AUDIO)); // this also sets CLR_CHKSM back to 0
#else
    // do nothing
#endif
}

void fpga_replay_rate(uint8_t repeat)
{
    if ((repeat == 0) || (repeat > FPGA_REP_RATE_MASK)) // cannot be zero, max 5 bits
    {
        NRFX_LOG_ERROR("fpga_replay_rate(), invalid input parameter %d", repeat);
        return;
    }
    fpga_write_byte(FPGA_REPLAY_RATE_CONTROL, repeat);
    //LOG("FPGA replay rate set to %d", repeat);
}

bool fpga_capture_done(void)
{
    return(fpga_read_byte(FPGA_CAPTURE_STATUS) & FPGA_CAPT_RD_VLD);
}

// Simple checksum calculation; matching the algorithm used on FPGA
// Sums a byte stream as 16-bit words (with first byte being least significant), adding carry back in, to return 16-bit result
// Precondition: length must be a multiple of 2 (bytes)
uint16_t fpga_calc_checksum(uint8_t *bytearray, uint32_t length)
{
    if((bytearray == NULL) || (length == 0)) return 0;
    if((length % 2) != 0) return 0;

    uint32_t checksum = 0;
    uint32_t loop = 0;
    for(loop=0; loop<length; loop=loop+2)
    {
        checksum = fpga_checksum_add(checksum, ((bytearray[loop+1]<<8) + bytearray[loop]));
    }
    return (uint16_t) checksum;
}

uint16_t fpga_checksum_add(uint16_t checksum1, uint16_t checksum2)
{
    uint32_t checksum = 0;
    uint32_t carry = 0;

    checksum = checksum1 + checksum2;
    if(checksum > 0x0000FFFF) // add carry (which if it exists must be 1) back in
    {
        carry = checksum & 0xFFFF0000;
        carry = carry >> 16;
        assert(carry == 1); // this should always be true, if so we can simplify the above
        checksum = (checksum & 0x0000FFFF) + carry;
        assert(checksum <= 0x0000FFFF);
    }
    return (uint16_t) checksum; //NOTE this line was missing until 2021-04-15!
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
            fpga_write_byte(FPGA_CAMERA_CONTROL, FPGA_EN_XCLK | FPGA_EN_CAM | zoom_bits); // FPGA_EN_ZOOM = 0
            break;
        case 2:
        case 4:
        case 8:
            zoom_bits = (level >> 2) << FPGA_ZOOM_SHIFT;
            fpga_write_byte(FPGA_CAMERA_CONTROL, FPGA_EN_XCLK | FPGA_EN_CAM | zoom_bits | FPGA_EN_ZOOM | FPGA_EN_LUMA_COR);
            break;
        default:
            NRFX_LOG_ERROR("FPGA Zoom invalid zoom level.");
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
    reg = fpga_read_byte(FPGA_CAMERA_CONTROL);
    if (!(reg & FPGA_EN_ZOOM)) return; // only valid when zoom is active
    luma_on = (reg & FPGA_EN_LUMA_COR);
    if ((luma_on && turn_on) || (!luma_on && !turn_on)) return; // already correctly set, nothing to do
    if (turn_on) {
        reg = reg | FPGA_EN_LUMA_COR;
        LOG("FPGA turn luma correction on.");
    } else {
        reg = reg & ~FPGA_EN_LUMA_COR;
        LOG("FPGA turn luma correction off.");
    }
    fpga_write_byte(FPGA_CAMERA_CONTROL, reg);
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
            LOG("FPGA display mode = off.");
            break;
        case 1:
            fpga_disp_live();
            LOG("FPGA display mode = video.");
            break;
        case 2:
            fpga_disp_busy();
            LOG("FPGA display mode = busy.");
            break;
        case 3:
            fpga_disp_bars();
            LOG("FPGA display mode = color bars.");
            break;
        default:
            NRFX_LOG_ERROR("FPGA display mode invalid.");
    }
}

/**
 * Read the version register of the FPGA.
 * @param major Major revision number.
 * @param minor Minor revision number.
 */
void fpga_get_version(uint8_t *major, uint8_t *minor)
{
    *major = fpga_read_byte(FPGA_VERSION_MAJOR);
    *minor = fpga_read_byte(FPGA_VERSION_MINOR);
    LOG("fpga_get_version(): %d.%d", *major, *minor);
}

/**
 * Inform the FPGA to discard the oldest capture buffer.
 * For now, the FPGA only supports one buffer, and it is discarded when a new capture is made.
 */
void fpga_discard_buffer(void)
{
    // TODO:
}
