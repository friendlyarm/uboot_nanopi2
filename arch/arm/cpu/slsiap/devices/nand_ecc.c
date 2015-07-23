/*
 * (C) Copyright 2009
 * KOO Bon-Gyu, Nexell Co, <freestyle@nexell.co.kr>
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
#define	U_BOOT_NAND		(1)

#if (U_BOOT_NAND)
#include <common.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <malloc.h>
#include <linux/mtd/nand.h>
#include <nand.h>
#include <platform.h>
#else
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/mtd/nand.h>
#include <asm/io.h>
#include <mach/platform.h>
#include <mach/soc.h>
#endif

#if	(0)
#define DBGOUT(msg...)		printk(msg)
#else
#define DBGOUT(msg...)		do {} while (0)
#endif
#if	(0)
#define ECCERR(msg...)		printk(msg)
#else
#define ECCERR(msg...)		do {} while (0)
#endif

#define ERROUT(msg...)		{ 					\
		printk(KERN_ERR "ERROR: %s, %s line %d: \n",		\
			__FILE__, __FUNCTION__, __LINE__),	\
		printk(KERN_ERR msg); }


#if defined (CONFIG_MTD_NAND_ECC_HW) || defined (CONFIG_SYS_NAND_HW_ECC)

#define	NAND_READ_RETRY		(1)

#include "nand_ecc.h"
#ifdef CONFIG_NAND_RANDOMIZER
#include "nx_randomizer.h"
static uint8_t *randomize_buf;
static uint32_t pages_per_block_mask;
#endif
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
static uint8_t *verify_buf;
#endif


/*
 * u-boot nand hw ecc
 */
static int iNX_BCH_VAR_K	 = ECC_PAGE_SIZE;	/* 512 or 1024 */
static int iNX_BCH_VAR_M	 = 14;				/* 13 or 14 */
static int iNX_BCH_VAR_T	 = 60;				/* 4, 8, 12, 16, 24, 40, 60 ... */
static int iNX_BCH_VAR_R	 = 104;				/* (iNX_BCH_VAR_K * iNX_BCH_VAR_M) / 8 - 1 */
static int iNX_BCH_VAR_TMAX  = 60;				/* eccsize == 512 ? 24 : 60 */

static struct NX_MCUS_RegisterSet * const _pNCTRL =
	(struct NX_MCUS_RegisterSet *)IO_ADDRESS(PHY_BASEADDR_MCUSTOP_MODULE);

static void __ecc_reset_decoder(void)
{
	_pNCTRL->NFCONTROL |= NX_NFCTRL_ECCRST;
	// disconnect syndrome path
	_pNCTRL->NFECCAUTOMODE = (_pNCTRL->NFECCAUTOMODE & ~(NX_NFACTRL_ELP)) | NX_NFACTRL_SYN;
}

static void __ecc_decode_enable(int eccsize)	/* 512 or 1024 */
{
	// connect syndrome path
	_pNCTRL->NFECCAUTOMODE = (_pNCTRL->NFECCAUTOMODE & ~(NX_NFACTRL_ELP | NX_NFACTRL_SYN));

	// run ecc
	_pNCTRL->NFECCCTRL =
		(1 << NX_NFECCCTRL_RUNECC_W)   |	   // run ecc
		(0 << NX_NFECCCTRL_ELPLOAD)   |
		(NX_NF_DECODE << NX_NFECCCTRL_DECMODE_W)	|
		(0 << NX_NFECCCTRL_ZEROPAD)	|
		((iNX_BCH_VAR_T & 0x7F) << NX_NFECCCTRL_ELPNUM)		|
		((iNX_BCH_VAR_R & 0xFF) << NX_NFECCCTRL_PDATACNT)	|
		(((eccsize-1) & 0x3FF)  << NX_NFECCCTRL_DATACNT);
}

static void __ecc_write_ecc_decode(unsigned int *ecc, int eccbyte)
{
	volatile U32 *pNFORGECC = _pNCTRL->NFORGECC;
	volatile int i, len;

	/* align 4byte */
	len = DIV_ROUND_UP(eccbyte, sizeof(U32));

	for(i = 0; len > i; i++)
		*pNFORGECC++ = *ecc++;
}

