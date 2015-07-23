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

#if (0)
#define DBGOUT(msg...)		{ printf("DISP: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

#ifndef printk
#define	printk		printf
#define	KERN_ERR
#define	KERN_INFO
#endif

void disp_lcd_device(int io)
{
	int grp = PAD_GET_GROUP(io);
	int bit = PAD_GET_BITNO(io);
	int wait;
	CBOOL Level = CTRUE;
	#ifdef CFG_IO_LCD_PWR_ENB_LEVEL
	Level = CFG_IO_LCD_PWR_ENB_LEVEL
	#endif

	NX_GPIO_SetOutputValue(grp, bit, Level);
	for (wait=0; 100 > wait; wait++) { ; }
}

void disp_initialize(void)
{
	/* INIT: prototype */
	NX_DISPLAYTOP_Initialize();
	NX_DISPTOP_CLKGEN_Initialize();
	NX_DPC_Initialize();
	NX_MLC_Initialize();

	/* BASE : TOP */
	NX_DISPLAYTOP_SetBaseAddress((void*)IO_ADDRESS(NX_DISPLAYTOP_GetPhysicalAddress()));
	NX_DISPLAYTOP_OpenModule();
}

void disp_topctl_reset(void)
{
	/* RESET: DISPLAYTOP = Low active ___|--- */
	int rstnum = NX_DISPLAYTOP_GetResetNumber();
	DBGOUT("Reset TOPCTRL\n");

#if defined(CONFIG_MACH_S5P4418)
	if (NX_RSTCON_GetnRST(rstnum))
		return;

	NX_RSTCON_SetnRST(rstnum, RSTCON_nDISABLE);
	NX_RSTCON_SetnRST(rstnum, RSTCON_nENABLE);
#elif defined(CONFIG_MACH_S5P6818)
	if (NX_RSTCON_GetRST(rstnum))
		return;

	NX_RSTCON_SetRST(rstnum, RSTCON_ASSERT);
	NX_RSTCON_SetRST(rstnum, RSTCON_NEGATE);
#endif
}

void disp_syncgen_reset(void)
{
	/* RESET: Dual Display = MLC0/DPC0 and MLC1/DPC1 = Low active ___|---  */
	int rstnum = NX_DUALDISPLAY_GetResetNumber(0);
	DBGOUT("Reset SYNCGEN\n");

#if defined(CONFIG_MACH_S5P4418)
	if (NX_RSTCON_GetnRST(rstnum))
		return;

	NX_RSTCON_SetnRST(rstnum, RSTCON_nDISABLE);
	NX_RSTCON_SetnRST(rstnum, RSTCON_nENABLE);
#elif defined(CONFIG_MACH_S5P6818)
	if (NX_RSTCON_GetRST(rstnum))
		return;

	NX_RSTCON_SetRST(rstnum, RSTCON_ASSERT);
	NX_RSTCON_SetRST(rstnum, RSTCON_NEGATE);
#endif
}

void disp_syncgen_init(int module)
{
	/* BASE : DPC */
	NX_DPC_SetBaseAddress(module, (void*)IO_ADDRESS(NX_DPC_GetPhysicalAddress(module)));
	NX_DPC_OpenModule(module);
	/* CLOCK: MLC PCLK */
	NX_DPC_SetClockPClkMode(module, NX_PCLKMODE_ALWAYS);
}

void disp_syncgen_enable(int module, int enable)
{
	CBOOL on = (enable ? CTRUE : CFALSE);
	/* START: DPC */
	NX_DPC_SetDPCEnable(module, on);
	/* START: CLKGEN */
	NX_DPC_SetClockDivisorEnable(module, on);
}

