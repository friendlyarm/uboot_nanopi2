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

static void disp_lvds_init(void)
{
	int index = 0;
	int clkid = DISP_CLOCK_LVDS;

	/* BASE : TOP_CLKGEN */
	NX_DISPTOP_CLKGEN_SetBaseAddress(clkid, (void*)IO_ADDRESS(NX_DISPTOP_CLKGEN_GetPhysicalAddress(clkid)));

	/* BASE : LVDS */
	NX_LVDS_Initialize();
	for (index = 0; NX_LVDS_GetNumberOfModule() > index; index++)
		NX_LVDS_SetBaseAddress(index, (void*)IO_ADDRESS(NX_LVDS_GetPhysicalAddress(index)));

	/* CLOCK: top CLKGEN not use BCLK */
	NX_DISPTOP_CLKGEN_SetClockPClkMode(clkid, NX_PCLKMODE_ALWAYS);
}

static void disp_lvds_enable(int enable)
{
	int clkid = DISP_CLOCK_LVDS;
	CBOOL on = (enable ? CTRUE : CFALSE);
	/* START: CLKGEN, LVDS is started in setup function*/
  	NX_DISPTOP_CLKGEN_SetClockDivisorEnable(clkid, on);
}

static int disp_lvds_setup(int module, int input, struct disp_vsync_info *psync, struct disp_lvds_param *plvds)
{
	int clkid = DISP_CLOCK_LVDS;
	int rstnum = NX_LVDS_GetResetNumber(0);
	unsigned int val;
	int format = 1;	// 1: JEiDA, 0: VESA, 2:User

	if (plvds)
		format = plvds->lcd_format;

	//-------- 미리 정의된 형식.
	// VESA에서 iTA만 iTE로 교체한  형식으로 넣어준다.
	// wire [34:0] loc_VideoIn = {4'hf, 4'h0, i_VDEN, i_VSYNC, i_HSYNC, i_VD[23:0] };
	U32 VSYNC = 25;
	U32 HSYNC = 24;
	U32 VDEN  = 26; // bit 위치.
	U32 ONE   = 34;
	U32 ZERO  = 27;

	//====================================================
	// 일단 현재는 location은 사용하고 있지 않다.
	//====================================================
	U32 LOC_A[7] = {ONE,ONE,ONE,ONE,ONE,ONE,ONE};
	U32 LOC_B[7] = {ONE,ONE,ONE,ONE,ONE,ONE,ONE};
	U32 LOC_C[7] = {VDEN,VSYNC,HSYNC,ONE, HSYNC, VSYNC, VDEN};
	U32 LOC_D[7] = {ZERO,ZERO,ZERO,ZERO,ZERO,ZERO,ZERO};
	U32 LOC_E[7] = {ZERO,ZERO,ZERO,ZERO,ZERO,ZERO,ZERO};

	switch (input) {
	case DISP_DEVICE_SYNCGEN0:	input = 0; break;
	case DISP_DEVICE_SYNCGEN1:	input = 1; break;
	case DISP_DEVICE_RESCONV  :	input = 2; break;
	default:
		return -EINVAL;
	}

	/*
	 * select TOP MUX
	 */
	NX_DISPTOP_CLKGEN_SetClockDivisorEnable (clkid, CFALSE);
	NX_DISPTOP_CLKGEN_SetClockSource (clkid, 0, psync->clk_src_lv0);
	NX_DISPTOP_CLKGEN_SetClockDivisor(clkid, 0, psync->clk_div_lv0);
	NX_DISPTOP_CLKGEN_SetClockSource (clkid, 1, psync->clk_src_lv1);  // CLKSRC_PLL0
	NX_DISPTOP_CLKGEN_SetClockDivisor(clkid, 1, psync->clk_div_lv1);

	//===============
	// LVDS Control Pin Setting
	//===============
	val =	(0<<30)  // CPU_I_VBLK_FLAG_SEL
			|	(0<<29)  // CPU_I_BVLK_FLAG
			|	(1<<28)  // SKINI_BST
			|	(1<<27)  // DLYS_BST
			|	(0<<26)  // I_AUTO_SEL
			|	(format<<19)  // JEiDA data packing
			|	(0x1B<<13)  // I_LOCK_PPM_SET, PPM setting for PLL lock
			|	(0x638<<1)  // I_DESKEW_CNT_SEL, period of de-skew region
			;
	NX_LVDS_SetLVDSCTRL0(    0, val );

	val =	(0<<28) // I_ATE_MODE, funtion mode
			|	(0<<27) // I_TEST_CON_MODE, DA (test ctrl mode)
			|	(0<<24) // I_TX4010X_DUMMY
			|	(0<<15) // SKCCK 0
			|	(0<<12) // SKC4 (TX output skew control pin at ODD ch4)
			|	(0<<9)  // SKC3 (TX output skew control pin at ODD ch3)
			|	(0<<6)  // SKC2 (TX output skew control pin at ODD ch2)
			|	(0<<3)  // SKC1 (TX output skew control pin at ODD ch1)
			|	(0<<0)  // SKC0 (TX output skew control pin at ODD ch0)
				;
	NX_LVDS_SetLVDSCTRL1(    0, val );

	val =	(0<<15) // CK_POL_SEL, Input clock, bypass
			|	(0<<14) // VSEL, VCO Freq. range. 0: Low(40MHz~90MHz), 1:High(90MHz~160MHz)
			|	(0x1<<12) // S (Post-scaler)
			|	(0xA<<6) // M (Main divider)
			|	(0xA<<0) // P (Pre-divider)
			;
	NX_LVDS_SetLVDSCTRL2(    0, val );

	val =	(0x03<<6) // SK_BIAS, Bias current ctrl pin
			|	(0<<5) // SKEWINI, skew selection pin, 0 : bypass, 1 : skew enable
			|	(0<<4) // SKEW_EN_H, skew block power down, 0 : power down, 1 : operating
			|	(1<<3) // CNTB_TDLY, delay control pin
			|	(0<<2) // SEL_DATABF, input clock 1/2 division control pin
			|	(0x3<<0) // SKEW_REG_CUR, regulator bias current selection in in SKEW block
			;
	NX_LVDS_SetLVDSCTRL3(    0, val );

	val =	(0<<28) // FLT_CNT, filter control pin for PLL
			|	(0<<27) // VOD_ONLY_CNT, the pre-emphasis's pre-diriver control pin (VOD only)
			|	(0<<26) // CNNCT_MODE_SEL, connectivity mode selection, 0:TX operating, 1:con check
			|	(0<<24) // CNNCT_CNT, connectivity ctrl pin, 0:tx operating, 1: con check
			|	(0<<23) // VOD_HIGH_S, VOD control pin, 1 : Vod only
			|	(0<<22) // SRC_TRH, source termination resistor select pin
			|	(0x20/*0x3F*/<<14) // CNT_VOD_H, TX driver output differential volatge level control pin
			|	(0x01<<6) // CNT_PEN_H, TX driver pre-emphasis level control
			|	(0x4<<3) // FC_CODE, vos control pin
			|	(0<<2) // OUTCON, TX Driver state selectioin pin, 0:Hi-z, 1:Low
			|	(0<<1) // LOCK_CNT, Lock signal selection pin, enable
			|	(0<<0) // AUTO_DSK_SEL, auto deskew selection pin, normal
			;
	NX_LVDS_SetLVDSCTRL4(    0, val );

	val =	(0<<24)	// I_BIST_RESETB
			|	(0<<23)	// I_BIST_EN
			|	(0<<21)	// I_BIST_PAT_SEL
			|	(0<<14) // I_BIST_USER_PATTERN
			|	(0<<13)	// I_BIST_FORCE_ERROR
			|	(0<<7)	// I_BIST_SKEW_CTRL
			|	(0<<5)	// I_BIST_CLK_INV
			|	(0<<3)	// I_BIST_DATA_INV
			|	(0<<0)	// I_BIST_CH_SEL
			;
	NX_LVDS_SetLVDSTMODE0( 0, val );

	// user do not need to modify this codes.
	val = (LOC_A[4]  <<24) | (LOC_A[3]  <<18) | (LOC_A[2]  <<12) | (LOC_A[1]  <<6) | (LOC_A[0]  <<0);
	NX_LVDS_SetLVDSLOC0( 0, val );

	val = (LOC_B[2]  <<24) | (LOC_B[1]  <<18) | (LOC_B[0]  <<12) | (LOC_A[6]  <<6) | (LOC_A[5]  <<0);
	NX_LVDS_SetLVDSLOC1( 0, val );

	val = (LOC_C[0]  <<24) | (LOC_B[6]  <<18) | (LOC_B[5]  <<12) | (LOC_B[4]  <<6) | (LOC_B[3]  <<0);
	NX_LVDS_SetLVDSLOC2( 0, val );

	val = (LOC_C[5]  <<24) | (LOC_C[4]  <<18) | (LOC_C[3]  <<12) | (LOC_C[2]  <<6) | (LOC_C[1]  <<0);
	NX_LVDS_SetLVDSLOC3( 0, val );

	val = (LOC_D[3]  <<24) | (LOC_D[2]  <<18) | (LOC_D[1]  <<12) | (LOC_D[0]  <<6) | (LOC_C[6]  <<0);
	NX_LVDS_SetLVDSLOC4( 0, val );

	val = (LOC_E[1]  <<24) | (LOC_E[0]  <<18) | (LOC_D[6]  <<12) | (LOC_D[5]  <<6) | (LOC_D[4]  <<0);
	NX_LVDS_SetLVDSLOC5( 0, val );

	val = (LOC_E[6]  <<24) | (LOC_E[5]  <<18) | (LOC_E[4]  <<12) | (LOC_E[3]  <<6) | (LOC_E[2]  <<0);
	NX_LVDS_SetLVDSLOC6( 0, val );

	NX_LVDS_SetLVDSLOCMASK0( 0, 0xffffffff );
	NX_LVDS_SetLVDSLOCMASK1( 0, 0xffffffff );

	NX_LVDS_SetLVDSLOCPOL0( 0, (0<<19) | ( 0<<18) );

	/*
	 * select TOP MUX
	 */
	NX_DISPLAYTOP_SetLVDSMUX(CTRUE, input);

	/*
	 * LVDS PHY Reset, make sure last.
	 */
#if defined(CONFIG_MACH_S5P4418)
	NX_RSTCON_SetnRST(rstnum, RSTCON_nDISABLE);
	NX_RSTCON_SetnRST(rstnum, RSTCON_nENABLE);
#elif defined(CONFIG_MACH_S5P6818)
	NX_RSTCON_SetRST(rstnum, RSTCON_ASSERT);
	NX_RSTCON_SetRST(rstnum, RSTCON_NEGATE);
#endif
	return 0;
}

