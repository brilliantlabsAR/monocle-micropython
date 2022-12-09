/*
 * Copyright (c) 2021 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#ifndef FPGA_H
#define FPGA_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Driver for configuring the SPI-controlled FPGA.
 * It controls the low-level read/write with the registers and bulk data transfer.
 * It provides a higher level API for:
 * - the FPGA itself,
 * - the Camera data path,
 * - the Microdisplay data path,
 * - the Microphone data,
 * - the Checksum calculation,
 * @defgroup fpga
 */

#define fpga_check_reg(reg) NRFX_LOG_ERROR("[0x%02X] %-s=%d", reg, #reg, fpga_read_register(reg))

/** number of capture buffers supported */
#define FPGA_BUFFERS_SUPPORTED    1 // single buffer

#define FPGA_NUM_VIDEO_FRAMES     62u
#define FPGA_FRAME_SIZE           512000u

/* for live video code, per Register Specification 2021-03-25 */
#define FPGA_SYSTEM_CONTROL       0x00 ///< RW
#define FPGA_DISPLAY_CONTROL      0x01 ///< RW
#define FPGA_MEMORY_CONTROL       0x02 ///< RW
#define FPGA_LED_CONTROL          0x03 ///< RW
#define FPGA_CAMERA_CONTROL       0x04 ///< RW
#define FPGA_SYSTEM_STATUS        0x05 ///< RO
#define FPGA_WR_BURST_SIZE_LO     0x06 ///< RW
#define FPGA_WR_BURST_SIZE_HI     0x07 ///< RW
#define FPGA_BURST_WR_DATA        0x08 ///< RW
#define FPGA_RD_BURST_SIZE_LO     0x09 ///< RW
#define FPGA_RD_BURST_SIZE_HI     0x0A ///< RW
#define FPGA_BURST_RD_DATA        0x0B ///< RO
#define FPGA_CAPTURE_CONTROL      0x0C ///< RW
#define FPGA_CAPTURE_STATUS       0x0D ///< RO
#define FPGA_CAPTURE_SIZE_0       0x0E ///< RO
#define FPGA_CAPTURE_SIZE_1       0x0F ///< RO
#define FPGA_CAPTURE_SIZE_2       0x10 ///< RO
#define FPGA_CAPTURE_SIZE_3       0x11 ///< RO
#define FPGA_CAPT_FRM_CHECKSUM_0  0x12 ///< RO
#define FPGA_CAPT_FRM_CHECKSUM_1  0x13 ///< RO
#define FPGA_REPLAY_RATE_CONTROL  0x14 ///< RW

/* updated Register Specification 2021-05-12 */
#define FPGA_MIC_CONTROL          0x15 ///< RW
#define FPGA_CAPT_BYTE_COUNT_0    0x16 ///< RO
#define FPGA_CAPT_BYTE_COUNT_1    0x17 ///< RO
#define FPGA_CAPT_BYTE_COUNT_2    0x18 ///< RO
#define FPGA_CAPT_BYTE_COUNT_3    0x19 ///< RO

/* Taken from monocal_mrb branch; current FPGA does not have these registers; reads will return 0 (& seems won't crash FPGA) */
// TODO: add support in the next FPGA release
#define FPGA_VERSION_MINOR        0x1E ///< RO FPGA build minor version number
#define FPGA_VERSION_MAJOR        0x1F ///< RO FPGA build major version number

#define FPGA_REGISTER_EXISTS(reg)  \
    ((reg) >= FPGA_SYSTEM_CONTROL && (reg) <= FPGA_VERSION_MAJOR)

#ifndef BIT
#define BIT(n) (0x01<<n)
#endif

/* System Control Register (0x00) */
// Reserved                       bits 7:1
#define FPGA_RST_SW               BIT(0) ///< Software reset. Set to 1, it will auto-reset to 0.
#define FPGA_SYSTEM_CONTROL_DEFAULT       0x00 // default value on reboot/reset

