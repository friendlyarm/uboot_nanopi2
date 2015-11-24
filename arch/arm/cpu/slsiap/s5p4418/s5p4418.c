/*
 * (C) Copyright 2009 Nexell Co.,
 * jung hyun kim<jhkim@nexell.co.kr>
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
#include <mach-api.h>
#include <pm.h>

//#define DEBUG_BUS_CONF

#define DBGOUT(msg...)		do { printf("sys: " msg); } while (0)
#define printk(msg...)		do { printf(msg); } while (0)

#if (CFG_BUS_RECONFIG_ENB == 1)
#include <s5p4418_bus.h>
void nxp_set_bus_config(void)
{
    volatile NX_DREX_REG *pdrex = (volatile NX_DREX_REG *)NX_VA_BASE_REG_DREX;
	u32 val;
	u32 num_si, num_mi;
	u32 i_slot, temp;
#if ((CFG_DREX_PORT0_QOS_ENB == 1) || (CFG_DREX_PORT1_QOS_ENB == 1))
	u32 drex_qos_bits = 0;
#endif
	u16 __g_DrexQoS[2] = { g_DrexQoS[0], g_DrexQoS[1] };

#if (CFG_DREX_PORT0_QOS_ENB == 1)
	drex_qos_bits  |= (1<<4) | (1<<0);
#endif
#if (CFG_DREX_PORT1_QOS_ENB == 1)
	drex_qos_bits  |= (1<<12) | (1<<8);
#endif

#if 0
	writel( 0xFFF1FFF1,     &pdrex->BRBRSVCONFIG );
#else

	temp    = ( 0xFF00FF00
			| ((g_DrexBRB_WR[1] & 0xF) <<   20)
			| ((g_DrexBRB_WR[0] & 0xF) <<   16)
			| ((g_DrexBRB_RD[1] & 0xF) <<    4)
			| ((g_DrexBRB_RD[0] & 0xF) <<    0));
	writel( temp,           &pdrex->BRBRSVCONFIG );
#endif
	writel( 0x00000033,     &pdrex->BRBRSVCONTROL );
#ifdef DEBUG_BUS_CONF
	printk("  ... BRBRSVCONFIG [%x], BRBRSVCONTROL [%x]\n", (pdrex->BRBRSVCONFIG), (pdrex->BRBRSVCONTROL));
#endif

#if ((CFG_DREX_PORT0_QOS_ENB == 1) || (CFG_DREX_PORT1_QOS_ENB == 1))
#if (CFG_DREX_PORT0_QOS_ENB == 1)
	__g_DrexQoS[0] = (u16)0x0000;
#endif
#if (CFG_DREX_PORT1_QOS_ENB == 1)
	__g_DrexQoS[1] = (u16)0x0000;
#endif
	writel( drex_qos_bits,  NX_VA_BASE_REG_TIEOFF + NX_TIEOFF_DREX_SLAVE_OFFSET );
#ifdef DEBUG_BUS_CONF
	printk("  ... TIEOFF DREX SLAVE [%x]\n", *(U32 *)(NX_VA_BASE_REG_TIEOFF + NX_TIEOFF_DREX_SLAVE_OFFSET));
#endif
#endif

	/* ------------- DREX QoS -------------- */
#if 1   //(CFG_BUS_RECONFIG_DREXQOS == 1)
	for (i_slot = 0; i_slot < 2; i_slot++)
	{
		val = readl(NX_VA_BASE_REG_DREX + NX_DREX_QOS_OFFSET + (i_slot<<3));
		if (val != __g_DrexQoS[i_slot])
			writel( __g_DrexQoS[i_slot], (NX_VA_BASE_REG_DREX + NX_DREX_QOS_OFFSET + (i_slot<<3)) );
	}
#ifdef DEBUG_BUS_CONF
	for (i_slot = 0; i_slot < 2; i_slot++)
	{
		val = readl(NX_VA_BASE_REG_DREX + NX_DREX_QOS_OFFSET + (i_slot<<3));
		printk("  ... DREX QoS slot: [%d], val: [%x]\n", i_slot, val);
	}
#endif
#endif /* (CFG_BUS_RECONFIG_DREXQOS == 1) */

	/* ------------- Bottom BUS ------------ */
	/* MI1 - Set SI QoS */
