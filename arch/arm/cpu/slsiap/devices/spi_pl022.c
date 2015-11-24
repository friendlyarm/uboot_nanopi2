/*
 * (C) Copyright 2010
 * Young bok Park, Nexell Co, <pybok@nexell.co.kr>
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
#include <spi.h>
#include <malloc.h>
#include <platform.h>
#include <mach-api.h>

#if (0)

#define DBGOUT(msg...)		{ printf("spi: " msg); }
#else
#define DBGOUT(msg...)
#endif

#define SPIMSG(msg...)		{ printf("spi: " msg); }
#define ERROUT(msg...)		{ printf("spi: line=%d ", __LINE__); printf(msg); }

#define SER_SR_READY		1<<0		// Ready bit
#define SER_SR_WEN			1<<1		// Write Enable indicate 0:not write clkgenEnabled 1:write clkgenEnabled
#define SER_SR_BPx			3<<2		// Block Write Protect bits
#define SER_SR_WPEN			1<<7		// Write Protect Enable bit

#define CPSDVR_MIN 0x02
#define CPSDVR_MAX 0xFE
#define SCR_MIN 0x00
#define SCR_MAX 0xFF

#define	EEPROM_PAGE_MASK		 (CONFIG_EEPROM_ERASE_SIZE - 1)
#define	EEPROM_ERASE_MASK		~(CONFIG_EEPROM_ERASE_SIZE - 1)

#define SPI_TYPE_EEPROM 	1
#define SPI_TYPE_DEVICE 	0

#define MAX_ADDR_LEN	8

static U8 spi_type[3];
static U8 cur_module = 0;

static void flash_sector_erase(U32 eraseaddr, int alen);
static U8 is_flash_ready(U8 status);

struct spi_gpio
{
	U32 pad;
 	U32 alt;
};
struct spi_pad
{
	struct spi_gpio clkio;
	struct spi_gpio fss;
	struct spi_gpio rxd;
	struct spi_gpio txd;
};

static const struct spi_pad _spi_pad[3] = {
	{
		.clkio.pad  = PAD_GPIO_C +  29, .clkio.alt = 1,
		.fss.pad 	= PAD_GPIO_C +  30, .fss.alt   = 1,
		.txd.pad    = PAD_GPIO_C +  31, .txd.alt   = 1,
		.rxd.pad    = PAD_GPIO_D +  0 , .rxd.alt   = 1,
	},
	{
		.clkio.pad = PAD_GPIO_E + 14, .clkio.alt = 2,
		.fss.pad   = PAD_GPIO_E + 15, .fss.alt   = 2,
		.rxd.pad   = PAD_GPIO_E + 18, .rxd.alt   = 2,
		.txd.pad   = PAD_GPIO_E + 19, .txd.alt   = 2,
	},
	{
		.clkio.pad = PAD_GPIO_C + 9 , .clkio.alt = 2,
		.fss.pad   = PAD_GPIO_C + 10, .fss.alt   = 2,
		.rxd.pad   = PAD_GPIO_C + 11, .rxd.alt   = 2,
		.txd.pad   = PAD_GPIO_C + 12, .txd.alt   = 2,
	},
};

struct spi_param  {
	/* CLOCK GEN */
	unsigned long hz;
	int 		 req;
	int 	clkgenEnable;
	int 	spi_type;
};

struct ssp_clock_params {
    u8 cpsdvsr; /* value from 2 to 254 (even only!) */
    u8 scr;     /* value from 0 to 255 */
};

struct spi_param _spi_param[3] = {
{
		#ifdef	CONFIG_SPI_MODULE_0
		/* SPI_CLOCK */
		.hz 			= CONFIG_SPI_MODULE_0_SOURCE_CLOCK,
		.req 			= CONFIG_SPI_MODULE_0_CLOCK,
		/* CLOCK GEN */
		.clkgenEnable    	= CTRUE,
		/* SPI_ClOCK Set */
	
		.spi_type 		= CONFIG_SPI_MODULE_0_EEPROM,
		#else
		0,
		#endif
	},