int disp_syncgen_setup(int module, struct disp_vsync_info *psync, struct disp_syncgen_param *par)
{
	NX_DPC_DITHER RDither, GDither, BDither;
	CBOOL RGBMode = CFALSE;
	CBOOL EmbSync = (par->out_format == DPC_FORMAT_CCIR656 ? CTRUE : CFALSE);
	CBOOL ClkInv  = par->clk_inv_lv0 | par->clk_inv_lv1;
	unsigned int delay_mask = par->delay_mask;
	int rgb_pvd = 0, hsync_cp1 = 7, vsync_fram = 7, de_cp2 = 7;
	int v_vso = 1, v_veo = 1, e_vso = 1, e_veo = 1;
	int interlace = 0;

	if (NULL == psync || NULL == par) {
		printk("Error, display.%d not set sync or pad clock inforamtion...\n", module);
		return -EINVAL;
	}
	interlace = psync->interlace;

	/* set delay mask */
	if (delay_mask & DISP_SYNCGEN_DELAY_RGB_PVD)
		rgb_pvd = par->d_rgb_pvd;
	if (delay_mask & DISP_SYNCGEN_DELAY_HSYNC_CP1)
		hsync_cp1 = par->d_hsync_cp1;
	if (delay_mask & DISP_SYNCGEN_DELAY_VSYNC_FRAM)
		vsync_fram = par->d_vsync_fram;
	if (delay_mask & DISP_SYNCGEN_DELAY_DE_CP)
		de_cp2 = par->d_de_cp2;

	if (par->vs_start_offset != 0 ||
		par->vs_end_offset 	 != 0 ||
		par->ev_start_offset != 0 ||
		par->ev_end_offset   != 0) {
		v_vso = par->vs_start_offset;
		v_veo = par->vs_end_offset;
		e_vso = par->ev_start_offset;
		e_veo = par->ev_end_offset;
	}

    if (((U32)NX_DPC_FORMAT_RGB555   == par->out_format) ||
		((U32)NX_DPC_FORMAT_MRGB555A == par->out_format) ||
		((U32)NX_DPC_FORMAT_MRGB555B == par->out_format))	{
		RDither = GDither = BDither = NX_DPC_DITHER_5BIT;
		RGBMode = CTRUE;
	}
	else if (((U32)NX_DPC_FORMAT_RGB565  == par->out_format) ||
			 ((U32)NX_DPC_FORMAT_MRGB565 == par->out_format))	{
		RDither = BDither = NX_DPC_DITHER_5BIT;
		GDither = NX_DPC_DITHER_6BIT, RGBMode = CTRUE;
	}
	else if (((U32)NX_DPC_FORMAT_RGB666  == par->out_format) ||
			 ((U32)NX_DPC_FORMAT_MRGB666 == par->out_format))	{
		RDither = GDither = BDither = NX_DPC_DITHER_6BIT;
		RGBMode = CTRUE;
	}
	else {
		RDither = GDither = BDither = NX_DPC_DITHER_BYPASS;
		RGBMode = CTRUE;
	}

	/* CLKGEN0/1 */
	NX_DPC_SetClockSource  (module, 0, psync->clk_src_lv0);
	NX_DPC_SetClockDivisor (module, 0, psync->clk_div_lv0);
	NX_DPC_SetClockSource  (module, 1, psync->clk_src_lv1);
	NX_DPC_SetClockDivisor (module, 1, psync->clk_div_lv1);

	NX_DPC_SetClockOutDelay(module, 0, par->clk_delay_lv0);
	NX_DPC_SetClockOutDelay(module, 1, par->clk_delay_lv1);

	/* LCD out */
	NX_DPC_SetMode(module, par->out_format, interlace, par->invert_field, RGBMode,
			par->swap_RB, par->yc_order, EmbSync, EmbSync, par->vclk_select,
			ClkInv, CFALSE);
	NX_DPC_SetHSync(module,  psync->h_active_len,
			psync->h_sync_width,  psync->h_front_porch,  psync->h_back_porch,  psync->h_sync_invert);
	NX_DPC_SetVSync(module,
			psync->v_active_len, psync->v_sync_width, psync->v_front_porch, psync->v_back_porch, psync->v_sync_invert,
			psync->v_active_len, psync->v_sync_width, psync->v_front_porch, psync->v_back_porch);
	NX_DPC_SetVSyncOffset(module, v_vso, v_veo, e_vso, e_veo);
	NX_DPC_SetDelay (module, rgb_pvd, hsync_cp1, vsync_fram, de_cp2);
   	NX_DPC_SetDither(module, RDither, GDither, BDither);

	/* MLC top screen size */
   	NX_MLC_SetScreenSize(module, psync->h_active_len, psync->v_active_len);

   	return 0;
}

void disp_multily_init(int module)
{
	/* BASE : MLC */
	NX_MLC_SetBaseAddress(module, (void*)IO_ADDRESS(NX_MLC_GetPhysicalAddress(module)));
	NX_MLC_OpenModule(module);
	/* CLOCK: MLC PCLK/BCLK */
	NX_MLC_SetClockPClkMode(module, NX_PCLKMODE_ALWAYS);
	NX_MLC_SetClockBClkMode(module, NX_BCLKMODE_ALWAYS);
}

void disp_multily_enable(int module, int layer, int enable)
{
	CBOOL on = (enable ? CTRUE : CFALSE);
	/* START: MLC TOP */
	NX_MLC_SetMLCEnable(module, on);
	NX_MLC_SetTopDirtyFlag(module);
	/* START: MLC FB Layer */
	NX_MLC_SetLayerEnable(module, layer, on);
	NX_MLC_SetDirtyFlag(module, layer);
	/* START: MLC no CLKGEN */
}

