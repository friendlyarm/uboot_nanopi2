/*
 * (C) Copyright 2012 SAMSUNG Electronics
 * Jaehoon Chung <jh80.chung@samsung.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,  MA 02111-1307 USA
 *
 */

#include <common.h>
#include <malloc.h>
#include <dwmmc.h>
#include <mach-api.h>
#include "dw_mmc.h"

static char *NXP_NAME = "NXP DWMMC";

#define DWMCI_CLKSEL			0x09C
#define DWMCI_SHIFT_0			0x0
#define DWMCI_SHIFT_1			0x1
#define DWMCI_SHIFT_2			0x2
#define DWMCI_SHIFT_3			0x3
#define DWMCI_SET_SAMPLE_CLK(x)	(x)
#define DWMCI_SET_DRV_CLK(x)	((x) << 16)
#define DWMCI_SET_DIV_RATIO(x)	((x) << 24)

static unsigned int dw_mci_get_clk(struct dwmci_host *host)
{
	struct clk *clk;
	int index = host->dev_index;
	char name[50];

	sprintf(name, "%s.%d", DEV_NAME_SDHC, index);
	clk = clk_get(NULL, name);
	if (!clk)
		return 0;

	return clk_get_rate(clk)/2;
}

static unsigned long dw_mci_set_clk(int dev_index, unsigned  rate)
{
	struct clk *clk;
	char name[50];

	sprintf(name, "%s.%d", DEV_NAME_SDHC, dev_index);
	clk = clk_get(NULL, name);
	if (!clk)
		return 0;

	rate = clk_set_rate(clk, rate);
	clk_enable(clk);

	return rate;
}

static void dw_mci_clksel(struct dwmci_host *host)
{
	u32 val;

	val = DWMCI_SET_SAMPLE_CLK(DWMCI_SHIFT_0) |
		DWMCI_SET_DRV_CLK(DWMCI_SHIFT_0) | DWMCI_SET_DIV_RATIO(3);

	dwmci_writel(host, DWMCI_CLKSEL, val);
}

static void dw_mci_clk_delay(u32 val, unsigned long regbase)
{
	writel(val, (regbase + DWMCI_CLKCTRL));
}

static void dw_mci_reset(int ch)
{
	int rst_id = RESET_ID_SDMMC0 + ch;

	NX_RSTCON_SetRST(rst_id, 0);
	NX_RSTCON_SetRST(rst_id, 1);
}

struct dwmci_host *host = NULL;
static int dw_mci_init(unsigned long regbase, int bus_width, int index, int max_clock, int ddr)
{
	int  fifo_size = 0x20;

	host = malloc(sizeof(struct dwmci_host));
	if (!host) {
		printf("dwmci_host malloc fail!\n");
		return 1;
	}
	memset(host, 0x00, sizeof(*host));

	dw_mci_set_clk(index, max_clock * 4);

	host->name = NXP_NAME;
	host->ioaddr = (void *)regbase;
	host->buswidth = bus_width;
	host->clksel = dw_mci_clksel;
	host->dev_index = index;
	host->get_mmc_clk = dw_mci_get_clk;
	host->fifoth_val = MSIZE(0x2) | RX_WMARK(fifo_size/2 -1) | TX_WMARK(fifo_size/2);

	if(ddr == 1)
		host->caps |= MMC_MODE_DDR_52MHz;// | MMC_MODE_4BIT | MMC_MODE_HS_52MHz ;

	add_dwmci(host, max_clock, 400000);

#ifdef CONFIG_MACH_S5P6818
	if(bus_width == 8)
		NX_TIEOFF_Set(TIEOFFINDEX_OF_MMC_8BIT , 1 );
#endif
	dw_mci_reset(index);
	return 0;
}

int board_mmc_init(bd_t *bis)
{
	int err = 0;
	int bus = 0, speed = 0,ddr = 0;

	#if(CONFIG_MMC0_ATTACH == TRUE)
	writel(readl(0xC0012004) | (1<<7), 0xC0012004);
	#endif

	#if(CONFIG_MMC0_CLOCK)
	speed = CONFIG_MMC0_CLOCK;
	#else
	speed = 52000000;
	#endif

	#if(CONFIG_MMC0_BUS_WIDTH)
	bus = CONFIG_MMC0_BUS_WIDTH;
	#else
	bus = 4;
	#endif

	#if(CONFIG_MMC0_TRANS_MODE)
	ddr = CONFIG_MMC0_TRANS_MODE;
	#else
	ddr = 0;
	#endif
	err = dw_mci_init(0xC0062000, bus, 0, speed, ddr);
	#ifdef CONFIG_MMC0_CLK_DELAY
	dw_mci_clk_delay(CONFIG_MMC0_CLK_DELAY, 0xC0062000);
	#endif

	#if(CONFIG_MMC1_CLOCK)
	speed = CONFIG_MMC1_CLOCK;
	#else
	speed = 52000000;
	#endif

	#if(CONFIG_MMC1_BUS_WIDTH)
	bus = CONFIG_MMC1_BUS_WIDTH;
	#else
	bus = 4;
	#endif
	#if(CONFIG_MMC1_TRANS_MODE)
	ddr = CONFIG_MMC1_TRANS_MODE;
	#else
	ddr = 0;
	#endif
	err = dw_mci_init(0xC0068000, bus, 1, speed, ddr);
	#ifdef CONFIG_MMC1_CLK_DELAY
	dw_mci_clk_delay(CONFIG_MMC1_CLK_DELAY, 0xC0068000);
	#endif

	#if(CONFIG_MMC2_ATTACH == TRUE)
	writel(readl(0xC0012004) | (1<<9), 0xC0012004);
	#endif

	#if(CONFIG_MMC2_CLOCK)
	speed = CONFIG_MMC2_CLOCK;
	#else
	speed = 52000000;
	#endif

	#if(CONFIG_MMC2_BUS_WIDTH)
	bus = CONFIG_MMC2_BUS_WIDTH;
	#else
	bus = 4;
	#endif

	#if(CONFIG_MMC2_TRANS_MODE)
	ddr = CONFIG_MMC2_TRANS_MODE;
	#else
	ddr = 0;
	#endif
	err = dw_mci_init(0xC0069000, bus, 2, speed, ddr);

	#ifdef CONFIG_MMC2_CLK_DELAY
	dw_mci_clk_delay(CONFIG_MMC2_CLK_DELAY, 0xC0069000);
	#endif

	return err;
}