	{
		#ifdef	CONFIG_SPI_MODULE_1
		/* SPI_CLOCK */
		.hz 			= CONFIG_SPI_MODULE_0_SOURCE_CLOCK,
		.req 			= CONFIG_SPI_MODULE_0_CLOCK,
		/* CLOCK GEN */
		.clkgenEnable   = CTRUE,
		/* SPI_ClOCK Set */
		.spi_type 		= CONFIG_SPI_MODULE_1_EEPROM
		#else
		0,
		#endif
	},

	{
		#ifdef	CONFIG_SPI_MODULE_2
		/* SPI_CLOCK */
		.hz 			= CONFIG_SPI_MODULE_0_SOURCE_CLOCK,
		.req 			= CONFIG_SPI_MODULE_0_CLOCK,
		/* CLOCK GEN */
		.clkgenEnable    = CTRUE,
	/* SPI_ClOCK Set */
		.spi_type 		= CONFIG_SPI_MODULE_2_EEPROM
		#else
		0,
		#endif
	},

};

/*global Variable */

#ifdef CONFIG_SPI_EEPROM_WRITE_PROTECT
static struct spi_gpio wp ={
	//.pad  = CONFIG_SPI_EEPROM_WP_PAD & 0xFFFF,
	.pad  = CFG_IO_SPI_EEPROM_WP & 0xFF,
	.alt  = (PAD_GET_FUNC(CFG_IO_SPI_EEPROM_WP) >> PAD_LEVEL_SHIFT),
};

static void WP_DI(void)
{
	NX_GPIO_SetOutputValue(wp.pad /32 ,wp.pad % 32, 1);
	NX_GPIO_SetOutputEnable(wp.pad /32 ,wp.pad % 32, 1);
}

static void WP_EN(void)
{
	NX_GPIO_SetOutputValue(wp.pad /32 ,wp.pad % 32, 0);
	NX_GPIO_SetOutputEnable(wp.pad /32 ,wp.pad % 32, 1);
}
#endif

static void CS_ON(void)
{
    NX_GPIO_SetOutputValue(_spi_pad[0].fss.pad /32, _spi_pad[0].fss.pad % 32 , 0);
  	NX_GPIO_SetOutputEnable(_spi_pad[0].fss.pad /32, _spi_pad[0].fss.pad % 32 , 1);
}

static void CS_OFF(void)
{
	NX_GPIO_SetOutputValue(_spi_pad[0].fss.pad /32, _spi_pad[0].fss.pad % 32 , 1);
	NX_GPIO_SetOutputEnable(_spi_pad[0].fss.pad /32, _spi_pad[0].fss.pad % 32 , 1);
}

static u32 spi_rate(u32 rate, u16 cpsdvsr, u16 scr)
{
	return rate / (cpsdvsr * (1 + scr));
}