#if (CFG_BUS_RECONFIG_BOTTOMBUSQOS == 1)
	val = readl(NX_BASE_REG_PL301_BOTT_QOS_TRDMARK + 0x20);
	if (val != g_BottomQoSSI[0])
		writel(g_BottomQoSSI[0], (NX_BASE_REG_PL301_BOTT_QOS_TRDMARK + 0x20) );

	val = readl(NX_BASE_REG_PL301_BOTT_QOS_CTRL + 0x20);
	if (val != g_BottomQoSSI[1])
		writel(g_BottomQoSSI[1], (NX_BASE_REG_PL301_BOTT_QOS_CTRL + 0x20) );
#ifdef DEBUG_BUS_CONF
	val = readl(NX_BASE_REG_PL301_BOTT_QOS_TRDMARK + 0x20);
	printk("  ... BOTT QOS TRDMARK : %x\n", val);
	val = readl(NX_BASE_REG_PL301_BOTT_QOS_CTRL + 0x20);
	printk("  ... BOTT QOS CTRL : %x\n", val);
#endif
#endif

#if (CFG_BUS_RECONFIG_BOTTOMBUSSI == 1)
	num_si = readl(NX_VA_BASE_REG_PL301_BOTT + 0xFC0);
	num_mi = readl(NX_VA_BASE_REG_PL301_BOTT + 0xFC4);

	/* Set progamming for AR */
	// MI0 - Slave Interface
	for (i_slot = 0; i_slot < num_mi; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  NX_BASE_REG_PL301_BOTT_AR );
		val = readl(NX_BASE_REG_PL301_BOTT_AR);
		if (val != i_slot)
			writel( (i_slot << SLOT_NUM_POS) | (i_slot << SI_IF_NUM_POS),  NX_BASE_REG_PL301_BOTT_AR );
	}
#ifdef DEBUG_BUS_CONF
	for (i_slot = 0; i_slot < num_mi; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  NX_BASE_REG_PL301_BOTT_AR );
		val = readl(NX_BASE_REG_PL301_BOTT_AR);
		printk("  ... MI0 BOTT AR slot: [%d], val: [%x]\n", i_slot, val);
	}
#endif

	// MI1 - Slave Interface
	for (i_slot = 0; i_slot < num_si; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  (NX_BASE_REG_PL301_BOTT_AR + 0x20) );
		val = readl(NX_BASE_REG_PL301_BOTT_AR + 0x20);
		if (val != g_BottomBusSI[i_slot])
			writel( (i_slot << SLOT_NUM_POS) | (g_BottomBusSI[i_slot] << SI_IF_NUM_POS),  (NX_BASE_REG_PL301_BOTT_AR + 0x20) );
	}
#ifdef DEBUG_BUS_CONF
	for (i_slot = 0; i_slot < num_mi; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  (NX_BASE_REG_PL301_BOTT_AR + 0x20) );
		val = readl(NX_BASE_REG_PL301_BOTT_AR + 0x20);
		printk("  ... MI1 BOTT AR slot: [%d], val: [%x]\n", i_slot, val);
	}
#endif

	/* Set progamming for AW */
	// MI0 - Slave Interface
	for (i_slot = 0; i_slot < num_mi; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  NX_BASE_REG_PL301_BOTT_AW );
		val = readl(NX_BASE_REG_PL301_BOTT_AW);
		if (val != i_slot)
			writel( (i_slot << SLOT_NUM_POS) | (i_slot << SI_IF_NUM_POS),  NX_BASE_REG_PL301_BOTT_AW );
	}
#ifdef DEBUG_BUS_CONF
	for (i_slot = 0; i_slot < num_mi; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  (NX_BASE_REG_PL301_BOTT_AW) );
		val = readl(NX_BASE_REG_PL301_BOTT_AW);
		printk("  ... MI0 BOTT AW slot: [%d], val: [%x]\n", i_slot, val);
	}
#endif

	// MI1 - Slave Interface
	for (i_slot = 0; i_slot < num_si; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  (NX_BASE_REG_PL301_BOTT_AW + 0x20) );
		val = readl(NX_BASE_REG_PL301_BOTT_AW + 0x20);
		if (val != g_BottomBusSI[i_slot])
			writel( (i_slot << SLOT_NUM_POS) | (g_BottomBusSI[i_slot] << SI_IF_NUM_POS),  (NX_BASE_REG_PL301_BOTT_AW + 0x20) );
	}
#ifdef DEBUG_BUS_CONF
	for (i_slot = 0; i_slot < num_mi; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  (NX_BASE_REG_PL301_BOTT_AW + 0x20) );
		val = readl(NX_BASE_REG_PL301_BOTT_AW + 0x20);
		printk("  ... MI1 BOTT AW slot: [%d], val: [%x]\n", i_slot, val);
	}
