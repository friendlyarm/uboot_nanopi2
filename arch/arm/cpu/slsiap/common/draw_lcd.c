/*
 * (C) Copyright 2009 Nexell Co.,
 * jung hyun kim<jhkim@nexell.co.kr>
 *
 * Configuation settings for the Nexell board.
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
#include <stdarg.h>
#include <malloc.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <asm/errno.h>
#include <platform.h>

#include "draw_lcd.h"
#include "font8x16.h"

#ifdef __cplusplus
extern "C" {
#endif

static void	lcd_draw_text_eng0816(char *string, int x, int y, int hscale, int vscale, int alpha);

static void (*DRAWTEXTFONT[])(char *, int, int, int, int, int) = {
	lcd_draw_text_eng0816,
};

#define RGB565_ALPHA_MASK	0xF7DE
#define	RGB888TO565(c)	((((c>>16)&0xFF)&0xF8)<<8) | 	\
		((((c>>8)&0xFF)&0xFC)<<3) | ((((c>>0 )&0xFF)&0xF8)>>3)

static void PutPixel888To565(unsigned int base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	*(U16*)((ulong)(base + (ypos * width + xpos) * 2)) = (U16)RGB888TO565(color);
}

static void PutPixel888To888(unsigned int base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	base = base + (ypos * width + xpos) * 3;
	*(U8*)((ulong)(base++)) = ((color>> 0)&0xFF);	// B
	*(U8*)((ulong)(base++)) = ((color>> 8)&0xFF);	// G
	*(U8*)((ulong)(base))   = ((color>>16)&0xFF);	// R
}

static void PutPixel888To8888(unsigned int base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	*(U32*)((ulong)(base + (ypos * width + xpos) * 4)) = (0xFF000000) | (color & 0xFFFFFF);
}

static void ALPHAPixel888To565(unsigned int base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	*(unsigned short*)((ulong)(base + (ypos * width + xpos) * 2)) =
		(*(unsigned short*)((ulong)(base + (ypos * width + xpos) * 2)) & RGB565_ALPHA_MASK>>1)  |
		(unsigned short)RGB888TO565(color);
}

static void ALPHAPixel888To888(unsigned int base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	base = base + (ypos * width + xpos) * 3;
	*(U8*)((ulong)(base)) = (*(U8*)((ulong)(base))>>1) | ((color>> 0)&0xFF);	base++;	// B
	*(U8*)((ulong)(base)) = (*(U8*)((ulong)(base))>>1) | ((color>> 8)&0xFF);	base++;	// G
	*(U8*)((ulong)(base)) = (*(U8*)((ulong)(base))>>1) | ((color>>16)&0xFF);			// R
}

static void ALPHAPixel888To8888(unsigned int base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	base = base + (ypos * width + xpos) * 4;
	*(unsigned int*)((ulong)base) = (0xFF000000) | (color & 0xFFFFFF) |
					(*(unsigned int*)((ulong)base));
}

static void (*PUTPIXELTABLE[])(U32, int, int, int, int, U32) =
{
	PutPixel888To565,
	PutPixel888To888,
	PutPixel888To8888,
};

static void (*ALPHAPIXELTABLE[])(U32, int, int, int, int, U32) =
{
	ALPHAPixel888To565,
	ALPHAPixel888To888,
	ALPHAPixel888To8888,
};


/*
 * Draw Debug message to LCD
 */
typedef struct st_msg_info {
	int h_start;
	int h_end;
	int h_pos;
	int v_start;
	int v_end;
	int v_pos;
	/* message */
	int raws;
	int cols;
	int s_raw;
	int e_raw;
	int colum;
	int line;
	int font_x_len;
	int font_y_len;
	char **msg_buffer;
	/* for debug string */
	unsigned char **font_type;
	/* for draw text */
	void (*draw_font)(char *, int, int, int, int, int);
} msg_info;

static lcd_info st_lcd_info;
static msg_info	st_msg_info;

