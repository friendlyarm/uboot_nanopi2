#
# (C) Copyright 2009
# jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

PLATFORM_RELFLAGS += -mcpu=cortex-a9

# remove "uses variable-size enums yet the output is to use 32-bit enums..."
PLATFORM_RELFLAGS += -fno-short-enums -fstrict-aliasing

# If armv7-a is not supported by GCC fall-back to armv5, which is
# supported by more tool-chains
PF_CPPFLAGS_ARMV7 := $(call cc-option, -march=armv7-a)
PLATFORM_CPPFLAGS += $(PF_CPPFLAGS_ARMV7)

# =========================================================================
#
# Supply options according to compiler version
#
# =========================================================================
PF_RELFLAGS_SLB_AT := $(call cc-option,-mshort-load-bytes,$(call cc-option,-malignment-traps,))
PLATFORM_RELFLAGS += $(PF_RELFLAGS_SLB_AT)

# SEE README.arm-unaligned-accesses
PF_NO_UNALIGNED := $(call cc-option, -mno-unaligned-access,)
PLATFORM_NO_UNALIGNED := $(PF_NO_UNALIGNED)

# =========================================================================
# Add cpu options
# =========================================================================
PROTOTYPE 	:= prototype
MODULES		:= module
BASEDIR		:= base

ARCH_CFLAGS	+= 	-I$(srctree)/arch/arm/cpu/$(CPU)/$(SOC)/$(PROTOTYPE)/$(MODULES)	\
			 	-I$(srctree)/arch/arm/cpu/$(CPU)/$(SOC)/$(PROTOTYPE)/$(BASEDIR)	\
			 	-I$(srctree)/arch/arm/include/asm/arch-$(SOC)				\
			 	-I$(srctree)/arch/arm/cpu/$(CPU)/common						\
			 	-I$(srctree)/arch/arm/cpu/$(CPU)/devices					\
			 	-I$(srctree)/board/$(VENDOR)/common

ifeq ($(CONFIG_PROTOTYPE_DEBUG),y)
ARCH_CFLAGS += -D__LINUX__ -DNX_DEBUG
else
ARCH_CFLAGS += -D__LINUX__ -DNX_RELEASE
endif

# =========================================================================
#   EWS FTL Build Option
# =========================================================================
ARCH_CFLAGS += -D__SUPPORT_MIO_UBOOT__
ifeq ($(CONFIG_MACH_S5P4418),y)
    ARCH_CFLAGS += -D__SUPPORT_MIO_UBOOT_CHIP_S5P4418__
else
    ifeq ($(CONFIG_MACH_S5P6818),y)
        ARCH_CFLAGS += -D__SUPPORT_MIO_UBOOT_CHIP_S5P6818__
    else
        ARCH_CFLAGS += -D__SUPPORT_MIO_UBOOT_CHIP_NXP4330__
    endif
endif



# =========================================================================
#	Build options for HOSTCC
# =========================================================================
PLATFORM_RELFLAGS  += $(ARCH_CFLAGS)

