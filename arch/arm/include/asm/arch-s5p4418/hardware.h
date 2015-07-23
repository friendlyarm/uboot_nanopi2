#ifndef __HARDWARE_H__
#define __HARDWARE_H__

/*
 * device name
 */
#define DEV_NAME_UART 			"nxp-uart"		// pl0115 (amba-pl011.c)
#define	DEV_NAME_FB				"nxp-fb"
#define	DEV_NAME_DISP			"nxp-disp"
#define	DEV_NAME_LCD			"nxp-lcd"
#define	DEV_NAME_LVDS			"nxp-lvds"
#define	DEV_NAME_HDMI			"nxp-hdmi"
#define	DEV_NAME_RESCONV		"nxp-resconv"
#define	DEV_NAME_MIPI			"nxp-mipi"
#define	DEV_NAME_PCM			"nxp-pcm"
#define	DEV_NAME_I2S			"nxp-i2s"
#define	DEV_NAME_SPDIF_TX		"nxp-spdif-tx"
#define	DEV_NAME_SPDIF_RX		"nxp-spdif-rx"
#define	DEV_NAME_I2C			"nxp-i2c"
#define	DEV_NAME_NAND			"nxp-nand"
#define	DEV_NAME_KEYPAD			"nxp-keypad"
#define	DEV_NAME_SDHC			"nxp-sdhc"
#define	DEV_NAME_PWM			"nxp-pwm"
#define	DEV_NAME_TIMER			"nxp-timer"
#define	DEV_NAME_SOC_PWM		"nxp-soc-pwm"
#define DEV_NAME_GPIO           "nxp-gpio"
#define DEV_NAME_RTC            "nxp-rtc"
#define	DEV_NAME_GMAC			"nxp-gmac"
#define	DEV_NAME_MPEGTSI		"nxp-mpegtsi"
#define	DEV_NAME_MALI			"nxp-mali"
#define	DEV_NAME_DIT			"nxp-deinterlace"
#define	DEV_NAME_PPM			"nxp-ppm"
#define	DEV_NAME_VIP			"nxp-vip"
#define	DEV_NAME_CODA			"nxp-coda"
#define	DEV_NAME_USB2HOST		"nxp-usb2h"
#define	DEV_NAME_CRYPTO			"nxp-crypto"
#define	DEV_NAME_SCALER			"nxp-scaler"
#define	DEV_NAME_PDM			"nxp-pdm"
#define	DEV_NAME_SPI			"nxp-spi"
#define	DEV_NAME_CPUFREQ		"nxp-cpufreq"

/*
 * clock generator
 */
#define CORECLK_NAME_PLL0 		"pll0"	/* cpu clock */
#define CORECLK_NAME_PLL1 		"pll1"
#define CORECLK_NAME_PLL2 		"pll2"
#define CORECLK_NAME_PLL3 		"pll3"
#define CORECLK_NAME_FCLK 		"fclk"
#define CORECLK_NAME_MCLK 		"mclk"
#define CORECLK_NAME_BCLK 		"bclk"
#define CORECLK_NAME_PCLK 		"pclk"
#define CORECLK_NAME_HCLK       "hclk"

#define CORECLK_ID_PLL0 		0
#define CORECLK_ID_PLL1 		1
#define CORECLK_ID_PLL2 		2
#define CORECLK_ID_PLL3 		3
#define CORECLK_ID_FCLK 		4
#define CORECLK_ID_MCLK 		5
#define CORECLK_ID_BCLK 		6
#define CORECLK_ID_PCLK 		7
#define CORECLK_ID_HCLK         8

#endif