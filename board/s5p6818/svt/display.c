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
#include <asm/arch/display.h>

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

#define	MIPI_BITRATE_750M

#ifdef MIPI_BITRATE_1G
#define	PLLPMS		0x33E8
#define	BANDCTL		0xF
#elif defined(MIPI_BITRATE_900M)
#define	PLLPMS		0x2258
#define	BANDCTL		0xE
#elif defined(MIPI_BITRATE_840M)
#define	PLLPMS		0x2230
#define	BANDCTL		0xD
#elif defined(MIPI_BITRATE_750M)
#define	PLLPMS		0x43E8
#define	BANDCTL		0xC
#elif defined(MIPI_BITRATE_660M)
#define	PLLPMS		0x21B8
#define	BANDCTL		0xB
#elif defined(MIPI_BITRATE_600M)
#define	PLLPMS		0x2190
#define	BANDCTL		0xA
#elif defined(MIPI_BITRATE_540M)
#define	PLLPMS		0x2168
#define	BANDCTL		0x9
#elif defined(MIPI_BITRATE_512M)
#define	PLLPMS		0x03200
#define	BANDCTL		0x9
#elif defined(MIPI_BITRATE_480M)
#define	PLLPMS		0x2281
#define	BANDCTL		0x8
#elif defined(MIPI_BITRATE_420M)
#define	PLLPMS		0x2231
#define	BANDCTL		0x7
#elif defined(MIPI_BITRATE_402M)
#define	PLLPMS		0x2219
#define	BANDCTL		0x7
#elif defined(MIPI_BITRATE_330M)
#define	PLLPMS		0x21B9
#define	BANDCTL		0x6
#elif defined(MIPI_BITRATE_300M)
#define	PLLPMS		0x2191
#define	BANDCTL		0x5
#elif defined(MIPI_BITRATE_210M)
#define	PLLPMS		0x2232
#define	BANDCTL		0x4
#elif defined(MIPI_BITRATE_180M)
#define	PLLPMS		0x21E2
#define	BANDCTL		0x3
#elif defined(MIPI_BITRATE_150M)
#define	PLLPMS		0x2192
#define	BANDCTL		0x2
#elif defined(MIPI_BITRATE_100M)
#define	PLLPMS		0x3323
#define	BANDCTL		0x1
#elif defined(MIPI_BITRATE_80M)
#define	PLLPMS		0x3283
#define	BANDCTL		0x0
#endif

#define	PLLCTL		0
#define	DPHYCTL		0

#define MIPI_DELAY 0xFF
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct data_val{
	u8 data[48];
};

struct mipi_reg_val{
	u32 cmd;
	u32 addr;
	u32 cnt;
	struct data_val data;
};

static struct mipi_reg_val mipi_init_data[]=
{
 {0x15, 0xB2,    1, {0x7D,}},
 {0x15, 0xAE,    1, {0x0B,}},
 {0x15, 0xB6,    1, {0x18,}},
 {0x15, 0xD2,    1, {0x64,}},
};

static void  mipilcd_dcs_long_write(U32 cmd, U32 ByteCount, U8* pByteData )
{
	U32 DataCount32 = (ByteCount+3)/4;
	int i = 0;
	U32 index = 0;
	volatile NX_MIPI_RegisterSet* pmipi = (volatile NX_MIPI_RegisterSet*)IO_ADDRESS(NX_MIPI_GetPhysicalAddress(index));

	NX_ASSERT( 512 >= DataCount32 );

#if 0
	printf("0x%02x %2d: ", cmd, ByteCount);
	for(i=0; i< ByteCount; i++)
		printf("%02x ", pByteData[i]);
	printf("\n");
#endif
	for( i=0; i<DataCount32; i++ )
	{
		pmipi->DSIM_PAYLOAD = (pByteData[3]<<24)|(pByteData[2]<<16)|(pByteData[1]<<8)|pByteData[0];
		pByteData += 4;
	}

	pmipi->DSIM_PKTHDR  = (cmd & 0xff) | (ByteCount<<8);
}

