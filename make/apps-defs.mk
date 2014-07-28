#
# Copyright (c) 2009-2013, The OpenPilot Team, http://www.openpilot.org
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

ifndef OPENPILOT_IS_COOL
    $(error Top level Makefile must be used to build this target)
endif

# Paths
TOPDIR		= .
OPSYSTEM	= $(TOPDIR)
BOARDINC	= $(TOPDIR)/..
OPSYSTEMINC	= $(OPSYSTEM)/inc
PIOSINC		= $(PIOS)/inc
PIOSCOMMON	= $(PIOS)/common
FLIGHTLIBINC	= $(FLIGHTLIB)/inc

## UAVTalk and UAVObject manager
OPUAVOBJINC	= $(OPUAVOBJ)/inc
OPUAVTALKINC	= $(OPUAVTALK)/inc

## Math
MATHLIB		= $(FLIGHTLIB)/math
MATHLIBINC	= $(FLIGHTLIB)/math

## FreeRTOS support
FREERTOS_DIR	 = $(PIOSCOMMON)/libraries/FreeRTOS
include $(FREERTOS_DIR)/library.mk

## Misc
OPTESTS		= $(TOPDIR)/Tests

## PIOS Hardware
ifeq ($(MCU),cortex-m3)
    include $(PIOS)/stm32f10x/library.mk
else ifeq ($(MCU),cortex-m4)
    include $(PIOS)/stm32f4xx/library.mk
else
    $(error Unsupported MCU: $(MCU))
endif

# List C source files here (C dependencies are automatically generated).
# Use file-extension c for "c-only"-files

## PIOS Hardware (Common Peripherals)
SRC += $(PIOSCOMMON)/pios_adxl345.c
SRC += $(PIOSCOMMON)/pios_bma180.c
SRC += $(PIOSCOMMON)/pios_bmp085.c
SRC += $(PIOSCOMMON)/pios_etasv3.c
SRC += $(PIOSCOMMON)/pios_gcsrcvr.c
SRC += $(PIOSCOMMON)/pios_hcsr04.c
SRC += $(PIOSCOMMON)/pios_hmc5843.c
SRC += $(PIOSCOMMON)/pios_hmc5883.c
SRC += $(PIOSCOMMON)/pios_i2c_esc.c
SRC += $(PIOSCOMMON)/pios_l3gd20.c
SRC += $(PIOSCOMMON)/pios_mpu6000.c
SRC += $(PIOSCOMMON)/pios_mpxv.c
SRC += $(PIOSCOMMON)/pios_ms4525do.c
SRC += $(PIOSCOMMON)/pios_ms5611.c
SRC += $(PIOSCOMMON)/pios_oplinkrcvr.c
SRC += $(PIOSCOMMON)/pios_video.c
SRC += $(PIOSCOMMON)/pios_wavplay.c

## PIOS Hardware (Common)
SRC += $(PIOSCOMMON)/pios_iap.c
SRC += $(PIOSCOMMON)/pios_com.c
SRC += $(PIOSCOMMON)/pios_com_msg.c
SRC += $(PIOSCOMMON)/pios_crc.c
SRC += $(PIOSCOMMON)/pios_flashfs_logfs.c
SRC += $(PIOSCOMMON)/pios_flash_jedec.c
SRC += $(PIOSCOMMON)/pios_debuglog.c
SRC += $(PIOSCOMMON)/pios_deltatime.c
SRC += $(PIOSCOMMON)/pios_rcvr.c
SRC += $(PIOSCOMMON)/pios_rfm22b.c
SRC += $(PIOSCOMMON)/pios_rfm22b_com.c
SRC += $(PIOSCOMMON)/pios_sbus.c
SRC += $(PIOSCOMMON)/pios_sdcard.c
SRC += $(PIOSCOMMON)/pios_led.c

## PIOS USB related files
SRC += $(PIOSCOMMON)/pios_usb_desc_hid_cdc.c
SRC += $(PIOSCOMMON)/pios_usb_desc_hid_only.c
SRC += $(PIOSCOMMON)/pios_usb_util.c