#endif
#endif /* (CFG_BUS_RECONFIG_BOTTOMBUSSI == 1) */

	/* ------------- Top BUS ------------ */
#if (CFG_BUS_RECONFIG_TOPBUSQOS == 1)
	/* MI0 - Set SI QoS */
	val = readl(NX_BASE_REG_PL301_TOP_QOS_TRDMARK);
	if (val != g_TopQoSSI[0])
		writel(g_TopQoSSI[0], NX_BASE_REG_PL301_TOP_QOS_TRDMARK);
#ifdef DEBUG_BUS_CONF
	val = readl(NX_BASE_REG_PL301_TOP_QOS_TRDMARK);
	printk(" ... TOP QOS TRDMARK : %x\n", val);
#endif

	val = readl(NX_BASE_REG_PL301_TOP_QOS_CTRL);
	if (val != g_TopQoSSI[1])
		writel(g_TopQoSSI[1], NX_BASE_REG_PL301_TOP_QOS_CTRL);
#ifdef DEBUG_BUS_CONF
	val = readl(NX_BASE_REG_PL301_TOP_QOS_CTRL);
	printk(" ... TOP QOS CTRL: %x\n", val);
#endif
#endif

#if (CFG_BUS_RECONFIG_TOPBUSSI == 1)
	num_si = readl(NX_VA_BASE_REG_PL301_TOP + 0xFC0);
	num_mi = readl(NX_VA_BASE_REG_PL301_TOP + 0xFC4);

	/* Set progamming for AR */
	// MI0 - Slave Interface
	for (i_slot = 0; i_slot < num_mi; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  NX_BASE_REG_PL301_TOP_AR );
		val = readl(NX_BASE_REG_PL301_TOP_AR);
		if (val != g_TopBusSI[i_slot])
			writel( (i_slot << SLOT_NUM_POS) | (g_TopBusSI[i_slot] << SI_IF_NUM_POS),  NX_BASE_REG_PL301_TOP_AR );
	}
#ifdef DEBUG_BUS_CONF
	for (i_slot = 0; i_slot < num_mi; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  NX_BASE_REG_PL301_TOP_AR );
		val = readl(NX_BASE_REG_PL301_TOP_AR);
		printk("  ... TOP AR slot: [%d], val: [%x]\n", i_slot, val);

	}
#endif

	// MI1 - Slave Interface
#if 0
	for (i_slot = 0; i_slot < num_si; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  (NX_BASE_REG_PL301_TOP_AR + 0x20) );
		val = readl(NX_BASE_REG_PL301_TOP_AR + 0x20);
		if (val != i_slot)
			writel( (i_slot << SLOT_NUM_POS) | (i_slot << SI_IF_NUM_POS),  (NX_BASE_REG_PL301_TOP_AR + 0x20) );
	}
#endif

	/* Set progamming for AW */
	// MI0 - Slave Interface
	for (i_slot = 0; i_slot < num_mi; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  NX_BASE_REG_PL301_TOP_AW );
		val = readl(NX_BASE_REG_PL301_TOP_AW);
		if (val != g_TopBusSI[i_slot])
			writel( (i_slot << SLOT_NUM_POS) | (g_TopBusSI[i_slot] << SI_IF_NUM_POS),  NX_BASE_REG_PL301_TOP_AW );
	}
#ifdef DEBUG_BUS_CONF
	for (i_slot = 0; i_slot < num_mi; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  NX_BASE_REG_PL301_TOP_AW );
		val = readl(NX_BASE_REG_PL301_TOP_AW);
		printk("  ... TOP AW slot: [%d], val: [%x]\n", i_slot, val);
	}
#endif

	// MI1 - Slave Interface
#if 0
	for (i_slot = 0; i_slot < num_si; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  (NX_BASE_REG_PL301_TOP_AW + 0x20) );
		val = readl(NX_BASE_REG_PL301_TOP_AW + 0x20);
		if (val != i_slot)
			writel( (i_slot << SLOT_NUM_POS) | (i_slot << SI_IF_NUM_POS),  (NX_BASE_REG_PL301_TOP_AW + 0x20) );
	}
#endif
#endif /* (CFG_BUS_RECONFIG_TOPBUSSI == 1) */

	/* ------------- Display BUS ----------- */
#if (CFG_BUS_RECONFIG_DISPBUSSI == 1)
	num_si = readl(NX_VA_BASE_REG_PL301_DISP + 0xFC0);
	num_mi = readl(NX_VA_BASE_REG_PL301_DISP + 0xFC4);

	/* Set progamming for AR */
	// Slave Interface
	for (i_slot = 0; i_slot < num_si; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  NX_BASE_REG_PL301_DISP_AR);
		val = readl(NX_BASE_REG_PL301_DISP_AR);
		if (val != g_DispBusSI[i_slot])
			writel( (i_slot << SLOT_NUM_POS) | (g_DispBusSI[i_slot] << SI_IF_NUM_POS),  NX_BASE_REG_PL301_DISP_AR );
	}
