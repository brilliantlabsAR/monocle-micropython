# Include the core environment definitions
include micropython/py/mkenv.mk

# Include py core make definitions
include micropython/py/py.mk

# Set makefile-level MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# Which python files to freeze into the firmware are listed in here
FROZEN_MANIFEST = manifest.py

# Define the toolchain prefix for ARM GCC
CROSS_COMPILE = arm-none-eabi-

# TODO optimize this away as we can choose ourselves
include micropython/extmod/extmod.mk

# Create the firmware version defines and final release file name
GIT_COMMIT = $(shell git rev-parse --short HEAD)
IS_DIRTY = $(shell git diff --quiet || echo '-dirty')
RELEASE_DATE = $(shell TZ= date +v%y.%j.%H%M)
BUILD_NAME = monocle-firmware-$(RELEASE_DATE)$(IS_DIRTY)

# Warning options
WARN = -Wall -Wdouble-promotion -Wfloat-conversion

# Build optimizations
OPT += -Os -fdata-sections -ffunction-sections 
OPT += -flto 
OPT += -fsingle-precision-constant
OPT += -fshort-enums
OPT += -fno-strict-aliasing
OPT += -fno-common
OPT += -g3

# TODO fix when fix is available
# https://github.com/micropython/micropython/issues/10562
OPT += -fno-tree-loop-distribute-patterns

# Save some code space for performance-critical code
CSUPEROPT = -Os

# Add required build options
OPT += -std=gnu17
OPT += -mthumb
OPT += -mtune=cortex-m4
OPT += -mcpu=cortex-m4
OPT += -mfpu=fpv4-sp-d16
OPT += -mfloat-abi=hard
OPT += -mabi=aapcs

# Set defines
DEFS += -DNRF52832_XXAA
DEFS += -DNDEBUG
DEFS += -DCONFIG_NFCT_PINS_AS_GPIOS
DEFS += -DGIT_COMMIT='"$(GIT_COMMIT)"'
DEFS += -DBUILD_VERSION='"$(BUILD_VERSION)"'

# Set linker options
LDFLAGS += -nostdlib
LDFLAGS += -Lnrfx/mdk -T nrf52832_linker_file.ld
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Xlinker -Map=$(@:.elf=.map)

INC += -I.
INC += -Ibuild
INC += -Imodules
INC += -Isegger_rtt
INC += -Imicropython
INC += -Imicropython/lib/cmsis/inc
INC += -Imicropython/shared/readline
INC += -Inrfx
INC += -Inrfx/drivers
INC += -Inrfx/drivers/include
INC += -Inrfx/drivers/src
INC += -Inrfx/hal
INC += -Inrfx/helpers
INC += -Inrfx/mdk
INC += -Inrfx/soc
INC += -Isoftdevice/include
INC += -Isoftdevice/include/nrf52

# Assemble the C flags variable
CFLAGS += $(WARN) $(OPT) $(INC) $(DEFS)

SRC_C += main.c
SRC_C += nrfx_log.c
SRC_C += mphalport.c
SRC_C += startup_nrf52832.c

SRC_C += driver/battery.c
SRC_C += driver/bluetooth_data_protocol.c
SRC_C += driver/bluetooth_low_energy.c
SRC_C += driver/ecx336cn.c
SRC_C += driver/flash.c
SRC_C += driver/fpga.c
SRC_C += driver/i2c.c
SRC_C += driver/iqs620.c
SRC_C += driver/ov5640.c
SRC_C += driver/spi.c
SRC_C += driver/timer.c
SRC_C += font.c
SRC_C += libgfx.c
SRC_C += libjojpeg.c

SRC_C += modules/camera.c
SRC_C += modules/display.c
SRC_C += modules/fpga.c
SRC_C += modules/led.c
SRC_C += modules/device.c
SRC_C += modules/time.c
SRC_C += modules/touch.c

SRC_C += segger_rtt/SEGGER_RTT.c
SRC_C += segger_rtt/SEGGER_RTT_Syscalls_GCC.c
SRC_C += segger_rtt/SEGGER_RTT_printf.c

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
SRC_C += micropython/shared/libc/printf.c
SRC_C += micropython/shared/libc/string0.c
SRC_C += micropython/shared/readline/readline.c
SRC_C += micropython/shared/runtime/interrupt_char.c
SRC_C += micropython/shared/runtime/pyexec.c
SRC_C += micropython/shared/runtime/sys_stdio_mphal.c
SRC_C += micropython/shared/timeutils/timeutils.c

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

OBJ += $(PY_CORE_O)
OBJ += $(addprefix build/, $(SRC_C:.c=.o))

# Link required libraries
LIB += -lm -lc -lgcc

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

release:
	sed -i 's/NEXT_RELEASE/$(RELEASE)/' CHANGELOG.md
	git commit -am "Release $(RELEASE)"
	git tag $(RELEASE)

include micropython/py/mkrules.mk

# TODO fix the zip creation
# Bluetooth settings generated by nrfutil below
# BLSETTINGS_HEX := build/blsettings.hex
# BOOTLOADER_HEX := bootloader/build/nrf52832_xxaa_s132.hex
# BOOTLOADER_KEY := bootloader/published_privkey.pem


########################################
# TODO turn these into one flash command, and enable plugin for OpenOCD

# OPENOCD = openocd -c 'gdb_port 2331'
# OPENOCD_FLASH = -c 'init; nrf52_recover; program build/firmware.hex verify; reset run; exit'
# OPENOCD_RTT = -c 'init; rtt setup 0x20000000 0x8000 "SEGGER RTT"; rtt start; rtt server start 9090 0'
# OPENOCD_STLINK = -f interface/stlink-dap.cfg -f target/nrf52.cfg
# OPENOCD_JLINK = -f interface/jlink.cfg -c 'transport select swd' -f target/nrf52.cfg

flash_openocd_stlink:
	$(OPENOCD) $(OPENOCD_STLINK) $(OPENOCD_FLASH)

flash_openocd_jlink:
	$(OPENOCD) $(OPENOCD_JLINK) $(OPENOCD_FLASH)

rtt_openocd_stlink:
	$(OPENOCD) $(OPENOCD_STLINK) $(OPENOCD_RTT)

rtt_openocd_jlink:
	$(OPENOCD) $(OPENOCD_JLINK) $(OPENOCD_RTT)

flash_nrfjprog_jlink:
	$(NRFJPROG) --family nrf52 --recover
	$(NRFJPROG) --family nrf52 --verify --program ${FIRMWARE_HEX} --debugreset

gdb_openocd_stlink:
	$(OPENOCD) $(OPENOCD_STLINK)

gdb_openocd_jlink:
	$(OPENOCD) $(OPENOCD_JLINK)

########################################
