#
# This file is part of the MicroPython for Monocle project:
#      https://github.com/brilliantlabsAR/monocle-micropython
#
# Authored by: Josuah Demangeon (me@josuah.net)
#              Raj Nakarja / Brilliant Labs Ltd. (raj@itsbrilliant.co)
#
# ISC Licence
#
# Copyright © 2023 Brilliant Labs Ltd.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#

# Include the core environment definitions
include micropython/py/mkenv.mk

# Set makefile-level MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# Which python files to freeze into the firmware are listed in here
FROZEN_MANIFEST = modules/frozen-manifest.py

# Include py core make definitions
include micropython/py/py.mk

# Define the toolchain prefix for ARM GCC
CROSS_COMPILE = arm-none-eabi-

# Use date and time as build version "vYY.DDD.HHMM". := forces evaluation once
BUILD_VERSION := $(shell TZ= date +v%y.%j.%H%M)

# Warning options
WARN = -Wall -Werror -Wdouble-promotion -Wfloat-conversion

# Build optimizations
OPT += -mcpu=cortex-m4
OPT += -mthumb
OPT += -mabi=aapcs
OPT += -mfloat-abi=hard
OPT += -mfpu=fpv4-sp-d16
OPT += -std=gnu17
OPT += -Os -g0
OPT += -fdata-sections -ffunction-sections 
OPT += -fsingle-precision-constant
OPT += -fshort-enums
OPT += -fno-strict-aliasing
OPT += -fno-common
OPT += -flto

# Save some code space for performance-critical code
CSUPEROPT = -Os

# Set defines
DEFS += -DNRF52832_XXAA
DEFS += -DNDEBUG
DEFS += -DCONFIG_NFCT_PINS_AS_GPIOS
DEFS += -DBUILD_VERSION='"$(BUILD_VERSION)"'
DEFS += -DLFS2_NO_ASSERT

# Set linker options
LDFLAGS += -Lnrfx/mdk -T monocle-core/monocle.ld
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Xlinker -Map=$(@:.elf=.map)
LDFLAGS += --specs=nano.specs

INC += -I.
INC += -Ibuild
INC += -Imicropython
INC += -Imicropython/lib/cmsis/inc
INC += -Imicropython/shared/readline
INC += -Imodules
INC += -Imodules/libvgrs/src
INC += -Imonocle-core
INC += -Inrfx
INC += -Inrfx/drivers
INC += -Inrfx/drivers/include
INC += -Inrfx/drivers/src
INC += -Inrfx/hal
INC += -Inrfx/helpers
INC += -Inrfx/mdk
INC += -Inrfx/soc
INC += -Isegger
INC += -Isoftdevice/include
INC += -Isoftdevice/include/nrf52

# Assemble the C flags variable
CFLAGS += $(WARN) $(OPT) $(INC) $(DEFS)

SRC_C += main.c
SRC_C += monocle-core/monocle-critical.c
SRC_C += monocle-core/monocle-drivers.c
SRC_C += monocle-core/monocle-startup.c
SRC_C += mphalport.c

SRC_C += micropython/extmod/moduasyncio.c
SRC_C += micropython/extmod/modubinascii.c
SRC_C += micropython/extmod/moduhashlib.c
SRC_C += micropython/extmod/modujson.c
SRC_C += micropython/extmod/moduos.c
SRC_C += micropython/extmod/modurandom.c
SRC_C += micropython/extmod/modure.c
SRC_C += micropython/extmod/moduselect.c
SRC_C += micropython/extmod/modutime.c
SRC_C += micropython/extmod/vfs_blockdev.c
SRC_C += micropython/extmod/vfs_lfs.c
SRC_C += micropython/extmod/vfs_lfsx_file.c
SRC_C += micropython/extmod/vfs_lfsx.c
SRC_C += micropython/extmod/vfs_reader.c
SRC_C += micropython/extmod/vfs.c
SRC_C += modules/bluetooth.c
SRC_C += modules/camera.c
SRC_C += modules/device.c
SRC_C += modules/display.c
SRC_C += modules/fpga.c
SRC_C += modules/led.c
SRC_C += modules/storage.c
SRC_C += modules/touch.c
SRC_C += modules/update.c
SRC_C += modules/libvgrs/src/modvgr2d.c
SRC_C += modules/libvgrs/src/vgr2dlib.c
SRC_C += modules/modvgr2d-glue.c

SRC_C += segger/SEGGER_RTT_printf.c
SRC_C += segger/SEGGER_RTT_Syscalls_GCC.c
SRC_C += segger/SEGGER_RTT.c

