/*
 * (C) Copyright 2010
 * sangjong han, Nexell Co, <hans@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __NX_OTG_HS_H__
#define __NX_OTG_HS_H__

#define CTRUE	1							///< true value is  integer one
#define CFALSE	0							///< false value is  integer zero

#define VENDORID	USBD_VID		// Nexell Vendor ID
#define PRODUCTID	USBD_PID		// NXP3xx0 Product ID

#define BASEADDR_BOOTSTATUS			(BASEADDR_SRAM+(INTERNAL_SRAM_SIZE/2))


#define	HIGH_USB_VER				0x0200	// 2.0
#define	HIGH_MAX_PKT_SIZE_EP0		64
#define	HIGH_MAX_PKT_SIZE_EP1		512		// bulk
#define	HIGH_MAX_PKT_SIZE_EP2		512		// bulk

#define	FULL_USB_VER				0x0110	// 1.1
#define	FULL_MAX_PKT_SIZE_EP0		8		// Do not modify
#define	FULL_MAX_PKT_SIZE_EP1		64		// bulk
#define	FULL_MAX_PKT_SIZE_EP2		64		// bulk

#define RX_FIFO_SIZE				512
#define NPTX_FIFO_START_ADDR		RX_FIFO_SIZE
#define NPTX_FIFO_SIZE				512
#define PTX_FIFO_SIZE				512

#define	DEVICE_DESCRIPTOR_SIZE		(18)
#define	CONFIG_DESCRIPTOR_SIZE		(9 + 9 + 7 + 7)

// SPEC1.1

// configuration descriptor: bmAttributes
enum CONFIG_ATTRIBUTES
{
	CONF_ATTR_DEFAULT			= 0x80,
	CONF_ATTR_REMOTE_WAKEUP 	= 0x20,
	CONF_ATTR_SELFPOWERED		= 0x40
};

// endpoint descriptor
enum ENDPOINT_ATTRIBUTES
{
	EP_ADDR_IN				= 0x80,
	EP_ADDR_OUT				= 0x00,

	EP_ATTR_CONTROL			= 0x00,
	EP_ATTR_ISOCHRONOUS		= 0x01,
	EP_ATTR_BULK			= 0x02,
	EP_ATTR_INTERRUPT		= 0x03
};

// Standard bRequest codes
enum STANDARD_REQUEST_CODE
{
	STANDARD_GET_STATUS			= 0,
	STANDARD_CLEAR_FEATURE		= 1,
	STANDARD_RESERVED_1			= 2,
	STANDARD_SET_FEATURE		= 3,
	STANDARD_RESERVED_2			= 4,
	STANDARD_SET_ADDRESS		= 5,
	STANDARD_GET_DESCRIPTOR		= 6,
	STANDARD_SET_DESCRIPTOR		= 7,
	STANDARD_GET_CONFIGURATION	= 8,
	STANDARD_SET_CONFIGURATION	= 9,
	STANDARD_GET_INTERFACE		= 10,
	STANDARD_SET_INTERFACE		= 11,
	STANDARD_SYNCH_FRAME		= 12
};

enum DESCRIPTORTYPE
{
	DESCRIPTORTYPE_DEVICE			= 1,
	DESCRIPTORTYPE_CONFIGURATION	= 2,
	DESCRIPTORTYPE_STRING			= 3,
	DESCRIPTORTYPE_INTERFACE		= 4,
	DESCRIPTORTYPE_ENDPOINT			= 5
};

#define CONTROL_EP		0
#define BULK_IN_EP		1
#define BULK_OUT_EP		2

/*
 * USB2.0 HS OTG
 */

