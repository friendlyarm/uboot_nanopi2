/*------------------------------------------------------------------------------
 *
 *	Copyright (C) 2009 Nexell Co., Ltd All Rights Reserved
 *	Nexell Co. Proprietary & Confidential
 *
 *	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
 *  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
 *  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
 *  FOR A PARTICULAR PURPOSE.
 *
 *	Module     : System memory config
 *	Description:
 *	Author     : Platform Team
 *	Export     :
 *	History    :
 *	   2009/05/13 first implementation
 ------------------------------------------------------------------------------*/
#ifndef __CFG_MAIN_H__
#define __CFG_MAIN_H__

#include <nx_type.h>

//------------------------------------------------------------------------------
// PLL input crystal
//------------------------------------------------------------------------------
#define	CFG_SYS_PLLFIN		24000000UL

/*------------------------------------------------------------------------------
 * 	System Name
 */
#define	CFG_SYS_CPU_NAME						"s5p6818"
#define	CFG_SYS_BOARD_NAME						"s5p6818-svt"

/*------------------------------------------------------------------------------
 * 	Debug Uart
 */
#define CFG_UART_DEBUG_CH						0
#define CFG_UART_DEBUG_BAUDRATE					115200
#define CFG_UART_DEBUG_USE_UART					CTRUE
#define	CFG_UART_CLKGEN_CLOCK_HZ				14750000	// 50000000

/*------------------------------------------------------------------------------
 * 	Timer List
 */
#define	CFG_TIMER_SYS_TICK_CH					0

/*------------------------------------------------------------------------------
 * 	Extern Ethernet
 */
#define CFG_ETHER_EXT_PHY_BASEADDR          	0x04000000	// DM9000: CS1
#define	CFG_ETHER_EXT_IRQ_NUM					(IRQ_GPIO_C_START + 26)

/*------------------------------------------------------------------------------
 * 	GMAC PHY
 */
#define	CFG_ETHER_GMAC_PHY_IRQ_NUM				(IRQ_GPIO_A_START + 9)
#define	CFG_ETHER_GMAC_PHY_RST_NUM				(PAD_GPIO_A + 10)

/*------------------------------------------------------------------------------
 * 	Nand (HWECC)
 */
#define CFG_NAND_ECC_BYTES                      1024            /* 512 - 4,8,16,24  1024 - 24,40,60  */
#define CFG_NAND_ECC_BITS                       40

/* FTL */
#define CFG_NAND_FTL_START_BLOCK				0x6000000	/* byte address, Must Be Multiple of 8MB */
#define CFG_BOOTIMG_OFFSET						0x100000	/* uboot.ecc */
#define CFG_BOOTIMG_REPEAT						32

/*------------------------------------------------------------------------------
 *	Nand (GPIO)
 */
#define CFG_IO_NAND_nWP							(PAD_GPIO_C + 27)		/* GPIO */

/*------------------------------------------------------------------------------
 * 	Display (DPC and MLC)
 */
#define CFG_DISP_OUTPUT_MODOLE           		0	// 0 : Primary, 1 : Secondary

#define CFG_DISP_PRI_SCREEN_LAYER               0
#define CFG_DISP_PRI_SCREEN_RGB_FORMAT          MLC_RGBFMT_A8R8G8B8
#define CFG_DISP_PRI_SCREEN_PIXEL_BYTE	        4
#define CFG_DISP_PRI_SCREEN_COLOR_KEY	        0x090909

#define CFG_DISP_PRI_VIDEO_PRIORITY				2	// 0, 1, 2, 3
#define CFG_DISP_PRI_BACK_GROUND_COLOR	     	0x0

#define CFG_DISP_PRI_MLC_INTERLACE              CFALSE

#define CFG_DISP_PRI_RESOL_WIDTH          		1024	// X Resolution
#define CFG_DISP_PRI_RESOL_HEIGHT				 600	// Y Resolution

#define CFG_DISP_PRI_HSYNC_SYNC_WIDTH           20
#define CFG_DISP_PRI_HSYNC_BACK_PORCH           140
#define CFG_DISP_PRI_HSYNC_FRONT_PORCH          160
#define CFG_DISP_PRI_HSYNC_ACTIVE_HIGH          CTRUE
#define CFG_DISP_PRI_VSYNC_SYNC_WIDTH           3
#define CFG_DISP_PRI_VSYNC_BACK_PORCH           20
#define CFG_DISP_PRI_VSYNC_FRONT_PORCH          12
#define CFG_DISP_PRI_VSYNC_ACTIVE_HIGH 	        CTRUE

