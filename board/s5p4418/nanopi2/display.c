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
#include <asm/arch/mach-api.h>
#include <asm/arch/display.h>
#include <nxp-fb.h>

#define	INIT_VIDEO_SYNC(name)								\
	struct disp_vsync_info name = {							\
		.h_active_len	= CFG_DISP_PRI_RESOL_WIDTH,         \
		.h_sync_width	= CFG_DISP_PRI_HSYNC_SYNC_WIDTH,    \
		.h_back_porch	= CFG_DISP_PRI_HSYNC_BACK_PORCH,    \
		.h_front_porch	= CFG_DISP_PRI_HSYNC_FRONT_PORCH,   \
		.h_sync_invert	= CFG_DISP_PRI_HSYNC_ACTIVE_HIGH,   \
		.v_active_len	= CFG_DISP_PRI_RESOL_HEIGHT,        \
		.v_sync_width	= CFG_DISP_PRI_VSYNC_SYNC_WIDTH,    \
		.v_back_porch	= CFG_DISP_PRI_VSYNC_BACK_PORCH,    \
		.v_front_porch	= CFG_DISP_PRI_VSYNC_FRONT_PORCH,   \
		.v_sync_invert	= CFG_DISP_PRI_VSYNC_ACTIVE_HIGH,   \
		.pixel_clock_hz	= CFG_DISP_PRI_PIXEL_CLOCK,   		\
		.clk_src_lv0	= CFG_DISP_PRI_CLKGEN0_SOURCE,      \
		.clk_div_lv0	= CFG_DISP_PRI_CLKGEN0_DIV,         \
		.clk_src_lv1	= CFG_DISP_PRI_CLKGEN1_SOURCE,      \
		.clk_div_lv1	= CFG_DISP_PRI_CLKGEN1_DIV,         \
	};

#define	INIT_PARAM_SYNCGEN(name)						\
	struct disp_syncgen_param name = {						\
		.interlace 		= CFG_DISP_PRI_MLC_INTERLACE,       \
		.out_format		= CFG_DISP_PRI_OUT_FORMAT,          \
		.lcd_mpu_type 	= 0,                                \
		.invert_field 	= CFG_DISP_PRI_OUT_INVERT_FIELD,    \
		.swap_RB		= CFG_DISP_PRI_OUT_SWAPRB,          \
		.yc_order		= CFG_DISP_PRI_OUT_YCORDER,         \
		.delay_mask		= 0,                                \
		.vclk_select	= CFG_DISP_PRI_PADCLKSEL,           \
		.clk_delay_lv0	= CFG_DISP_PRI_CLKGEN0_DELAY,       \
		.clk_inv_lv0	= CFG_DISP_PRI_CLKGEN0_INVERT,      \
		.clk_delay_lv1	= CFG_DISP_PRI_CLKGEN1_DELAY,       \
		.clk_inv_lv1	= CFG_DISP_PRI_CLKGEN1_INVERT,      \
		.clk_sel_div1	= CFG_DISP_PRI_CLKSEL1_SELECT,		\
	};

#define	INIT_PARAM_MULTILY(name)					\
	struct disp_multily_param name = {						\
		.x_resol		= CFG_DISP_PRI_RESOL_WIDTH,			\
		.y_resol		= CFG_DISP_PRI_RESOL_HEIGHT,		\
		.pixel_byte		= CFG_DISP_PRI_SCREEN_PIXEL_BYTE,	\
		.fb_layer		= CFG_DISP_PRI_SCREEN_LAYER,		\
		.video_prior	= CFG_DISP_PRI_VIDEO_PRIORITY,		\
		.mem_lock_size	= 16,								\
		.rgb_format		= CFG_DISP_PRI_SCREEN_RGB_FORMAT,	\
		.bg_color		= CFG_DISP_PRI_BACK_GROUND_COLOR,	\
		.interlace		= CFG_DISP_PRI_MLC_INTERLACE,		\
	};

#define	INIT_PARAM_RGB(name)						\
	struct disp_rgb_param name = {							\
		.lcd_mpu_type	= 0,								\
	};

