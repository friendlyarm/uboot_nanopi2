/*
 * (C) Copyright 2009
 * jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <config.h>
#include <common.h>
#include <mmc.h>
#include <pwm.h>
#include <asm/io.h>
#include <asm/gpio.h>
//#include <asm/sections.h>

#include <platform.h>
#include <mach-api.h>
#include <rtc_nxp.h>
#include <pm.h>

#include <draw_lcd.h>
#include <onewire.h>
#include <nxp-fb.h>

#if defined(CONFIG_PMIC)
#include <power/pmic.h>
#endif

#if defined(CONFIG_PMIC_NXE2000)
#include <nxe2000-private.h>
#endif

#if defined(CONFIG_PMIC_AXP228)
#include <power/axp228.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#include "eth.c"

#if (0)
#define DBGOUT(msg...)		{ printf("BD: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

/*------------------------------------------------------------------------------
 * intialize nexell soc and board status.
 */

static void bd_gpio_init(void)
{
	int index, bit;
	int mode, func, out, lv, plup, stren;
	U32 gpio;

	const U32 pads[NUMBER_OF_GPIO_MODULE][32] = {
	{	/* GPIO_A */
	PAD_GPIOA0 , PAD_GPIOA1 , PAD_GPIOA2 , PAD_GPIOA3 , PAD_GPIOA4 , PAD_GPIOA5 , PAD_GPIOA6 , PAD_GPIOA7 , PAD_GPIOA8 , PAD_GPIOA9 ,
	PAD_GPIOA10, PAD_GPIOA11, PAD_GPIOA12, PAD_GPIOA13, PAD_GPIOA14, PAD_GPIOA15, PAD_GPIOA16, PAD_GPIOA17, PAD_GPIOA18, PAD_GPIOA19,
	PAD_GPIOA20, PAD_GPIOA21, PAD_GPIOA22, PAD_GPIOA23, PAD_GPIOA24, PAD_GPIOA25, PAD_GPIOA26, PAD_GPIOA27, PAD_GPIOA28, PAD_GPIOA29,
	PAD_GPIOA30, PAD_GPIOA31
	}, { /* GPIO_B */
	PAD_GPIOB0 , PAD_GPIOB1 , PAD_GPIOB2 , PAD_GPIOB3 , PAD_GPIOB4 , PAD_GPIOB5 , PAD_GPIOB6 , PAD_GPIOB7 , PAD_GPIOB8 , PAD_GPIOB9 ,
	PAD_GPIOB10, PAD_GPIOB11, PAD_GPIOB12, PAD_GPIOB13, PAD_GPIOB14, PAD_GPIOB15, PAD_GPIOB16, PAD_GPIOB17, PAD_GPIOB18, PAD_GPIOB19,
	PAD_GPIOB20, PAD_GPIOB21, PAD_GPIOB22, PAD_GPIOB23, PAD_GPIOB24, PAD_GPIOB25, PAD_GPIOB26, PAD_GPIOB27, PAD_GPIOB28, PAD_GPIOB29,
	PAD_GPIOB30, PAD_GPIOB31
	}, { /* GPIO_C */
	PAD_GPIOC0 , PAD_GPIOC1 , PAD_GPIOC2 , PAD_GPIOC3 , PAD_GPIOC4 , PAD_GPIOC5 , PAD_GPIOC6 , PAD_GPIOC7 , PAD_GPIOC8 , PAD_GPIOC9 ,
	PAD_GPIOC10, PAD_GPIOC11, PAD_GPIOC12, PAD_GPIOC13, PAD_GPIOC14, PAD_GPIOC15, PAD_GPIOC16, PAD_GPIOC17, PAD_GPIOC18, PAD_GPIOC19,
	PAD_GPIOC20, PAD_GPIOC21, PAD_GPIOC22, PAD_GPIOC23, PAD_GPIOC24, PAD_GPIOC25, PAD_GPIOC26, PAD_GPIOC27, PAD_GPIOC28, PAD_GPIOC29,
	PAD_GPIOC30, PAD_GPIOC31
	}, { /* GPIO_D */
	PAD_GPIOD0 , PAD_GPIOD1 , PAD_GPIOD2 , PAD_GPIOD3 , PAD_GPIOD4 , PAD_GPIOD5 , PAD_GPIOD6 , PAD_GPIOD7 , PAD_GPIOD8 , PAD_GPIOD9 ,
	PAD_GPIOD10, PAD_GPIOD11, PAD_GPIOD12, PAD_GPIOD13, PAD_GPIOD14, PAD_GPIOD15, PAD_GPIOD16, PAD_GPIOD17, PAD_GPIOD18, PAD_GPIOD19,
	PAD_GPIOD20, PAD_GPIOD21, PAD_GPIOD22, PAD_GPIOD23, PAD_GPIOD24, PAD_GPIOD25, PAD_GPIOD26, PAD_GPIOD27, PAD_GPIOD28, PAD_GPIOD29,
	PAD_GPIOD30, PAD_GPIOD31
	}, { /* GPIO_E */
	PAD_GPIOE0 , PAD_GPIOE1 , PAD_GPIOE2 , PAD_GPIOE3 , PAD_GPIOE4 , PAD_GPIOE5 , PAD_GPIOE6 , PAD_GPIOE7 , PAD_GPIOE8 , PAD_GPIOE9 ,
	PAD_GPIOE10, PAD_GPIOE11, PAD_GPIOE12, PAD_GPIOE13, PAD_GPIOE14, PAD_GPIOE15, PAD_GPIOE16, PAD_GPIOE17, PAD_GPIOE18, PAD_GPIOE19,
	PAD_GPIOE20, PAD_GPIOE21, PAD_GPIOE22, PAD_GPIOE23, PAD_GPIOE24, PAD_GPIOE25, PAD_GPIOE26, PAD_GPIOE27, PAD_GPIOE28, PAD_GPIOE29,
	PAD_GPIOE30, PAD_GPIOE31
	},
	};

	/* GPIO pad function */
	for (index = 0; NUMBER_OF_GPIO_MODULE > index; index++) {

		NX_GPIO_ClearInterruptPendingAll(index);

		for (bit = 0; 32 > bit; bit++) {
			gpio  = pads[index][bit];
			func  = PAD_GET_FUNC(gpio);
			mode  = PAD_GET_MODE(gpio);
			lv    = PAD_GET_LEVEL(gpio);
			stren = PAD_GET_STRENGTH(gpio);
			plup  = PAD_GET_PULLUP(gpio);

			/* get pad alternate function (0,1,2,4) */
			switch (func) {
			case PAD_GET_FUNC(PAD_FUNC_ALT0): func = NX_GPIO_PADFUNC_0;	break;
			case PAD_GET_FUNC(PAD_FUNC_ALT1): func = NX_GPIO_PADFUNC_1;	break;
			case PAD_GET_FUNC(PAD_FUNC_ALT2): func = NX_GPIO_PADFUNC_2;	break;
			case PAD_GET_FUNC(PAD_FUNC_ALT3): func = NX_GPIO_PADFUNC_3;	break;
			default: printf("ERROR, unknown alt func (%d.%02d=%d)\n", index, bit, func);
				continue;
			}

			switch (mode) {
			case PAD_GET_MODE(PAD_MODE_ALT): out = 0;
			case PAD_GET_MODE(PAD_MODE_IN ): out = 0;
			case PAD_GET_MODE(PAD_MODE_INT): out = 0; break;
			case PAD_GET_MODE(PAD_MODE_OUT): out = 1; break;
			default: printf("ERROR, unknown io mode (%d.%02d=%d)\n", index, bit, mode);
				continue;
			}

			NX_GPIO_SetPadFunction(index, bit, func);
			NX_GPIO_SetOutputEnable(index, bit, (out ? CTRUE : CFALSE));
			NX_GPIO_SetOutputValue(index, bit,  (lv  ? CTRUE : CFALSE));
			NX_GPIO_SetInterruptMode(index, bit, (lv));

			NX_GPIO_SetPullEnable(index, bit, (NX_GPIO_PULL)plup );
			NX_GPIO_SetDriveStrength(index, bit, (NX_GPIO_DRVSTRENGTH)stren); /* pad strength */
		}
	}
}

