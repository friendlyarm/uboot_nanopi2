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
#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "mach-api.h"

struct disp_vsync_info {
	int	interlace;
	int	h_active_len;
	int	h_sync_width;
	int	h_back_porch;
	int	h_front_porch;
	int	h_sync_invert;		/* default active low */
	int	v_active_len;
	int	v_sync_width;
	int	v_back_porch;
	int	v_front_porch;
	int	v_sync_invert;		/* default active low */
	int pixel_clock_hz;		/* HZ */
	/* clock gen */
	int clk_src_lv0;
	int clk_div_lv0;
	int clk_src_lv1;
	int clk_div_lv1;
};

/* syncgen control (DPC) */
#define	DISP_SYNCGEN_DELAY_RGB_PVD		(1<<0)
#define	DISP_SYNCGEN_DELAY_HSYNC_CP1	(1<<1)
#define	DISP_SYNCGEN_DELAY_VSYNC_FRAM	(1<<2)
#define	DISP_SYNCGEN_DELAY_DE_CP		(1<<3)

struct disp_syncgen_param {
	/* scan format */
	int 		 interlace;
	/* syncgen format */
	unsigned int out_format;
	int 		 lcd_mpu_type;		/* set when lcd type is mpu */
	int			 invert_field;		/* 0= Normal Field(Low is odd field), 1: Invert Field(low is even field) */
	int			 swap_RB;
	unsigned int yc_order;			/* for CCIR output */
	/* exten sync delay  */
	int			delay_mask;			/* if 0, set defalut delays (rgb_pvd, hsync_cp1, vsync_fram, de_cp2 */
	int 		d_rgb_pvd;			/* the delay value for RGB/PVD signal   , 0 ~ 16, default  0 */
	int			d_hsync_cp1;		/* the delay value for HSYNC/CP1 signal , 0 ~ 63, default 12 */
	int			d_vsync_fram;		/* the delay value for VSYNC/FRAM signal, 0 ~ 63, default 12 */
	int			d_de_cp2;			/* the delay value for DE/CP2 signal    , 0 ~ 63, default 12 */
	/* exten sync delay  */
	int			vs_start_offset;	/* start veritcal sync offset, defatult 0 */
	int			vs_end_offset;		/* end veritcla sync offset  , defatult 0 */
	int			ev_start_offset;	/* start even veritcal sync offset, defatult 0 */
	int			ev_end_offset;		/* end even veritcal sync offset  , defatult 0 */
	/* pad clock seletor */
	int			vclk_select;		/* 0=vclk0, 1=vclk2 */
	int			clk_inv_lv0;		/* OUTCLKINVn */
	int			clk_delay_lv0;		/* OUTCLKDELAYn */
	int			clk_inv_lv1;		/* OUTCLKINVn */
	int			clk_delay_lv1;		/* OUTCLKDELAYn */
	int			clk_sel_div1;		/* 0=clk1_inv, 1=clk1_div_2_ns */
};

/*
 * multilayer control (MLC)
 */
struct disp_multily_param {
	int 		 x_start;
	int 		 y_start;
	int			 x_resol;
	int			 y_resol;
	int 		 pixel_byte;
	unsigned int rgb_format;
	int 		 fb_layer;
	int			 video_prior;		/* 0 = video > RGB0.., 1 = RGB0 > vidoe > RGB1 .., 2 = RGB0 > RGB1 > vidoe .. */
	int			 interlace;
	unsigned int bg_color;
	unsigned int mem_lock_size;		/* lock size for memory access, 4, 8, 16 only valid (default 8byter) */
};

/*
 *	display device number
 */
enum {
	DISP_DEVICE_RESCONV		= 0,
	DISP_DEVICE_LCDIF		= 1,
	DISP_DEVICE_HDMI  		= 2,
	DISP_DEVICE_MIPI  		= 3,
	DISP_DEVICE_LVDS  		= 4,
	DISP_DEVICE_SYNCGEN0	= 5,
	DISP_DEVICE_SYNCGEN1	= 6,
	DISP_DEVICE_END			,
};