struct NX_USB_OTG_GCSR_RegisterSet {
	volatile U32 GOTGCTL;						/* 0x000 R/W OTG Control and Status Register */
	volatile U32 GOTGINT;						/* 0x004 R/W OTG Interrupt Register */
	volatile U32 GAHBCFG;						/* 0x008 R/W Core AHB Configuration Register */
	volatile U32 GUSBCFG;						/* 0x00C R/W Core USB Configuration Register */
	volatile U32 GRSTCTL;						/* 0x010 R/W Core Reset Register */
	volatile U32 GINTSTS;						/* 0x014 R/W Core Interrupt Register */
	volatile U32 GINTMSK;						/* 0x018 R/W Core Interrupt Mask Register */
	volatile U32 GRXSTSR;						/* 0x01C R   Receive Status Debug Read Register */
	volatile U32 GRXSTSP;						/* 0x020 R/W Receive Status Debug Pop Register */
	volatile U32 GRXFSIZ;						/* 0x024 R/W Receive FIFO Size Register */
	volatile U32 GNPTXFSIZ;						/* 0x028 R   Non-Periodic Transmit FIFO Size Register */
	volatile U32 GNPTXSTS;						/* 0x02C R/W Non-Periodic Transmit FIFO/Queue Status Register */
	volatile U32 GReserved0;					/* 0x030     Reserved */
	volatile U32 GReserved1;					/* 0x034     Reserved */
	volatile U32 GReserved2;					/* 0x038     Reserved */
	volatile U32 GUID;							/* 0x03C R   User ID Register */
	volatile U32 GSNPSID;						/* 0x040 R   Synopsys ID Register */
	volatile U32 GHWCFG1;						/* 0x044 R   User HW Config1 Register */
	volatile U32 GHWCFG2;						/* 0x048 R   User HW Config2 Register */
	volatile U32 GHWCFG3;						/* 0x04C R   User HW Config3 Register */
	volatile U32 GHWCFG4;						/* 0x050 R   User HW Config4 Register */
	volatile U32 GLPMCFG;						/* 0x054 R/W Core LPM Configuration Register */
	volatile U32 GReserved3[(0x100-0x058)/4];	/* 0x058 ~ 0x0FC */
	volatile U32 HPTXFSIZ;						/* 0x100 R/W Host Periodic Transmit FIFO Size Register */
	volatile U32 DIEPTXF[15];					/* 0x104 ~ 0x13C R/W Device IN Endpoint Transmit FIFO Size Register */
	volatile U32 GReserved4[(0x400-0x140)/4];	/* 0x140 ~ 0x3FC */
};

struct NX_USB_OTG_Host_Channel_RegisterSet {
	volatile U32 HCCHAR;						/* 0xn00 R/W Host Channel-n Characteristics Register */
	volatile U32 HCSPLT;						/* 0xn04 R/W Host Channel-n Split Control Register */
	volatile U32 HCINT;							/* 0xn08 R/W Host Channel-n Interrupt Register */
	volatile U32 HCINTMSK;						/* 0xn0C R/W Host Channel-n Interrupt Mask Register */
	volatile U32 HCTSIZ;						/* 0xn10 R/W Host Channel-n Transfer Size Register */
	volatile U32 HCDMA;							/* 0xn14 R/W Host Channel-n DMA Address Register */
	volatile U32 HCReserved[2];					/* 0xn18, 0xn1C Reserved */
};
struct NX_USB_OTG_HMCSR_RegisterSet {
	volatile U32 HCFG;							/* 0x400 R/W Host Configuration Register */
	volatile U32 HFIR;							/* 0x404 R/W Host Frame Interval Register */
	volatile U32 HFNUM;							/* 0x408 R   Host Frame Number/Frame Time Remaining Register */
	volatile U32 HReserved0;					/* 0x40C     Reserved */
	volatile U32 HPTXSTS;						/* 0x410 R/W Host Periodic Transmit FIFO/Queue Status Register */
	volatile U32 HAINT;							/* 0x414 R   Host All Channels Interrupt Register */
	volatile U32 HAINTMSK;						/* 0x418 R/W Host All Channels Interrupt Mask Register */
	volatile U32 HReserved1[(0x440-0x41C)/4];	/* 0x41C ~ 0x43C Reserved */
	volatile U32 HPRT;							/* 0x440 R/W Host Port Control and Status Register */
	volatile U32 HReserved2[(0x500-0x444)/4];	/* 0x444 ~ 0x4FC Reserved */
	struct NX_USB_OTG_Host_Channel_RegisterSet HCC[16];	/* 0x500 ~ 0x6FC */
	volatile U32 HReserved3[(0x800-0x700)/4];	/* 0x700 ~ 0x7FC */
};