static void bd_alive_init(void)
{
	int index, bit;
	int mode, out, lv, plup, detect;
	U32 gpio;

	const U32 pads[] = {
	PAD_GPIOALV0, PAD_GPIOALV1, PAD_GPIOALV2,
	PAD_GPIOALV3, PAD_GPIOALV4, PAD_GPIOALV5
	};

	index = sizeof(pads)/sizeof(pads[0]);

	/* Alive pad function */
	for (bit = 0; index > bit; bit++) {
		NX_ALIVE_ClearInterruptPending(bit);
		gpio = pads[bit];
		mode = PAD_GET_MODE(gpio);
		lv   = PAD_GET_LEVEL(gpio);
		plup = PAD_GET_PULLUP(gpio);

		switch (mode) {
		case PAD_GET_MODE(PAD_MODE_IN ):
		case PAD_GET_MODE(PAD_MODE_INT): out = 0; break;
		case PAD_GET_MODE(PAD_MODE_OUT): out = 1; break;
		case PAD_GET_MODE(PAD_MODE_ALT):
			printf("ERROR, alive.%d not support alt function\n", bit);
			continue;
		default :
			printf("ERROR, unknown alive mode (%d=%d)\n", bit, mode);
			continue;
		}

		NX_ALIVE_SetOutputEnable(bit, (out ? CTRUE : CFALSE));
		NX_ALIVE_SetOutputValue (bit, (lv));
		NX_ALIVE_SetPullUpEnable(bit, (plup & 1 ? CTRUE : CFALSE));
		/* set interrupt mode */
		for (detect = 0; 6 > detect; detect++) {
			if (mode == PAD_GET_MODE(PAD_MODE_INT))
				NX_ALIVE_SetDetectMode(detect, bit, (lv == detect ? CTRUE : CFALSE));
			else
				NX_ALIVE_SetDetectMode(detect, bit, CFALSE);
		}
		NX_ALIVE_SetDetectEnable(bit, (mode == PAD_MODE_INT ? CTRUE : CFALSE));
	}
}