#define CFG_DISP_PRI_CLKGEN0_SOURCE             DPC_VCLK_SRC_PLL0
#define CFG_DISP_PRI_CLKGEN0_DIV                16
#define CFG_DISP_PRI_CLKGEN0_DELAY              0
#define CFG_DISP_PRI_CLKGEN0_INVERT				0
#define CFG_DISP_PRI_CLKGEN1_SOURCE             DPC_VCLK_SRC_VCLK2
#define CFG_DISP_PRI_CLKGEN1_DIV                1
#define CFG_DISP_PRI_CLKGEN1_DELAY              0
#define CFG_DISP_PRI_CLKGEN1_INVERT				0
#define CFG_DISP_PRI_CLKSEL1_SELECT				0
#define CFG_DISP_PRI_PADCLKSEL                  DPC_PADCLKSEL_VCLK	/* VCLK=CLKGEN1, VCLK12=CLKGEN0 */

#define	CFG_DISP_PRI_PIXEL_CLOCK				80000000

#define	CFG_DISP_PRI_OUT_SWAPRB 				CFALSE
#define CFG_DISP_PRI_OUT_FORMAT                 DPC_FORMAT_RGB666
#define CFG_DISP_PRI_OUT_YCORDER                DPC_YCORDER_CbYCrY
#define CFG_DISP_PRI_OUT_INTERLACE              CFALSE
#define CFG_DISP_PRI_OUT_INVERT_FIELD           CFALSE

/*------------------------------------------------------------------------------
 * 	LVDS
 */
#define CFG_DISP_LVDS_LCD_FORMAT             	LVDS_LCDFORMAT_JEIDA

/*------------------------------------------------------------------------------
 * 	I2C
 */

/*------------------------------------------------------------------------------
 *  SPI
 */
#define CFG_SPI0_SRC_CLK                            100*1000*1000
#define CFG_SPI0_OUT_CLK                            20*1000*1000

#define CFG_SPI1_SRC_CLK                            100*1000*1000
#define CFG_SPI1_OUT_CLK                            30*1000*1000

#define CFG_SPI2_SRC_CLK                            100*1000*1000
#define CFG_SPI2_OUT_CLK                            30*1000*1000


/*------------------------------------------------------------------------------
 * 	TIMER/PWM
 */
#define CFG_LCD_PRI_PWM_CH                      0
#define CFG_LCD_PRI_PWM_FREQ                    10000
#define CFG_LCD_PRI_PWM_DUTYCYCLE               50      /* (%) */

//------------------------------------------------------------------------------
// Static Bus #0 ~ #9, NAND, IDE configuration
//------------------------------------------------------------------------------
//	_BW	  : Staic Bus width for Static #0 ~ #9            : 8 or 16
//
//	_TACS : adress setup time before chip select          : 0 ~ 15
//	_TCOS : chip select setup time before nOE is asserted : 0 ~ 15
//	_TACC : access cycle                                  : 1 ~ 256
//	_TSACC: burst access cycle for Static #0 ~ #9 & IDE   : 1 ~ 256
//	_TOCH : chip select hold time after nOE not asserted  : 0 ~ 15
//	_TCAH : address hold time after nCS is not asserted   : 0 ~ 15
//
//	_WAITMODE : wait enable control for Static #0 ~ #9 & IDE : 1=disable, 2=Active High, 3=Active Low
//	_WBURST	  : burst write mode for Static #0 ~ #9          : 0=disable, 1=4byte, 2=8byte, 3=16byte
//	_RBURST   : burst  read mode for Static #0 ~ #9          : 0=disable, 1=4byte, 2=8byte, 3=16byte
//
//------------------------------------------------------------------------------
#define CFG_SYS_STATICBUS_CONFIG( _name_, bw, tACS, tCOS, tACC, tSACC, tCOH, tCAH, wm, rb, wb )	\
	enum {											\
		CFG_SYS_ ## _name_ ## _BW		= bw,		\
		CFG_SYS_ ## _name_ ## _TACS		= tACS,		\
		CFG_SYS_ ## _name_ ## _TCOS		= tCOS,		\
		CFG_SYS_ ## _name_ ## _TACC		= tACC,		\
		CFG_SYS_ ## _name_ ## _TSACC	= tSACC,	\
		CFG_SYS_ ## _name_ ## _TCOH		= tCOH,		\
		CFG_SYS_ ## _name_ ## _TCAH		= tCAH,		\
		CFG_SYS_ ## _name_ ## _WAITMODE	= wm, 		\
		CFG_SYS_ ## _name_ ## _RBURST	= rb, 		\
		CFG_SYS_ ## _name_ ## _WBURST	= wb		\
	};

//                      ( _name_ , bw, tACS tCOS tACC tSACC tOCH tCAH, wm, rb, wb )
CFG_SYS_STATICBUS_CONFIG( STATIC0 ,  8,    1,   1,   6,    6,   1,   1,  1,  0,  0 )		// 0x0000_0000
CFG_SYS_STATICBUS_CONFIG( STATIC1 ,  8,    6,   6,  32,   32,   6,   6,  1,  0,  0 )		// 0x0400_0000
CFG_SYS_STATICBUS_CONFIG(    NAND ,  8,    0,   0xf,0x3f,  1, 0xf,   0,  1,  0,  0 )		// 0x2C00_0000, tOCH, tCAH must be greter than 0


#endif /* __CFG_MAIN_H__ */