static int calculate_effective_freq(struct clk *clk, int freq, struct
				    ssp_clock_params * clk_freq)
{
	/* Lets calculate the frequency parameters */
	u16 cpsdvsr = CPSDVR_MIN, scr = SCR_MIN;
	u32 rate, max_tclk, min_tclk, best_freq = 0, best_cpsdvsr = 0,
		best_scr = 0, tmp, found = 0;
	
	rate = clk_get_rate(clk);
	/* cpsdvscr = 2 & scr 0 */
	max_tclk = spi_rate(rate, CPSDVR_MIN, SCR_MIN);
	/* cpsdvsr = 254 & scr = 255 */
	min_tclk = spi_rate(rate, CPSDVR_MAX, SCR_MAX);

	if (freq > max_tclk)
		SPIMSG("Max speed that can be programmed is %d Hz, you requested %d\n",
			max_tclk, freq);

	if (freq < min_tclk) {
		SPIMSG(	"Requested frequency: %d Hz is less than minimum possible %d Hz\n",
			freq, min_tclk);
		return -1;
	}

	/*
	 * best_freq will give closest possible available rate (<= requested
	 * freq) for all values of scr & cpsdvsr.
	 */
	while ((cpsdvsr <= CPSDVR_MAX) && !found) {
		while (scr <= SCR_MAX) {
			tmp = spi_rate(rate, cpsdvsr, scr);

			if (tmp > freq) {
				/* we need lower freq */
				scr++;
				continue;
			}
			/*
			 * If found exact value, mark found and break.
			 * If found more closer value, update and break.
			 */
			if (tmp > best_freq) {
				best_freq = tmp;
				best_cpsdvsr = cpsdvsr;
				best_scr = scr;

				if (tmp == freq)
					found = 1;
			}
			/*
			 * increased scr will give lower rates, which are not
			 * required
			 */
			break;
		}
		cpsdvsr += 2;
		scr = SCR_MIN;
	}

	clk_freq->cpsdvsr = (u8) (best_cpsdvsr & 0xFF);
	clk_freq->scr = (u8) (best_scr & 0xFF);

	DBGOUT(	"SSP Target Frequency is: %u, Effective Frequency is %u\n",
		freq, best_freq);
	DBGOUT( "SSP cpsdvsr = %d, scr = %d\n",
		clk_freq->cpsdvsr, clk_freq->scr);

	return 0;
}

void spi_init_f (void)
{
	int ModuleIndex =0;
	
	struct clk *clk = NULL;
	struct ssp_clock_params clk_freq = {0};
	char name[10]= {0, };
	unsigned long hz  = 10* 1000 * 1000;
	unsigned long rate = 0;
	DBGOUT("%s \n ",__func__);
	flush_dcache_all();
	NX_SSP_Initialize();
	for(ModuleIndex = 0 ; ModuleIndex < NX_SSP_GetNumberOfModule();ModuleIndex++ )
	{
		if(_spi_param[ModuleIndex].clkgenEnable == 1 )
		{
			/* GPIO Setting */
		    NX_GPIO_SetPadFunction(_spi_pad[ModuleIndex].clkio.pad /32, _spi_pad[ModuleIndex].clkio.pad % 32, _spi_pad[ModuleIndex].clkio.alt);
		    NX_GPIO_SetPadFunction(_spi_pad[ModuleIndex].rxd.pad /32, _spi_pad[ModuleIndex].rxd.pad % 32 , _spi_pad[ModuleIndex].rxd.alt);
		    NX_GPIO_SetPadFunction(_spi_pad[ModuleIndex].txd.pad /32, _spi_pad[ModuleIndex].txd.pad % 32, _spi_pad[ModuleIndex].txd.alt);

		    NX_GPIO_SetPadFunction(_spi_pad[ModuleIndex].fss.pad /32, _spi_pad[ModuleIndex].fss.pad % 32 , 0);
            NX_GPIO_SetOutputValue(_spi_pad[ModuleIndex].fss.pad /32, _spi_pad[ModuleIndex].fss.pad % 32 , 1);
            NX_GPIO_SetOutputEnable(_spi_pad[ModuleIndex].fss.pad /32, _spi_pad[ModuleIndex].fss.pad % 32 , 1);

		    /* RSTCON Control */

			NX_SSP_SetBaseAddress( ModuleIndex, (U32)NX_SSP_GetPhysicalAddress(ModuleIndex) );
			sprintf(name,"nxp-spi%d",ModuleIndex);
			clk= clk_get(NULL, name);
			hz = _spi_param[ModuleIndex].hz;
			rate = clk_set_rate(clk,hz);
			clk_enable(clk);
						
		    NX_RSTCON_SetnRST(NX_SSP_GetResetNumber( ModuleIndex, NX_SSP_PRESETn ), RSTCON_nDISABLE);
			NX_RSTCON_SetnRST(NX_SSP_GetResetNumber( ModuleIndex, NX_SSP_nSSPRST ), RSTCON_nDISABLE);
		    NX_RSTCON_SetnRST(NX_SSP_GetResetNumber( ModuleIndex, NX_SSP_PRESETn ), RSTCON_nENABLE);
		   	NX_RSTCON_SetnRST(NX_SSP_GetResetNumber( ModuleIndex, NX_SSP_nSSPRST ), RSTCON_nENABLE);

			calculate_effective_freq(clk, _spi_param[ModuleIndex].req, &clk_freq);
			NX_SSP_SetClockPrescaler( ModuleIndex, clk_freq.cpsdvsr, clk_freq.scr );
			NX_SSP_SetEnable( ModuleIndex, CFALSE ); 			// SSP operation disable
			NX_SSP_SetProtocol( ModuleIndex, 0); 				// Protocol : Motorola SPI

			NX_SSP_SetClockPolarityInvert( ModuleIndex, 1);
			NX_SSP_SetClockPhase( ModuleIndex, 1);

			NX_SSP_SetBitWidth( ModuleIndex, 8 ); 				// 8 bit
			NX_SSP_SetSlaveMode( ModuleIndex, CFALSE ); 		// master mode
			NX_SSP_SetInterruptEnable( ModuleIndex,0, CFALSE );
			NX_SSP_SetInterruptEnable( ModuleIndex,1, CFALSE );
			NX_SSP_SetInterruptEnable( ModuleIndex,2, CFALSE );
			NX_SSP_SetInterruptEnable( ModuleIndex,3, CFALSE );
			NX_SSP_SetDMATransferMode( ModuleIndex, CFALSE );   //DMA_Not use
		}
		spi_type[ModuleIndex] = _spi_param[ModuleIndex].spi_type;
	}

	#ifdef CONFIG_SPI_EEPROM_WRITE_PROTECT
		NX_GPIO_SetPadFunction(wp.pad /32  , wp.pad % 32, wp.alt);
		NX_GPIO_SetOutputEnable(wp.pad /32  , wp.pad % 32, 1);
		NX_GPIO_SetOutputValue(wp.pad /32  , wp.pad % 32, 1);
	#endif
}

