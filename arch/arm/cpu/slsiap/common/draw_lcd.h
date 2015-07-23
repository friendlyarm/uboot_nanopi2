//------------------------------------------------------------------------------
//
//	Copyright (C) 2005 MagicEyes Digital Co., Ltd All Rights Reserved
//	MagicEyes Digital Co. Proprietary & Confidential
//
//	MAGICEYES INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT
//  NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR
//  A PARTICULAR PURPOSE.
//
//	Author     : Rohan (rohan@mesdigital.com)
//	History    :
//	   2008/07/18 rohan first implementation
//------------------------------------------------------------------------------

#ifndef _LCDDRAW_H_
#define _LCDDRAW_H_

#define	COLOR_OPAQUE		0
#define	COLOR_HALFALPHA		1

enum lcd_fonts {
	eng_8x16,
};

typedef struct st_lcd_info {
	unsigned int fb_base;
	int	bit_per_pixel;
	int	lcd_width;
	int	lcd_height;
	unsigned int back_color;
	unsigned int text_color;
	unsigned int line_count;
	int h_scale_lv;
	int v_scale_lv;
	int dbg_win_left;
	int dbg_win_width;
	int dbg_win_top;
	int dbg_win_height;
	enum lcd_fonts font;
	int alphablend;
} lcd_info;

/* Initialize display information. */
int		lcd_debug_init(lcd_info *lcd);

/*
 * Draw debug message functions
 * note> not support format %nd ex>. %3d
 *		 only support format %d type
 */
void 	lcd_debug_string(const char *fmt, ...);

/* Draw text functions. */
void	lcd_set_text_color(unsigned int color);
void	lcd_set_back_color(unsigned int color);
void 	lcd_set_transparency (int mode);
int		lcd_draw_text(char *string, int x, int y, int hscale, int vscale, bool alpha);
int		lcd_draw_string(int x, int y, int hscale, int vscale, bool alpha, const char *fmt, ...);

// Draw functions.
void 	lcd_line_horizontal(int sx, int sy, int width, unsigned int color, bool alpha);
void 	lcd_line_vertical(int sy, int sx, int height, unsigned int color, bool alpha);
void 	lcd_fill_rectangle(int left, int top, int width, int height, unsigned int color, bool alpha);


#endif // _LCDDRAW_H_