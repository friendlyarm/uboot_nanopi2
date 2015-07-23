/*
 * (C) Copyright 2009
 * jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <config.h>
#include <common.h>
#include <errno.h>
#include "display_dev.h"

#if (0)
#define DBGOUT(msg...)		{ printf(KERN_INFO msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

static const u8 hdmiphy_preset74_25[32] = {
    0xd1, 0x1f, 0x10, 0x40, 0x40, 0xf8, 0xc8, 0x81,
    0xe8, 0xba, 0xd8, 0x45, 0xa0, 0xac, 0x80, 0x56,
    0x80, 0x09, 0x84, 0x05, 0x22, 0x24, 0x86, 0x54,
    0xa5, 0x24, 0x01, 0x00, 0x00, 0x01, 0x10, 0x80,
};

static const u8 hdmiphy_preset148_5[32] = {
    0xd1, 0x1f, 0x00, 0x40, 0x40, 0xf8, 0xc8, 0x81,
    0xe8, 0xba, 0xd8, 0x45, 0xa0, 0xac, 0x80, 0x66,
    0x80, 0x09, 0x84, 0x05, 0x22, 0x24, 0x86, 0x54,
    0x4b, 0x25, 0x03, 0x00, 0x00, 0x01, 0x80,
};

#define HDMIPHY_PRESET_TABLE_SIZE   (32)

enum NXP_HDMI_PRESET {
    NXP_HDMI_PRESET_720P = 0,   /* 1280 x 720 */
    NXP_HDMI_PRESET_1080P,      /* 1920 x 1080 */
    NXP_HDMI_PRESET_MAX
};

static inline void hdmi_reset(void)
{
#if defined(CONFIG_MACH_S5P4418)
    NX_RSTCON_SetnRST(NX_HDMI_GetResetNumber(0, i_nRST_VIDEO), RSTCON_nDISABLE);
    NX_RSTCON_SetnRST(NX_HDMI_GetResetNumber(0, i_nRST_SPDIF), RSTCON_nDISABLE);
    NX_RSTCON_SetnRST(NX_HDMI_GetResetNumber(0, i_nRST_TMDS), RSTCON_nDISABLE);
    NX_RSTCON_SetnRST(NX_HDMI_GetResetNumber(0, i_nRST_VIDEO), RSTCON_nENABLE);
    NX_RSTCON_SetnRST(NX_HDMI_GetResetNumber(0, i_nRST_SPDIF), RSTCON_nENABLE);
    NX_RSTCON_SetnRST(NX_HDMI_GetResetNumber(0, i_nRST_TMDS), RSTCON_nENABLE);
#elif defined(CONFIG_MACH_S5P6818)
    NX_RSTCON_SetRST(NX_HDMI_GetResetNumber(0, i_nRST_VIDEO), RSTCON_ASSERT);
    NX_RSTCON_SetRST(NX_HDMI_GetResetNumber(0, i_nRST_SPDIF), RSTCON_ASSERT);
    NX_RSTCON_SetRST(NX_HDMI_GetResetNumber(0, i_nRST_TMDS), RSTCON_ASSERT);
    NX_RSTCON_SetRST(NX_HDMI_GetResetNumber(0, i_nRST_VIDEO), RSTCON_NEGATE);
    NX_RSTCON_SetRST(NX_HDMI_GetResetNumber(0, i_nRST_SPDIF), RSTCON_NEGATE);
    NX_RSTCON_SetRST(NX_HDMI_GetResetNumber(0, i_nRST_TMDS), RSTCON_NEGATE);
#endif
}

