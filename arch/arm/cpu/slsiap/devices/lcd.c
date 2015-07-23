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
#include <common.h>
#include <lcd.h>

/* nexell soc headers */
#include <platform.h>

#if	(1)
#define DBGOUT(msg...)		{ printf("lcd: " msg); }
#define	ERROUT(msg...)		{ printf("lcd: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#define	ERROUT(msg...)		do {} while (0)
#endif

DECLARE_GLOBAL_DATA_PTR;

/*------------------------------------------------------------------------------
 * u-boot lcd interface
 */
#ifdef	CONFIG_LCD

#define	LCD_WIDTH 		CFG_DISP_PRI_RESOL_WIDTH
#define	LCD_HEIGHT 		CFG_DISP_PRI_RESOL_HEIGHT
#define	LCD_BYTEPIXEL 	CFG_DISP_SCREEN_PIXEL_BYTE
#define LCD_BITPIXEL	(LCD_BYTEPIXEL * 8)

vidinfo_t panel_info = {
	vl_col : LCD_WIDTH,		/* LCD width */
	vl_row : LCD_HEIGHT,	/* LCD height */
	vl_bpix: 4,				/* LCD 2^vl_bpix = bitperpixel */
	NULL,
};

void  * lcd_base;
int 	lcd_line_length;
int 	lcd_color_fg;
int 	lcd_color_bg;

void  * lcd_console_address;	/* Start of console buffer	*/
short 	console_col 	= 0;
short 	console_row		= 0;

void lcd_ctrl_init (void *lcdbase)
{
	DBGOUT("%s\n", __func__);

	lcd_line_length = (LCD_WIDTH * LCD_BYTEPIXEL);
}

void lcd_setcolreg (ushort regno, ushort red, ushort green, ushort blue)
{
	DBGOUT("%s\n", __func__);
}

void lcd_enable (void)
{
	DBGOUT("%s\n", __func__);
}

/*
 * Calculate fb size for VIDEOLFB_ATAG. Size returned contains fb,
 * descriptors and palette areas.
 */
ulong calc_fbsize (void)
{
	ulong size;
	int line_length = (panel_info.vl_col * NBITS (panel_info.vl_bpix)) / 8;

	DBGOUT("%s\n", __func__);

	size = line_length * panel_info.vl_row;
	size += PAGE_SIZE;

	return size;
}

#ifdef CONFIG_LCD_INFO

#include <version.h>
#include <nand.h>
#include <flash.h>

void lcd_show_board_info(void)
{
	ulong dram_size, nand_size;
	int i;

	lcd_printf ("%s\n", U_BOOT_VERSION);
	lcd_printf ("(C) 2009 Nexell Co\n");
	lcd_printf ("jhkim@nexell.co.kr\n");

	dram_size = 0;
	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++)
		dram_size += gd->bd->bi_dram[i].size;

	nand_size = 0;
	for (i = 0; i < CONFIG_SYS_MAX_NAND_DEVICE; i++)
		nand_size += nand_info[i].size;

	lcd_printf ("%ld MB SDRAM, %ld MB NAND\n",
		dram_size >> 20,
		nand_size >> 20
		);
}
#endif /* CONFIG_LCD_INFO */

#endif /* CONFIG_LCD */