#define _ALIGN_R(x, a)	(((x+a-1)/a)*a)
#define _ALIGN_L(x, a)	((x/a)*a)

static void	lcd_draw_text_eng0816(char *string, int x, int y, int hscale, int vscale, int alpha)
{
	lcd_info *plcd = &st_lcd_info;
	int xpos = x, ypos = y, pixelbyte = plcd->bit_per_pixel/8;
	unsigned char *	pfont = (unsigned char *)&font8x16[(*string & 0xff)][0] ;
	unsigned char	bitmask = 0;

	int	len = strlen((char *)string);
	int	i, w, h;
	int xsh, ysh;

	void (*draw_pixel)(unsigned int, int, int, int, int, unsigned int)
		= draw_pixel = PUTPIXELTABLE[pixelbyte-2];
	unsigned int color;

	if (!hscale) hscale = 1;
	if (!vscale) vscale = 1;

	if (alpha)
		draw_pixel = ALPHAPIXELTABLE[pixelbyte-2];

	xsh = 1<<(hscale-1);
	ysh = 1<<(vscale-1);
	w = xpos + ( 8 * hscale);
	h = ypos + (16 * vscale);

	if (w > plcd->lcd_width)
		w = plcd->lcd_width;

	if (h > plcd->lcd_height)
		h = plcd->lcd_height;

	for (i = 0; i < len; i++) {
		pfont = (unsigned char *)&font8x16[(*string++ & 0xff)][0] ;
		for (y = ypos; y < h; y++ ) {
			bitmask = *pfont;
			for (x = xpos; x < w; x++) {
				color = bitmask & 0x80 ? plcd->text_color : plcd->back_color;
				draw_pixel(plcd->fb_base, x, y, plcd->lcd_width, plcd->lcd_height, color);

				// Horizontal scale
				bitmask <<= (xsh&0x1);
				xsh >>= 1;
				if (!xsh)
					xsh = 1<<(hscale-1);
			}

			// Vertical scale
			ysh >>= 1;
			if (!ysh) {
				ysh = 1<<(vscale-1);
				pfont++;
			}
		}
		xpos += (8 * hscale);
		w = xpos + (8 * hscale);
	}
	flush_dcache_all();
}

int lcd_debug_init(lcd_info *lcd)
{
	lcd_info *plcd = &st_lcd_info;
	msg_info *pmsg = &st_msg_info;
	int fx, fy, rows, cols;
	int left, top, right, bottom;

	switch(lcd->font) {
	case eng_8x16 : fx =  8, fy = 16;
					pmsg->font_type = (unsigned char**)font8x16;
					pmsg->draw_font = DRAWTEXTFONT[0];
					break;
	default: printf("Error: Unknown font type !!!\n");
		return -EINVAL;
	}
	rows = (lcd->lcd_width /fx);
	cols = (lcd->lcd_height/fy);

#ifdef CONFIG_LCD_DEBUG_STRING
	if (pmsg->msg_buffer)
		free(pmsg->msg_buffer);

	pmsg->msg_buffer = malloc(rows * cols);
	if (! pmsg->msg_buffer) {
		printf("Fail : malloc %d !!!\n", rows * cols);
		return -EINVAL;
	}
#endif

	/* lcd */
	plcd->fb_base = lcd->fb_base;
	plcd->bit_per_pixel = lcd->bit_per_pixel;
	plcd->lcd_width = lcd->lcd_width;
	plcd->lcd_height = lcd->lcd_height;
	plcd->back_color = lcd->back_color;
	plcd->text_color = lcd->text_color;
	plcd->alphablend = lcd->alphablend;

	/* message window */
	left   = _ALIGN_R((lcd->dbg_win_left ? : 0), fx);
	top    = _ALIGN_R((lcd->dbg_win_top  ? : 0), fy);
	right  = _ALIGN_L((lcd->dbg_win_width  ? lcd->dbg_win_left + lcd->dbg_win_width : lcd->lcd_width ), fx);
	bottom = _ALIGN_L((lcd->dbg_win_height ? lcd->dbg_win_top  + lcd->dbg_win_height: lcd->lcd_height), fy);

	pmsg->h_start = left;
	pmsg->v_start = top;
	pmsg->h_end = right;
	pmsg->v_end = bottom;
	pmsg->h_pos = pmsg->h_start;
	pmsg->v_pos = pmsg->v_start;

	pmsg->raws = rows;
	pmsg->cols = cols;
	pmsg->s_raw = 0;
	pmsg->e_raw = 0;
	pmsg->colum = 0;
	pmsg->line = cols;
	pmsg->font_x_len = fx;
	pmsg->font_y_len = fy;

	return 0;
}