static int hdmi_phy_enable(int preset, int enable)
{
	const u8 *table = NULL;
    int size = 0;
	u32 addr, i = 0;

	if (!enable)
		return 0;

	switch (preset) {
    case NXP_HDMI_PRESET_720P:	table = hdmiphy_preset74_25; size = 32; break;
    case NXP_HDMI_PRESET_1080P: table = hdmiphy_preset148_5; size = 31;  break;
    default: printf("hdmi: phy not support preset %d\n", preset);
        return -EINVAL;
	}

    NX_HDMI_SetReg(0, HDMI_PHY_Reg7C, (0<<7));
    NX_HDMI_SetReg(0, HDMI_PHY_Reg7C, (0<<7));
    NX_HDMI_SetReg(0, HDMI_PHY_Reg04, (0<<4));
    NX_HDMI_SetReg(0, HDMI_PHY_Reg04, (0<<4));
    NX_HDMI_SetReg(0, HDMI_PHY_Reg24, (1<<7));
    NX_HDMI_SetReg(0, HDMI_PHY_Reg24, (1<<7));

    for (i=0, addr=HDMI_PHY_Reg04; size > i; i++, addr+=4) {
        NX_HDMI_SetReg(0, addr, table[i]);
        NX_HDMI_SetReg(0, addr, table[i]);
	}

    NX_HDMI_SetReg(0, HDMI_PHY_Reg7C, 0x80);
    NX_HDMI_SetReg(0, HDMI_PHY_Reg7C, 0x80);
    NX_HDMI_SetReg(0, HDMI_PHY_Reg7C, (1<<7));
    NX_HDMI_SetReg(0, HDMI_PHY_Reg7C, (1<<7));
    DBGOUT("%s: preset = %d\n", __func__, preset);

    return 0;
}

static inline bool hdmi_wait_phy_ready(void)
{
    int count = 500;
    do {
        u32 val = NX_HDMI_GetReg(0, HDMI_LINK_PHY_STATUS_0);
        if (val & 0x01) {
            printf("HDMI: PHY Ready!!!\n");
            return true;
        }
        mdelay(10);
    } while (count--);

    return false;
}

static inline int hdmi_get_vsync(int preset,
				struct disp_vsync_info *psync, struct disp_syncgen_param *par)
{
    switch (preset) {
    case NXP_HDMI_PRESET_720P:	/* 720p: 1280x720 */
        psync->h_active_len  = 1280;
        psync->h_sync_width  =   40;
        psync->h_back_porch  =  220;
        psync->h_front_porch =  110;
        psync->h_sync_invert =    0;
        psync->v_active_len  =  720;
        psync->v_sync_width  =    5;
        psync->v_back_porch  =   20;
        psync->v_front_porch =    5;
        psync->v_sync_invert =    0;
        break;

    case NXP_HDMI_PRESET_1080P:	/* 1080p: 1920x1080 */
        psync->h_active_len  = 1920;
        psync->h_sync_width  =   44;
        psync->h_back_porch  =  148;
        psync->h_front_porch =   88;
        psync->h_sync_invert =    0;
        psync->v_active_len =  1080;
        psync->v_sync_width =     5;
        psync->v_back_porch =    36;
        psync->v_front_porch =    4;
        psync->v_sync_invert =    0;
        break;
    default:
        printf("HDMI: not support preset sync %d\n", preset);
        return -EINVAL;
    }

    psync->clk_src_lv0 = 4;
    psync->clk_div_lv0 = 1;
    psync->clk_src_lv1 = 7;
    psync->clk_div_lv1 = 1;

	par->out_format	= OUTPUTFORMAT_RGB888;
	par->delay_mask = (DISP_SYNCGEN_DELAY_RGB_PVD | DISP_SYNCGEN_DELAY_HSYNC_CP1 |
					   DISP_SYNCGEN_DELAY_VSYNC_FRAM | DISP_SYNCGEN_DELAY_DE_CP);
	par->d_rgb_pvd = 0;
	par->d_hsync_cp1 = 0;
	par->d_vsync_fram = 0;
	par->d_de_cp2 = 7;

	//	HFP + HSW + HBP + AVWidth-VSCLRPIXEL- 1;
	par->vs_start_offset = (psync->h_front_porch + psync->h_sync_width +
						psync->h_back_porch + psync->h_active_len - 1);
	par->vs_end_offset = 0;
	// HFP + HSW + HBP + AVWidth-EVENVSCLRPIXEL- 1
	par->ev_start_offset = (psync->h_front_porch + psync->h_sync_width +
						psync->h_back_porch + psync->h_active_len - 1);
	par->ev_end_offset = 0;
    DBGOUT("%s: preset = %d\n", __func__, preset);

    return 0;
}

