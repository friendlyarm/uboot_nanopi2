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

#ifndef __ARCH_API_H__
#define __ARCH_API_H__

#include <config.h>
#include "clk.h"

extern void	nxp_cpu_init(void);
extern void nxp_periph_init(void);
extern void nxp_print_cpuinfo(void);
extern void nxp_preboot_os(void);
extern void nxp_before_linux(void);

extern unsigned int logo_get_logo_bmp_addr(void);
extern void 		lcd_set_logo_bmp_addr(unsigned int bmp_base);
extern void 		lcd_draw_boot_logo(unsigned int framebase, int x_resol, int y_resol, int pixelbyte);

#ifdef CONFIG_SUPPORT_BOARD_INIT_FUNC
extern void nxp_bd_gpio_init(void);
extern void nxp_bd_alive_init(void);
extern void nxp_bd_bus_init(void);
#endif

enum {
 	NXP_CLOCK_PLL0,
	NXP_CLOCK_PLL1,
	NXP_CLOCK_PLL2,
	NXP_CLOCK_PLL3,
	NXP_CLOCK_FCLK,
	NXP_CLOCK_MCLK,
	NXP_CLOCK_BCLK,
	NXP_CLOCK_PCLK,
};

extern void nxp_clk_init(void);
extern void nxp_clk_print(void);
extern struct clk *clk_get(struct device *dev, const char *id);
extern void clk_put(struct clk *clk);
extern unsigned long clk_get_rate(struct clk *clk);
extern long clk_round_rate(struct clk *clk, unsigned long rate);
extern int clk_set_rate(struct clk *clk, unsigned long rate);
extern int clk_enable(struct clk *clk);
extern void clk_disable(struct clk *clk);

extern unsigned int nxp_cpu_version(void);

#endif	/* __ARCH_API_H__ */