static void __ecc_wait_for_decode(void)
{
	while (0 ==(_pNCTRL->NFECCSTATUS & NX_NFECCSTATUS_DECDONE));
	{ ; }
}

static unsigned int __ecc_decode_error(void)
{
	return (int)(_pNCTRL->NFECCSTATUS & NX_NFECCSTATUS_ERROR);
}

static void __ecc_start_correct(int eccsize)
{
	// load elp
	_pNCTRL->NFECCCTRL =
		(0 << NX_NFECCCTRL_RUNECC_W)   |
		(1 << NX_NFECCCTRL_ELPLOAD)    |	   // load elp
		(NX_NF_DECODE << NX_NFECCCTRL_DECMODE_W)	|
		(0 << NX_NFECCCTRL_ZEROPAD)	|
 		((iNX_BCH_VAR_T & 0x07F) << NX_NFECCCTRL_ELPNUM )	|
		((iNX_BCH_VAR_R & 0x0FF) << NX_NFECCCTRL_PDATACNT)	|
	 	(((eccsize - 1) & 0x3FF) << NX_NFECCCTRL_DATACNT);
}

static void __ecc_wait_for_correct(void)
{
	while (_pNCTRL->NFECCSTATUS & NX_NFECCSTATUS_BUSY)
	{ ; }
}

static int __ecc_get_err_location(unsigned int *pLocation)
{
	volatile U32 *pELoc = _pNCTRL->NFERRLOCATION;
	volatile int len = ECC_HW_BITS/2;
	volatile int err, i;

	// it's not error correctable
	if (((_pNCTRL->NFECCSTATUS & NX_NFECCSTATUS_NUMERR) >>  4) !=
		((_pNCTRL->NFECCSTATUS & NX_NFECCSTATUS_ELPERR) >> 16))
		return -1;

	for (i = 0; len > i; i++) {
		register U32 regvalue = *pELoc++;
		*pLocation++ = (regvalue>>0  & 0x3FFF)^0x7;
		*pLocation++ = (regvalue>>14 & 0x3FFF)^0x7;
	}

	err = (_pNCTRL->NFECCSTATUS & NX_NFECCSTATUS_NUMERR) >> 4;
	return err;
}

static void __ecc_setup_encoder(void)
{
    NX_MCUS_SetNANDRWDataNum(iNX_BCH_VAR_K);
    NX_MCUS_SetParityCount(iNX_BCH_VAR_R);
    NX_MCUS_SetNumOfELP(iNX_BCH_VAR_T);
}

static void __ecc_encode_enable(void)
{
	NX_MCUS_SetNFDecMode(NX_MCUS_DECMODE_ENCODER);
	NX_MCUS_RunECCEncDec();
}

static void __ecc_read_ecc_encode(unsigned int *ecc, int eccbyte)
{
	volatile U32 *pNFECC = _pNCTRL->NFECC;
	volatile int i, len;

	/* align 4byte */
	len = DIV_ROUND_UP(eccbyte, sizeof(U32));

	for(i = 0; len > i; i++)
		*ecc++ = *pNFECC++;
}

static void __ecc_wait_for_encode(void)
{
	while ( 0==(_pNCTRL->NFECCSTATUS & NX_NFECCSTATUS_ENCDONE) )
	{ ; }
}

/*
 * u-boot nand hw ecc interface
 */

/* ECC related defines */
#define	ECC_HW_MAX_BYTES		((106/32)*32 + 32) 	/* 128 */

static struct nand_ecclayout nand_ecc_oob = {
	.eccbytes 	=   0,
	.eccpos 	= { 0, },
	.oobfree 	= { {.offset = 0, .length = 0} }
};


static int nand_sw_ecc_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;

	for (i = 0; len > i; i++)
		if (buf[i] != readb(chip->IO_ADDR_R))
			return -EFAULT;
	return 0;
}

static uint32_t  eccbuff[ECC_HW_MAX_BYTES/4];
static int errpos[ECC_HW_BITS];