/* Display Control Register (0x01) */
// NOTE: only one of the below may be set at a time (don't combine the bits, except EN_RB_SHIFT which can combine with any other)
// Reserved                       bits 7:4
#ifndef FPGA_RELEASE_20210709
#define FPGA_EN_RB_SHIFT          BIT(3) ///< enable Red Blue shift (for display optics compensation)
#endif
#define FPGA_DISP_BARS            BIT(2) ///< display 8 vertical color bars
#define FPGA_DISP_BUSY            BIT(1) ///< display busy indicator (grey screen)
#define FPGA_DISP_CAM             BIT(0) ///< display on, video from camera/buffer
#define FPGA_DISP_OFF             0x00   // display off (video sync & data signals off; TODO: PLL off)
#define FPGA_DISPLAY_CONTROL_DEFAULT 0x01 ///< default value on reboot/reset

/* Memory Control Register (0x02) */
#define FPGA_MEMORY_CONTROL_DEFAULT 0x02 ///< default value on reboot/reset

/* LED Control Register (0x03) */
#define FPGA_LED_CONTROL_DEFAULT  0x00 ///< default value on reboot/reset

/* Camera Control Register (0x04) */
// Reserved                       bits 7:6
#define FPGA_EN_LUMA_COR          BIT(5) ///< enable luma correction for zoom mode
#define FPGA_EN_ZOOM              BIT(4) ///< enable zoom
#define FPGA_ZOOM                 (BIT(3) | BIT(2)) // zoom factor
#define FPGA_ZOOM_SHIFT           0x02
#define FPGA_ZOOM_MASK            0x03
#define FPGA_EN_CAM               BIT(1) ///< enable camera
#define FPGA_EN_XCLK              BIT(0) ///< enable 24MHz XCLK to camera
#define FPGA_CAMERA_CONTROL_DEFAULT       0x00 // default value on reboot/reset

/* System Status Register (0x05) */
// Reserved                       bit 7
#define FPGA_CAPT_FIFO_UNDERRUN   BIT(6) ///< camera FIFO underrun (empty error)
#define FPGA_CAPT_FIFO_OVERRUN    BIT(5) ///< camera FIFO overrun (full error)
#define FPGA_CAM_FIFO_UNDERRUN    BIT(4) ///< camera FIFO underrun (empty error)
#define FPGA_CAM_FIFO_OVERRUN     BIT(3) ///< camera FIFO overrun (full error)
#define FPGA_RD_ERROR             BIT(2) ///< read error
#define FPGA_WR_ERROR             BIT(1) ///< write error
#define FPGA_MEM_INIT_DONE        BIT(0) ///< memory initialization done
#define FPGA_SYSTEM_STATUS_DEFAULT        0x01 // default value on reboot/reset

/* Capture Control Register (0x0C) */
#define FPGA_CLR_CHKSM            BIT(6) ///< [1] clear checksum calculation for previously captured frame/video/audio
#define FPGA_RESUME_FILL          BIT(5) ///< [1] resume video; only effective if eAPT_EN is set
#define FPGA_RD_AUDIO             BIT(4) ///< select for read (after capture) of audio
#define FPGA_CAPT_AUDIO           BIT(3) ///< select for capture of audio (can be with video)
#define FPGA_CAPT_VIDEO           BIT(2) ///< select for capture of video
#define FPGA_CAPT_FRM             BIT(1) ///< select for capture of single frame
#define FPGA_CAPT_EN              BIT(0) ///< [1] enable capture, use together with 1 or 2 of CAPT_FRM, _VIDEO, _AUDIO
#define FPGA_CAPTURE_CONTROL_DEFAULT      0x00 // default value on reboot/reset
// [1] These bits are triggered by rising edge: write 0 followed by 1 to activate.