/*
 * Draw to LCD
 */
void lcd_set_back_color(unsigned int color)
{
	lcd_info *plcd = &st_lcd_info;
	plcd->back_color = color;
}

void lcd_set_alphablend(int mode)
{
	lcd_info *plcd = &st_lcd_info;
	plcd->alphablend = mode;
}

void lcd_set_text_color(unsigned int color)
{
	lcd_info *plcd = &st_lcd_info;
	plcd->text_color = color;
}

void lcd_line_horizontal(int sx, int sy, int width, unsigned int color, bool alpha)
{
	lcd_info *plcd = &st_lcd_info;
    int pixelbyte = plcd->bit_per_pixel/8;
    int x;

	void (*draw_pixel)(unsigned int, int, int, int, int, unsigned int)
		= PUTPIXELTABLE[pixelbyte-2];

	if (alpha)
		draw_pixel = ALPHAPIXELTABLE[pixelbyte-2];

    for (x = sx; x < (sx + width); x++)
		draw_pixel(plcd->fb_base, x, sy, plcd->lcd_width, plcd->lcd_height, color);

	flush_dcache_all();
}

void lcd_line_vertical(int sy, int sx, int height, unsigned int color, bool alpha)
{
	lcd_info *plcd = &st_lcd_info;
    int pixelbyte = plcd->bit_per_pixel/8;
    int y;

	void (*draw_pixel)(unsigned int, int, int, int, int, unsigned int)
		= PUTPIXELTABLE[pixelbyte-2];

	if (alpha)
		draw_pixel = ALPHAPIXELTABLE[pixelbyte-2];

    for (y = sy; y < (sy + height); y++)
        draw_pixel(plcd->fb_base, sx, y, plcd->lcd_width, plcd->lcd_height, color);

	flush_dcache_all();
}

void lcd_fill_rectangle(int left, int top, int width, int height, unsigned int color, bool alpha)
{
	lcd_info *plcd = &st_lcd_info;
	int pixelbyte = plcd->bit_per_pixel/8;
	int right = left + width;
	int bottom = top + height;
	int xend = right;
	int yend = bottom;
	int x = left, y = top;

	void (*draw_pixel)(unsigned int, int, int, int, int, unsigned int)
		= PUTPIXELTABLE[pixelbyte-2];

	if (alpha)
		draw_pixel = ALPHAPIXELTABLE[pixelbyte-2];

    for (y = top; y < yend; y++)
   	for (x = left; x < xend; x++)
   		draw_pixel(plcd->fb_base, x, y, plcd->lcd_width, plcd->lcd_height, color);

   	flush_dcache_all();
}

int lcd_draw_text(char *string, int x, int y, int hscale, int vscale, bool alpha)
{
	msg_info *pmsg = &st_msg_info;
	pmsg->draw_font(string, x, y, hscale, vscale, (int)alpha);
	return 0;
}