static int nand_hw_ecc_read_page(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf, int oob_required, int page)
{
	int i, k, n, ret = 0, retry = 0;

	int eccsteps = chip->ecc.steps;
	int eccbytes = chip->ecc.bytes;
	int eccsize  = chip->ecc.size;
	int eccrange = 8 * eccsize;

	uint8_t  *ecccode = (uint8_t*)eccbuff;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	uint8_t  *p = buf;

	uint32_t *errdat;
	int err = 0, errcnt = 0;
	uint32_t corrected = 0, failed = 0;
	uint32_t max_bitflips = 0;
	int is_erasedpage = 0;

	DBGOUT("%s, page=%d, ecc mode=%d, bytes=%d, page=%d, step=%d\n",
		__func__, page, ECC_HW_BITS, eccbytes, mtd->writesize, eccsteps);
	do {
		/* reset value */
		eccsteps = chip->ecc.steps;
		p = buf;
#ifndef NO_ISSUE_MTD_BITFLIP_PATCH	/* freestyle@2013.09.26 */
		corrected = failed = 0;
#endif

		if (512 >= mtd->writesize) {
			chip->ecc.read_oob(mtd, chip, page);
			chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);
		} else {
			chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
			chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
			chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);
		}

		for (n = 0; eccsteps; eccsteps--, p += eccsize) {

			memset (eccbuff, 0x00, sizeof eccbuff);

			for (i = 0; i < eccbytes; i++, n++)
				ecccode[i] = chip->oob_poi[eccpos[n]];

			/* set hw ecc */
			__ecc_reset_decoder();	/* discon syndrome */
			__ecc_write_ecc_decode((unsigned int*)ecccode, eccbytes);
			__ecc_decode_enable(eccsize);

			/* read data */
			chip->read_buf(mtd, p, eccsize);

			__ecc_wait_for_decode();
			err = __ecc_decode_error();
			if (err) {
				/* check erase status */
				for (i = 0 ; eccbytes > i; i++)
					if (0xFF != ecccode[i]) break;
				if (i == eccbytes) {
					is_erasedpage = 1;
					continue;
				}

				__ecc_start_correct(eccsize);
				__ecc_wait_for_correct();

#if (0)
				if (((_pNCTRL->NFECCSTATUS & NX_NFECCSTATUS_ELPERR) >>  16) >= chip->ecc.strength)
					printk ("  page: %d, step:%d, numerr: %d, elperr: %d\n", page, 
							(chip->ecc.steps-eccsteps),
							((_pNCTRL->NFECCSTATUS & NX_NFECCSTATUS_NUMERR) >>  4),
							((_pNCTRL->NFECCSTATUS & NX_NFECCSTATUS_ELPERR) >> 16));
#endif

				/* correct Error */
				errcnt = __ecc_get_err_location((unsigned int *)errpos);
				if (0 >= errcnt) {
					ERROUT("page %d step %2d ecc error, can't %s ...\n",
						page, (chip->ecc.steps-eccsteps), 0==errcnt?"detect":"correct");
					failed++;
					ret = -EBADMSG;
					printk("read retry page %d, retry: %d \n", page, retry);
					goto retry_rd;	/* EXIT */
				} else {
					ECCERR("page %d step %2d, ecc error %2d\n", page, (chip->ecc.steps-eccsteps), errcnt);
					for (k = 0; errcnt > k; k++) {
						errdat = (uint32_t*)p;
						ECCERR("offs = 0x%04x: 0x%4x -> ",
							((chip->ecc.steps-eccsteps)*eccsize)+((errpos[k]/32)*4), errdat[errpos[k]/32]);
						/* Error correct */
						if (errpos[k] >= eccrange) 		/* skip ecc error in oob */
							continue;
						errdat[errpos[k] / 32] ^= 1U<<(errpos[k] % 32);
						ECCERR("0x%4x\n", errdat[errpos[k]/32]);
					}

					#if !(U_BOOT_NAND)
					corrected += errcnt;
					#endif
					max_bitflips = max_t(unsigned int, max_bitflips, errcnt);
				}
			}
		}

#ifdef CONFIG_NAND_RANDOMIZER
		if (!no_nand_randomize && !is_erasedpage)
		{
			randomizer_page (page & pages_per_block_mask, buf, mtd->writesize);
			//printk("  page: %d ------->    derandomize\n", page);
		}