/* Capture Status Register (0x0D) */
// Reserved                       bits 7:5
#define FPGA_VIDEO_CAPT_DONE      BIT(4) ///< Last byte of video capture read. Indicates captured video read completely
#define FPGA_AUDIO_CAPT_DONE      BIT(3) ///< Last byte of audio capture read. Indicates captured audio read completely.
#define FPGA_FRAME_CAPT_DONE      BIT(2) ///< Last byte of frame capture read. Indicates captured frame read completely
#define FPGA_START_OF_CAPT        BIT(1) ///< Start of capture indicates start of video frame or start of audio. This is to make sure that the MCU is in sync with FPGA.
#define FPGA_CAPT_RD_VLD          BIT(0) ///< Captured data is valid and available for read
#define FPGA_CAPTURE_STATUS_DEFAULT       0x00 // default value on reboot/reset

/* Replay Rate Control Register (0x14) */
// Reserved                       bits 7:5
// NOTE: must not be set to zero
#define FPGA_REP_RATE_CONTROL     (BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)) ///< in replay mode, number of times each frame in buffer is repeated to OLED
#define FPGA_REP_RATE_SHIFT       0x00
#define FPGA_REP_RATE_MASK        0x1F
#define FPGA_REPLAY_RATE_CONTROL_DEFAULT  0x03 ///< default value on reboot/reset

/* Microphone Control Register (0x15) */
#define FPGA_EN_MIC               BIT(0) ///< enable mic
#define FPGA_MIC_CONTROL_DEFAULT          0x00 ///< default value on reboot/reset

void fpga_prepare(void);
void fpga_init(void);
void fpga_deinit(void);

// TODO: these should eventually be hidden, after we are done debugging (& move #defines above to .c file)
void fpga_write_register(uint8_t addr, uint8_t byte);
uint8_t fpga_read_register(uint8_t addr);
// TODO: should be hidden after writing a higher-level burst read function
void fpga_write_burst(uint8_t *buf, uint16_t len);
void fpga_read_burst(uint8_t *buf, uint16_t len);
uint32_t fpga_get_capture_size(void);
uint32_t fpga_get_bytes_read(void);
uint16_t fpga_get_checksum(void);
uint16_t fpga_calc_checksum(uint8_t *bytearray, uint32_t len);
uint16_t fpga_checksum_add(uint16_t checksum1, uint16_t checksum2);
bool fpga_is_buffer_at_start(void);
bool fpga_is_buffer_read_done(void);

// for validation testing
bool fpga_test_reset(void);

bool fpga_ram_check(void);
void fpga_xclk_on(void);
void fpga_camera_on(void);
void fpga_camera_off(void);
void fpga_mic_on(void);
void fpga_mic_off(void);
#if defined(MONOCLE_BOARD_MK9B) || defined(MONOCLE_BOARD_MK10) // only MK9B and MK10 have LEDs controlled by FPGA
// NOTE it is recommended to use the LED interface in led.h instead of calling these directly from main 
void fpga_led_on(uint8_t led_number);
void fpga_led_off(uint8_t led_number);
void fpga_led_toggle(uint8_t led_number);
void fpga_led_on_all(void);
void fpga_led_off_all(void);
#endif
void fpga_disp_live(void);
void fpga_disp_busy(void);
void fpga_disp_bars(void);
#ifndef FPGA_RELEASE_20210709
void fpga_disp_RB_shift(bool enable);
#endif
void fpga_disp_off(void);
void fpga_image_capture(void);
void fpga_video_capture(void);
void fpga_prep_read_audio(void);
void fpga_resume_live_video(void);
void fpga_replay_rate(uint8_t repeat);
bool fpga_capture_done(void);
void fpga_set_zoom(uint8_t level);
void fpga_set_luma(bool turn_on);
void fpga_set_display(uint8_t mode);
// TODO: future implementation (refer to monocal_mrb branch)
void fpga_get_version(uint8_t *major, uint8_t *minor);
void fpga_discard_buffer(void);

// debug
void fpga_check_pins(char const *msg);

#endif
