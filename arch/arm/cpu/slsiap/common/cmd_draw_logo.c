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
#include <platform.h>

/*------------------------------------------------------------------------------
 */
#if (0)
#define DBGOUT(msg...)		{ printf("LOGO: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

#if (0)
#ifndef U32
typedef unsigned int 	U32;
#endif	/* U32 */
#ifndef U16
typedef unsigned short 	U16;
#endif	/* U16 */
#ifndef U8
typedef unsigned char 	U8;
#endif	/* U8  */
#endif

#ifndef BOOL
#define	BOOL	int
#define	TRUE	1
#define	FALSE	0
#endif

//#ifndef BITMAPFILEHEADER
typedef struct tagBITMAPFILEHEADER {
// 	U16 	bfType;
  	U32   	bfSize;
  	U16 	bfReserved1;
	U16 	bfReserved2;
  	U32 	bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;
//#endif

//#ifndef BITMAPINFOHEADER
typedef struct tagBITMAPINFOHEADER {
  	U32 		biSize;
  	U32 		biWidth;
  	U32 		biHeight;
  	U16 		biPlanes;
  	U16			biBitCount;
  	U32 		biCompression;
  	U32 		biSizeImage;
  	U32 		biXPelsPerMeter;
  	U32 		biYPelsPerMeter;
  	U32 		biClrUsed;
  	U32 		biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;
//#endif

//#ifndef RGBQUAD
typedef struct tagRGBQUAD {
	U8 		rgbBlue;
	U8 		rgbGreen;
	U8 		rgbRed;
	U8 		rgbReserved;
} RGBQUAD, *PRGBQUAD;
//#endif

#ifndef EdbgOutputDebugString
#define	EdbgOutputDebugString	printf
#endif

static U8 dither_pattern6[4][2][2] = {
	{ {1, 0}, {0, 0} },
	{ {1, 0}, {0, 1} },
	{ {1, 1}, {0, 1} },
	{ {1, 1}, {1, 1} },
};

static U8 dither_pattern5[8][3][3] = {
	{ {0, 0, 0}, {0, 1, 0}, {0, 0, 0} },
	{ {0, 0, 0}, {0, 1, 0}, {1, 0, 0} },
	{ {1, 0, 0}, {0, 1, 0}, {1, 0, 0} },
	{ {1, 0, 1}, {0, 1, 0}, {1, 0, 0} },
	{ {1, 0, 1}, {0, 1, 0}, {1, 0, 1} },
	{ {1, 0, 1}, {1, 1, 0}, {1, 0, 1} },
	{ {1, 1, 1}, {1, 1, 0}, {1, 0, 1} },
	{ {1, 1, 1}, {1, 1, 0}, {1, 1, 1} }
};

#define	RGB555TO565(col) 	(((col>>10)&0x1F) << 11) | (((col>> 5)&0x1F) << 6) | ((col<< 0)&0x1F)

static void
PutPixel555To565(
		U32  base,
		int  xpos,
		int  ypos,
		int  width,
		int  height,
		U32  color
		)
{
	*(U16*)((ulong)(base + (ypos * width + xpos) * 2)) = (U16)RGB555TO565(color);
}

/*
static void
PutPixel565To565(
		U32  base,
		int  xpos,
		int  ypos,
		int  width,
		int  height,
		U32  color
		)
{
	*(U16*)(base + (ypos * width + xpos) * 2) = (U16)color & 0xFFFF;
}
*/

static void
PutPixel565To888(
		U32  base,
		int  xpos,
		int  ypos,
		int  width,
		int  height,
		U32  color
		)
{
	*(U8*)((ulong)(base + (ypos * width + xpos) * 3 + 0)) =
		(((color >> 0 ) << 3) & 0xf8) | (((color >> 0 ) >> 2) & 0x7);	// B
	*(U8*)((ulong)(base + (ypos * width + xpos) * 3 + 1)) =
		(((color >> 5 ) << 2) & 0xfc) | (((color >> 5 ) >> 4) & 0x3);	// G
	*(U8*)((ulong)(base + (ypos * width + xpos) * 3 + 2)) =
		(((color >> 11) << 3) & 0xf8) | (((color >> 11) >> 2) & 0x7);	// R
}

static void
PutPixel565To8888(
		U32  base,
		int  xpos,
		int  ypos,
		int  width,
		int  height,
		U32  color
		)
{
	*(U8*)((ulong)(base + (ypos * width + xpos) * 4 + 0)) =
		(((color >> 0 ) << 3) & 0xf8) | (((color >> 0 ) >> 2) & 0x7);	// B
	*(U8*)((ulong)(base + (ypos * width + xpos) * 4 + 1)) =
		(((color >> 5 ) << 2) & 0xfc) | (((color >> 5 ) >> 4) & 0x3);	// G
	*(U8*)((ulong)(base + (ypos * width + xpos) * 4 + 2)) =
		(((color >> 11) << 3) & 0xf8) | (((color >> 11) >> 2) & 0x7);	// R
	*(U8*)((ulong)(base + (ypos * width + xpos) * 4 + 3)) = 0;	// Alpha
}

#define	RGB888TO565(col) 	((((col>>16)&0xFF)&0xF8)<<8) | ((((col>>8)&0xFF)&0xFC)<<3) | ((((col>>0 )&0xFF)&0xF8)>>3)

static void
PutPixel888To565(
		U32  base,
		int  xpos,
		int  ypos,
		int  width,
		int  height,
		U32  color
		)
{
	*(U16*)((ulong)(base + (ypos * width + xpos) * 2)) = (U16)RGB888TO565(color);
}

static void
PutPixel888To888(
		U32  base,
		int  xpos,
		int  ypos,
		int  width,
		int  height,
		U32  color
		)
{
	base = base + (ypos * width + xpos) * 3;
	*(U8*)((ulong)(base++)) = ((color>> 0)&0xFF);	// B
	*(U8*)((ulong)(base++)) = ((color>> 8)&0xFF);	// G
	*(U8*)((ulong)(base))   = ((color>>16)&0xFF);	// R
}

static void
PutPixel888To8888(
		U32  base,
		int  xpos,
		int  ypos,
		int  width,
		int  height,
		U32  color
		)
{
	*(U32*)((ulong)(base + (ypos * width + xpos) * 4)) = (0xFF000000) | (color & 0xFFFFFF);
}

static void (*PUTPIXELTABLE[])(U32, int, int, int, int, U32) =
{
	PutPixel555To565,
	PutPixel565To888,
	PutPixel565To8888,
	PutPixel888To565,
	PutPixel888To888,
	PutPixel888To8888,
};

//------------------------------------------------------------------------------
static unsigned int _bmp_base   = 0;
static unsigned int _lcd_width  = 0;
static unsigned int _lcd_height = 0;
static unsigned int _lcd_bpp    = 0;

unsigned int logo_get_logo_bmp_addr(void)
{
	return _bmp_base;
}

void lcd_set_logo_bmp_addr(unsigned int base)
{
	_bmp_base = base;
}

#define	BMP_BASE	logo_get_logo_bmp_addr()
static void fill_lcd(U32 FrameBase, int XResol, int YResol, U32 PixelByte);

void lcd_draw_boot_logo(unsigned int framebase, int x_resol, int y_resol, int pixelbyte)
{
	U32	 			  BMPBase  = BMP_BASE;
	BITMAPFILEHEADER  BMPFile  = { 0, };
	BITMAPINFOHEADER  BMPInfo  = { 0, };

	U8 *pBitMap  = NULL;
	int BMPPixelByte;

	int bmpsx, bmpsy, bmpex, bmpey;
	int lcdsx, lcdsy;
	int lx, ly, bx, by;

	U8 *pPixel;
	U32 Color;
	U16 BMPID = BMPBase ? *(U16*)((ulong)BMPBase) : 0;
	U32 BMP_Align;

	void (*PutPixel)(U32, int, int, int, int, U32) = NULL;

	_lcd_width  = x_resol;
	_lcd_height = y_resol;
	_lcd_bpp    = pixelbyte * 8;

	// Check logo file type.
	if (BMPID != 0x4D42) {
		if (BMPBase)
			printf("can't find bmp at 0x%x (type:0x%x), fb:0x%x...\n",
				BMPBase, BMPID, framebase);

		if (0 >= x_resol ||	0 >= y_resol ||
			0 >= pixelbyte)
			return;

		fill_lcd(framebase, x_resol, y_resol, pixelbyte);
		flush_dcache_all();
		return;
	}

	// Get BMP header
	memcpy((void*)&BMPFile, (const void*)(BMPBase + sizeof(BMPID)), sizeof(BITMAPFILEHEADER));

	// Get BMP info
	memcpy ((void*)&BMPInfo,
			(const void*)(BMPBase + sizeof(BMPID) + sizeof(BITMAPFILEHEADER)),
			sizeof(BITMAPINFOHEADER));

	DBGOUT("\nBMP File Heade \r\n");
	DBGOUT("Type	: 0x%x \r\n", BMPID);
	DBGOUT("Size	: %d   \r\n", BMPFile.bfSize);
	DBGOUT("Offs	: %d   \r\n", BMPFile.bfOffBits);
	DBGOUT("\nBMP Info Header  \r\n");
	DBGOUT("Size     : %d\r\n", BMPInfo.biSize);
	DBGOUT("Width    : %d\r\n", BMPInfo.biWidth);
	DBGOUT("Height   : %d\r\n", BMPInfo.biHeight);
	DBGOUT("Planes   : %d\r\n", BMPInfo.biPlanes);
	DBGOUT("BitCount : %d\r\n", BMPInfo.biBitCount);
	DBGOUT("Compress : %d\r\n", BMPInfo.biCompression);
	DBGOUT("SizeImage: %d\r\n", BMPInfo.biSizeImage);
	DBGOUT("XPels    : %d\r\n", BMPInfo.biXPelsPerMeter);
	DBGOUT("YPels    : %d\r\n", BMPInfo.biYPelsPerMeter);
	DBGOUT("ClrUsed  : %d\r\n", BMPInfo.biClrUsed);
	DBGOUT("ClrImport: %d\r\n", BMPInfo.biClrImportant);
	DBGOUT("\r\n");

	BMPPixelByte = BMPInfo.biBitCount/8;

	pBitMap = (U8*)((ulong)(BMPBase + BMPFile.bfOffBits));	// BMP file end point.
	lcdsx   = 0, lcdsy = 0;
	bmpsx   = 0, bmpsy = 0, bmpex = BMPInfo.biWidth-1, bmpey = BMPInfo.biHeight-1;

	if ( ( (BMPBase <= framebase) && ((BMPBase + BMPFile.bfSize) >= framebase) ) || (
		 ( (BMPBase <= (framebase + (BMPInfo.biWidth * BMPInfo.biHeight * BMPPixelByte))) ) &&
		 ( (BMPBase + BMPFile.bfSize) >= (framebase + (BMPInfo.biWidth * BMPInfo.biHeight * BMPPixelByte)) ) )
		) {
		 printf("BMP posiotion 0x%08x is overwrite FB position 0x%08x (size=%d)\n",
		 	BMPBase, framebase, BMPInfo.biWidth * BMPInfo.biHeight * BMPPixelByte);
	}

	printf("DONE: Logo bmp %d by %d (%dbpp), len=%d \r\n",
		BMPInfo.biWidth, BMPInfo.biHeight, BMPPixelByte, BMPFile.bfSize);
	printf("DRAW: 0x%08x -> 0x%08x \r\n", BMP_BASE, framebase);

	if (BMPInfo.biWidth  > x_resol) {
		bmpsx = (BMPInfo.biWidth - x_resol)/2;
		bmpex = bmpsx + x_resol;
	} else if (BMPInfo.biWidth  < x_resol) {
		lcdsx += (x_resol- BMPInfo.biWidth)/2;
	}

	if (BMPInfo.biHeight > y_resol) {
		bmpsy = (BMPInfo.biHeight - y_resol)/2;
		bmpey = bmpsy + y_resol;
	} else if (BMPInfo.biHeight < y_resol) {
		lcdsy += (y_resol- BMPInfo.biHeight)/2;
	}

	// Select put pixel function
	//
	switch(BMPPixelByte)
	{
	case 2:	PutPixel = PUTPIXELTABLE[0 + pixelbyte-2];	break;	// 565 To 565/888
	case 3: PutPixel = PUTPIXELTABLE[3 + pixelbyte-2];	break;	// 888 To 565/888
	default:
		printf("\nNot support BitPerPixel (%d) ...\r\n", BMPPixelByte);
		return;
	}

	/*
	 * BMP stride ,
	 * a line size of BMP must be aligned on DWORD boundary.
	 */
	BMP_Align = (((BMPInfo.biWidth*3)+3)/4)*4;

	// Draw 16 BitperPixel image on the frame buffer base.
	// RGB555
	if (BMPPixelByte == 2 && BMPInfo.biCompression == 0x00000000) {
		for(ly = lcdsy, by = bmpey; by>=bmpsy; ly++, by--) {
			for(lx = lcdsx, bx = bmpsx; bx<=bmpex; lx++, bx++) {
				Color = *(U16*)(pBitMap + (by * BMP_Align) + (bx * BMPPixelByte));
				Color = (U16)RGB555TO565(Color);
				PutPixel(framebase, lx, ly, x_resol, y_resol, Color);
			}
		}
	}

	// Draw 16 BitperPixel image on the frame buffer base.
	// RGB565
	if (BMPPixelByte == 2 && BMPInfo.biCompression == 0x00000003) {
		for(ly = lcdsy, by = bmpey; by>=bmpsy; ly++, by--) {
			for(lx = lcdsx, bx = bmpsx; bx<=bmpex; lx++, bx++) {
				Color = *(U16*)(pBitMap + (by * BMP_Align) + (bx * BMPPixelByte));
				PutPixel(framebase, lx, ly, x_resol, y_resol, Color);
			}
		}
	}

	// Draw 24 BitperPixel image on the frame buffer base.
	//
	if (BMPPixelByte == 3) {
		U32	RColor, GColor, BColor;
		for(ly = lcdsy, by = bmpey; by>=bmpsy; ly++, by--) {
			for(lx = lcdsx, bx = bmpsx; bx<=bmpex; lx++, bx++) {
				pPixel = (U8*)(pBitMap + (by * BMP_Align) + (bx * BMPPixelByte));
				RColor  = *(pPixel+2);
				GColor  = *(pPixel+1);
				BColor  = *(pPixel+0);
				if (2 == pixelbyte) {
					RColor  = dither_pattern5[RColor & 0x7][lx%3][ly%3] + (RColor>>3);	RColor= (RColor>31) ? 31: RColor;
					GColor  = dither_pattern6[GColor & 0x3][lx%2][ly%2] + (GColor>>2);	GColor= (GColor>63) ? 63: GColor;
					BColor  = dither_pattern5[BColor & 0x7][lx%3][ly%3] + (BColor>>3);	BColor= (BColor>31) ? 31: BColor;
					Color	= (RColor<<11) | (GColor<<5) | (BColor);
					*(U16*)((ulong)(framebase + (ly * x_resol + lx) * 2)) = (U16)Color;
				} else {
					Color = ((RColor&0xFF)<<16) | ((GColor&0xFF)<<8) | (BColor&0xFF);
					PutPixel(framebase, lx, ly, x_resol, y_resol, Color);
				}
			}
		}
	}

	flush_dcache_all();
}

// Draw Color Bar
//
#if (1)
static void fill_lcd(U32 FrameBase, int XResol, int YResol, U32 PixelByte)
{
	void (*PutPixel)(U32, int, int, int, int, U32) = NULL;

	int sx, sy, ex, ey, x, y;
	int pxl, num, col, div, dep, dec;
	U8  R0, G0, B0, R, G, B;
	U32 RGB;
	U32 EgColor;

	printf("LOGO: DRAW FB=0x%08x, X=%4d, Y=%4d, Bpp=%d\n",
		FrameBase, XResol, YResol, PixelByte*8);

	EgColor = 0xFFFFFF;

	col = 8;		// colorbar count.
	dep = XResol > 256 ? 256 : XResol;		// gratation depth.

	div = (YResol/col);
	dec = 256/dep;

	sx  = 0;
	ex  = XResol;
	sy  = (YResol%col)/2;
	ey  = (YResol/col)*col + sy;

	pxl = (XResol/dep);

	if((XResol%dep) != 0)
		pxl += 1;

	// 888To565 or 888To888
	PutPixel = PUTPIXELTABLE[3 + PixelByte - 2];

	for (y=sy; y<ey; y++) {
		switch (y/div) {
		case 0:	R = 0xFF, G = 0xFF, B = 0xFF; break;	// White
		case 1:	R = 0xFF, G = 0x00, B = 0x00; break;	// Red
		case 2:	R = 0x00, G = 0xFF, B = 0x00; break;	// Green
		case 3:	R = 0x00, G = 0x00, B = 0xFF; break;	// Blue
		case 4:	R = 0xFF, G = 0xFF, B = 0x00; break;	// RG
		case 5:	R = 0x00, G = 0xFF, B = 0xFF; break;	// GB
		case 6:	R = 0xFF, G = 0x00, B = 0xFF; break;	// RB
		case 7:	R = 0x00, G = 0x00, B = 0x00; break;	// Black
		default:
			return;
		}

		// Separate line
		if (0 == y%div || 1 == y%div) {
			for(x=0 ; x<XResol; x++) {
			RGB = 0x0;	// RGB888
			PutPixel(FrameBase, x, y, XResol, YResol, RGB);
			}
			continue;
		}

		R0  = R;
		G0  = G;
		B0  = B;
		num = pxl;

		// Gratation color bar
		for (x=sx ; x<ex; x++, num--) {
			if (0 == num) {
				if (R0==0xFF) R -= dec;
				if (G0==0xFF) G -= dec;
				if (B0==0xFF) B -= dec;
				num = pxl;
			}
			RGB = (U32)(R<<16 | G<<8 | B);	// RGB888
			PutPixel(FrameBase, x, y, XResol, YResol, RGB);
		}
	}

	/* draw left and right edge */
	for (x = 0; XResol > x; x++) {
		PutPixel(FrameBase, x, 0, XResol, YResol, EgColor);
		PutPixel(FrameBase, x, (YResol-1), XResol, YResol, EgColor);
	}

	/* draw top and bottom edge */
	for (y = 0; YResol > y; y++) {
		PutPixel(FrameBase, 0, y, XResol, YResol, EgColor);
		PutPixel(FrameBase, (XResol-1), y, XResol, YResol, EgColor);
	}
}
#else
static void fill_lcd(U32 FrameBase, int XResol, int YResol, U32 PixelByte)
{
	void (*PutPixel)(U32, int, int, int, int, U32) = NULL;

	int x = 0, y = 0;
	U32 BgColor, EgColor;

	BgColor = 0x00FF00;
	EgColor = 0xFF0000;

	// 888To565 or 888To888
	PutPixel = PUTPIXELTABLE[3 + PixelByte - 2];

	printf("LOGO: DRAW FB=0x%08x, X=%4d, Y=%4d, Bpp=%d\n",
		FrameBase, XResol, YResol, PixelByte*8);


	/* clear */
	for (y = 0; YResol > y; y++)
	for (x = 0; XResol > x; x++)
		PutPixel(FrameBase, x, y, XResol, YResol, BgColor);

	/* draw left and right edge */
	for (x = 0; XResol > x; x++) {
		PutPixel(FrameBase, x, 0, XResol, YResol, EgColor);
		PutPixel(FrameBase, x, (YResol-1), XResol, YResol, EgColor);
	}
	for (y = 0; YResol > y; y++) {
		PutPixel(FrameBase, 0, y, XResol, YResol, EgColor);
		PutPixel(FrameBase, 1, y, XResol, YResol, EgColor);
		PutPixel(FrameBase, (XResol-1), y, XResol, YResol, EgColor);
		PutPixel(FrameBase, (XResol-2), y, XResol, YResol, EgColor);
	}

	/* draw top and bottom edge */
	for (y = 0; YResol > y; y++) {
		PutPixel(FrameBase, 0, y, XResol, YResol, EgColor);
		PutPixel(FrameBase, 1, y, XResol, YResol, EgColor);
		PutPixel(FrameBase, (XResol-1), y, XResol, YResol, EgColor);
		PutPixel(FrameBase, (XResol-2), y, XResol, YResol, EgColor);
	}

//	for (y = 0; YResol > y; y++)
//		PutPixel(FrameBase, y, y, XResol, YResol, EgColor);

}
#endif


int do_drawbmp (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int bmp_base, fb_base;
	unsigned int x_resol, y_resol;
	int pixelbyte;

	if (2 != argc) {
		cmd_usage(cmdtp);
		return 1;
	}

	bmp_base = simple_strtol(argv[1], NULL, 16);
	lcd_set_logo_bmp_addr(bmp_base);

	fb_base   = IO_ADDRESS(CONFIG_FB_ADDR);
	x_resol	  = _lcd_width;
    y_resol   = _lcd_height;
	pixelbyte = _lcd_bpp/8;

	lcd_draw_boot_logo(fb_base, x_resol, y_resol, pixelbyte);
	return 0;
}

U_BOOT_CMD(
	drawbmp, 3, 1,	do_drawbmp,
	"darw bmpfile on address 'addr' to framebuffer ",
	"addr\n"
	"    - bmpfile on address 'addr'\n"
);