#ifdef DEBUG_BUS_CONF
	for (i_slot = 0; i_slot < num_si; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  NX_BASE_REG_PL301_DISP_AR );
		val = readl(NX_BASE_REG_PL301_DISP_AR);
		printk("  ... DISP AR slot: [%d], val: [%x]\n", i_slot, val);
	}
#endif

	/* Set progamming for AW */
	// Slave Interface
	for (i_slot = 0; i_slot < num_si; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  NX_BASE_REG_PL301_DISP_AW);
		val = readl(NX_BASE_REG_PL301_DISP_AW);
		if (val != g_DispBusSI[i_slot])
			writel( (i_slot << SLOT_NUM_POS) | (g_DispBusSI[i_slot] << SI_IF_NUM_POS),  NX_BASE_REG_PL301_DISP_AW );
	}
#ifdef DEBUG_BUS_CONF
	for (i_slot = 0; i_slot < num_si; i_slot++)
	{
		writel( (0xFF000000 | i_slot),  NX_BASE_REG_PL301_DISP_AW);
		val = readl(NX_BASE_REG_PL301_DISP_AW);
		printk("  ... DISP AW slot: [%d], val: [%x]\n", i_slot, val);
	}
#endif
#endif /* (CFG_BUS_RECONFIG_DISPBUSSI == 1) */

	return;
}
#endif	/* #if (CFG_BUS_RECONFIG_ENB == 1) */

static void cpu_base_init(void)
{
	U32 tie_reg, val;
	int i = 0;

	NX_RSTCON_Initialize();
	NX_RSTCON_SetBaseAddress((void*)IO_ADDRESS(NX_RSTCON_GetPhysicalAddress()));

	NX_TIEOFF_Initialize();
	NX_TIEOFF_SetBaseAddress((void*)IO_ADDRESS(NX_TIEOFF_GetPhysicalAddress()));

	NX_CLKGEN_Initialize();
	for (i = 0; NX_CLKGEN_GetNumberOfModule() > i; i++)
		NX_CLKGEN_SetBaseAddress(i, (void*)IO_ADDRESS(NX_CLKGEN_GetPhysicalAddress(i)));

	NX_GPIO_Initialize();
	for (i = 0; NX_GPIO_GetNumberOfModule() > i; i++) {
		NX_GPIO_SetBaseAddress(i, (void*)IO_ADDRESS(NX_GPIO_GetPhysicalAddress(i)));
		NX_GPIO_OpenModule(i);
	}

	NX_ALIVE_Initialize();
	NX_ALIVE_SetBaseAddress((void*)IO_ADDRESS(NX_ALIVE_GetPhysicalAddress()));
	NX_ALIVE_OpenModule();

	NX_CLKPWR_Initialize();
	NX_CLKPWR_SetBaseAddress((void*)IO_ADDRESS(NX_CLKPWR_GetPhysicalAddress()));
	NX_CLKPWR_OpenModule();

    /*
     * NOTE> ALIVE Power Gate must enable for RTC register access.
     * 		 must be clear wfi jump address
 	 */
	NX_ALIVE_SetWriteEnable(CTRUE);
	__raw_writel(0xFFFFFFFF, SCR_ARM_SECOND_BOOT);

	/*
	 * NOTE> Control for ACP register access.
	 */
	tie_reg = (U32)IO_ADDRESS(NX_TIEOFF_GetPhysicalAddress());

	val = __raw_readl(tie_reg + 0x70) & ~((3 << 30) | (3 << 10));
	writel(val,   (tie_reg + 0X70));

	val = __raw_readl(tie_reg + 0x80) & ~(3 << 3);
	writel(val,   (tie_reg + 0x80));

	// write 0xf0 on alive scratchpad reg for boot success check
	NX_ALIVE_SetScratchReg(NX_ALIVE_GetScratchReg() | 0xF0);

	NX_WDT_Initialize();
	NX_WDT_SetBaseAddress(0, (void*)IO_ADDRESS(NX_WDT_GetPhysicalAddress(0)));
	NX_WDT_OpenModule(0);

	// watchdog disable
	if (NX_WDT_GetEnable(0)) {
		NX_WDT_SetEnable(0, CFALSE);
		NX_WDT_SetResetEnable(0, CFALSE);
		NX_WDT_ClearInterruptPending(0, NX_WDT_GetInterruptNumber(0));
	}
}

