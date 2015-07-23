include $(srctree)/config.mk
#include config.mk

#obj-y += include/
#obj-y += ftl/
#obj-y += nfc/


$(shell cp $(srctree)/arch/arm/cpu/$(CPU)/devices/mio/media/include/include.o.prebuilt \
	$(srctree)/arch/arm/cpu/$(CPU)/devices/mio/media/include/include.o)
$(shell cp $(srctree)/arch/arm/cpu/$(CPU)/devices/mio/media/ftl/ftl.o.prebuilt \
	$(srctree)/arch/arm/cpu/$(CPU)/devices/mio/media/ftl/ftl.o)
$(shell cp $(srctree)/arch/arm/cpu/$(CPU)/devices/mio/media/nfc/nfc.sche.o.prebuilt \
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
