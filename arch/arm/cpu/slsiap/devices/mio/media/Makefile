include $(srctree)/config.mk
#include config.mk

#obj-y += include/
#obj-y += ftl/
#obj-y += nfc/

ifdef CONFIG_ARM64
	PREBUILT_INCLUDE := include.x64.o.prebuilt
	PREBUILT_FTL := ftl.x64.o.prebuilt
	PREBUILT_SCHE := nfc.sche.x64.o.prebuilt
else
	PREBUILT_INCLUDE := include.o.prebuilt
	PREBUILT_FTL := ftl.o.prebuilt
	PREBUILT_SCHE := nfc.sche.o.prebuilt
endif

$(shell cp $(srctree)/arch/arm/cpu/$(CPU)/devices/mio/media/include/$(PREBUILT_INCLUDE) \
	$(srctree)/arch/arm/cpu/$(CPU)/devices/mio/media/include/include.o)
$(shell cp $(srctree)/arch/arm/cpu/$(CPU)/devices/mio/media/ftl/$(PREBUILT_FTL) \
	$(srctree)/arch/arm/cpu/$(CPU)/devices/mio/media/ftl/ftl.o)
$(shell cp $(srctree)/arch/arm/cpu/$(CPU)/devices/mio/media/nfc/$(PREBUILT_SCHE) \
	$(srctree)/arch/arm/cpu/$(CPU)/devices/mio/media/nfc/nfc.sche.o)


obj-y += include/include.o
obj-y += ftl/ftl.o
obj-y += nfc/nfc.sche.o


#include/include.o:
#	$(shell cp include/include.o.prebuilt include/include.o)
#ftl/ftl.o:
#	$(shell cp ftl/ftl.o.prebuilt ftl/ftl.o)
#nfc/nfc.sche.o:
#	$(shell cp nfc/nfc.sche.o.prebuilt nfc/nfc.sche.o)