## PIOS system code
SRC += $(PIOSCOMMON)/pios_task_monitor.c
SRC += $(PIOSCOMMON)/pios_callbackscheduler.c
SRC += $(PIOSCOMMON)/pios_notify.c
SRC += $(PIOSCOMMON)/pios_instrumentation.c
SRC += $(PIOSCOMMON)/pios_mem.c
## Misc library functions
SRC += $(FLIGHTLIB)/fifo_buffer.c
SRC += $(FLIGHTLIB)/sanitycheck.c
SRC += $(FLIGHTLIB)/CoordinateConversions.c
SRC += $(MATHLIB)/sin_lookup.c
SRC += $(MATHLIB)/pid.c
SRC += $(MATHLIB)/mathmisc.c
# Choose between printf-stdarg.c and printf2.c in the appropriate board Makefile

## Modules
SRC += $(foreach mod, $(MODULES), $(sort $(wildcard $(OPMODULEDIR)/$(mod)/*.c)))
SRC += $(foreach mod, $(OPTMODULES), $(sort $(wildcard $(OPMODULEDIR)/$(mod)/*.c)))

# Declare all non-optional modules as built-in to force inclusion.
# Built-in modules are always enabled and cannot be disabled.
MODNAMES := $(notdir $(subst /revolution,,$(MODULES)))
MODULES_BUILTIN := $(foreach mod, $(MODNAMES), -DMODULE_$(shell $(ECHO) $(mod) | tr '[:lower:]' '[:upper:]')_BUILTIN)
CDEFS += $(MODULES_BUILTIN)

# List C source files here which must be compiled in ARM-Mode (no -mthumb).
# Use file-extension c for "c-only"-files
SRCARM +=

# List C++ source files here.
# Use file-extension .cpp for C++-files (not .C)
CPPSRC +=

# List C++ source files here which must be compiled in ARM-Mode.
# Use file-extension .cpp for C++-files (not .C)
CPPSRCARM +=

# List Assembler source files here.
# Make them always end in a capital .S. Files ending in a lowercase .s
# will not be considered source files but generated files (assembler
# output from the compiler), and will be deleted upon "make clean"!
# Even though the DOS/Win* filesystem matches both .s and .S the same,
# it will preserve the spelling of the filenames, and gcc itself does
# care about how the name is spelled on its command-line.
ASRC +=

# List Assembler source files here which must be assembled in ARM-Mode.
ASRCARM +=

# List any extra directories to look for include files here.
#    Each directory must be seperated by a space.
EXTRAINCDIRS += $(PIOS)
EXTRAINCDIRS += $(PIOSINC)
EXTRAINCDIRS += $(BOARDINC)
EXTRAINCDIRS += $(FLIGHTLIBINC)
EXTRAINCDIRS += $(PIOSCOMMON)
EXTRAINCDIRS += $(OPSYSTEMINC)
EXTRAINCDIRS += $(MATHLIBINC)
EXTRAINCDIRS += $(OPUAVOBJINC)
EXTRAINCDIRS += $(OPUAVTALKINC)
EXTRAINCDIRS += $(OPUAVSYNTHDIR)

# Modules
EXTRAINCDIRS += $(foreach mod, $(OPTMODULES) $(MODULES), $(OPMODULEDIR)/$(mod)/inc) $(OPMODULEDIR)/System/inc

# List any extra directories to look for library files here.
# Also add directories where the linker should search for
# includes from linker-script to the list
#     Each directory must be seperated by a space.
EXTRA_LIBDIRS +=

# Extra Libraries
#    Each library-name must be seperated by a space.
#    i.e. to link with libxyz.a, libabc.a and libefsl.a:
#    EXTRA_LIBS = xyz abc efsl
# for newlib-lpc (file: libnewlibc-lpc.a):
#    EXTRA_LIBS = newlib-lpc
EXTRA_LIBS += m

# Compiler flags
CFLAGS +=

# Set linker-script name depending on selected submodel name
ifeq ($(MCU),cortex-m3)
    LDFLAGS += -T$(LINKER_SCRIPTS_PATH)/link_$(BOARD)_memory.ld
    LDFLAGS += -T$(LINKER_SCRIPTS_PATH)/link_$(BOARD)_sections.ld
else ifeq ($(MCU),cortex-m4)
    LDFLAGS += $(addprefix -T,$(LINKER_SCRIPTS_APP))
endif

# Add jtag targets (program and wipe)
$(eval $(call JTAG_TEMPLATE,$(OUTDIR)/$(TARGET).bin,$(FW_BANK_BASE),$(FW_BANK_SIZE),$(OPENOCD_JTAG_CONFIG),$(OPENOCD_CONFIG)))