/* DEFAULT channel for SD/eMMC boot */
static int mmc_boot_dev = 2;

int board_mmc_bootdev(void) {
	return mmc_boot_dev;
}

#define SCR_SYSRSTCONFIG	IO_ADDRESS(0xC001023C)
#define	MMC_BOOT_CH0		(0)
#define	MMC_BOOT_CH1		(1 <<  3)
#define	MMC_BOOT_CH2		(1 << 19)

static void bd_bootdev_init(void)
{
	unsigned int rst = readl(SCR_SYSRSTCONFIG);

	rst &= (1 << 19) | (1 << 3);
	if (rst == MMC_BOOT_CH0) {
		/* NanoPi 2 or SD boot for Smart4418 */
		mmc_boot_dev = 0;
	}
}

#define SCR_DREX_BASE		IO_ADDRESS(PHY_BASEADDR_DREX)
#define MEM_CHIP_MSK		0xf0000

static void bd_mem_fixup(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	unsigned int memctl = readl(SCR_DREX_BASE + 0x04);

	if (memctl & MEM_CHIP_MSK) {
		gd->bd->bi_dram[0].size += 0x40000000;
	}
}

static void bd_onewire_init(void)
{
	unsigned char lcd;
	unsigned short fw_ver;

	onewire_init();
	onewire_set_backlight(0);
	onewire_get_info(&lcd, &fw_ver);
}

