
obj-y += $(PROTOTYPE)/$(BASEDIR)/nx_bit_accessor.o

obj-y += $(PROTOTYPE)/$(MODULES)/nx_rstcon.o
obj-y += $(PROTOTYPE)/$(MODULES)/nx_clkgen.o
obj-y += $(PROTOTYPE)/$(MODULES)/nx_clkpwr.o
obj-y += $(PROTOTYPE)/$(MODULES)/nx_mcus.o
obj-y += $(PROTOTYPE)/$(MODULES)/nx_alive.o
obj-y += $(PROTOTYPE)/$(MODULES)/nx_gpio.o
obj-y += $(PROTOTYPE)/$(MODULES)/nx_pwm.o

obj-y += $(PROTOTYPE)/$(MODULES)/nx_usb20host.o
obj-y += $(PROTOTYPE)/$(MODULES)/nx_usb20otg.o
obj-y += $(PROTOTYPE)/$(MODULES)/nx_tieoff.o

obj-$(CONFIG_CMD_MMC)				+= $(PROTOTYPE)/$(MODULES)/nx_sdmmc.o
obj-$(CONFIG_SPI)					+= $(PROTOTYPE)/$(MODULES)/nx_ssp.o

obj-$(CONFIG_DISPLAY_OUT)			+= 	$(PROTOTYPE)/$(MODULES)/nx_displaytop.o		\
								   		$(PROTOTYPE)/$(MODULES)/nx_disptop_clkgen.o	\
								   		$(PROTOTYPE)/$(MODULES)/nx_dualdisplay.o		\
								   		$(PROTOTYPE)/$(MODULES)/nx_mlc.o				\
								   		$(PROTOTYPE)/$(MODULES)/nx_dpc.o

obj-$(CONFIG_DISPLAY_OUT_LVDS)	+= $(PROTOTYPE)/$(MODULES)/nx_lvds.o
obj-$(CONFIG_DISPLAY_OUT_MIPI)	+= $(PROTOTYPE)/$(MODULES)/nx_mipi.o
obj-$(CONFIG_DISPLAY_OUT_RESCONV)	+= $(PROTOTYPE)/$(MODULES)/nx_resconv.o

obj-$(CONFIG_DISPLAY_OUT_HDMI)	+= $(PROTOTYPE)/$(MODULES)/nx_hdmi.o
obj-$(CONFIG_DISPLAY_OUT_HDMI)	+= $(PROTOTYPE)/$(MODULES)/nx_ecid.o
obj-$(CONFIG_DISPLAY_OUT_HDMI)	+= $(PROTOTYPE)/$(MODULES)/nx_i2s.o
# obj-$(CONFIG_DISPLAY_OUT_HDMI)	+= 	$(PROTOTYPE)/$(MODULES)/nx_displaytop.o		\
# 								   		$(PROTOTYPE)/$(MODULES)/nx_disptop_clkgen.o	\
# 								   		$(PROTOTYPE)/$(MODULES)/nx_dualdisplay.o		\
# 								   		$(PROTOTYPE)/$(MODULES)/nx_mlc.o				\
# 								   		$(PROTOTYPE)/$(MODULES)/nx_dpc.o
obj-y	+= $(PROTOTYPE)/$(MODULES)/nx_rtc.o