struct NX_USB_OTG_Device_EPI_RegisterSet {
	volatile U32 DIEPCTL;						/* 0xn00 R/W Device Control IN endpoint n Control Register */
	volatile U32 DReserved0;					/* 0xn04     Reserved */
	volatile U32 DIEPINT;						/* 0xn08 R/W Device Endpoint-n Interrupt Register */
	volatile U32 DReserved1;					/* 0xn0C     Reserved */
	volatile U32 DIEPTSIZ;						/* 0xn10 R/W Device Endpoint-n Transfer Size Register */
	volatile U32 DIEPDMA;						/* 0xn14 R/W Device Endpoint-n DMA Address Register */
	volatile U32 DTXFSTS;						/* 0xn18 R   Device IN Endpoint Transmit FIFO Status Register */
	volatile U32 DIEPDMAB;						/* 0xn1C R   Device Endpoint-n DMA Buffer Address Register */
};
struct NX_USB_OTG_Device_EPO_RegisterSet {
	volatile U32 DOEPCTL;						/* 0xn00 R/W Device Control OUT endpoint n Control Register */
	volatile U32 DReserved0;					/* 0xn04     Reserved */
	volatile U32 DOEPINT;						/* 0xn08 R/W Device Endpoint-n Interrupt Register */
	volatile U32 DReserved1;					/* 0xn0C     Reserved */
	volatile U32 DOEPTSIZ;						/* 0xn10 R/W Device Endpoint-n Transfer Size Register */
	volatile U32 DOEPDMA;						/* 0xn14 R/W Device Endpoint-n DMA Address Register */
	volatile U32 DReserved2;					/* 0xn18     Reserved */
	volatile U32 DOEPDMAB;						/* 0xn1C R   Device Endpoint-n DMA Buffer Address Register */
};
struct NX_USB_OTG_DMCSR_RegisterSet {
	volatile U32 DCFG;							/* 0x800 R/W Device Configuration Register */
	volatile U32 DCTL;							/* 0x804 R/W Device Control Register */
	volatile U32 DSTS;							/* 0x808 R   Device Status Register */
	volatile U32 DReserved0;					/* 0x80C     Reserved */
	volatile U32 DIEPMSK;						/* 0x810 R/W Device IN Endpoint Common Interrupt Mask Register */
	volatile U32 DOEPMSK;						/* 0x814 R/W Device OUT Endpoint Common Interrupt Mask Register */
	volatile U32 DAINT;							/* 0x818 R   Device All Endpoints Interrupt Register */
	volatile U32 DAINTMSK;						/* 0x81C R/W Device All Endpoints Interrupt Mask Register */
	volatile U32 DReserved1;					/* 0x820     Reserved */
	volatile U32 DReserved2;					/* 0x824     Reserved */
	volatile U32 DVBUSDIS;						/* 0x828 R/W Device VBUS Discharge Time Register */
	volatile U32 DVBUSPULSE;					/* 0x82C R/W Device VBUS Pulsing Time Register */
	volatile U32 DTHRCTL;						/* 0x830 R/W Device Threshold Control Register */
	volatile U32 DIEPEMPMSK;					/* 0x834 R/W Device IN Endpoint FIFO Empty Interrupt Mask Register */
	volatile U32 DReserved3;					/* 0x838     Reserved */
	volatile U32 DReserved4;					/* 0x83C     Reserved */
	volatile U32 DReserved5[0x10];				/* 0x840 ~ 0x87C    Reserved */
	volatile U32 DReserved6[0x10];				/* 0x880 ~ 0x8BC    Reserved */
	volatile U32 DReserved7[0x10];				/* 0x8C0 ~ 0x8FC    Reserved */
	struct NX_USB_OTG_Device_EPI_RegisterSet DEPIR[16];	/* 0x900 ~ 0xAFC */
	struct NX_USB_OTG_Device_EPO_RegisterSet DEPOR[16]; /* 0xB00 ~ 0xCFC */
};

