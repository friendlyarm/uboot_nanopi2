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
#include <errno.h>
#include "display_dev.h"

static void disp_mipi_reset(void)
{
	int index = 0;

	NX_TIEOFF_Set(TIEOFFINDEX_OF_MIPI0_NX_DPSRAM_1R1W_EMAA, 3);
	NX_TIEOFF_Set(TIEOFFINDEX_OF_MIPI0_NX_DPSRAM_1R1W_EMAB, 3);

#if defined(CONFIG_MACH_S5P4418)
    NX_RSTCON_SetnRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST       ) , RSTCON_nDISABLE);
    NX_RSTCON_SetnRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST_DSI_I ) , RSTCON_nDISABLE);
    NX_RSTCON_SetnRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST_CSI_I ) , RSTCON_nDISABLE);
    NX_RSTCON_SetnRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST_PHY_S ) , RSTCON_nDISABLE);
    NX_RSTCON_SetnRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST_PHY_M ) , RSTCON_nDISABLE);
    NX_RSTCON_SetnRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST       ) , RSTCON_nENABLE);
    NX_RSTCON_SetnRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST_DSI_I ) , RSTCON_nENABLE);
	NX_RSTCON_SetnRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST_PHY_S ) , RSTCON_nENABLE);
    NX_RSTCON_SetnRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST_PHY_M ) , RSTCON_nENABLE);
#elif defined(CONFIG_MACH_S5P6818)
    NX_RSTCON_SetRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST       ) , RSTCON_ASSERT);
    NX_RSTCON_SetRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST_DSI_I ) , RSTCON_ASSERT);
    NX_RSTCON_SetRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST_CSI_I ) , RSTCON_ASSERT);
    NX_RSTCON_SetRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST_PHY_S ) , RSTCON_ASSERT);
    NX_RSTCON_SetRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST_PHY_M ) , RSTCON_ASSERT);
    NX_RSTCON_SetRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST       ) , RSTCON_NEGATE);
    NX_RSTCON_SetRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST_DSI_I ) , RSTCON_NEGATE);
	NX_RSTCON_SetRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST_PHY_S ) , RSTCON_NEGATE);
    NX_RSTCON_SetRST(NX_MIPI_GetResetNumber(index, NX_MIPI_RST_PHY_M ) , RSTCON_NEGATE);
#endif
}

static void disp_mipi_init(void)
{
	int clkid = DISP_CLOCK_MIPI;

	/* BASE : TOP_CLKGEN */
	/* CLOCK: top CLKGEN not use BCLK */
	NX_DISPTOP_CLKGEN_SetBaseAddress(clkid, (void*)IO_ADDRESS(NX_DISPTOP_CLKGEN_GetPhysicalAddress(clkid)));
	NX_DISPTOP_CLKGEN_SetClockPClkMode(clkid, NX_PCLKMODE_ALWAYS);

	/* BASE : MIPI */
	NX_MIPI_Initialize();
    NX_MIPI_SetBaseAddress(0, (void*)IO_ADDRESS(NX_MIPI_GetPhysicalAddress(0)));
	NX_MIPI_OpenModule(0);
}

static void disp_mipi_enable(int enable)
{
	int clkid = DISP_CLOCK_MIPI;
	CBOOL on = (enable ? CTRUE : CFALSE);

	/* SPDIF and MIPI */
    NX_DISPTOP_CLKGEN_SetClockDivisorEnable(clkid, CTRUE);

	/* START: CLKGEN, MIPI is started in setup function*/
  	NX_DISPTOP_CLKGEN_SetClockDivisorEnable(clkid, on);
	NX_MIPI_DSI_SetEnable(0, on);
}