enum {
	DISP_CLOCK_RESCONV		= 0,
	DISP_CLOCK_LCDIF		= 1,
	DISP_CLOCK_MIPI  		= 2,
	DISP_CLOCK_LVDS  		= 3,
	DISP_CLOCK_HDMI  		= 4,
	DISP_CLOCK_END			,
};

/*
 * LVDS
 */
#define	LVDS_PCLK_L_MIN	 	 40000000
#define	LVDS_PCLK_L_MAX	 	 80000000
#define	LVDS_PCLK_H_MIN	 	 80000000
#define	LVDS_PCLK_H_MAX		160000000

struct disp_lvds_param {
	unsigned int lcd_format;		/* 0:VESA, 1:JEIDA, 2: Location Setting */
	int			 inv_hsync;			/* hsync polarity invert for VESA, JEIDA */
	int			 inv_vsync;			/* bsync polarity invert for VESA, JEIDA */
	int			 inv_de;			/* de polarity invert for VESA, JEIDA */
	int			 inv_inclk_pol;		/* input clock(pixel clock) polarity invert */
	/* Location settting */
	unsigned int loc_map[9];		/* when lcd format is "Location Setting", LCDn = 8bit * 35 = 35byte := 9dword */
	unsigned int loc_mask[2];		/* when lcd format is "Location Setting", 0 ~ 34 */
	unsigned int loc_pol[2];		/* when lcd format is "Location Setting", 0 ~ 34 */
};

/*
 * MIPI
 */
struct disp_mipi_param {
	unsigned int pllpms;	/* Use LN28LPP_MipiDphyCore1p5Gbps_Supplement. */
	unsigned int bandctl;	/* [3:0] Use LN28LPP_MipiDphyCore1p5Gbps_Supplement. */
	unsigned int pllctl;
	unsigned int phyctl;	/* Refer to 10.2.3 M_PLLCTL of MIPI_D_PHY_USER_GUIDE.pdf or NX_MIPI_PHY_B_DPHYCTL enum or LN28LPP_MipiDphyCore1p5Gbps_Supplement. */
	int	(*lcd_init)	(int width, int height, void *private_data);
	int	(*lcd_exit)	(int width, int height, void *private_data);
	void *private_data;
};

/*
 * RGB
 */
struct disp_rgb_param {
	int lcd_mpu_type;
};

/*
 * display APIs
 */
#if defined(CONFIG_DISPLAY_OUT_LVDS)
extern void display_lvds(int module, unsigned int fbbase,
				struct disp_vsync_info *pvsync, struct disp_syncgen_param *psgen,
				struct disp_multily_param *pmly, struct disp_lvds_param *plvds);
#endif

#if defined(CONFIG_DISPLAY_OUT_RGB)
extern void display_rgb(int module, unsigned int fbbase,
				struct disp_vsync_info *pvsync, struct disp_syncgen_param *psgen,
				struct disp_multily_param *pmly, struct disp_rgb_param *prgb);
#endif

#if defined(CONFIG_DISPLAY_OUT_HDMI)
extern void display_hdmi(int module, int preset, unsigned int fbbase,
				struct disp_vsync_info *pvsync, struct disp_syncgen_param *psgen,
				struct disp_multily_param *pmly);
#endif

#if defined(CONFIG_DISPLAY_OUT_MIPI)
extern void display_mipi(int module, unsigned int fbbase,
				struct disp_vsync_info *pvsync, struct disp_syncgen_param *psgen,
				struct disp_multily_param *pmly, struct disp_mipi_param *pmipi);
#endif

extern int disp_mlc_set_enable(int module, int layer, int on);
extern int disp_mlc_set_address(int module, int layer, unsigned int address);
extern int disp_mlc_wait_vsync(int module, int layer, int fps);

#endif /* __DISPLAY_H__ */
