MICROPY_ROM_TEXT_COMPRESSION ?= 1
MICROPY_VFS_LFS2 = 1
MICROPY_VFS_FAT = 0
FROZEN_MANIFEST = modules/manifest.py
CROSS_COMPILE = arm-none-eabi-

include micropython/py/mkenv.mk
include micropython/extmod/extmod.mk
include micropython/py/py.mk

SD = s140
SOFTDEVICE_VERSION = 6.1.1
SOFTDEVICE_HEX = softdevice/$(SD)_nrf52_$(SOFTDEVICE_VERSION)_softdevice.hex
HEXMERGE = hexmerge.py
OPENOCD = openocd
OPENOCD_FLASH = -c "init; nrf52_recover; program build/firmware.hex verify; exit"
OPENOCD_GDB = -c "init; gdb_port 3334"
OPENOCD_STLINK = -f interface/stlink-dap.cfg -f target/nrf52.cfg
OPENOCD_JLINK = -f interface/jlink.cfg -c "transport select swd" -f target/nrf52.cfg
NRFJPROG = nrfjprog
JLINKGDBSERVERCL = JLinkGDBServerCLExe
GDB = gdb-multiarch
PYTHON = python3

CFLAGS += $(DEF) $(INC) $(CFLAGS_EXTRA) $(CFLAGS_MOD) $(CFLAGS_MCU_m4)
CFLAGS += -mthumb
CFLAGS += -mabi=aapcs
CFLAGS += -fsingle-precision-constant
CFLAGS += -Wdouble-promotion
CFLAGS += -mtune=cortex-m4
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mfpu=fpv4-sp-d16
CFLAGS += -mfloat-abi=hard
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -Wall
CFLAGS += -ansi
CFLAGS += -std=c11
CFLAGS += -nostdlib
CFLAGS += -fno-strict-aliasing
CFLAGS += -g -Os

LDFLAGS = $(CFLAGS)
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Xlinker -Map=$(@:.elf=.map)
LDFLAGS += -mthumb -mabi=aapcs -Tnrf52832.ld

LIBS += -lgcc -lm

INC += -I.
INC += -Ibuild
INC += -Idrivers
INC += -Igenhdr
INC += -Imicropython
INC += -Imicropython/lib/cmsis/inc
INC += -Imicropython/shared/readline
INC += -Imodules/board
INC += -Imodules/machine
INC += -Inrfx
INC += -Inrfx/drivers
INC += -Inrfx/drivers/include
INC += -Inrfx/drivers/src
INC += -Inrfx/hal
INC += -Inrfx/mdk
INC += -Isoftdevice/include
INC += -Isoftdevice/include/nrf52

DEF += -DNRF52832_XXAA
DEF += -DNRF52832
DEF += -DCONFIG_GPIO_AS_PINRESET
DEF += -DNRF5_HAL_H="<nrf52832_hal.h>"
DEF += -DSOFTDEVICE_PRESENT
DEF += -DBLUETOOTH_SD=140
DEF += -DBLUETOOTH_SD_DEBUG=1
DEF += -DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
DEF += -DMICROPY_MODULE_FROZEN_MPY
DEF += -DMICROPY_MODULE_FROZEN_STR

SRC += help.c
SRC += main.c
SRC += mphalport.c
SRC += nrf52832.c

SRC += drivers/monocle_battery.c
SRC += drivers/monocle_ble.c
SRC += drivers/monocle_board.c
SRC += drivers/monocle_checksum.c
SRC += drivers/monocle_fpga.c
SRC += drivers/monocle_i2c.c
SRC += drivers/monocle_iqs620.c
SRC += drivers/monocle_max77654.c
SRC += drivers/monocle_ecx335af.c
SRC += drivers/monocle_ov5640.c
SRC += drivers/monocle_spi.c
#SRC += drivers/monocle_touch.c

SRC += modules/board/led.c
SRC += modules/board/modboard.c
SRC += modules/machine/modmachine.c
SRC += modules/machine/pin.c
SRC += modules/machine/pin_gen.c
SRC += modules/machine/rtcounter.c
SRC += modules/machine/timer.c

