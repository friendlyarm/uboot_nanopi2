# =========================================================================
#   EWS FTL Build Option
# =========================================================================
ARCH_CFLAGS += -D__SUPPORT_MIO_UBOOT__

obj-y	+= cmd.o
obj-$(CONFIG_LOGO_NEXELL)			+= cmd_draw_logo.o cmd_load_logo.o draw_lcd.o
obj-$(CONFIG_CMD_SPI_EEPROM_UPDATE)	+= cmd_spi_eeprom.o
obj-$(CONFIG_CMD_UPDATE_NAND)   	+= cmd_update_nand.o
obj-$(CONFIG_USB_GADGET)			+= usbid.o
obj-$(CONFIG_NXP_USBDOWN)			+= cmd_usbdown.o
obj-$(CONFIG_MMC_PARTITIONS)		+= cmd_mmcparts.o
obj-$(CONFIG_FASTBOOT)				+= cmd_fastboot.o otg_bind_chk.o
obj-$(CONFIG_CMD_MMC_UPDATE)		+= cmd_mmc_update.o
obj-$(CONFIG_CMD_UPDATE_SDCARD)		+= cmd_update_sdcard.o
obj-$(CONFIG_CMD_EXT4) 				+= decompress_ext4.o
obj-$(CONFIG_VIP)					+= camera.o
obj-$(CONFIG_NAND_FTL)				+= cmd_mio.o cmd_nandparts.o
obj-$(CONFIG_SMP) 					+= smp_start.o smp_cpu.o
