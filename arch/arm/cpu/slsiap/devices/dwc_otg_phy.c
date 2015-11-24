/*
 *  Copyright (C) 2013 NEXELL SOC Lab.
 *  BongKwan Kook <kook@nexell.co.kr>
 *
 * Configuation settings for the Nexell board.
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

#include <common.h>
#include <asm/io.h>

#include <platform.h>
#include <clk.h>
#include <nx_tieoff.h>
#include "dwc_otg_phy.h"

#define SOC_PA_RSTCON		PHY_BASEADDR_RSTCON
#define	SOC_VA_RSTCON		IO_ADDRESS(SOC_PA_RSTCON)

#define SOC_PA_TIEOFF		PHY_BASEADDR_TIEOFF
#define	SOC_VA_TIEOFF		IO_ADDRESS(SOC_PA_TIEOFF)

void otg_clk_enable(void)
{
#if 0
    struct clk *hsotg_clk;
    hsotg_clk = clk_get(NULL, DEV_NAME_USB2HOST);
    clk_enable(hsotg_clk);
#endif
}

void otg_clk_disable(void)
{
#if 0
    struct clk *hsotg_clk;
    hsotg_clk = clk_get(NULL, DEV_NAME_USB2HOST);
    clk_disable(hsotg_clk);
#endif
}

void otg_phy_init(void)
{
    ulong addr;

    // 1. Release otg common reset
    addr = (SOC_VA_RSTCON + 0x04);
    writel(readl(addr) & ~(1<<25), addr);       // reset on
    udelay(10);
    writel(readl(addr) |  (1<<25), addr);       // reset off
    udelay(10);

    // 2. Program scale mode to real mode
    addr = (SOC_VA_TIEOFF + 0x30);
    writel(readl(addr) & ~(3<<0), addr);

    // 3. Select word interface and enable word interface selection
    addr = (SOC_VA_TIEOFF + 0x38);
    writel(readl(addr) |  (3<<8), addr);        // 2'b01 8bit, 2'b11 16bit word

    // 4. Select VBUS
    addr = (SOC_VA_TIEOFF + 0x34);
//    writel(readl(addr) |  (3<<24), addr);   /* Select VBUS 3.3V */
    writel(readl(addr) & ~(3<<24), addr);   /* Select VBUS 5V */
    udelay(10);

    // 5. POR of PHY
    writel(readl(addr) |  (3<<7), addr);
    udelay(10);
    writel(readl(addr) & ~(2<<7), addr);
    udelay(40); // 40us delay need.

    // 6. UTMI reset
    writel(readl(addr) |  (1<<3), addr);
    udelay(10); // 10 clock need

    // 7. AHB reset
    writel(readl(addr) |  (1<<2), addr);
    udelay(10); // 10 clock need
}

void otg_phy_off(void)
{
    ulong addr;

    addr = (SOC_VA_TIEOFF + 0x34);

    // 0. Select VBUS
    writel(readl(addr) |  (3<<24), addr);   /* Select VBUS 3.3V */
//    writel(readl(addr) & ~(3<<24), addr);   /* Select VBUS 5V */
    udelay(10);
    // 1. UTMI reset
    writel(readl(addr) & ~(1<<3), addr);
    udelay(10); // 10 clock need

    // 2. AHB reset
    writel(readl(addr) & ~(1<<2), addr);
    udelay(10); // 10 clock need

    // 3. POR of PHY
    writel(readl(addr) |  (3<<7), addr);
    writel(readl(addr) & ~(1<<7), addr);
    udelay(10); // 10 clock need

    // 4. Release otg common reset
    addr = (SOC_VA_RSTCON + 0x04);
    writel(readl(addr) & ~(1<<25), addr);     // reset on
    udelay(10);
}