static void hdmi_clock(void)
{
    NX_DISPTOP_CLKGEN_SetBaseAddress(ToMIPI_CLKGEN,
            (void*)IO_ADDRESS(NX_DISPTOP_CLKGEN_GetPhysicalAddress(ToMIPI_CLKGEN)));
    NX_DISPTOP_CLKGEN_SetClockDivisorEnable(ToMIPI_CLKGEN, CFALSE);
    NX_DISPTOP_CLKGEN_SetClockPClkMode(ToMIPI_CLKGEN, NX_PCLKMODE_ALWAYS);
    NX_DISPTOP_CLKGEN_SetClockSource(ToMIPI_CLKGEN, HDMI_SPDIF_CLKOUT, 2); // pll2
    NX_DISPTOP_CLKGEN_SetClockDivisor(ToMIPI_CLKGEN, HDMI_SPDIF_CLKOUT, 2);
    NX_DISPTOP_CLKGEN_SetClockSource(ToMIPI_CLKGEN, 1, 7);
    NX_DISPTOP_CLKGEN_SetClockDivisorEnable(ToMIPI_CLKGEN, CTRUE);

	// must initialize this !!
	NX_DISPLAYTOP_HDMI_SetVSyncHSStartEnd(0, 0);
	NX_DISPLAYTOP_HDMI_SetVSyncStart(0); // from posedge VSync
	NX_DISPLAYTOP_HDMI_SetHActiveStart(0); // from posedge HSync
	NX_DISPLAYTOP_HDMI_SetHActiveEnd(0); // from posedge HSync
}

static void hdmi_vsync(struct disp_vsync_info *psync)
{
    int width = psync->h_active_len;
    int hsw = psync->h_sync_width;
    int hbp  = psync->h_back_porch;
    int height = psync->v_active_len;
    int vsw = psync->v_sync_width;
    int vbp = psync->v_back_porch;

	int v_sync_s = vsw + vbp + height - 1;
    int h_active_s = hsw + hbp;
    int h_active_e = width + hsw + hbp;
    int v_sync_hs_se0 = hsw + hbp + 1;
    int v_sync_hs_se1 = hsw + hbp + 2;

	NX_DISPLAYTOP_HDMI_SetVSyncStart(v_sync_s);
	NX_DISPLAYTOP_HDMI_SetHActiveStart(h_active_s);
	NX_DISPLAYTOP_HDMI_SetHActiveEnd(h_active_e);
	NX_DISPLAYTOP_HDMI_SetVSyncHSStartEnd(v_sync_hs_se0, v_sync_hs_se1);
}

