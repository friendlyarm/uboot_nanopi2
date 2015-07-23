include $(srctree)/config.mk

#LIB	:= $(obj)libmio.o

obj-y += mio.uboot.o
obj-y += mio.uboot.rwtest.o
obj-y += media/exchange.o
obj-y += media/nfc/phy/nfc.phy.o
obj-y += media/nfc/phy/nfc.phy.scan.o
obj-y += media/nfc/phy/nfc.phy.lowapi.rawread.o
obj-y += media/nfc/phy/nfc.phy.lowapi.o
obj-y += media/nfc/phy/nfc.phy.readretry.o
obj-y += media/nfc/phy/nfc.phy.rand.o

obj-y += media/

#all:	$(LIB)

#$(LIB):	$(obj).depend $(OBJS)
#	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
#include $(SRCTREE)/rules.mk

#sinclude $(obj).depend
#include $(srctree)/arch/arm/cpu/$(CPU)/$(SOC)/prototype.mk
#include $(srctree)/arch/arm/cpu/$(CPU)/$(SOC)/devices.mk

#########################################################################