ssize_t spi_read  (uchar *addr, int alen, uchar *buffer, int len)
{
	U32 device = cur_module ;
	U32 dummycount=0;
	U32 index = 0,i=0;
	volatile U8 tmp;
	DBGOUT(" %s moudele = %d\n", __func__, device);
	
	spi_init_f();

	if(alen > MAX_ADDR_LEN)
	{
		SPIMSG("fail : addrlen small than %d \n",MAX_ADDR_LEN);
		return -1;
	}
	dummycount = alen;

	/* cmd and addr send */
	if( _spi_param[device].spi_type == SPI_TYPE_EEPROM )	//if EEPROM send CMD_SPI_READ
	{
		NX_SSP_PutByte(device,CMD_SPI_READ);	//send addr data
		dummycount += 1;
	}

	for(i = 0 ; i < alen; i++)
	{
		NX_SSP_PutByte(device,addr[i]);	//send addr data
	}

	CS_ON();

	NX_SSP_SetEnable( device, CTRUE );

	//len = lencnt;

	while( len + dummycount)
	{
		if(!(NX_SSP_IsTxFIFOFull(device)))	// check receive buffer is not empty
		{
			NX_SSP_PutByte(device, 0); //send dummy data for read			// send dummy data for receive read data.
			while(NX_SSP_IsRxFIFOEmpty(device)) ;

			if(dummycount != 0)
			{
				tmp =  NX_SSP_GetByte(device);
				dummycount--;
			}
			else
			{
				buffer[index++] = NX_SSP_GetByte(device);
				len--;
			}
		}
	}

	while(!(NX_SSP_IsTxFIFOEmpty(device)));		// wait until tx buffer

	do{
		tmp = NX_SSP_GetByte(device);
	}while(!(NX_SSP_IsRxFIFOEmpty(device)));			// wait until reception buffer is not empty

	NX_SSP_SetEnable( device, CFALSE );
	CS_OFF();
	
	return len;
}