SRC += micropython/lib/libm/acoshf.c
SRC += micropython/lib/libm/asinfacosf.c
SRC += micropython/lib/libm/asinhf.c
SRC += micropython/lib/libm/atan2f.c
SRC += micropython/lib/libm/atanf.c
SRC += micropython/lib/libm/atanhf.c
SRC += micropython/lib/libm/ef_rem_pio2.c
SRC += micropython/lib/libm/ef_sqrt.c
SRC += micropython/lib/libm/erf_lgamma.c
SRC += micropython/lib/libm/fmodf.c
SRC += micropython/lib/libm/kf_cos.c
SRC += micropython/lib/libm/kf_rem_pio2.c
SRC += micropython/lib/libm/kf_sin.c
SRC += micropython/lib/libm/kf_tan.c
SRC += micropython/lib/libm/log1pf.c
SRC += micropython/lib/libm/math.c
SRC += micropython/lib/libm/nearbyintf.c
SRC += micropython/lib/libm/roundf.c
SRC += micropython/lib/libm/sf_cos.c
SRC += micropython/lib/libm/sf_erf.c
SRC += micropython/lib/libm/sf_frexp.c
SRC += micropython/lib/libm/sf_ldexp.c
SRC += micropython/lib/libm/sf_modf.c
SRC += micropython/lib/libm/sf_sin.c
SRC += micropython/lib/libm/sf_tan.c
SRC += micropython/lib/libm/wf_lgamma.c
SRC += micropython/lib/libm/wf_tgamma.c
SRC += micropython/shared/libc/string0.c
SRC += micropython/shared/readline/readline.c
SRC += micropython/shared/runtime/interrupt_char.c
SRC += micropython/shared/runtime/pyexec.c
SRC += micropython/shared/runtime/sys_stdio_mphal.c
SRC += micropython/shared/timeutils/timeutils.c

SRC += nrfx/drivers/src/nrfx_clock.c
SRC += nrfx/drivers/src/nrfx_gpiote.c
SRC += nrfx/drivers/src/nrfx_nvmc.c
SRC += nrfx/drivers/src/nrfx_rtc.c
SRC += nrfx/drivers/src/nrfx_saadc.c
SRC += nrfx/drivers/src/nrfx_spi.c
SRC += nrfx/drivers/src/nrfx_spim.c
SRC += nrfx/drivers/src/nrfx_systick.c
SRC += nrfx/drivers/src/nrfx_timer.c
SRC += nrfx/drivers/src/nrfx_twi.c
SRC += nrfx/drivers/src/prs/nrfx_prs.c
SRC += nrfx/helpers/nrfx_flag32_allocator.c
SRC += nrfx/mdk/system_nrf52.c

SRC_QSTR += $(SRC) $(SRC_MOD)

OBJ += $(PY_O)
OBJ += $(addprefix build/, $(SRC:.c=.o))
OBJ += $(addprefix build/, $(SRC_MOD:.c=.o))

all: build/firmware.hex

flash_openocd_stlink:
	$(OPENOCD) $(OPENOCD_STLINK) $(OPENOCD_FLASH)

flash_openocd_jlink:
	$(OPENOCD) $(OPENOCD_JLINK) $(OPENOCD_FLASH)

flash_nrfjprog_jlink:
	$(NRFJPROG) --sectorerase --verify --family nrf52 --program build/firmware.hex

gdb_openocd_stlink:
	$(OPENOCD) $(OPENOCD_STLINK) -c "gdb_port 2331"

gdb_openocd_jlink:
	$(OPENOCD) $(OPENOCD_JLINK) -c "gdb_port 2331"

gdb_segger_jlink:
	$(JLINKGDBSERVERCL) -device nrf52832_XXAA -if SWD

gdb:
	$(GDB) \
	    -ex "target extended-remote :2331" \
	    -ex "monitor reset halt" \
	    -ex "continue" build/application.elf

shell:
	@echo "You should soon get a Python shell over Bluetooth"
	@echo "  CTRL-B then Enter to switch to the friendly REPL"
	@echo "  CTRL-A to switch to the raw REPL"
	@echo "  CTRL-\\ to exit"
	$(PYTHON) serial_console.py

build/application.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LDFLAGS_MOD) $(LIBS)
	$(Q)$(SIZE) $@

build/firmware.hex: $(SOFTDEVICE_HEX) build/application.hex
	$(HEXMERGE) -o $@ $(SOFTDEVICE_HEX) build/application.hex

.SUFFIXES: .elf .hex

.elf.hex:
	$(OBJCOPY) -O ihex $< $@

include ./micropython/py/mkrules.mk