static int hdmi_setup(struct disp_vsync_info *psync)
{
    int width = psync->h_active_len;
    int hsw = psync->h_sync_width;
    int hfp = psync->h_front_porch;
    int hbp = psync->h_back_porch;
    int height = psync->v_active_len;
    int vsw = psync->v_sync_width;
    int vfp = psync->v_front_porch;
    int vbp = psync->v_back_porch;

    u32 h_blank, h_line, h_sync_start, h_sync_end;
    u32 v_blank, v2_blank, v_actline, v_line;
    u32 v_sync_line_bef_1, v_sync_line_bef_2;

    u32 fixed_ffff = 0xffff;

    /* calculate sync variables */
    h_blank = hfp + hsw + hbp;
    v_blank = vfp + vsw + vbp;
    v_actline = height;
    v2_blank = height + vfp + vsw + vbp;
    v_line = height + vfp + vsw + vbp; /* total v */
    h_line = width + hfp + hsw + hbp;  /* total h */
    h_sync_start = hfp;
    h_sync_end = hfp + hsw;
    v_sync_line_bef_1 = vfp;
    v_sync_line_bef_2 = vfp + vsw;


    /* no blue screen mode, encoding order as it is */
    NX_HDMI_SetReg(0, HDMI_LINK_HDMI_CON_0, (0<<5)|(1<<4));

    /* set HDMI_LINK_BLUE_SCREEN_* to 0x0 */
    NX_HDMI_SetReg(0, HDMI_LINK_BLUE_SCREEN_R_0, 0x5555);
    NX_HDMI_SetReg(0, HDMI_LINK_BLUE_SCREEN_R_1, 0x5555);
    NX_HDMI_SetReg(0, HDMI_LINK_BLUE_SCREEN_G_0, 0x5555);
    NX_HDMI_SetReg(0, HDMI_LINK_BLUE_SCREEN_G_1, 0x5555);
    NX_HDMI_SetReg(0, HDMI_LINK_BLUE_SCREEN_B_0, 0x5555);
    NX_HDMI_SetReg(0, HDMI_LINK_BLUE_SCREEN_B_1, 0x5555);

    /* set HDMI_CON_1 to 0x0 */
    NX_HDMI_SetReg(0, HDMI_LINK_HDMI_CON_1, 0x0);
    NX_HDMI_SetReg(0, HDMI_LINK_HDMI_CON_2, 0x0);

    /* set interrupt : enable hpd_plug, hpd_unplug */
    NX_HDMI_SetReg(0, HDMI_LINK_INTC_CON_0, (1<<6)|(1<<3)|(1<<2));

    /* set STATUS_EN to 0x17 */
    NX_HDMI_SetReg(0, HDMI_LINK_STATUS_EN, 0x17);

    /* TODO set HDP to 0x0 : later check hpd */
    NX_HDMI_SetReg(0, HDMI_LINK_HPD, 0x0);

    /* set MODE_SEL to 0x02 */
    NX_HDMI_SetReg(0, HDMI_LINK_MODE_SEL, 0x2);

    /* set H_BLANK_*, V1_BLANK_*, V2_BLANK_*, V_LINE_*, H_LINE_*, H_SYNC_START_*, H_SYNC_END_ *
     * V_SYNC_LINE_BEF_1_*, V_SYNC_LINE_BEF_2_*
     */
    NX_HDMI_SetReg(0, HDMI_LINK_H_BLANK_0, h_blank%256);
    NX_HDMI_SetReg(0, HDMI_LINK_H_BLANK_1, h_blank>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V1_BLANK_0, v_blank%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V1_BLANK_1, v_blank>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V2_BLANK_0, v2_blank%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V2_BLANK_1, v2_blank>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V_LINE_0, v_line%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_LINE_1, v_line>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_H_LINE_0, h_line%256);
    NX_HDMI_SetReg(0, HDMI_LINK_H_LINE_1, h_line>>8);

    if (width == 1280) {
        NX_HDMI_SetReg(0, HDMI_LINK_HSYNC_POL, 0x1);
        NX_HDMI_SetReg(0, HDMI_LINK_VSYNC_POL, 0x1);
    } else {
        NX_HDMI_SetReg(0, HDMI_LINK_HSYNC_POL, 0x0);
        NX_HDMI_SetReg(0, HDMI_LINK_VSYNC_POL, 0x0);
    }

    NX_HDMI_SetReg(0, HDMI_LINK_INT_PRO_MODE, 0x0);

    NX_HDMI_SetReg(0, HDMI_LINK_H_SYNC_START_0, (h_sync_start%256)-2);
    NX_HDMI_SetReg(0, HDMI_LINK_H_SYNC_START_1, h_sync_start>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_H_SYNC_END_0, (h_sync_end%256)-2);
    NX_HDMI_SetReg(0, HDMI_LINK_H_SYNC_END_1, h_sync_end>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_BEF_1_0, v_sync_line_bef_1%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_BEF_1_1, v_sync_line_bef_1>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_BEF_2_0, v_sync_line_bef_2%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_BEF_2_1, v_sync_line_bef_2>>8);

    /* Set V_SYNC_LINE_AFT*, V_SYNC_LINE_AFT_PXL*, VACT_SPACE* */
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_1_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_1_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_2_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_2_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_3_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_3_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_4_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_4_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_5_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_5_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_6_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_6_1, fixed_ffff>>8);

    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_1_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_1_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_2_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_2_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_3_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_3_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_4_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_4_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_5_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_5_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_6_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_6_1, fixed_ffff>>8);

    NX_HDMI_SetReg(0, HDMI_LINK_VACT_SPACE1_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_VACT_SPACE1_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_VACT_SPACE2_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_VACT_SPACE2_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_VACT_SPACE3_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_VACT_SPACE3_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_VACT_SPACE4_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_VACT_SPACE4_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_VACT_SPACE5_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_VACT_SPACE5_1, fixed_ffff>>8);
    NX_HDMI_SetReg(0, HDMI_LINK_VACT_SPACE6_0, fixed_ffff%256);
    NX_HDMI_SetReg(0, HDMI_LINK_VACT_SPACE6_1, fixed_ffff>>8);

    NX_HDMI_SetReg(0, HDMI_LINK_CSC_MUX, 0x0);
    NX_HDMI_SetReg(0, HDMI_LINK_SYNC_GEN_MUX, 0x0);

    NX_HDMI_SetReg(0, HDMI_LINK_SEND_START_0, 0xfd);
    NX_HDMI_SetReg(0, HDMI_LINK_SEND_START_1, 0x01);
    NX_HDMI_SetReg(0, HDMI_LINK_SEND_END_0, 0x0d);
    NX_HDMI_SetReg(0, HDMI_LINK_SEND_END_1, 0x3a);
    NX_HDMI_SetReg(0, HDMI_LINK_SEND_END_2, 0x08);

    /* Set DC_CONTROL to 0x00 */
    NX_HDMI_SetReg(0, HDMI_LINK_DC_CONTROL, 0x0);