static int eeprom_write_enable (unsigned dev_addr, int state)
{
	U8 cmd;
	U8 tmp;
	U32 device = cur_module ;

	if( _spi_param[device].spi_type != SPI_TYPE_EEPROM )	//if EEPROM send CMD_SPI_READ
	{
		SPIMSG("fail : device type not EEPROM \n");
		return -1;
	}
	if(state == 1)
		cmd = CMD_SPI_WREN;
	else
		cmd = CMD_SPI_WRDI;
		NX_SSP_PutByte(device, cmd);

		CS_ON();
		NX_SSP_SetEnable( device, CTRUE );

		while(!(NX_SSP_IsTxFIFOEmpty(device)));
		while((NX_SSP_IsRxFIFOEmpty(device)));
		tmp = NX_SSP_GetByte(device );
		NX_SSP_SetEnable( device, CFALSE );
		CS_OFF();

		return 0;
}

static void flash_sector_erase(U32 eraseaddr, int alen)
{
	U32 device = cur_module ;
	U8 cmd = CMD_SPI_SE;
	U8 tmp ,i, j = alen;
	u8 addr[4];

	DBGOUT("Sector Erase 0x%06X\n", eraseaddr);

	for(i = 0 ; i < alen; i++)
	{
		addr[i] = eraseaddr >> (( j - 1)*8 ) & 0xff;
		j--;
	}

	NX_SSP_PutByte(device, cmd);

	for(i = 0 ; i < alen; i++)
	{
		NX_SSP_PutByte(device, addr[i]); //send erase addr
	}
	CS_ON();
	NX_SSP_SetEnable( device, CTRUE );

	while(!(NX_SSP_IsTxFIFOEmpty(device))); // Wait until send data done.

	while(!(NX_SSP_IsRxFIFOEmpty(device)))
	{
		tmp = NX_SSP_GetByte(device);
	}

	NX_SSP_SetEnable( device, CFALSE );
	CS_OFF();
}

static U8 flash_page_program(U32 dwFlashAddr, int alen, uchar * databuffer, U32 dwDataSize)
{
	U32 device = cur_module ;
	U8 cmd = CMD_SPI_WRITE;
	volatile U8 temp ,i,j=alen;
	U32 index = 0;
	u8 addr[4] ={0 , };

	for(i = 0 ; i < alen; i++)
	{
		addr[i] = dwFlashAddr >> (( j - 1)*8 ) & 0xff;
		j--;
	}

	if(alen > CONFIG_EEPROM_WRITE_PAGE_SIZE)
	{
		SPIMSG("fail : alen small  %d \n",CONFIG_EEPROM_WRITE_PAGE_SIZE);
		return -1;
	}
	if(_spi_param[device].spi_type != SPI_TYPE_EEPROM )	//if EEPROM send CMD_SPI_READ
	{
		SPIMSG("fail : device type not EEPROM \n");
		return -1;
	}

	do {
		eeprom_write_enable(dwFlashAddr,1);
		udelay(1000);
		temp = is_flash_ready(SER_SR_READY);
	} while( !((temp & SER_SR_WEN) == SER_SR_WEN) || (temp & SER_SR_READY));
	
	CS_ON();
	NX_SSP_PutByte(device, cmd); 		//send WRITE COMMAND

	for(i = 0 ; i < alen; i++)
	{
		NX_SSP_PutByte(device, addr[i]); //send addr
	}
	
	NX_SSP_SetEnable( device, CTRUE );
	while(!NX_SSP_IsTxFIFOEmpty(device));// ready to Fifo Empty
	while(!(NX_SSP_IsRxFIFOEmpty(device)))
	{
		temp = NX_SSP_GetByte(device);	//read dummy data
	}

	while(dwDataSize)
	{
		if(!(NX_SSP_IsTxFIFOFull(device)))
		{
			NX_SSP_PutByte(device, databuffer[index++]); //send addr
			dwDataSize--;
			while(NX_SSP_IsTxRxEnd(device));
			//while(!NX_SSP_IsTxFIFOEmpty(device));// ready to Fifo Empty
			//while(!NX_SSP_IsTxFIFOEmpty(device));// ready to Fifo Empty
			while(!(NX_SSP_IsRxFIFOEmpty(device))){
				temp = NX_SSP_GetByte(device);	//read dummy data
			}
		}
	}

	while(!NX_SSP_IsTxFIFOEmpty(device));// ready to Fifo Empty
	while(!(NX_SSP_IsRxFIFOEmpty(device))){
		temp = NX_SSP_GetByte(device);	//read dummy data
	}
	while(!(NX_SSP_IsRxFIFOEmpty(device)))
	{
		temp = NX_SSP_GetByte(device);	//read dummy data
	}
	NX_SSP_SetEnable( device, CFALSE );
	CS_OFF();

	return 0;
}