int lcd_draw_string(int x, int y, int hscale, int vscale, bool alpha, const char *fmt, ...)
{
	msg_info *pmsg = &st_msg_info;
	char buf[128] = { 0, };
	char *p = buf;
	ulong l;
    unsigned char c;
	const char *s;

    va_list vl;
    va_start(vl, fmt);

    while (*fmt) {
        c = (unsigned char)*fmt++;
        switch (c) {
            case (unsigned char)'%':
            	c = (unsigned char)*fmt++;
            	switch (c) {
                	case 'x':
                		l  = va_arg(vl, long);
                		p += sprintf(p, "%x", (unsigned int)l);
	                	break;
                	case 'X':
                		l  = va_arg(vl, long);
                		p += sprintf(p, "%X", (unsigned int)l);
	                	break;
                	case 'd':
                		l  = va_arg(vl, long);
                		p += sprintf(p, "%d", (int)l);
	                	break;
               		case 'u':
                		l  = va_arg(vl, long);
                		p += sprintf(p, "%u", (unsigned int)l);
	                	break;
					case 's':
						s = va_arg(vl, char *);
    					while (*s) {
    					    if (*s == '\n') *p++ = '\r';
    					    *p++ = *s++;
    					}
                		break;
        			case '%':
                		*p++ = '%';
                		break;
        			case 'c':
                		*p++ = va_arg(vl, int);
                		break;
        			default:
                		*p++ = ' ';
                		break;
        		}
            	break;
            case '\n':
            	*p++ = '\r';
            default:
            	*p++ = c;
        }
    }
    va_end(vl);

	pmsg->draw_font(buf, x, y, hscale, vscale, (int)alpha);
	return 0;
}

/*
 * string print
 */
#ifdef CONFIG_LCD_DEBUG_STRING
#define	_INCPOS_(p, l)	{ p++; if ((int)p > (l-1)) p=0;}
#define	_DECPOS_(p, l)	{ p--; if ((int)p < 0) p = (l-1);}

static void lcd_print_msg_buffer(void)
{
	lcd_info *plcd = &st_lcd_info;
	msg_info *pmsg = &st_msg_info;
	char *pstring;
	int y = pmsg->v_start;
	int tail = pmsg->e_raw;
	int head = pmsg->s_raw;
	char (*msgs)[pmsg->cols] = (char(*)[pmsg->cols])pmsg->msg_buffer;
	int count = 0;

	// clear screen
	lcd_fill_rectangle(pmsg->h_start, pmsg->v_start,
		(pmsg->h_start + pmsg->h_end), (pmsg->v_start + pmsg->v_end),
		plcd->back_color, plcd->alphablend);

	while(1) {
		_INCPOS_(tail, pmsg->line);
		if (tail == head)
			break;

		pstring = &msgs[tail][0];

		pmsg->draw_font(pstring, pmsg->h_start, y, 1, 1, (int)plcd->alphablend);
		y += pmsg->font_y_len;
		count++;
	}
	_INCPOS_(pmsg->e_raw, pmsg->line);
}