static void nxp_platform_disp_init(struct nxp_lcd *lcd,
		struct disp_vsync_info *vsync,
		struct disp_syncgen_param *syncgen,
		struct disp_multily_param *multily)
{
	struct nxp_lcd_timing *timing;
	u32 clk = 800000000;
	u32 div;

	if (lcd) {
		timing = &lcd->timing;

		vsync->h_active_len	= lcd->width;
		vsync->h_sync_width	= timing->h_sw;
		vsync->h_back_porch	= timing->h_bp;
		vsync->h_front_porch	= timing->h_fp;
		vsync->h_sync_invert	= !lcd->polarity.inv_hsync;

		vsync->v_active_len	= lcd->height;
		vsync->v_sync_width	= timing->v_sw;
		vsync->v_back_porch	= timing->v_bp;
		vsync->v_front_porch	= timing->v_fp;
		vsync->v_sync_invert	= !lcd->polarity.inv_vsync;

		/* calculates pixel clock */
		div  = timing->h_sw + timing->h_bp + timing->h_fp + lcd->width;
		div *= timing->v_sw + timing->v_bp + timing->v_fp + lcd->height;
		div *= lcd->freq ? : 60;
		clk /= div;

		vsync->pixel_clock_hz= div;
		vsync->clk_src_lv0	= CFG_DISP_PRI_CLKGEN0_SOURCE;
		vsync->clk_div_lv0	= clk;
		vsync->clk_src_lv1	= CFG_DISP_PRI_CLKGEN1_SOURCE;
		vsync->clk_div_lv1	= CFG_DISP_PRI_CLKGEN1_DIV;
		//vsync->clk_out_inv	= lcd->polarity.rise_vclk;

		if (lcd->gpio_init)
			lcd->gpio_init();

		multily->x_resol = lcd->width;
		multily->y_resol = lcd->height;
	}
}

static void bd_disp_rgb(void)
{
#if defined(CONFIG_DISPLAY_OUT_RGB)
	struct nxp_lcd *lcd = nanopi2_get_lcd();

	INIT_VIDEO_SYNC(vsync);
	INIT_PARAM_SYNCGEN(syncgen);
	INIT_PARAM_MULTILY(multily);
	INIT_PARAM_RGB(rgb);

	nxp_platform_disp_init(lcd, &vsync, &syncgen, &multily);

	display_rgb(CFG_DISP_OUTPUT_MODOLE, CONFIG_FB_ADDR,
			&vsync, &syncgen, &multily, &rgb);
	mdelay(50);

	printf("DISP: W=%4d, H=%4d, Bpp=%d\n", lcd->width, lcd->height,
			CFG_DISP_PRI_SCREEN_PIXEL_BYTE*8);
#endif
}

static void bd_disp_hdmi(void)
{
#if defined(CONFIG_DISPLAY_OUT_HDMI)
	struct nxp_lcd *lcd = nanopi2_get_lcd();
	int width = CFG_DISP_PRI_RESOL_WIDTH;
	int height = CFG_DISP_PRI_RESOL_HEIGHT;
	int preset = 0;

	INIT_VIDEO_SYNC(vsync);
	INIT_PARAM_SYNCGEN(syncgen);
	INIT_PARAM_MULTILY(multily);

	nxp_platform_disp_init(lcd, &vsync, &syncgen, &multily);
	width = lcd->width;
	height = lcd->height;

#define IS_720P(w, h)	((w) == 1280 && (h) ==  720)
#define IS_1080P(w, h)	((w) == 1920 && (h) == 1080)

	if (IS_720P(width, height))
		preset = 0;
	else if (IS_1080P(width, height))
		preset = 1;
	else
		printf("hdmi not support %dx%d\n", width, height);

	display_hdmi(CFG_DISP_OUTPUT_MODOLE, preset, CONFIG_FB_ADDR,
			&vsync, &syncgen, &multily);
#endif
}

int bd_display(void)
{
	const char *name = nanopi2_get_lcd_name();

	if (strncmp(name, "HDMI", 4) != 0) {
#if defined(CONFIG_DISPLAY_OUT_RGB)
		bd_disp_rgb();
#endif
	} else {
#if defined(CONFIG_DISPLAY_OUT_HDMI)
		bd_disp_hdmi();
#endif
	}

	return 0;
}