static U8 is_flash_ready(U8 status)
{
	U32 device = cur_module;
	U8 cmd = 0x05;//CMD_SPI_SE;
	volatile U8 tmp, ret;

	if( _spi_param[device].spi_type != SPI_TYPE_EEPROM )	//if EEPROM send CMD_SPI_READ
	{
		SPIMSG("fail : device type not EEPROM \n");
		return -1;
	}

//	do {
		NX_SSP_PutByte(device, cmd);
		NX_SSP_PutByte(device, 0);

		CS_ON();
		NX_SSP_SetEnable( device, CTRUE );

		while(!(NX_SSP_IsTxFIFOEmpty(device)));
		while((NX_SSP_IsRxFIFOEmpty(device)));
		
		tmp = NX_SSP_GetByte(device);

		while((NX_SSP_IsRxFIFOEmpty(device)));

		ret = NX_SSP_GetByte(device);

		while(!(NX_SSP_IsRxFIFOEmpty(device)));

		
		NX_SSP_SetEnable( device, CFALSE );
		CS_OFF();
//	}	while(!(ret & status) );
	return ret;
}

static void SPIFifoReset(void)
{
	U32 device = cur_module ;
	U8 tmp =0;

	while(!(NX_SSP_IsRxFIFOEmpty(device)))
	{
		tmp = NX_SSP_GetByte(device);	//read dummy data
	}
}

