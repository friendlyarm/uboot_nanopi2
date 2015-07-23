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

static void disp_rgb_init(void)
{
}

static void disp_rgb_enable(int enable)
{
}

static int disp_rgb_setup(int module, int input, struct disp_vsync_info *psync, struct disp_rgb_param *prgb)
{
	int mpu = prgb->lcd_mpu_type;
	int rsc = 0;
	int sel = 0;

	switch (module) {
	case 0:	sel = mpu ? 1 : 0; break;
	case 1:	sel = rsc ? 3 : 2; break;
	default:
		printf("Fail, %s nuknown module %d\n", __func__, module);
		return -1;
	}

	NX_DISPLAYTOP_SetPrimaryMUX(sel);
	return 0;
}

/*
 */
void display_rgb(int module, unsigned int fbbase,
				struct disp_vsync_info *pvsync,
				struct disp_syncgen_param *psgen,
				struct disp_multily_param *pmly,
				struct disp_rgb_param *prgb)
{
	int input = module == 0 ? DISP_DEVICE_SYNCGEN0 : DISP_DEVICE_SYNCGEN1;
	int layer  = pmly->fb_layer;
	printf("RGB: display.%d\n", module);

	disp_initialize();
	disp_topctl_reset();
	disp_syncgen_reset();

	/* device init */
	disp_multily_init(module);
	disp_syncgen_init(module);
	disp_rgb_init();

	/* set mlc top/rgb layer */
	disp_multily_setup (module, pmly, fbbase);
	disp_multily_enable(module, layer, 1);

	/* set display control  */
	disp_syncgen_setup (module, pvsync, psgen);
	disp_syncgen_enable(module, 1);

	/* set rgb control */
	disp_rgb_setup(module, input, pvsync, prgb);
	disp_rgb_enable(1);

	/* LCD Device Power On */
	#ifdef CFG_IO_LCD_PWR_ENB
	disp_lcd_device(CFG_IO_LCD_PWR_ENB);
	#endif
}