#endif

		mtd->ecc_stats.corrected += corrected;
		if (failed > 0)
			mtd->ecc_stats.failed++;

		DBGOUT("DONE %s, ret=%d\n", __func__, ret);
		return max_bitflips;

retry_rd:
		retry++;
	} while (NAND_READ_RETRY > retry);

	mtd->ecc_stats.corrected += corrected;
	if (failed > 0)
		mtd->ecc_stats.failed++;

	DBGOUT("FAIL %s, ret=%d, retry=%d\n", __func__, ret, retry);
	return ret;
}

static int nand_hw_ecc_write_page(struct mtd_info *mtd, struct nand_chip *chip,
				  const uint8_t *buf, int oob_required)
{
	int i, n;
	int eccsteps = chip->ecc.steps;
	int eccbytes = chip->ecc.bytes;
	int eccsize  = chip->ecc.size;

	uint8_t  *ecccode = (uint8_t*)eccbuff;
	uint32_t *eccpos   = chip->ecc.layout->eccpos;
	uint8_t  *p = (uint8_t *)buf;

	DBGOUT("%s\n", __func__);

    __ecc_setup_encoder();

	/* write data and get ecc */
	for (n = 0; eccsteps; eccsteps--, p += eccsize) {
		memset (eccbuff, 0x00, sizeof eccbuff);

		__ecc_encode_enable();

		chip->write_buf(mtd, p, eccsize);

		/* get ecc code from ecc register */
		__ecc_wait_for_encode();
		__ecc_read_ecc_encode((uint32_t *)ecccode, eccbytes);

		/* set oob with ecc */
		for (i = 0; i < eccbytes; i++, n++)
			chip->oob_poi[eccpos[n]] = ecccode[i];
	}

	/* write oob */
	chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);

	return 0;
}

static int nand_hw_write_page(struct mtd_info *mtd, struct nand_chip *chip,
			   const uint8_t *buf, int oob_required, int page, int cached, int raw)
{
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	struct mtd_ecc_stats stats;
	int ret = 0;
#endif
	int status;
	uint8_t *funcbuf = (uint8_t *)buf;

#ifdef CONFIG_NAND_RANDOMIZER
	if (!no_nand_randomize && randomize_buf) {
		memcpy (randomize_buf, buf, mtd->writesize);

		randomizer_page (page & pages_per_block_mask, randomize_buf, mtd->writesize);

		funcbuf = randomize_buf;
	}
#endif

	DBGOUT("%s page %d, raw=%d\n", __func__, page, raw);
	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);

	/* for hynix H27UBG8T2BTR */
	//ndelay(200);

	/* not verify */
	if (raw)
		chip->ecc.write_page_raw(mtd, chip, funcbuf, oob_required);
	else
		chip->ecc.write_page(mtd, chip, funcbuf, oob_required);

	/*
	 * Cached progamming disabled for now, Not sure if its worth the
	 * trouble. The speed gain is not very impressive. (2.3->2.6Mib/s)
	 */
	cached = 0;

	if (!cached || !(chip->options & NAND_CACHEPRG)) {

		chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
		status = chip->waitfunc(mtd, chip);
		/*
		 * See if operation failed and additional status checks are
		 * available
		 */
		if ((status & NAND_STATUS_FAIL) && (chip->errstat))
			status = chip->errstat(mtd, chip, FL_WRITING, status,
					       page);

		if (status & NAND_STATUS_FAIL)
			return -EIO;
	} else {
		chip->cmdfunc(mtd, NAND_CMD_CACHEDPROG, -1, -1);
		status = chip->waitfunc(mtd, chip);
	}

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	if (raw)
		return 0;

	/* Send command to read back the data */
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);
	ret = chip->ecc.read_page(mtd, chip, verify_buf, oob_required, page);
	if (ret < 0)
	{
		ERROUT ("  read page (%d) for write-verify failed!\n", page);
		return -EIO; //		return ret;
	}

	if (memcmp (verify_buf, buf, mtd->writesize))
	{
		ERROUT ("%s fail verify %d page\n", __func__, page);
		return -EIO;
	}

	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);
#endif
	return 0; // mtd->ecc_stats.corrected - stats.corrected ? -EUCLEAN : 0
}