SRC_C += micropython/shared/readline/readline.c
SRC_C += micropython/shared/runtime/gchelper_generic.c
SRC_C += micropython/shared/runtime/interrupt_char.c
SRC_C += micropython/shared/runtime/pyexec.c
SRC_C += micropython/shared/runtime/stdout_helpers.c
SRC_C += micropython/shared/runtime/sys_stdio_mphal.c
SRC_C += micropython/shared/timeutils/timeutils.c

SRC_C += micropython/lib/libm/acoshf.c
SRC_C += micropython/lib/libm/asinfacosf.c
SRC_C += micropython/lib/libm/asinhf.c
SRC_C += micropython/lib/libm/atan2f.c
SRC_C += micropython/lib/libm/atanf.c
SRC_C += micropython/lib/libm/atanhf.c
SRC_C += micropython/lib/libm/ef_rem_pio2.c
SRC_C += micropython/lib/libm/ef_sqrt.c
SRC_C += micropython/lib/libm/erf_lgamma.c
SRC_C += micropython/lib/libm/fmodf.c
SRC_C += micropython/lib/libm/kf_cos.c
SRC_C += micropython/lib/libm/kf_rem_pio2.c
SRC_C += micropython/lib/libm/kf_sin.c
SRC_C += micropython/lib/libm/kf_tan.c
SRC_C += micropython/lib/libm/log1pf.c
SRC_C += micropython/lib/libm/math.c
SRC_C += micropython/lib/libm/nearbyintf.c
SRC_C += micropython/lib/libm/roundf.c
SRC_C += micropython/lib/libm/sf_cos.c
SRC_C += micropython/lib/libm/sf_erf.c
SRC_C += micropython/lib/libm/sf_frexp.c
SRC_C += micropython/lib/libm/sf_ldexp.c
SRC_C += micropython/lib/libm/sf_modf.c
SRC_C += micropython/lib/libm/sf_sin.c
SRC_C += micropython/lib/libm/sf_tan.c
SRC_C += micropython/lib/libm/wf_lgamma.c
SRC_C += micropython/lib/libm/wf_tgamma.c
SRC_C += micropython/lib/littlefs/lfs2_util.c
SRC_C += micropython/lib/littlefs/lfs2.c
SRC_C += micropython/lib/uzlib/crc32.c

SRC_C += nrfx/drivers/src/nrfx_clock.c
SRC_C += nrfx/drivers/src/nrfx_gpiote.c
SRC_C += nrfx/drivers/src/nrfx_nvmc.c
SRC_C += nrfx/drivers/src/nrfx_rtc.c
SRC_C += nrfx/drivers/src/nrfx_saadc.c
SRC_C += nrfx/drivers/src/nrfx_spi.c
SRC_C += nrfx/drivers/src/nrfx_spim.c
SRC_C += nrfx/drivers/src/nrfx_systick.c
SRC_C += nrfx/drivers/src/nrfx_timer.c
SRC_C += nrfx/drivers/src/nrfx_twim.c
SRC_C += nrfx/drivers/src/prs/nrfx_prs.c
SRC_C += nrfx/helpers/nrfx_flag32_allocator.c
SRC_C += nrfx/mdk/system_nrf52.c

SRC_QSTR += $(SRC_C)

OBJ += $(PY_O)
OBJ += $(addprefix build/, $(SRC_C:.c=.o))

# Link required libraries
LIB += -lm -lc -lnosys -lgcc

all: build/application.hex

build/application.hex: build/application.elf
	$(OBJCOPY) -O ihex $< $@

build/application.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LIB)
	$(Q)$(SIZE) $@

flash: build/application.hex
	nrfjprog --program softdevice/*.hex --chiperase -f nrf52 --verify
	nrfjprog --program $< -f nrf52 --verify
	nrfjprog --reset -f nrf52

release: clean build/application.hex
	nrfutil settings generate --family NRF52 --application build/application.hex --application-version 0 --bootloader-version 0 --bl-settings-version 2 build/settings.hex
	mergehex -m build/settings.hex build/application.hex softdevice/s132_nrf52_7.3.0_softdevice.hex bootloader/build/nrf52832_xxaa_s132.hex -o build/monocle-micropython-$(BUILD_VERSION).hex
	nrfutil pkg generate --hw-version 52 --application-version 0 --application build/application.hex --sd-req 0x0124 --key-file bootloader/published_privkey.pem build/monocle-micropython-$(BUILD_VERSION).zip

include micropython/py/mkrules.mk