int disp_multily_setup(int module, struct disp_multily_param *par, unsigned int fbbase)
{
	int sx =  par->x_start;
	int sy =  par->y_start;
	int	x_resol = par->x_resol;
	int	y_resol	= par->y_resol;
	int	interlace = par->interlace;
	int	video_prior	= par->video_prior;
	int	pixel_byte = par->pixel_byte;
	int	mem_lock_size = par->mem_lock_size;
	int layer = par->fb_layer;
	unsigned int rgb_format = par->rgb_format;
	unsigned int bg_color = par->bg_color;

	mem_lock_size = 16; /* fix mem lock size */

	/* MLC TOP layer */
	NX_MLC_SetLayerPriority	(module, video_prior);
	NX_MLC_SetBackground(module, bg_color);
	NX_MLC_SetFieldEnable(module, interlace);
	NX_MLC_SetRGBLayerGamaTablePowerMode(module, CFALSE, CFALSE, CFALSE);
	NX_MLC_SetRGBLayerGamaTableSleepMode(module, CTRUE, CTRUE, CTRUE);
	NX_MLC_SetRGBLayerGammaEnable(module, CFALSE);
	NX_MLC_SetDitherEnableWhenUsingGamma(module, CFALSE);
	NX_MLC_SetGammaPriority(module, CFALSE);
    NX_MLC_SetTopPowerMode(module, CTRUE);
    NX_MLC_SetTopSleepMode(module, CFALSE);

	/* MLC FB layer */
	NX_MLC_SetLockSize(module, layer, mem_lock_size);
	NX_MLC_SetAlphaBlending(module, layer, CFALSE, 15);
	NX_MLC_SetTransparency(module, layer, CFALSE, 0);
	NX_MLC_SetColorInversion(module, layer, CFALSE, 0);
	NX_MLC_SetRGBLayerInvalidPosition(module, layer, 0, 0, 0, 0, 0, CFALSE);
	NX_MLC_SetRGBLayerInvalidPosition(module, layer, 1, 0, 0, 0, 0, CFALSE);
	NX_MLC_SetFormatRGB(module, layer, rgb_format);
	NX_MLC_SetPosition(module, layer, sx, sy, sx+x_resol-1, sy+y_resol-1);
	NX_MLC_SetRGBLayerStride(module, layer, pixel_byte, x_resol*pixel_byte);
	NX_MLC_SetRGBLayerAddress(module, layer, fbbase);

	return 0;
}

int disp_mlc_set_enable(int module, int layer, int on)
{
	if (MLC_LAYER_VIDEO == layer) {
		if (on) {
    	    NX_MLC_SetVideoLayerLineBufferPowerMode(module, CTRUE);
   		    NX_MLC_SetVideoLayerLineBufferSleepMode(module, CFALSE);
			NX_MLC_SetLayerEnable(module, MLC_LAYER_VIDEO, CTRUE);
			NX_MLC_SetDirtyFlag(module, MLC_LAYER_VIDEO);
		} else {
			CBOOL hl, hc, vl, vc;
 			NX_MLC_SetLayerEnable(module, MLC_LAYER_VIDEO, CFALSE);
			NX_MLC_SetDirtyFlag(module, MLC_LAYER_VIDEO);
			NX_MLC_GetVideoLayerScaleFilter(module, &hl, &hc, &vl, &vc);
			if (hl | hc | vl | vc)
				NX_MLC_SetVideoLayerScaleFilter(module, 0, 0, 0, 0);
    	    NX_MLC_SetVideoLayerLineBufferPowerMode(module, CFALSE);
   		    NX_MLC_SetVideoLayerLineBufferSleepMode(module, CTRUE);
			NX_MLC_SetDirtyFlag(module, MLC_LAYER_VIDEO);
		}
	}  else {
		NX_MLC_SetLayerEnable(module, layer, (on ? CTRUE : CFALSE));
		NX_MLC_SetDirtyFlag(module, layer);
	}
	return 0;
}

int disp_mlc_set_address(int module, int layer, unsigned int address)
{
	NX_MLC_SetRGBLayerAddress(module, layer, address);
	NX_MLC_SetDirtyFlag(module, layer);
	return 0;
}

int disp_mlc_wait_vsync(int module, int layer, int fps)
{
	int cnt = 0;
	if (0 == fps)
		return 	(int)NX_MLC_GetDirtyFlag(module, layer);

	while (fps > cnt++) {
		while(NX_MLC_GetDirtyFlag(module, layer)) { };
		NX_MLC_SetDirtyFlag(module, layer);
	}
	return 0;
}