#if (1)
    NX_HDMI_SetReg(0, HDMI_LINK_VIDEO_PATTERN_GEN, 0x0);
#else
	/* PATTERN */
    NX_HDMI_SetReg(0, HDMI_LINK_VIDEO_PATTERN_GEN, 0x1);
#endif
  	NX_HDMI_SetReg(0, HDMI_LINK_GCP_CON, 0x0a);
    return 0;
}

static void disp_hdmi_init(void)
{
   /**
     * [SEQ 2] set the HDMI CLKGEN's PCLKMODE to always enabled
     */
    NX_DISPTOP_CLKGEN_SetBaseAddress(HDMI_CLKGEN,
            (void*)IO_ADDRESS(NX_DISPTOP_CLKGEN_GetPhysicalAddress(HDMI_CLKGEN)));
    NX_DISPTOP_CLKGEN_SetClockPClkMode(HDMI_CLKGEN, NX_PCLKMODE_ALWAYS);

    NX_HDMI_SetBaseAddress(0, (void*)IO_ADDRESS(NX_HDMI_GetPhysicalAddress(0)));
    NX_HDMI_Initialize();

    /**
     * [SEQ 3] set the 0xC001100C[0] to 1
     */
    NX_TIEOFF_SetBaseAddress((void*)IO_ADDRESS(NX_TIEOFF_GetPhysicalAddress()));
    NX_TIEOFF_Set(TIEOFFINDEX_OF_DISPLAYTOP0_i_HDMI_PHY_REFCLK_SEL, 1);

    /**
     * [SEQ 4] release the resets of HDMI.i_PHY_nRST and HDMI.i_nRST
     */
#if defined(CONFIG_MACH_S5P4418)
    NX_RSTCON_SetnRST(NX_HDMI_GetResetNumber(0, i_nRST_PHY), RSTCON_nDISABLE);
    NX_RSTCON_SetnRST(NX_HDMI_GetResetNumber(0, i_nRST), RSTCON_nDISABLE);
    NX_RSTCON_SetnRST(NX_HDMI_GetResetNumber(0, i_nRST_PHY), RSTCON_nENABLE);
    NX_RSTCON_SetnRST(NX_HDMI_GetResetNumber(0, i_nRST), RSTCON_nENABLE);
#elif defined(CONFIG_MACH_S5P6818)
    NX_RSTCON_SetRST(NX_HDMI_GetResetNumber(0, i_nRST_PHY), RSTCON_ASSERT);
    NX_RSTCON_SetRST(NX_HDMI_GetResetNumber(0, i_nRST), RSTCON_ASSERT);
    NX_RSTCON_SetRST(NX_HDMI_GetResetNumber(0, i_nRST_PHY), RSTCON_NEGATE);
    NX_RSTCON_SetRST(NX_HDMI_GetResetNumber(0, i_nRST), RSTCON_NEGATE);
#endif
}