void lcd_putc(const char c)
{
	lcd_info *plcd = &st_lcd_info;
	msg_info *pmsg = &st_msg_info;
	unsigned char (*mfont)[16] = (unsigned char(*)[16])pmsg->font_type;
	unsigned char *	pfont = (unsigned char *)&mfont[(c & 0xff)][0];
	int pixelbyte = plcd->bit_per_pixel/8;

	int w = pmsg->font_x_len;
	int h = pmsg->font_y_len;
	int x, y, xpos, ypos;
	unsigned char bitmask = 0;
	char (*msgs)[pmsg->cols] = (char(*)[pmsg->cols])pmsg->msg_buffer;

	void (*draw_pixel)(unsigned int, int, int, int, int, unsigned int)
		= PUTPIXELTABLE[pixelbyte-2];
	unsigned int color;

	if (plcd->alphablend)
		draw_pixel = ALPHAPIXELTABLE[pixelbyte-2];

	// save message
	msgs[pmsg->s_raw][pmsg->colum] = c;

	if (c == '\n') {
		msgs[pmsg->s_raw][pmsg->colum] = 0;
		pmsg->colum = 0;
		pmsg->v_pos += pmsg->font_y_len;	// next vertical start
		pmsg->h_pos  = pmsg->h_start;		// next horizontal start

		_INCPOS_(pmsg->s_raw, pmsg->line);

		if (pmsg->v_pos >= pmsg->v_end) {
			pmsg->v_pos = pmsg->v_end-pmsg->font_y_len;
			lcd_print_msg_buffer();
		}
	}

	xpos = pmsg->h_pos;
	ypos = pmsg->v_pos;
	w += xpos;
	h += ypos;

	if (w > plcd->lcd_width)
		w = plcd->lcd_width;

	if (h > plcd->lcd_height)
		h = plcd->lcd_height;

	for (y = ypos; y < h; y++) {
		bitmask = *pfont++;
		for (x = xpos; x < w; x++) {
			color = bitmask & 0x80 ? plcd->text_color : plcd->back_color;
			draw_pixel(plcd->fb_base, x, y, plcd->lcd_width, plcd->lcd_height, color);
			bitmask <<= 1;
		}
	}

	if (c != '\n') {
		pmsg->colum++;
		pmsg->h_pos += pmsg->font_x_len;
	}

	if (pmsg->h_pos >= pmsg->h_end) {
		pmsg->colum = 0;
		pmsg->v_pos += pmsg->font_y_len;	// next vertical start
		pmsg->h_pos  = pmsg->h_start;		// next horizontal start

		_INCPOS_(pmsg->s_raw, pmsg->line);

		if (pmsg->v_pos >= pmsg->v_end) {
			pmsg->v_pos = pmsg->v_end-pmsg->font_y_len;
			lcd_print_msg_buffer();
		}
	}
}

void lcd_puts(const char *s)
{
	while (*s)
		lcd_putc(*s++);
}

static void lcd_out_hex(unsigned long n, long depth)
{
    if (depth)
        depth--;

    if ((n & ~0xf) || depth) {
        lcd_out_hex(n >> 4, depth);
        n &= 0xf;
    }

    if (n < 10)
        lcd_putc((unsigned char)(n + '0'));
    else
	    lcd_putc((unsigned char)(n - 10 + 'A'));

}

static void lcd_out_decimal (unsigned long n)
{
    if (n >= 10) {
        lcd_out_decimal(n / 10);
        n %= 10;
    }
    lcd_putc((unsigned char)(n + '0'));
}

static void lcd_out_string(const char *s)
{
    while (*s) {
        if (*s == '\n')
            lcd_putc('\r');
        lcd_putc(*s++);
    }
}

void lcd_debug_string(const char *sz, ...)
{
    unsigned char c;
    va_list vl;
    va_start(vl, sz);

    while (*sz) {
        c = (unsigned char)*sz++;
        switch (c) {
            case (unsigned char)'%':
            	c = (unsigned char)*sz++;
            	switch (c) {
                	case 'x':
                	lcd_out_hex(va_arg(vl, unsigned long), 0);
                	break;
                	case 'B':
                	lcd_out_hex(va_arg(vl, unsigned long), 2);
                	break;
                	case 'H':
                	lcd_out_hex(va_arg(vl, unsigned long), 4);
                	break;
                	case 'X':
                	lcd_out_hex(va_arg(vl, unsigned long), 8);
                	break;
                	case 'd':
                		{
                		long l;
                		l = va_arg(vl, long);
                		if (l < 0) {
	                    	lcd_putc('-');
    	                	l = - l;
        	        	}
            	    	lcd_out_decimal((unsigned long)l);
		            	}
        		       	break;
					case 'u':
                		lcd_out_decimal(va_arg(vl, unsigned long));
                		break;
					case 's':
                		lcd_out_string(va_arg(vl, char *));
                		break;
        			case '%':
                		lcd_putc('%');
                		break;
        			case 'c':
                		c = va_arg(vl, int);
                		lcd_putc(c);
                		break;
        			default:
                		lcd_putc(' ');
                		break;
        		}
            	break;
            case '\n':
            	lcd_putc('\r');
            	// fall through
            default:
            	lcd_putc(c);
        }
    }
    va_end(vl);
}
#endif

#ifdef __cplusplus
};
#endif