static void mipilcd_dcs_write( unsigned int id, unsigned int data0, unsigned int data1 )
{
	U32 index = 0;
	volatile NX_MIPI_RegisterSet* pmipi = (volatile NX_MIPI_RegisterSet*)IO_ADDRESS(NX_MIPI_GetPhysicalAddress(index));

#if 0
	switch(id)
	{
		case 0x05:
			printf("0x05  1: %02x \n", data0);
			break;

		case 0x13:
			printf("0x13  2: %02x %02x \n", data0, data1);
			break;

		case 0x15:
			printf("0x15  2: %02x %02x \n", data0, data1);
			break;
	}
#endif

	pmipi->DSIM_PKTHDR = id | (data0<<8) | (data1<<16);
}

static int MIPI_LCD_INIT(int width, int height, void *data)
{
	int i=0;
	int size=ARRAY_SIZE(mipi_init_data);
	u32 index = 0;
	u32 value = 0;
	u8 pByteData[48];
	u8 bitrate=BANDCTL;

	volatile NX_MIPI_RegisterSet* pmipi = (volatile NX_MIPI_RegisterSet*)IO_ADDRESS(NX_MIPI_GetPhysicalAddress(index));
	value = pmipi->DSIM_ESCMODE;
	pmipi->DSIM_ESCMODE = value|(3 << 6);
	value = pmipi->DSIM_ESCMODE;
	printf("DSIM_ESCMODE : 0x%x\n", value);
	switch(bitrate)
	{
		case 0xF:	printf("MIPI clk: 1000MHz \n");	break;
		case 0xE:	printf("MIPI clk:  900MHz \n");	break;
		case 0xD:	printf("MIPI clk:  840MHz \n");	break;
		case 0xC:	printf("MIPI clk:  760MHz \n");	break;
		case 0xB:	printf("MIPI clk:  660MHz \n");	break;
		case 0xA:	printf("MIPI clk:  600MHz \n");	break;
		case 0x9:	printf("MIPI clk:  540MHz \n");	break;
		case 0x8:	printf("MIPI clk:  480MHz \n");	break;
		case 0x7:	printf("MIPI clk:  420MHz \n");	break;
		case 0x6:	printf("MIPI clk:  330MHz \n");	break;
		case 0x5:	printf("MIPI clk:  300MHz \n");	break;
		case 0x4:	printf("MIPI clk:  210MHz \n");	break;
		case 0x3:	printf("MIPI clk:  180MHz \n");	break;
		case 0x2:	printf("MIPI clk:  150MHz \n");	break;
		case 0x1:	printf("MIPI clk:  100MHz \n");	break;
		case 0x0:	printf("MIPI clk:   80MHz \n");	break;
		default :	printf("MIPI clk:  unknown \n");	break;
	}

	mdelay(10);

	for(i=0; i<size; i++)
	{
		switch(mipi_init_data[i].cmd)
		{
#if 0 // all long packet
			case 0x05:
				//pByteData[0] = mipi_init_data[i].addr;
				//memcpy(&pByteData[1], &mipi_init_data[i].data.data[0], 7);
				mipilcd_dcs_long_write(0x39, mipi_init_data[i].cnt, &mipi_init_data[i].data.data[0]);
				break;
			case 0x15:
				pByteData[0] = mipi_init_data[i].addr;
				memcpy(&pByteData[1], &mipi_init_data[i].data.data[0], 7);
				mipilcd_dcs_long_write(0x39, mipi_init_data[i].cnt+1, &pByteData);
				break;
#else
			case 0x05:
				mipilcd_dcs_write(mipi_init_data[i].cmd, mipi_init_data[i].data.data[0], 0x00);
				break;

			case 0x13:
				mipilcd_dcs_write(mipi_init_data[i].cmd, mipi_init_data[i].addr, mipi_init_data[i].data.data[0]);
				break;

			case 0x15:
				mipilcd_dcs_write(mipi_init_data[i].cmd, mipi_init_data[i].addr, mipi_init_data[i].data.data[0]);
				break;
#endif
 			case 0x39:
				pByteData[0] = mipi_init_data[i].addr;
				memcpy(&pByteData[1], &mipi_init_data[i].data.data[0], 48);
				mipilcd_dcs_long_write(mipi_init_data[i].cmd, mipi_init_data[i].cnt+1, &pByteData[0]);
				break;
			case MIPI_DELAY:
				//printf("delay %d\n", mipi_init_data[i].addr);
				mdelay(mipi_init_data[i].addr);
				break;
		}
		mdelay(1);
	}

	value = pmipi->DSIM_ESCMODE;
	pmipi->DSIM_ESCMODE = value&(~(3 << 6));
	value = pmipi->DSIM_ESCMODE;
	printf("DSIM_ESCMODE : 0x%x\n", value);

	mdelay(10);
	return 0;
}

