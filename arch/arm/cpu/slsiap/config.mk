
#
#	u-boot.lds path
#
CDIR=arch/arm/cpu$(if $(CPU),/$(CPU),)/core
SDIR=arch/arm/cpu$(if $(CPU),/$(CPU),)$(if $(SOC),/$(SOC),)
LDPPFLAGS += -DSDIR=$(SDIR) -DCDIR=$(CDIR)

#
# avoid build error complict with multiple defined rasie,,,, in eabi_compat.c
# when export CROSS_COMPILE	(arch/arm/config.mk)
#
PLATFORM_LIBS :=

# Cross compiler
ifneq ($(CONFIG_ARM64), y)
CROSS_COMPILE := arm-linux-
else
CROSS_COMPILE := aarch64-linux-gnu-
endif

GCCMACHINE =  $(shell $(CC) -dumpmachine | cut -f1 -d-)
GCCVERSION =  $(shell $(CC) -dumpversion | cut -f2 -d.)

ifeq "$(GCCMACHINE)" "arm"
ifneq "$(GCCVERSION)" "3"
PLATFORM_RELFLAGS += -Wno-unused-but-set-variable
endif
ifeq "$(GCCVERSION)" "7"
PLATFORM_RELFLAGS += -mno-unaligned-access
endif
ifeq "$(GCCVERSION)" "8"
PLATFORM_RELFLAGS += -mno-unaligned-access
endif
endif

PLATFORM_RELFLAGS += $(call cc-ifversion, -ge, 0409, -mno-unaligned-access)

ifeq ($(CONFIG_ARM64), y)
PLATFORM_RELFLAGS += -mstrict-align
PLATFORM_RELFLAGS += -Wint-to-pointer-cast
endif

#
# option "-pie" when ld, -pie must be used with -fPIC gcc option
# set use relocate code
#
BUILD_PIC_OPTION := n
ifeq ($(BUILD_PIC_OPTION),y)
PLATFORM_RELFLAGS += -fPIC
else
#when under u-boot-2013.x
#LDFLAGS_u-boot :=
endif