static void bd_lcd_init(void)
{
	struct nxp_lcd *cfg;
	int myid;
	int width, height;
	int ret;

	myid = onewire_get_lcd_id();
	/* -1: onwire probe failed
	 *  0: bad
	 * >0: identified */

	ret = nanopi2_setup_lcd_by_id(myid);
	if (myid <= 0 || ret != myid) {
		printf("LCD  = N/A (%d)\n", myid);
		nanopi2_setup_lcd_by_name("HDMI720P60");

		/* May be setup to 1080P in bd_update_env() */
		width = 1920;
		height = 1080;
	} else {
		printf("LCD  = %s\n", nanopi2_get_lcd_name());

		cfg = nanopi2_get_lcd();
		width  = cfg->width;
		height = cfg->height;
	}

#if defined(CONFIG_DISPLAY_OUT)
	/* Clear framebuffer */
	memset((void *)CONFIG_FB_ADDR, 0, width * height * 4);
#endif
}

static void bd_update_env(void)
{
	char *lcdtype = getenv("lcdtype");
	char *lcddpi = getenv("lcddpi");
	char *bootargs = getenv("bootargs");
	const char *name;
	char *p = NULL;

#define CMDLINE_LCD		" lcd="
	char cmdline[CONFIG_SYS_CBSIZE];
	int n = 1;

	if (lcdtype) {
		/* Setup again as user specified LCD in env */
		nanopi2_setup_lcd_by_name(lcdtype);
	}

	name = nanopi2_get_lcd_name();

	if (bootargs)
		n = strlen(bootargs);	/* isn't 0 for NULL */
	else
		cmdline[0] = '\0';

	if ((n + strlen(name) + sizeof(CMDLINE_LCD)) > sizeof(cmdline)) {
		printf("Error: `bootargs' is too large (%d)\n", n);
		return;
	}

	if (bootargs) {
		p = strstr(bootargs, CMDLINE_LCD);
		if (p) {
			n = (p - bootargs);
			p += strlen(CMDLINE_LCD);
		}
		strncpy(cmdline, bootargs, n);
	}

	/* add `lcd=NAME,NUMdpi' */
	strncpy(cmdline + n, CMDLINE_LCD, strlen(CMDLINE_LCD));
	n += strlen(CMDLINE_LCD);

	strcpy(cmdline + n, name);
	n += strlen(name);

	if (lcddpi) {
		n += sprintf(cmdline + n, ",%sdpi", lcddpi);
	} else {
		int dpi = nanopi2_get_lcd_density();

		if (dpi > 0 && dpi < 600) {
			n += sprintf(cmdline + n, ",%ddpi", dpi);
		}
	}

	/* copy remaining of bootargs */
	if (p) {
		p = strstr(p, " ");
		if (p) {
			strcpy(cmdline + n, p);
			n += strlen(p);
		}
	}

	/* append `bootdev=2' */
#define CMDLINE_BDEV	" bootdev="
	if (board_mmc_bootdev() == 2 && !strstr(cmdline, CMDLINE_BDEV)) {
		n += sprintf(cmdline + n, "%s2", CMDLINE_BDEV);
	}

	/* finally, let's update uboot env & save it */
	if (!strncmp(name, "HDMI", 4)) {
		char image[32];
		sprintf(image, "%s.hdmi", CONFIG_KERNELIMAGE);

		setenv("kernel", image);
	} else {
		setenv("kernel", CONFIG_KERNELIMAGE);
	}

	if (bootargs && strncmp(cmdline, bootargs, sizeof(cmdline))) {
		setenv("bootargs", cmdline);
		saveenv();
	}
}