void disp_hdmi_enable(int input, int preset, struct disp_vsync_info *psync, int enable)
{
	if (enable) {
		// HDMI system enable
		NX_HDMI_SetReg(0, HDMI_LINK_HDMI_CON_0, (NX_HDMI_GetReg(0, HDMI_LINK_HDMI_CON_0) | 0x1));
		hdmi_vsync(psync);	// Last set HDMI video
	} else {
		hdmi_phy_enable(preset, 0);
	}
}

static int disp_hdmi_setup(int input, int preset,
				struct disp_vsync_info *psync, struct disp_syncgen_param *par)
{
	U32 HDMI_SEL = 0;
    int ret;

   	switch (input) {
	case DISP_DEVICE_SYNCGEN0: HDMI_SEL = PrimaryMLC;  		break;
	case DISP_DEVICE_SYNCGEN1: HDMI_SEL = SecondaryMLC; 	break;
   	case DISP_DEVICE_RESCONV:  HDMI_SEL = ResolutionConv; 	break;
   	default: printf("HDMI: not support source device %d\n", input);
		return -EINVAL;
   	}

	/**
	 * [SEQ 5] set up the HDMI PHY to specific video clock.
	 */
	ret = hdmi_phy_enable(preset, 1);
	if (0 > ret)
	    return ret;

	/**
	 * [SEQ 6] I2S (or SPDIFTX) configuration for the source audio data
	 * this is done in another user app  - ex> Android Audio HAL
	 */

	/**
	 * [SEQ 7] Wait for ECID ready
	 */

	/**
	 * [SEQ 8] release the resets of HDMI.i_VIDEO_nRST and HDMI.i_SPDIF_nRST and HDMI.i_TMDS_nRST
	 */
	hdmi_reset();

	/**
	 * [SEQ 9] Wait for HDMI PHY ready (wait until 0xC0200020.[0], 1)
	 */
	if (false == hdmi_wait_phy_ready()) {
	    printf("%s: failed to wait for hdmiphy ready\n", __func__);
	    hdmi_phy_enable(preset, 0);
	    return -EIO;
	}
	/* set mux */
	NX_DISPLAYTOP_SetHDMIMUX(CTRUE, HDMI_SEL);

	/**
	 * [SEC 10] Set the DPC CLKGEN¡¯s Source Clock to HDMI_CLK & Set Sync Parameter
	 */
	hdmi_clock(); /* set hdmi link clk to clkgen  vs default is hdmi phy clk */

	/**
	 * [SEQ 11] Set up the HDMI Converter parameters
	 */
	hdmi_get_vsync(preset, psync, par);
	hdmi_setup(psync);
    return 0;
}

void display_hdmi(int module, int preset, unsigned int fbbase,
				struct disp_vsync_info *pvsync,
				struct disp_syncgen_param *psgen,
				struct disp_multily_param *pmly)
{
	int input = module == 0 ? DISP_DEVICE_SYNCGEN0 : DISP_DEVICE_SYNCGEN1;
	int layer  = pmly->fb_layer;

	switch(preset) {
	case 0:	pmly->x_resol = 1280, pmly->y_resol =  720;
			pvsync->h_active_len = 1280, pvsync->v_active_len = 720;
			break;
	case 1:	pmly->x_resol = 1920, pmly->y_resol = 1080;
			pvsync->h_active_len = 1920, pvsync->v_active_len = 1080;
			break;
	default:
		printf("hdmi not support preset %d\n", preset);
		return;
	}

	printf("HDMI: display.%d, preset %d (%4d * %4d)\n",
		module, preset, pmly->x_resol, pmly->y_resol);

	disp_initialize();
	disp_topctl_reset();
	disp_syncgen_reset();

	/* device init */
	disp_multily_init(module);	/* set BASE and P/BCLK (MLC)*/
	disp_syncgen_init(module);	/* set BASE and PCLK (DPC)*/
	disp_hdmi_init();

	disp_hdmi_setup(input, preset, pvsync, psgen);

	disp_multily_setup (module, pmly, fbbase);
	disp_multily_enable(module, layer, 1);

	disp_syncgen_setup (module, pvsync, psgen);
	disp_syncgen_enable(module, 1);

	disp_hdmi_enable(input, preset, pvsync, 1);
}