int nand_ecc_alloc_buffer(struct mtd_info *mtd)
{
	int ret = 0;

#ifdef CONFIG_NAND_RANDOMIZER
	pages_per_block_mask = (mtd->erasesize/mtd->writesize) - 1;

	randomize_buf = kzalloc(mtd->writesize, GFP_KERNEL);
	if (!randomize_buf) {
		ERROUT("randomize buffer alloc failed\n");
	}
	//printk  ("    [%s:%d] randomize_buf: %p, mtd->writesize: %d, pages_per_block_mask: %x\n",
	//	__func__, __LINE__, randomize_buf, mtd->writesize, pages_per_block_mask);


	// kfree ...
#endif

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	verify_buf = kmalloc(NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE, GFP_KERNEL);

	// kfree
#endif

	return ret;
}

int nand_ecc_layout_hwecc(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct nand_ecclayout *layout = chip->ecc.layout;
	struct nand_oobfree *oobfree  = chip->ecc.layout->oobfree;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	int ecctotal = chip->ecc.total;
	int oobsize	 = mtd->oobsize;
	int i = 0, n = 0;
	int ret = 0;

	if (512 > mtd->writesize) {
		printk(KERN_INFO "NAND ecc: page size %d not support hw ecc\n",
			mtd->writesize);
		chip->ecc.mode 			= NAND_ECC_SOFT;
		chip->ecc.read_page 	= NULL;
		chip->ecc.read_subpage 	= NULL;
		chip->ecc.write_page 	= NULL;
		chip->ecc.layout		= NULL;
		chip->verify_buf		= nand_sw_ecc_verify_buf;

		if ( chip->buffers &&
			!(chip->options & NAND_OWN_BUFFERS)) {
			kfree(chip->buffers);
			chip->buffers = NULL;
		}
		ret = nand_scan_tail(mtd);
		printk(KERN_INFO "NAND ecc: Software \n");
		return ret;
	}

	if (ecctotal > oobsize)  {
		printk(KERN_INFO "\n");
		printk(KERN_INFO "==================================================\n");
		printk(KERN_INFO "error: %d bit hw ecc mode requires ecc %d byte	\n", ECC_HW_BITS, ecctotal);
		printk(KERN_INFO "       it's over the oob %d byte for page %d byte	\n", oobsize, mtd->writesize);
		printk(KERN_INFO "==================================================\n");
		printk(KERN_INFO "\n");
		return -EINVAL;
	}

	/*
	 * set ecc layout
	 */
	if (16 >= mtd->oobsize) {
		for (i = 0, n = 0; ecctotal>i; i++, n++) {
			if (5 == n) n += 1;	// Bad marker
			eccpos[i] = n;
		}
		oobfree->offset  = n;
		oobfree->length  = mtd->oobsize - ecctotal - 1;
		layout->oobavail = oobfree->length;

    	mtd->oobavail = oobfree->length;
		printk("hw ecc %2d bit, oob %3d, bad '5', ecc 0~4,6~%d (%d), free %d~%d (%d) ",
			ECC_HW_BITS, oobsize, ecctotal+1-1, ecctotal, oobfree->offset,
			oobfree->offset+oobfree->length-1, oobfree->length);
	} else {

		oobfree->offset  = 2;
		oobfree->length  = mtd->oobsize - ecctotal - 2;
		layout->oobavail = oobfree->length;

		n = oobfree->offset + oobfree->length;
		for (i = 0; i < ecctotal; i++, n++)
			eccpos[i] = n;

    	mtd->oobavail = oobfree->length;
		printk("hw ecc %2d bit, oob %3d, bad '0,1', ecc %d~%d (%d), free 2~%d (%d) ",
			ECC_HW_BITS, oobsize, oobfree->offset+oobfree->length, n-1,
			ecctotal, oobfree->length+2-1, oobfree->length);
	}

	/* must reset mtd */
	mtd->ecclayout = chip->ecc.layout;
	mtd->oobavail  = chip->ecc.layout->oobavail;
	return ret;
}