struct NX_USB_OTG_PHYCTRL_RegisterSet
{
	volatile U32 PCReserved0[0x40/4];			/* 0x00 ~ 0x3C	Reserved */
	volatile U32 PHYPOR;						/* 0x40 */
	volatile U32 VBUSINTENB;					/* 0x44 */
	volatile U32 VBUSPEND;						/* 0x48 */
	volatile U32 TESTPARM3;						/* 0x4C */
	volatile U32 TESTPARM4;						/* 0x50 */
	volatile U32 LINKCTL;						/* 0x54 */
	volatile U32 TESTPARM6;						/* 0x58 */
	volatile U32 TESTPARM7;						/* 0x5C */
	volatile U32 TESTPARM8;						/* 0x60 */
	volatile U32 TESTPARM9;						/* 0x64 */
	volatile U32 PCReserved4[(0x100-0x68)/4];	/* 0x68 ~ 0xFC	Reserved */
};
struct NX_USB_OTG_IFCLK_RegisterSet
{
	volatile U32 IFReserved0[0xC0/4];			/* 0x00 ~ 0xBC	Reserved  */
	volatile U32 IFCLK_MODE;					/* 0xC0 */
	volatile U32 IFCLKGEN;						/* 0xC4 */
	volatile U32 IFReserved1[(0x100-0xC8)/4];	/* 0xC8 ~ 0xFC */
};
struct NX_USB_OTG_RegisterSet
{
	struct NX_USB_OTG_GCSR_RegisterSet  GCSR;		/* 0x0000 ~ 0x03FC */
	struct NX_USB_OTG_HMCSR_RegisterSet HCSR;		/* 0x0400 ~ 0x07FC */
	struct NX_USB_OTG_DMCSR_RegisterSet DCSR;		/* 0x0800 ~ 0x0CFC */
	volatile U32 GReserved0[(0xE00-0xD00)/4];		/* 0x0D00 ~ 0x0DFC	Reserved */
	volatile U32 PCGCCTL;							/* 0x0E00 R/W Power and Clock Gating Control Register */
	volatile U32 GReserved1[(0x1000-0xE04)/4];		/* 0x0E04 ~ 0x0FFC	Reserved */
	volatile U32 EPFifo[15][1024];					/* 0x1000 ~ 0xFFFC Endpoint Fifo */
//	volatile U32 EPFifo[16][1024];					/* 0x1000 ~ 0x10FFC Endpoint Fifo */
//	volatile U32 GReserved2[(0x20000-0x11000)/4];	/* 0x11000 ~ 0x20000 Reserved */
//	volatile U32 DEBUGFIFO[0x8000];					/* 0x20000 ~ 0x3FFFC Debug Purpose Direct Fifo Acess Register */
};

/*definitions related to CSR setting */
/* USB Global Interrupt Status register(GINTSTS) setting value */
#define WkUpInt				(1u<<31)
#define OEPInt				(1<<19)
#define IEPInt				(1<<18)
#define EnumDone			(1<<13)
#define USBRst				(1<<12)
#define USBSusp				(1<<11)
#define RXFLvl				(1<<4)

/* NX_OTG_GOTGCTL*/
#define B_SESSION_VALID			(0x1<<19)
#define A_SESSION_VALID			(0x1<<18)

/* NX_OTG_GAHBCFG*/
#define PTXFE_HALF				(0<<8)
#define PTXFE_ZERO				(1<<8)
#define NPTXFE_HALF				(0<<7)
#define NPTXFE_ZERO				(1<<7)
#define MODE_SLAVE				(0<<5)
#define MODE_DMA				(1<<5)
#define BURST_SINGLE			(0<<1)
#define BURST_INCR				(1<<1)
#define BURST_INCR4				(3<<1)
#define BURST_INCR8				(5<<1)
#define BURST_INCR16			(7<<1)
#define GBL_INT_UNMASK			(1<<0)
#define GBL_INT_MASK			(0<<0)

/* NX_OTG_GRSTCTL*/
#define AHB_MASTER_IDLE			(1u<<31)
#define CORE_SOFT_RESET			(0x1<<0)

/* NX_OTG_GINTSTS/NX_OTG_GINTMSK core interrupt register */
#define INT_RESUME				(1u<<31)
#define INT_DISCONN				(0x1<<29)
#define INT_CONN_ID_STS_CNG		(0x1<<28)
#define INT_OUT_EP				(0x1<<19)
#define INT_IN_EP				(0x1<<18)
#define INT_ENUMDONE			(0x1<<13)
#define INT_RESET				(0x1<<12)
#define INT_SUSPEND				(0x1<<11)
#define INT_TX_FIFO_EMPTY		(0x1<<5)
#define INT_RX_FIFO_NOT_EMPTY	(0x1<<4)
#define INT_SOF					(0x1<<3)
#define INT_DEV_MODE			(0x0<<0)
#define INT_HOST_MODE			(0x1<<1)