static int disp_mipi_setup(int module, int input, struct disp_vsync_info *psync, struct disp_mipi_param *pmipi)
{
	int index = 0;
	int clkid = DISP_CLOCK_MIPI;
	int width  = psync->h_active_len;
	int height = psync->v_active_len;
	int ret = 0;

	int HFP = psync->h_front_porch;
	int HBP = psync->h_back_porch;
	int HS  = psync->h_sync_width;
	int VFP = psync->v_front_porch;
	int VBP = psync->v_back_porch;
	int VS  = psync->v_sync_width;

	unsigned int pllpms  = pmipi->pllpms;
	unsigned int bandctl = pmipi->bandctl;
	unsigned int pllctl  = pmipi->pllctl;
	unsigned int phyctl  = pmipi->phyctl;

	switch (input) {
	case DISP_DEVICE_SYNCGEN0:	input = 0; break;
	case DISP_DEVICE_SYNCGEN1:	input = 1; break;
	case DISP_DEVICE_RESCONV  :	input = 2; break;
	default:
		return -EINVAL;
	}

	NX_MIPI_DSI_SetPLL(index
			,CTRUE      // CBOOL Enable      ,
            ,0xFFFFFFFF // U32 PLLStableTimer,
            ,pllpms     // 19'h033E8: 1Ghz  // Use LN28LPP_MipiDphyCore1p5Gbps_Supplement.
            ,bandctl    // 4'hF     : 1Ghz  // Use LN28LPP_MipiDphyCore1p5Gbps_Supplement.
            ,pllctl     // U32 M_PLLCTL      , // Refer to 10.2.2 M_PLLCTL of MIPI_D_PHY_USER_GUIDE.pdf  Default value is all "0". If you want to change register values, it need to confirm from IP Design Team
            ,phyctl		// U32 B_DPHYCTL       // Refer to 10.2.3 M_PLLCTL of MIPI_D_PHY_USER_GUIDE.pdf or NX_MIPI_PHY_B_DPHYCTL enum or LN28LPP_MipiDphyCore1p5Gbps_Supplement. default value is all "0". If you want to change register values, it need to confirm from IP Design Team
			);

	if (pmipi->lcd_init) {
		NX_MIPI_DSI_SoftwareReset(index);
	    NX_MIPI_DSI_SetClock (index
	    		,0  // CBOOL EnableTXHSClock    ,
	            ,0  // CBOOL UseExternalClock   , // CFALSE: PLL clock CTRUE: External clock
	            ,1  // CBOOL EnableByteClock    , // ByteClock means (D-PHY PLL clock / 8)
	            ,1  // CBOOL EnableESCClock_ClockLane,
	            ,1  // CBOOL EnableESCClock_DataLane0,
	            ,0  // CBOOL EnableESCClock_DataLane1,
	            ,0  // CBOOL EnableESCClock_DataLane2,
	            ,0  // CBOOL EnableESCClock_DataLane3,
	            ,1  // CBOOL EnableESCPrescaler , // ESCClock = ByteClock / ESCPrescalerValue
	            ,5  // U32   ESCPrescalerValue
	   			);

		NX_MIPI_DSI_SetPhy( index
				,0 // U32   NumberOfDataLanes , // 0~3
	            ,1 // CBOOL EnableClockLane   ,
	            ,1 // CBOOL EnableDataLane0   ,
	            ,0 // CBOOL EnableDataLane1   ,
	            ,0 // CBOOL EnableDataLane2   ,
	            ,0 // CBOOL EnableDataLane3   ,
	            ,0 // CBOOL SwapClockLane     ,
	            ,0 // CBOOL SwapDataLane      )
				);

		ret = pmipi->lcd_init(width, height, pmipi->private_data);
		if (0 > ret)
			return ret;
	}

	NX_MIPI_DSI_SoftwareReset(index);
    NX_MIPI_DSI_SetClock (index
    		,1  // CBOOL EnableTXHSClock    ,
            ,0  // CBOOL UseExternalClock   , // CFALSE: PLL clock CTRUE: External clock
            ,1  // CBOOL EnableByteClock    , // ByteClock means (D-PHY PLL clock / 8)
            ,1  // CBOOL EnableESCClock_ClockLane,
            ,1  // CBOOL EnableESCClock_DataLane0,
            ,1  // CBOOL EnableESCClock_DataLane1,
            ,1  // CBOOL EnableESCClock_DataLane2,
            ,1  // CBOOL EnableESCClock_DataLane3,
            ,1  // CBOOL EnableESCPrescaler , // ESCClock = ByteClock / ESCPrescalerValue
            ,5  // U32   ESCPrescalerValue
   			);

	NX_MIPI_DSI_SetPhy( index
			,3 // U32   NumberOfDataLanes , // 0~3
            ,1 // CBOOL EnableClockLane   ,
            ,1 // CBOOL EnableDataLane0   ,
            ,1 // CBOOL EnableDataLane1   ,
            ,1 // CBOOL EnableDataLane2   ,
            ,1 // CBOOL EnableDataLane3   ,
            ,0 // CBOOL SwapClockLane     ,
            ,0 // CBOOL SwapDataLane      )
			);

	NX_MIPI_DSI_SetConfigVideoMode  (index
			,1   // CBOOL EnableAutoFlushMainDisplayFIFO ,
			,0   // CBOOL EnableAutoVerticalCount        ,
			,1,NX_MIPI_DSI_SYNCMODE_EVENT // CBOOL EnableBurst, NX_MIPI_DSI_SYNCMODE SyncMode,
			//,0,NX_MIPI_DSI_SYNCMODE_PULSE // CBOOL EnableBurst, NX_MIPI_DSI_SYNCMODE SyncMode,
			,1   // CBOOL EnableEoTPacket                ,
			,1   // CBOOL EnableHsyncEndPacket           , // Set HSEMode=1
			,1   // CBOOL EnableHFP                      , // Set HFPMode=0
			,1   // CBOOL EnableHBP                      , // Set HBPMode=0
			,1   // CBOOL EnableHSA                      , // Set HSAMode=0
			,0   // U32   NumberOfVirtualChannel         , // 0~3
			,NX_MIPI_DSI_FORMAT_RGB888   // NX_MIPI_DSI_FORMAT Format            ,
			,HFP  // U32   NumberOfWordsInHFP             , // ~65535
			,HBP  // U32   NumberOfWordsInHBP             , // ~65535
			,HS   // U32   NumberOfWordsInHSYNC           , // ~65535
			,VFP  // U32   NumberOfLinesInVFP             , // ~2047
			,VBP   // U32   NumberOfLinesInVBP             , // ~2047
			,VS    // U32   NumberOfLinesInVSYNC           , // ~1023
			,0 // U32   NumberOfLinesInCommandAllow
    		);

	NX_MIPI_DSI_SetSize(index, width, height);

	NX_DISPLAYTOP_SetMIPIMUX(CTRUE, input);

	// 0 is spdif, 1 is mipi vclk
	NX_DISPTOP_CLKGEN_SetClockSource (clkid, 0, psync->clk_src_lv0);
	NX_DISPTOP_CLKGEN_SetClockDivisor(clkid, 0, psync->clk_div_lv0);
	NX_DISPTOP_CLKGEN_SetClockSource (clkid, 1, psync->clk_src_lv1);  // CLKSRC_PLL0
	NX_DISPTOP_CLKGEN_SetClockDivisor(clkid, 1, psync->clk_div_lv1);

	return 0;
}