static void cpu_bus_init(void)
{
	/* MCUS for Static Memory. */
	NX_MCUS_Initialize();
	NX_MCUS_SetBaseAddress((void*)IO_ADDRESS(NX_MCUS_GetPhysicalAddress()));
	NX_MCUS_OpenModule();

	/*
	 * NAND Bus config
	 */
#if 0
	NX_MCUS_SetNANDBUSConfig
	(
		0, /* NF */
		CFG_SYS_NAND_TACS,		// tACS  ( 0 ~ 3 )
		CFG_SYS_NAND_TCAH,		// tCAH  ( 0 ~ 3 )
		CFG_SYS_NAND_TCOS,		// tCOS  ( 0 ~ 3 )
		CFG_SYS_NAND_TCOH,		// tCOH  ( 0 ~ 3 )
		CFG_SYS_NAND_TACC		// tACC  ( 1 ~ 16)
	);
#endif

	/*
	 * MCU-Static config: Static Bus #0 ~ #1
	 */
	#define STATIC_BUS_CONFIGUTATION( _n_ )								\
	NX_MCUS_SetStaticBUSConfig											\
	( 																	\
		NX_MCUS_SBUSID_STATIC ## _n_, 									\
		CFG_SYS_STATIC ## _n_ ## _BW, 									\
		CFG_SYS_STATIC ## _n_ ## _TACS, 								\
		CFG_SYS_STATIC ## _n_ ## _TCAH, 								\
		CFG_SYS_STATIC ## _n_ ## _TCOS, 								\
		CFG_SYS_STATIC ## _n_ ## _TCOH, 								\
		CFG_SYS_STATIC ## _n_ ## _TACC, 								\
		CFG_SYS_STATIC ## _n_ ## _TSACC,								\
		(NX_MCUS_WAITMODE ) CFG_SYS_STATIC ## _n_ ## _WAITMODE, 		\
		(NX_MCUS_BURSTMODE) CFG_SYS_STATIC ## _n_ ## _RBURST, 			\
		(NX_MCUS_BURSTMODE) CFG_SYS_STATIC ## _n_ ## _WBURST			\
	);

	STATIC_BUS_CONFIGUTATION( 0);
	STATIC_BUS_CONFIGUTATION( 1);
}

/*------------------------------------------------------------------------------
 *	CPU initialize
 */
void nxp_periph_init(void)
{
	#if		(CFG_UART_DEBUG_CH == 0)
	int id = RESET_ID_UART0;
	char *dev = "nxp-uart.0";
	#elif	(CFG_UART_DEBUG_CH == 1)
	int id = RESET_ID_UART1;
	char *dev = "nxp-uart.1";
	#elif	(CFG_UART_DEBUG_CH == 2)
	int id = RESET_ID_UART2;
	char *dev = "nxp-uart.2";
	#elif	(CFG_UART_DEBUG_CH == 3)
	int id = RESET_ID_UART3;
	char *dev = "nxp-uart.3";
	#elif	(CFG_UART_DEBUG_CH == 4)
	int id = RESET_ID_UART4;
	char *dev = "nxp-uart.4";
	#elif	(CFG_UART_DEBUG_CH == 5)
	int id = RESET_ID_UART5;
	char *dev = "nxp-uart.5";
	#endif

	struct clk *clk = clk_get(NULL, (const char*)dev);

	/* reset control: Low active ___|---   */
	NX_RSTCON_SetnRST(id, RSTCON_nDISABLE);
	NX_RSTCON_SetnRST(id, RSTCON_nENABLE);

	/* set clock   */
	clk_disable(clk);
	clk_set_rate(clk, CFG_UART_CLKGEN_CLOCK_HZ);
	clk_enable(clk);
}

void nxp_cpu_init(void)
{
	cpu_base_init();
	cpu_bus_init();

#if (CFG_BUS_RECONFIG_ENB == 1)
	nxp_set_bus_config();
#endif
}

void nxp_before_linux(void)
{
#ifdef CONFIG_HW_WATCHDOG
    // enable hw watchdog
	hw_watchdog_restart();
#endif
}

unsigned int nxp_cpu_version(void)
{
	unsigned int revision = readl(0x0100);
	unsigned int version = 0;
	switch(revision) {
	case 0xe153000a: version = 1; break;
	default:		 version = 0; break;
	}
	return version;
}

void nxp_print_cpuinfo(void)
{
	nxp_clk_print();
}