/* call from u-boot */
int board_early_init_f(void)
{
	bd_gpio_init();
	bd_alive_init();
#if (defined(CONFIG_PMIC_NXE2000)||defined(CONFIG_PMIC_AXP228))&& !defined(CONFIG_PMIC_REG_DUMP)
	bd_pmic_init();
#endif
#if defined(CONFIG_NXP_RTC_USE)
	nxp_rtc_init();
#endif
	bd_bootdev_init();
	bd_onewire_init();
	return 0;
}

int board_init(void)
{
	bd_lcd_init();
	bd_mem_fixup();

	DBGOUT("%s : done board init ...\n", CFG_SYS_BOARD_NAME);
	return 0;
}

#if defined(CONFIG_PMIC_NXE2000)||defined(CONFIG_PMIC_AXP228)
int power_init_board(void)
{
	int ret = 0;
#if defined(CONFIG_PMIC_REG_DUMP)
	bd_pmic_init();
#endif
	ret = power_pmic_function_init();
	return ret;
}
#endif

extern void	bd_display(void);

static void auto_update(int io, int wait)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	int level = 1, i = 0;
	char *cmd = "fastboot";

	for (i = 0; wait > i; i++) {
		switch (io & ~(32-1)) {
		case PAD_GPIO_A:
		case PAD_GPIO_B:
		case PAD_GPIO_C:
		case PAD_GPIO_D:
		case PAD_GPIO_E:
			level = NX_GPIO_GetInputValue(grp, bit);	break;
		case PAD_GPIO_ALV:
			level = NX_ALIVE_GetInputValue(bit);	break;
		};
		if (level)
			break;
		mdelay(1);
	}

	if (i == wait)
		run_command(cmd, 0);
}

void bd_display_run(char *cmd, int bl_duty, int bl_on)
{
	static int display_init = 0;

	if (!display_init) {
		bd_display();

#if defined(CFG_LCD_PRI_PWM_CH)
		pwm_init(CFG_LCD_PRI_PWM_CH, 0, 0);
		pwm_config(CFG_LCD_PRI_PWM_CH,
			TO_DUTY_NS(bl_duty, CFG_LCD_PRI_PWM_FREQ),
			TO_PERIOD_NS(CFG_LCD_PRI_PWM_FREQ));
#endif

		display_init = 1;
	}

	if (cmd) {
		struct nxp_lcd *lcd = nanopi2_get_lcd();

		run_command(cmd, 0);
		lcd_draw_boot_logo(CONFIG_FB_ADDR, lcd->width, lcd->height,
			CFG_DISP_PRI_SCREEN_PIXEL_BYTE);
	}

	if (bl_on) {
#if defined(CFG_LCD_PRI_PWM_CH)
		pwm_enable(CFG_LCD_PRI_PWM_CH);
#endif
		onewire_set_backlight(127);
	}
}