/* NX_OTG_GRXSTSP STATUS*/
#define GLOBAL_OUT_NAK				(0x1<<17)
#define OUT_PKT_RECEIVED			(0x2<<17)
#define OUT_TRNASFER_COMPLETED		(0x3<<17)
#define SETUP_TRANSACTION_COMPLETED	(0x4<<17)
#define SETUP_PKT_RECEIVED			(0x6<<17)

/* NX_OTG_DCTL device control register */
#define NORMAL_OPERATION		(0x1<<0)
#define SOFT_DISCONNECT			(0x1<<1)

/* NX_OTG_DAINT device all endpoint interrupt register */
#define INT_IN_EP0				(0x1<<0)
#define INT_IN_EP1				(0x1<<1)
#define INT_IN_EP3				(0x1<<3)
#define INT_OUT_EP0				(0x1<<16)
#define INT_OUT_EP2				(0x1<<18)
#define INT_OUT_EP4				(0x1<<20)

/* NX_OTG_DIEPCTL0/NX_OTG_DOEPCTL0 */
#define DEPCTL_EPENA			(0x1u<<31)
#define DEPCTL_EPDIS			(0x1<<30)
#define DEPCTL_SNAK				(0x1<<27)
#define DEPCTL_CNAK				(0x1<<26)
#define DEPCTL_STALL			(0x1<<21)
#define DEPCTL_ISO_TYPE			(EP_TYPE_ISOCHRONOUS<<18)
#define DEPCTL_BULK_TYPE		(EP_TYPE_BULK<<18)
#define DEPCTL_INTR_TYPE		(EP_TYPE_INTERRUPT<<18)
#define DEPCTL_USBACTEP			(0x1<<15)

/*ep0 enable, clear nak, next ep0, max 64byte */
#define EPEN_CNAK_EP0_64 (DEPCTL_EPENA|DEPCTL_CNAK|(CONTROL_EP<<11)|(0<<0))

/*ep0 enable, clear nak, next ep0, 8byte */
#define EPEN_CNAK_EP0_8 (DEPCTL_EPENA|DEPCTL_CNAK|(CONTROL_EP<<11)|(3<<0))

/* DIEPCTLn/DOEPCTLn */
#define BACK2BACK_SETUP_RECEIVED		(0x1<<6)
#define INTKN_TXFEMP					(0x1<<4)
#define NON_ISO_IN_EP_TIMEOUT			(0x1<<3)
#define CTRL_OUT_EP_SETUP_PHASE_DONE	(0x1<<3)
#define AHB_ERROR						(0x1<<2)
#define TRANSFER_DONE					(0x1<<0)


typedef struct
{
	U8 bmRequestType;
	U8 bRequest;
	U16 wValue;
	U16 wIndex;
	U16 wLength;
} SetupPacket;

typedef enum
{
	USB_HIGH,
	USB_FULL,
	USB_LOW
//	,0xFFFFFFFFUL
} USB_SPEED;

typedef enum
{
	EP_TYPE_CONTROL, EP_TYPE_ISOCHRONOUS, EP_TYPE_BULK, EP_TYPE_INTERRUPT
} EP_TYPE;

/*------------------------------------------------*/
/* EP0 state */
enum EP0_STATE
{
	EP0_STATE_INIT					= 0,
	EP0_STATE_GET_DSCPT				= 1,
	EP0_STATE_GET_INTERFACE			= 2,
	EP0_STATE_GET_CONFIG			= 3,
	EP0_STATE_GET_STATUS			= 4
};

typedef struct __attribute__((aligned(4))) tag_USBBOOTSTATUS
{
	volatile CBOOL	bDownLoading;
	U8		*RxBuffAddr;
	S32		iRxSize;

	U32		ep0_state;
	USB_SPEED speed;
	U32		ctrl_max_pktsize;
	U32		bulkin_max_pktsize;
	U32		bulkout_max_pktsize;

	U8*		Current_ptr;
	U32		Current_Fifo_Size;
	U32		Remain_size;

	U32		up_addr;
	U32		up_size;
	U8*		up_ptr;

	U8		CurConfig;
	U8		CurInterface;
	U8		CurSetting;
	U8		__Reserved;

	const U8	* DeviceDescriptor;
	const U8	* ConfigDescriptor;
} USBBOOTSTATUS;

CBOOL iUSBBOOT(void);

#endif	// __NX_OTG_HS_H__