int nand_hw_ecc_init_device(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	int eccbyte = 0, eccsize = ECC_PAGE_SIZE, eccidx;
	NX_MCUS_ECCMODE eccmode;

	/*
	 * HW ECC bytes:
	 *
	 *  4 bit ecc need " 4 * 13 =  52" bit (  6.5B) ecc code per  512 Byte
	 *  8 bit ecc need " 8 * 13 = 104" bit ( 13.0B) ecc code per  512 Byte
     * 12 bit ecc need "12 * 13 = 156" bit ( 19.5B) ecc code per  512 Byte
	 * 16 bit ecc need "16 * 13 = 208" bit ( 26.0B) ecc code per  512 Byte
	 * 24 bit ecc need "24 * 13 = 312" bit ( 39.0B) ecc code per  512 Byte
	 * 24 bit ecc need "24 * 14 = 336" bit ( 42.0B) ecc code per 1024 Byte
	 * 40 bit ecc need "40 * 14 = 560" bit ( 70.0B) ecc code per 1024 Byte
	 * 60 bit ecc need "60 * 14 = 840" bit (105.0B) ecc code per 1024 Byte
	 *
	 *  Page  512 Byte +  16 Byte
	 *  Page 2048 Byte +  64 Byte
	 *  Page 4096 Byte + 128 Byte
     *
     *  Page 8192 Byte + 436 Byte (MLC)
	 */
	switch (ECC_HW_BITS) {
	case  4: eccbyte =   7, eccidx = 13, eccmode = NX_MCUS_4BITECC;
			if (512 != eccsize) goto _ecc_fail;
			break;
	case  8: eccbyte =  13, eccidx = 13,  eccmode = NX_MCUS_8BITECC;
			if (512 != eccsize) goto _ecc_fail;
			break;
    case 12: eccbyte =  20, eccidx = 13,  eccmode = NX_MCUS_12BITECC;
    		if (512 != eccsize) goto _ecc_fail;
    		break;
	case 16: eccbyte =  26, eccidx = 13,  eccmode = NX_MCUS_16BITECC;
			if (512 != eccsize) goto _ecc_fail;
			break;
	case 24: 
			if (eccsize == 512)
				eccbyte = 39, eccidx = 13, eccmode = NX_MCUS_24BITECC_512;
			else
				eccbyte = 42, eccidx = 14, eccmode = NX_MCUS_24BITECC;
			break;
	case 40: eccbyte =  70, eccidx = 14,  eccmode = NX_MCUS_40BITECC;
			if (1024 != eccsize) goto _ecc_fail;
			break;
	case 60: eccbyte = 105, eccidx = 14,  eccmode = NX_MCUS_60BITECC;
			if (1024 != eccsize) goto _ecc_fail;
			break;
	default:
		goto _ecc_fail;
		break;
	}


	iNX_BCH_VAR_M			= eccidx;			/* 13 or 14 */
	iNX_BCH_VAR_T			= ECC_HW_BITS;		/* 4, 8, 12, 16, 24, 40, 60 ... */
	iNX_BCH_VAR_R			= DIV_ROUND_UP(iNX_BCH_VAR_M * iNX_BCH_VAR_T, 8) - 1;
	iNX_BCH_VAR_TMAX		= (eccsize == 512 ? 24 : 60);
	DBGOUT("%s ecc %d bit, eccsize=%d, parity=%d, eccbyte=%d, eccindex=%d\n",
		__func__, ECC_HW_BITS, eccsize, iNX_BCH_VAR_R, eccbyte, eccidx);

	chip->ecc.mode 			= NAND_ECC_HW;
	chip->ecc.size 			= eccsize;			/* per 512 or 1024 bytes */
	chip->ecc.bytes 		= eccbyte;
	chip->ecc.layout		= &nand_ecc_oob;
	chip->ecc.read_page 	= nand_hw_ecc_read_page;
	chip->ecc.write_page 	= nand_hw_ecc_write_page;
	chip->write_page		= nand_hw_write_page;
	chip->ecc.strength		= ((eccbyte * 8 / fls (8*eccsize)) * 80 / 100);

	NX_MCUS_ResetNFECCBlock();
	NX_MCUS_SetECCMode(eccmode);

	return 0;

_ecc_fail:
	printk("Fail: not support ecc %d bits for pagesize %d !!!\n", ECC_HW_BITS, eccsize);
	return -EINVAL;
}
#endif /* CONFIG_MTD_NAND_ECC_HW */