ssize_t spi_write (uchar *addr, int alen, uchar *buffer, int len)
{

	U32 device = cur_module ;
	volatile U8 tmp =0;

	U32  TargetAddr = ((addr[0]<<16) | (addr[1]<<8) | (addr[2] & 0xff)) ;
	U8  *pDatBuffer = buffer;

	U8  *pTmpBuffer = NULL, *pWBuffer = NULL;
	U32	 FlashAddr = 0;
	U32  StartRest = 0, BlockCnt = 0;
	U32  StartOffs = 0, EndOffs = 0;
	U32  WriteSize = 0, StartBlock = 1;
	U32  Tsize = len;

	if(alen > MAX_ADDR_LEN)
	{
		SPIMSG("fail : addrlen small than %d \n",MAX_ADDR_LEN);
		return -1;
	}
	
	if(spi_type[device] == SPI_TYPE_EEPROM )
	{
		WriteSize = len;
		FlashAddr = (TargetAddr & EEPROM_ERASE_MASK);
		BlockCnt  = ((TargetAddr& EEPROM_PAGE_MASK) + len) /
					(CONFIG_EEPROM_ERASE_SIZE) + (((TargetAddr + len) & EEPROM_PAGE_MASK) ? 1 : 0);

		StartOffs = (TargetAddr & EEPROM_PAGE_MASK);
		EndOffs   = (TargetAddr + len) & EEPROM_PAGE_MASK;
		StartRest = StartOffs     ? ((CONFIG_EEPROM_ERASE_SIZE) - StartOffs) : 0;
		pWBuffer  = pDatBuffer;

		pTmpBuffer = malloc(CONFIG_EEPROM_ERASE_SIZE);

		DBGOUT("Writesize %d FlashAddr %x BlockC00nt %d  StartOffs %x \n ",
						 WriteSize, FlashAddr, BlockCnt, StartOffs, StartRest);

		if (StartRest) {
			U8 offset[3] = { (FlashAddr >> 16), (FlashAddr >>  8), (FlashAddr & 0xFF) };
			int size = 0;

			if (BlockCnt > 1)
				WriteSize  += ((CONFIG_EEPROM_ERASE_SIZE) - StartRest), size = StartRest;
			else
				WriteSize   = (CONFIG_EEPROM_ERASE_SIZE), size = len;

			Tsize = WriteSize ;
			DBGOUT("\n Copy 0x%08x to offset 0x%08x size %d\n", (U32)pDatBuffer, StartOffs, size);
			memset(pTmpBuffer, 0xff, CONFIG_EEPROM_ERASE_SIZE);
			spi_read(offset, 3, pTmpBuffer, CONFIG_EEPROM_ERASE_SIZE);			// read block

			if (size != 0)
			memcpy(pTmpBuffer + StartOffs , pDatBuffer, size);	// merge data

			pDatBuffer -= ((CONFIG_EEPROM_ERASE_SIZE) - StartRest);

			if (size != 0 )
			pWBuffer    = pTmpBuffer;
			else
			pWBuffer    = pDatBuffer;
		}
		else
		pWBuffer  = pDatBuffer;
		#ifdef CONFIG_SPI_EEPROM_WRITE_PROTECT
			WP_DI();
		#endif
		printf("\n\r");
		while (BlockCnt != 0)
		{
			// Erase Block
			if (0 == (FlashAddr & EEPROM_PAGE_MASK))
			{

				DBGOUT("\n Erase 0x%08x, BlockCnt %d\n", FlashAddr, BlockCnt);
				if (1 == BlockCnt && (CONFIG_EEPROM_ERASE_SIZE) > WriteSize) {
					U8 offset[3] = { (FlashAddr >> 16), (FlashAddr >>  8), (FlashAddr & 0xFF) };
					DBGOUT("\n ECopy 0x%08x to offset 0x%08x size %d\n", (U32)pDatBuffer, offset, WriteSize);
					spi_read(offset, 3, pTmpBuffer, CONFIG_EEPROM_ERASE_SIZE);	// read block
					memcpy(pTmpBuffer, pDatBuffer, Tsize);	// merge data
					pWBuffer  = pTmpBuffer;
					WriteSize = (CONFIG_EEPROM_ERASE_SIZE);
				} else {
					if (! StartBlock)
					{
						pWBuffer = pDatBuffer;
					}
				}
				do {
					eeprom_write_enable(FlashAddr,1);
					udelay(1000);
					tmp = is_flash_ready(SER_SR_WEN);

				} while(!(tmp & SER_SR_WEN)) ;

				flash_sector_erase(FlashAddr,alen);
				mdelay(10);
				do {
					tmp = is_flash_ready(SER_SR_READY);
					udelay(1000);
				} while((tmp & SER_SR_READY));

				BlockCnt -= 1;
				StartBlock = 0;
			}
				mdelay(10);
			// Write Page
			SPIFifoReset();

			WriteSize= CONFIG_EEPROM_ERASE_SIZE;
			while(WriteSize > 0)
			{
				printf(".........\r");
				SPIFifoReset();
				flash_page_program(FlashAddr , alen , pWBuffer , CONFIG_EEPROM_WRITE_PAGE_SIZE);

				FlashAddr  += CONFIG_EEPROM_WRITE_PAGE_SIZE;
				pDatBuffer += CONFIG_EEPROM_WRITE_PAGE_SIZE;
				pWBuffer   += CONFIG_EEPROM_WRITE_PAGE_SIZE;	// write page size
				WriteSize  -= CONFIG_EEPROM_WRITE_PAGE_SIZE;
				Tsize -= CONFIG_EEPROM_WRITE_PAGE_SIZE;

				if (0 >= WriteSize)
					break;
			}
		}
		free(pTmpBuffer);
		printf("\n");
		#ifdef CONFIG_SPI_EEPROM_WRITE_PROTECT
			WP_EN();
		#endif
	}
	return len;
}


