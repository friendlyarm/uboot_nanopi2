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
#include <platform.h>
#include <asm/arch/display.h>

#if (0)
#define DBGOUT(msg...)		{ printf("BD: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

#ifndef printk
#define	printk		printf
#define	KERN_ERR
#define	KERN_INFO
#endif

#define	MLC_LAYER_RGB_0			0	/* number of RGB layer 0 */
#define	MLC_LAYER_RGB_1			1	/* number of RGB layer 1 */
#define	MLC_LAYER_VIDEO			3	/* number of Video layer: 3 = VIDEO */

extern void disp_lcd_device(int io);
extern void disp_initialize(void);
extern void disp_topctl_reset(void);
extern void disp_syncgen_reset(void);
extern void disp_syncgen_init(int module);
extern void disp_syncgen_enable(int module, int enable);
extern int  disp_syncgen_setup(int module, struct disp_vsync_info *psync, struct disp_syncgen_param *par);
extern void disp_multily_init(int module);
extern void disp_multily_enable(int module, int layer, int enable);
extern int disp_multily_setup(int module, struct disp_multily_param *par, unsigned int fbbase);
