/*
 * SAMSUNG EXYNOS USB HOST EHCI Controller
 *
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 *	Vivek Gautam <gautam.vivek@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#include <common.h>
#include <fdtdec.h>
#include <libfdt.h>
#include <malloc.h>
#include <usb.h>
#include <asm-generic/errno.h>
#include <linux/compat.h>
#include "ehci.h"
#include <asm/io.h>

#include <nx_chip.h>
#include <nx_usb20host.h>
#include <nx_tieoff.h>
#include <nx_rstcon.h>
#include <nx_clkgen.h>
#include <nx_gpio.h>

#undef	NX_CONSOLE_Printf
//#define	NX_CONSOLE_Printf
#ifdef NX_CONSOLE_Printf
	#define	NX_CONSOLE_Printf(fmt,args...)	printf (fmt ,##args)
#else
	#define NX_CONSOLE_Printf(fmt,args...)
#endif

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

/*
 * EHCI-initialization
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */

int ehci_hcd_init(int index, enum usb_init_type init,
		struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	//reset
	//1. ohci
	writel(readl(0xc0011014) & ~(1<<18), 0xc0011014);
	//2. auxwell
	writel(readl(0xc0011014) & ~(1<<19), 0xc0011014);
	//3. ahb
	writel(readl(0xc0011014) & ~(1<<17), 0xc0011014);
	//4. hsic utmi
	writel(readl(0xc0011014) & ~(1<<22), 0xc0011014);
	//5. ehci utmi
	writel(readl(0xc0011014) & ~(1<<21), 0xc0011014);
	//6. free
	writel(readl(0xc0011014) & ~(1<<20), 0xc0011014);
	//7. hsic por
	writel(readl(0xc0011028) |  (3<<18), 0xc0011028);
	//8. echi por
	writel(readl(0xc0011020) |  (3<<7 ), 0xc0011020);
	//9. resetcon
	writel(readl(0xc0012004) & ~(1<<24), 0xc0012004);			// reset on

#if defined( CONFIG_USB_HSIC_MODE )
	// GPIO Reset
	NX_GPIO_SetPadFunction( 4, 22, 0 );
	NX_GPIO_SetOutputEnable( 4, 22, CTRUE );
#if defined( CONFIG_MACH_S5P4418 )
	NX_GPIO_SetPullSelect( 4, 22, CTRUE );
	NX_GPIO_SetPullEnable( 4, 22, CFALSE );
#else
	NX_GPIO_SetPullEnable( 4, 22, NX_GPIO_PULL_UP );
#endif
	NX_GPIO_SetOutputValue( 4, 22, CTRUE );
	udelay( 100 );
	NX_GPIO_SetOutputValue( 4, 22, CFALSE );
	udelay( 100 );
	NX_GPIO_SetOutputValue( 4, 22, CTRUE );
#endif

	// Release common reset of host controller
	writel(readl(0xc0012004) |  (1<<24), 0xc0012004);			// reset off
	NX_CONSOLE_Printf( "\nRSTCON[24](0xc0012004) set 1'b1 0x%08x\n", readl( 0xc0012004 ) );

#if defined( CONFIG_USB_HSIC_MODE )
	// HSIC 12M rerference Clock setting
	writel( 0x02, 0xc006b000 );
	writel( 0x0C, 0xc006b004 );	// 8 : ok, c : no
	writel( 0x10, 0xc006b00c );
	writel( 0x30, 0xc006b00c );
	writel( 0x06, 0xc006b000 );

	// HSIC 480M clock setting
	writel(readl(0xc0011014) & ~(3<<23), 0xc0011014);
	writel(readl(0xc0011014) |  (2<<23), 0xc0011014);

	// 2. HSIC Enable in PORT1 of LINK
	writel(readl(0xc0011014) & ~(7<<14), 0xc0011014);
	writel(readl(0xc0011014) |  (2<<14), 0xc0011014);
#endif

	// Program AHB Burst type
	writel(readl(0xc001101c) & ~(7<<25), 0xc001101c);
	writel(readl(0xc001101c) |  (7<<25), 0xc001101c);
	NX_CONSOLE_Printf( "TIEOFFREG7[27:25](0xc001101c) set 3'b111 0x%08x\n", readl( 0xc001101c ) );

	// Select word interface and enable word interface selection in link
	writel(readl(0xc0011014) & ~(3<<25), 0xc0011014);
	writel(readl(0xc0011014) |  (3<<25), 0xc0011014);			// 2'b01 8bit, 2'b11 16bit word
	NX_CONSOLE_Printf( "TIEOFFREG5[26:25](0xc0011014) set 2'b11 0x%08x\n", readl( 0xc0011014 ) );

	// Select word interface and enable word interface selection in link in EHCI PHY
	writel(readl(0xc0011024) & ~(3<<8), 0xc0011024);
	writel(readl(0xc0011024) |  (3<<8), 0xc0011024);				// 2'b01 8bit, 2'b11 16bit word
	NX_CONSOLE_Printf( "TIEOFFREG9[10:9](0xc0011024) set 2'b11 0x%08x\n", readl( 0xc0011024 ) );

	// POR of EHCI PHY
	writel(readl(0xc0011020) & ~(3<<7), 0xc0011020);
	writel(readl(0xc0011020) |  (1<<7), 0xc0011020);
	NX_CONSOLE_Printf( "TIEOFFREG8[8:7](0xc0011020) set 2'b01 0x%08x\n", readl( 0xc0011020 ) );

	// Wait clock of PHY - about 40 micro seconds
	udelay(40); // 40us delay need.

#if defined( CONFIG_USB_HSIC_MODE )
	NX_CONSOLE_Printf( "\n HSIC PHY Init!!!\n" );

	udelay(10); // 10us delay need.

	// word interface in HSIC PHY
	writel(readl(0xc001102c) & ~(3<<12), 0xc001102c);
	writel(readl(0xc001102c) |  (3<<12), 0xc001102c);				// 2'b01 8bit, 2'b11 16bit word
//	NX_CONSOLE_Printf( "TIEOFFREG9[10:9](0xc001102c) set 2'b11 0x%08x\n", readl( 0xc001102c ) );

	// POR of HSIC PHY
	writel(readl(0xc0011028) & ~(3<<18), 0xc0011028);
	writel(readl(0xc0011028) |  (1<<18), 0xc0011028);
//	NX_CONSOLE_Printf( "TIEOFFREG8[19:18](0xc0011028) set 2'b01 0x%08x\n", readl( 0xc0011028 ) );

	// 5. Wait clock of PHY - about 40 micro seconds
	//udelay(40); // 40us delay need.
	udelay(400); // 40us delay need.
#endif

	// Release phy free clock reset
	writel(readl(0xc0011014) |  (1<<20), 0xc0011014);
	udelay(2);

#if defined( CONFIG_USB_EHCI_MODE )
	// Release ehci utmi reset
	writel(readl(0xc0011014) |  (1<<21), 0xc0011014);
	udelay(2);
	NX_CONSOLE_Printf( "TIEOFFREG5[21:20](0xc0011014) set 2'b11 0x%08x\n", readl( 0xc0011014 ) );
#endif

#if defined( CONFIG_USB_HSIC_MODE )
	// Release hsic utmi reset
	writel(readl(0xc0011014) |  (1<<22), 0xc0011014);
	udelay(2);
#endif

	//Release ahb reset of EHCI
	writel(readl(0xc0011014) |  (1<<17), 0xc0011014);

	//Release ahb reset of OHCI
	writel(readl(0xc0011014) |  (1<<18), 0xc0011014);

	//Release auxwell reset of EHCI, OHCI
	writel(readl(0xc0011014) |  (1<<19), 0xc0011014);


	// set Base Address
	NX_USB20HOST_SetBaseAddress( 0, (void*)NX_USB20HOST_GetPhysicalAddress(0) );

	*hccr	= ( struct ehci_hccr * )( NX_USB20HOST_GetBaseAddress(0) );
	*hcor	= ( struct ehci_hcor * )( NX_USB20HOST_GetBaseAddress(0) + 0x10 );
//	NX_CONSOLE_Printf( "Set EHCI Base Address : hccr( 0x%08x ), hcor( 0x%08x )\n" , *hccr, *hcor );

	return 0;
}

/*
 * Destroy the appropriate control structures corresponding
 * the EHCI host controller.
 */
int ehci_hcd_stop(int index)
{
	writel(readl(0xc0012004) & ~(1<<24), 0xc0012004);			// reset on

	return 0;
}