/*
 * disply (EX)
 *							 (get resol)
 *	[Primary] -----	[RESCONV]----------	[LVDS] (800*480)
 *				|
 *				|
 *				|----------------------	[HDMI] (1280*720)
 *				(fix resol)
 */
void display_lvds(int module, unsigned int fbbase,
				struct disp_vsync_info *pvsync,
				struct disp_syncgen_param *psgen,
				struct disp_multily_param *pmly,
				struct disp_lvds_param *plvds)
{
	int input = module == 0 ? DISP_DEVICE_SYNCGEN0 : DISP_DEVICE_SYNCGEN1;
	int layer  = pmly->fb_layer;

	disp_initialize();
	disp_topctl_reset();
	disp_syncgen_reset();

	/* device init */
	disp_multily_init(module);
	disp_syncgen_init(module);
	disp_lvds_init();

	/* set mlc top/rgb layer */
	disp_multily_setup (module, pmly, fbbase);
	disp_multily_enable(module, layer, 1);

	/* set display control  */
	disp_syncgen_setup (module, pvsync, psgen);
	disp_syncgen_enable(module, 1);

	/* set lvds control */
	disp_lvds_setup(module, input, pvsync, plvds);
	disp_lvds_enable(1);

	/* LCD Device Power On */
	#ifdef CFG_IO_LCD_PWR_ENB
	disp_lcd_device(CFG_IO_LCD_PWR_ENB);
	#endif
}