/*
 * disply
 * MIPI DSI Setting
 *		(1) Initiallize MIPI(DSIM,DPHY,PLL)
 *		(2) Initiallize LCD
 *		(3) ReInitiallize MIPI(DSIM only)
 *		(4) Turn on display(MLC,DPC,...)
 */
void display_mipi(int module, unsigned int fbbase,
				struct disp_vsync_info *pvsync, struct disp_syncgen_param *psgen,
				struct disp_multily_param *pmly, struct disp_mipi_param *pmipi)
{
	int input = module == 0 ? DISP_DEVICE_SYNCGEN0 : DISP_DEVICE_SYNCGEN1;
	int layer = pmly->fb_layer;
	printf("MIPI: display.%d\n", module);

	disp_initialize();
	disp_topctl_reset();
	disp_syncgen_reset();
	disp_mipi_reset();

	/* device init */
	disp_multily_init(module);	/* set BASE and P/BCLK (MLC)*/
	disp_syncgen_init(module);	/* set BASE and PCLK (DPC)*/
	disp_mipi_init();			/* set BASE and PCLK (TOP CLKGEN)*/

	/* set mlc top/rgb layer */
	disp_multily_setup (module, pmly, fbbase);
	disp_multily_enable(module, layer, 1);

	/* set mipi control */
	disp_mipi_setup(module, input, pvsync, pmipi);
	disp_mipi_enable(1);

	/* set display control  */
	disp_syncgen_setup (module, pvsync, psgen);
	disp_syncgen_enable(module, 1);

	/* LCD Device Power On */
	#ifdef CFG_IO_LCD_PWR_ENB
	disp_lcd_device(CFG_IO_LCD_PWR_ENB);
	#endif
}