#endif

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

#define	INIT_PARAM_LVDS(name)							\
	struct disp_lvds_param name = {							\
		.lcd_format 	= CFG_DISP_LVDS_LCD_FORMAT,         \
	};

#define	INIT_PARAM_RGB(name)							\
	struct disp_rgb_param name = {							\
		.lcd_mpu_type 	= 0,                                \
	};

#define	INIT_PARAM_MIPI(name)	\
	struct disp_mipi_param name = {	\
		.pllpms 	= PLLPMS,       \
		.bandctl	= BANDCTL,      \
		.pllctl		= PLLCTL,    	\
		.phyctl		= DPHYCTL,      \
		.lcd_init	= MIPI_LCD_INIT	\
	};

int bd_display(void)
{
	INIT_VIDEO_SYNC(vsync);
	INIT_PARAM_SYNCGEN(syncgen);
	INIT_PARAM_MULTILY(multily);

#if defined(CONFIG_DISPLAY_OUT_LVDS)
	INIT_PARAM_LVDS(lvds);
	display_lvds(CFG_DISP_OUTPUT_MODOLE, CONFIG_FB_ADDR,
		&vsync, &syncgen, &multily, &lvds);
#endif

#if defined(CONFIG_DISPLAY_OUT_RGB)
	INIT_PARAM_RGB(rgb);

	display_rgb(CFG_DISP_OUTPUT_MODOLE, CONFIG_FB_ADDR,
		&vsync, &syncgen, &multily, &rgb);
#endif

#if defined(CONFIG_DISPLAY_OUT_MIPI)
	INIT_PARAM_MIPI(mipi);

	/*
	 * set multilayer parameters
	 */
	multily.x_resol =  800;
	multily.y_resol = 1280;

	/*
	 * set vsync parameters
	 */
	vsync.h_active_len =  800;
	vsync.v_active_len = 1280;
	vsync.h_sync_width = 8;
	vsync.h_back_porch = 40;
	vsync.h_front_porch = 16;
	vsync.v_sync_width = 1;
	vsync.v_back_porch = 2;
	vsync.v_front_porch = 4;

	/*
	 * set syncgen parameters
	 */
	syncgen.delay_mask = DISP_SYNCGEN_DELAY_RGB_PVD | DISP_SYNCGEN_DELAY_HSYNC_CP1 |
						  	DISP_SYNCGEN_DELAY_VSYNC_FRAM | DISP_SYNCGEN_DELAY_DE_CP;

	syncgen.d_rgb_pvd = 0;
	syncgen.d_hsync_cp1	= 0;
	syncgen.d_vsync_fram = 0;
	syncgen.d_de_cp2 = 7;
	syncgen.vs_start_offset = (vsync.h_front_porch + vsync.h_sync_width +
								vsync.h_back_porch + vsync.h_active_len - 1);
	syncgen.ev_start_offset = (vsync.h_front_porch + vsync.h_sync_width +
								vsync.h_back_porch + vsync.h_active_len - 1);
	syncgen.vs_end_offset = 0;
	syncgen.ev_end_offset = 0;

	lcd_draw_boot_logo(CONFIG_FB_ADDR, multily.x_resol, multily.y_resol, multily.pixel_byte);

	display_mipi(CFG_DISP_OUTPUT_MODOLE, CONFIG_FB_ADDR,
		&vsync, &syncgen, &multily, &mipi);
#endif
	return 0;
}