#ifdef CONFIG_CMD_NET
static int bd_eth_init(void)
{
#if defined(CONFIG_DESIGNWARE_ETH)
	u32 addr;

	// Clock control
	NX_CLKGEN_Initialize();
	addr = NX_CLKGEN_GetPhysicalAddress(CLOCKINDEX_OF_DWC_GMAC_MODULE);
	NX_CLKGEN_SetBaseAddress(CLOCKINDEX_OF_DWC_GMAC_MODULE, (void *)IO_ADDRESS(addr));

	NX_CLKGEN_SetClockSource(CLOCKINDEX_OF_DWC_GMAC_MODULE, 0, 4);		// Sync mode for 100 & 10Base-T : External RX_clk
	NX_CLKGEN_SetClockDivisor(CLOCKINDEX_OF_DWC_GMAC_MODULE, 0, 1);		// Sync mode for 100 & 10Base-T
	NX_CLKGEN_SetClockOutInv(CLOCKINDEX_OF_DWC_GMAC_MODULE, 0, CFALSE);	// TX Clk invert off : 100 & 10Base-T
	NX_CLKGEN_SetClockDivisorEnable(CLOCKINDEX_OF_DWC_GMAC_MODULE, CTRUE);

	// Reset control
	NX_RSTCON_Initialize();
	addr = NX_RSTCON_GetPhysicalAddress();
	NX_RSTCON_SetBaseAddress( (void *)IO_ADDRESS(addr));
	NX_RSTCON_SetRST(RESETINDEX_OF_DWC_GMAC_MODULE_aresetn_i, RSTCON_ASSERT);
	udelay(100);
	NX_RSTCON_SetRST(RESETINDEX_OF_DWC_GMAC_MODULE_aresetn_i, RSTCON_NEGATE);
	udelay(100);

	// Set interrupt config.
	gpio_direction_input(CFG_ETHER_GMAC_PHY_IRQ_NUM);

	// Set GPIO nReset
	gpio_direction_output(CFG_ETHER_GMAC_PHY_RST_NUM, 1);
	udelay(100);
	gpio_set_value(CFG_ETHER_GMAC_PHY_RST_NUM, 0);
	udelay(100);
	gpio_set_value(CFG_ETHER_GMAC_PHY_RST_NUM, 1);
#endif  /* CONFIG_DESIGNWARE_ETH */

	return 0;
}
#endif	/* CONFIG_CMD_NET */


#define	UPDATE_KEY			(PAD_GPIO_ALV + 0)
#define	UPDATE_CHECK_TIME	(3000)	/* ms */

int board_late_init(void)
{
#if defined(CONFIG_SYS_MMC_BOOT_DEV)
	char boot[16];
	sprintf(boot, "mmc dev %d", board_mmc_bootdev());
	run_command(boot, 0);
#endif

	if (board_mmc_bootdev() == 0 && !getenv("firstboot")) {
#ifdef CONFIG_LOADCMD_CH0
		setenv("bloader", CONFIG_LOADCMD_CH0);
#endif
#ifdef CONFIG_BOOTCMD_CH0
		setenv("bootcmd", CONFIG_BOOTCMD_CH0);
#endif
		setenv("firstboot", "0");
		saveenv();
	}

	bd_update_env();

#if defined(CONFIG_RECOVERY_BOOT)
    if (RECOVERY_SIGNATURE == readl(SCR_RESET_SIG_READ)) {
        writel((-1UL), SCR_RESET_SIG_RESET); /* clear */

        printf("RECOVERY BOOT\n");
        bd_display_run(CONFIG_CMD_LOGO_WALLPAPERS, CFG_LCD_PRI_PWM_DUTYCYCLE, 1);
        run_command(CONFIG_CMD_RECOVERY_BOOT, 0);	/* recovery boot */
    }

    writel((-1UL), SCR_RESET_SIG_RESET);
#endif /* CONFIG_RECOVERY_BOOT */

#if defined(CONFIG_BAT_CHECK)
	{
		int ret = 0;
		int bat_check_skip = 0;

	    // psw0523 for cts
	    // bat_check_skip = 1;

#if defined(CONFIG_DISPLAY_OUT)
		ret = power_battery_check(bat_check_skip, bd_display_run);
#else
		ret = power_battery_check(bat_check_skip, NULL);
#endif

		if (ret == 1)
			auto_update(UPDATE_KEY, UPDATE_CHECK_TIME);
	}
#else /* CONFIG_BAT_CHECK */

#if defined(CONFIG_DISPLAY_OUT)
	bd_display_run(CONFIG_CMD_LOGO_WALLPAPERS, CFG_LCD_PRI_PWM_DUTYCYCLE, 1);
#endif
	onewire_set_backlight(127);

#ifdef CONFIG_CMD_NET
	bd_eth_init();
#endif

	/* Temp check gpio to update */
	if (!getenv("autoupdate"))
		auto_update(UPDATE_KEY, UPDATE_CHECK_TIME);

#endif /* CONFIG_BAT_CHECK */

	return 0;
}

