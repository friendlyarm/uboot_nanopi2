
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
