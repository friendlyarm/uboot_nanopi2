/*
 * (C) Copyright 2010
 * KOO Bon-Gyu, Nexell Co, <freestyle@nexell.co.kr>
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

#include <common.h>
#include <errno.h>
#include <nand.h>

#include <asm/io.h>
#include <platform.h>
#include <mach-api.h>
#include <asm/gpio.h>

#include "nand_ecc.h"

#if	(0)
#define TM_DBGOUT(msg...)		{ printk(KERN_INFO "nand: " msg); }
#else
#define TM_DBGOUT(msg...)		do {} while (0)
#endif

#if	(0)
#define DBGOUT(msg...)		do { printk(KERN_INFO "nand: " msg); } while (0)
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

#define ERROUT(msg...)		{ 					\
		printk(KERN_ERR "ERROR: %s, %s line %d: \n",		\
			__FILE__, __FUNCTION__, __LINE__),	\
		printk(KERN_ERR msg); }

#define CLEAR_RnB(r)							\
	r = NX_MCUS_GetInterruptPending(0);			\
	if (r) {									\
		NX_MCUS_ClearInterruptPending(0); 		\
		NX_MCUS_GetInterruptPending(0); 		\
	}

/*------------------------------------------------------------------------------
 * nand interface
 */
static void nand_select_chip(struct mtd_info *mtd, int chipnr)
{
	DBGOUT("%s, chipnr=%d\n", __func__, chipnr);

#if defined(CFG_NAND_OPTIONS)
	struct nand_chip *chip = mtd->priv;
	chip->options |= CFG_NAND_OPTIONS;
#endif

	if (chipnr > 4) {
		ERROUT("not support nand chip index %d\n", chipnr);
		return;
	}

	if (-1 == chipnr) {
		NX_MCUS_SetNFCSEnable(CFALSE);		// nand chip select control disable
	} else {
		NX_MCUS_SetNFBank(chipnr);
		NX_MCUS_SetNFCSEnable(CTRUE);
	}
}

#define MASK_CLE	0x10	/* NFCM   + 2C00_0000 */
#define MASK_ALE	0x18	/* NFADDR + 2C00_0000 */

static void nand_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *chip = mtd->priv;
	void __iomem* addr = chip->IO_ADDR_W;
	int ret = 0;

	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE)
	{
		if (cmd != NAND_CMD_STATUS &&
			cmd != NAND_CMD_READID &&
			cmd != NAND_CMD_RESET)
			CLEAR_RnB(ret);

		//printk ("  [%s:%d] command: %02x\n", __func__, __LINE__, (unsigned char)cmd);
		writeb(cmd, addr + MASK_CLE);
	}
	else if (ctrl & NAND_ALE)
	{
		//printk ("  [%s:%d] address: %02x\n", __func__, __LINE__, (unsigned char)cmd);
		writeb(cmd, addr + MASK_ALE);
	}
}

struct nand_timings {
	uint32_t tACS;			// tACS
	uint32_t tCAH;			// tCAH
	uint32_t tCOS;			// tCOS
	uint32_t tOCH;			// tOCH
	uint32_t tACC;			// tACC
};

struct nand_timing_mode5_params {
	uint32_t tCS;
	uint32_t tCH;
	uint32_t tCLS_tALS;
	uint32_t tCLH_tALH;
	uint32_t tWP;
	uint32_t tWH;
	uint32_t tWC;
	uint32_t tDS;
	uint32_t tDH;
	uint32_t tCEA;		// max
	uint32_t tREA;		// max
	uint32_t tRP;
	uint32_t tREH;
	uint32_t tRC;
	uint32_t tCOH;
} NAND_TM5_PARAM[6] =
{//	tCS tCH tCLS tCLH tWP tWH  tWC tDS tDH tCEA tREA tRP tREH  tRC tCOH
	{70, 20,  50,  20, 50, 30, 100, 40, 20, 100,  40, 50,  30, 100,  0},		// mode 0
	{35, 10,  25,  10, 25, 15,  45, 20, 10,  45,  30, 25,  15,  50, 15},		// mode 1
	{25, 10,  15,  10, 17, 15,  35, 15,  5,  30,  25, 17,  15,  35, 15},		// mode 2
	{25,  5,  10,   5, 15, 10,  30, 10,  5,  25,  20, 15,  10,  30, 15},		// mode 3
	{20,  5,  10,   5, 12, 10,  25, 10,  5,  25,  20, 12,  10,  25, 15},		// mode 4
	{15,  5,  10,   5, 10,  7,  20,  7,  5,  25,  16, 10,   7,  20, 15}			// mode 5
};



/**
 * nand_calc_timing_mode - calculate based on the timing mode
 * @mode: timing mode. (0 ~ 5)
 * @clkhz: BCLK clock rate in hz
 * @timings: value to set the timing register [Returns]
 */
static int nand_calc_timing_mode (uint32_t mode, uint32_t clkhz,
			struct nand_timings *timings)
{
	uint32_t nclk; 
	uint32_t tCS, tCH, tCLS, tCLH, tWP, tWH, tWC, tDS, tDH, tCEA, tREA, tRP, tREH, tRC, tCOH;
	uint32_t tRCS, tWCS0, tWCS1, tRACC, tWACC, tRCH, tWCH0, tWCH1;
	uint32_t tCOS, tACC, tOCH;
	struct nand_timing_mode5_params *pntmp;

	// error check
	if(mode >= 6)
		mode = 0;
	if(clkhz < 1000000)		// BCLK is minimum > 1MHz
		clkhz = 1000000;
	if(!timings)
		return -1;

	// varient convertion
	nclk = 1000000000/(clkhz/1000);	// convert to pico second
	pntmp = &NAND_TM5_PARAM[mode];
	tCS = pntmp->tCS*1000000;
	tCH = pntmp->tCH*1000000;
	tCLS = pntmp->tCLS_tALS*1000000;
	tCLH = pntmp->tCLH_tALH*1000000;
	tWP = pntmp->tWP*1000000;
	tWH = pntmp->tWH*1000000;
	tWC = pntmp->tWC*1000000;
	tDS = pntmp->tDS*1000000;
	tDH = pntmp->tDH*1000000;
	tCEA = pntmp->tCEA*1000000;
	tREA = pntmp->tREA*1000000;
	tRP = pntmp->tRP*1000000;
	tREH = pntmp->tREH*1000000;
	tRC = pntmp->tRC*1000000;
	tCOH = pntmp->tCOH*1000000;

	TM_DBGOUT("nclk: %u, mode: %u\n", nclk, mode);
	TM_DBGOUT("tCS: %u, tCH: %u tCLS: %u, tCLH: %u, tWP: %u, tWH: %u, tWC: %u\n",
		tCS, tCH, tCLS, tCLH, tWP, tWH, tWC);
	TM_DBGOUT("tDS: %u, tDH: %u, tCEA: %u, tREA: %u,\n  tRP: %u, tREH: %u, tRC: %u, tCOH: %u\n", 
		tDS, tDH, tCEA, tREA, tRP, tREH, tRC, tCOH);

	// timing calculation
	tRCS = (tCEA-tREA)/nclk;	//(tCEA-tREA)/nclk
	tWCS0 = (tCS-tWP)/nclk;		//(tCS-tWP)/nclk
	tWCS1 = (tCLS-tWP)/nclk;	//(tCLS-tWP)/nclk
	tRACC = ((tREA+nclk*2000)>tRP?(tREA+nclk*2000):tRP)/nclk;	//MAX(tREA+nclk*2, tRP)/nclk
	tWACC = ((tWP>tDS)?tWP:tDS)/nclk;	//MAX(tWP,tDS)/nclk
	tRCH = ((tRC-tRP)>tREH?(tRC-tRP):tREH)/nclk-tRCS;	//MAX(tRC-tRP,tREH)/nclk-tRCS
	tWCH0 = ((tWC-tWP)>tWH?(tWC-tWP):tWH)/nclk-(tWCS0>tWCS1?tWCS0:tWCS1);//MAX(tWC-tWP, tWH)/nclk - MAX(tWCS0, tWCS1)
	tWCH1 = ((tCH>tCLH?tCH:tCLH)>tDH?(tCH>tCLH?tCH:tCLH):tDH)/nclk;		//MAX(tCH,tCLH,tDH)/nclk

	TM_DBGOUT("tRCS: %u, tWCS0: %u, tWCS1: %u, tRACC: %u, tWACC: %u, tRCH: %u, tWCH0: %u, tWCH1: %u\n",
		tRCS, tWCS0, tWCS1, tRACC, tWACC, tRCH, tWCH0, tWCH1);

	// convertion to clock base asynchronous nand controller state machine
	tCOS = (tRCS>tWCS0?tRCS:tWCS0)>tWCS1?(tRCS>tWCS0?tRCS:tWCS0):tWCS1;//MAX(tRCS, tWCS0, tWCS1);
	tACC = tRACC>tWACC?tRACC:tWACC;		//MAX(tRACC, tWACC);
	tOCH = (tRCH>tWCH0?tRCH:tWCH0)>tWCH1?(tRCH>tWCH0?tRCH:tWCH0):tWCH1;//MAX(tRCH, tWCH0, tWCH1);

	TM_DBGOUT("tCOS: %u, tACC: %u, tOCH: %u\n", tCOS, tACC, tOCH);

	// convert to register value
	tCOS += 999;	// round up tCOS
	tACC += 999;	// round up tACC
	tOCH += 999;	// round up tOCH

	// fillup paramter	
	timings->tACS = 0;
	timings->tCOS = tCOS/1000;
	timings->tACC = tACC/1000;
	timings->tOCH = tOCH/1000;
	timings->tCAH = 0;

	TM_DBGOUT("  fill - tCOS: %u, tACC: %u, tOCH: %u\n", tCOS/1000, tACC/1000, tOCH/1000);

	return 0;
}

static int nand_onfi_timing_set(struct mtd_info *mtd, uint32_t mode)
{
	struct clk *clk;
	uint32_t clkhz;
	struct nand_timings tmgs;
	int ret;


	clk = clk_get (NULL, CORECLK_NAME_BCLK), clkhz = clk_get_rate(clk), clk_put(clk);
	TM_DBGOUT(" BCLK: %u HZ\n", clkhz);

	// setting - nand flash

	// setting - nand controller timming
	NX_MCUS_GetNANDBUSConfig
	(
		0,
		&tmgs.tACS,
		&tmgs.tCAH,
		&tmgs.tCOS,
		&tmgs.tOCH,
		&tmgs.tACC
	);
	TM_DBGOUT("[BEFORE]  tACS: %u, tCAH: %u, tCOS: %u, tOCH: %u, tACC: %u\n", 
		tmgs.tACS, tmgs.tCAH, tmgs.tCOS, tmgs.tOCH, tmgs.tACC);

	ret = nand_calc_timing_mode (mode, clkhz, &tmgs);
	if (ret < 0)
		return -1;

	NX_MCUS_SetNANDBUSConfig
	(
		0,
		tmgs.tACS,
		tmgs.tCAH,
		tmgs.tCOS,
		tmgs.tOCH,
		tmgs.tACC
	);

	NX_MCUS_GetNANDBUSConfig
	(
		0,
		&tmgs.tACS,
		&tmgs.tCAH,
		&tmgs.tCOS,
		&tmgs.tOCH,
		&tmgs.tACC
	);
	TM_DBGOUT("[AFTER]  tACS: %u, tCAH: %u, tCOS: %u, tOCH: %u, tACC: %u\n", 
		tmgs.tACS, tmgs.tCAH, tmgs.tCOS, tmgs.tOCH, tmgs.tACC);


	return 0;
}

/* timing set */
static int nexell_nand_timing_set(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	uint32_t ret = ONFI_TIMING_MODE_UNKNOWN, mode;

#ifdef CONFIG_SYS_NAND_ONFI_DETECTION
	ret = onfi_get_async_timing_mode(chip);
#endif
	if (ret == ONFI_TIMING_MODE_UNKNOWN)
	{
		NX_MCUS_SetNANDBUSConfig
		(
			 0, /* NF */
			 CFG_SYS_NAND_TACS,              // tACS  ( 0 ~ 3 )
			 CFG_SYS_NAND_TCAH,              // tCAH  ( 0 ~ 3 )
			 CFG_SYS_NAND_TCOS,              // tCOS  ( 0 ~ 3 )
			 CFG_SYS_NAND_TCOH,              // tCOH  ( 0 ~ 3 )
			 CFG_SYS_NAND_TACC               // tACC  ( 1 ~ 16)
		);

		return 0;
	}

	mode = fls(ret) - 1;
	TM_DBGOUT("ONFI TIMING MODE (%d) \n", mode);

	nand_onfi_timing_set (mtd, mode);

	return 0;
}


/*
 * Enable NAND write protect
 */
static void nxp_wp_enable(void)
{
	gpio_set_value(CFG_IO_NAND_nWP, 0);
}

/*
 * Disable NAND write protect
 */
static void nxp_wp_disable(void)
{
	gpio_set_value(CFG_IO_NAND_nWP, 1);
}


static int nand_dev_ready(struct mtd_info *mtd)
{
	int ret = 0;
	CLEAR_RnB(ret);
	DBGOUT("[%s, RnB=%d]\n", ret?"READY":"BUSY", NX_MCUS_IsNFReady());
	return ret;
}

static void nand_dev_init(struct mtd_info *mtd)
{
	NX_MCUS_SetAutoResetEnable(CTRUE);
	NX_MCUS_ClearInterruptPending(0);
	NX_MCUS_SetInterruptEnableAll(CFALSE);
	NX_MCUS_SetNFBank(0);
	NX_MCUS_SetNFCSEnable(CFALSE);

	gpio_direction_output (CFG_IO_NAND_nWP, 1);
}

/*------------------------------------------------------------------------------
 * u-boot nand module
 */
#if defined (CONFIG_MTD_NAND_ECC_BCH)
static uint8_t *verify_page;
static int nand_bch_write_page(struct mtd_info *mtd, struct nand_chip *chip,
			   const uint8_t *buf, int oob_required, int page, int cached, int raw)
{
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	int ret = 0;
#endif
	int status;

	DBGOUT("%s page %d, raw=%d\n", __func__, page, raw);
	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);

	/* not verify */
	if (raw)
		chip->ecc.write_page_raw(mtd, chip, buf, oob_required);
	else
		chip->ecc.write_page(mtd, chip, buf, oob_required);

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
	chip->ecc.read_page(mtd, chip, (uint8_t *)verify_page, oob_required, page);
	if (ret < 0)
		return -EIO;

	if (memcmp (verify_page, buf, mtd->writesize))
	{
		ERROUT ("%s fail verify %d page\n", __func__, page);
		return -EIO;
	}

	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);
#endif
	return 0; // mtd->ecc_stats.corrected - stats.corrected ? -EUCLEAN : 0
}

static int nand_ecc_layout_swbch(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct nand_ecclayout *layout = &chip->ecc.layout[0];
	struct nand_oobfree *oobfree  = layout->oobfree;
	int ecctotal = chip->ecc.total;
	int oobsize	 = mtd->oobsize;

	printk("sw bch ecc %d bit, oob %2d, bad '0,1', ecc %d~%d (%d), free %d~%d (%d) ",
		ECC_BCH_BITS, oobsize, oobfree->offset+oobfree->length, oobsize-1, ecctotal,
		oobfree->offset, oobfree->length + 1, oobfree->length);

	verify_page = kzalloc(mtd->writesize, GFP_KERNEL);
	if (!verify_page)
		return -ENOMEM;

	return 0;
}
#endif

int nand_ecc_layout_check(struct mtd_info *mtd)
{
	int ret = 0;
#if defined (CONFIG_MTD_NAND_ECC_HW)
	ret = nand_ecc_layout_hwecc(mtd);
#elif defined (CONFIG_MTD_NAND_ECC_BCH)
	ret = nand_ecc_layout_swbch(mtd);
#endif
	return ret;
}

int nand_ecc_post_scan(struct mtd_info *mtd)
{
	int ret = 0;

	ret = nand_ecc_layout_check(mtd);
#ifdef CONFIG_MTD_NAND_ECC_HW
	nand_ecc_alloc_buffer(mtd);
#endif

	return ret;
}

/*
 * calling from nand_init (drivers/mtd/nand/nand.c)
 */
int board_nand_init(struct nand_chip *chip)
{
	struct mtd_info  *mtd = &nand_info[0];
	int ret = 0;

	nand_dev_init(mtd);

	/*
	* nand callbacks
	*/
	chip->IO_ADDR_R 	= (void __iomem *)CONFIG_SYS_NAND_BASE;
	chip->IO_ADDR_W 	= (void __iomem *)CONFIG_SYS_NAND_BASE;
	chip->cmd_ctrl 		= nand_cmd_ctrl;
	chip->dev_ready 	= nand_dev_ready;
	chip->select_chip 	= nand_select_chip;
	chip->chip_delay 	= 15;
#if defined (CONFIG_MTD_NAND_ECC_BCH)
	chip->write_page	= nand_bch_write_page;
#endif

	/*
	 * error correct mode
	 */
#if   defined (CONFIG_MTD_NAND_ECC_HW)
	ret = nand_hw_ecc_init_device(mtd);
	printk(KERN_INFO "NAND ecc: Hardware (delay %d)\n", chip->chip_delay);
#elif defined (CONFIG_MTD_NAND_ECC_BCH)
	chip->ecc.mode = NAND_ECC_SOFT_BCH;

	/* refer to nand_ecc.c */
	switch (ECC_BCH_BITS) {
	case  4: chip->ecc.bytes =   7; chip->ecc.size  =  512; break;
	case  8: chip->ecc.bytes =  13; chip->ecc.size  =  512; break;
    case 12: chip->ecc.bytes =  20; chip->ecc.size  =  512; break;
	case 16: chip->ecc.bytes =  26; chip->ecc.size  =  512; break;
	case 24: chip->ecc.bytes =  42; chip->ecc.size  = 1024; break;
	case 40: chip->ecc.bytes =  70; chip->ecc.size  = 1024; break;
	//case 60: chip->ecc.bytes = 105; chip->ecc.size  = 1024; break;	/* not test */
	default:
		printk("Fail: not supoort bch ecc %d mode !!!\n", ECC_BCH_BITS);
		return -1;
	}
	printk(KERN_INFO "NAND ecc: Software BCH %d \n", ECC_BCH_BITS);
#else
	chip->ecc.mode = NAND_ECC_SOFT;
	printk(KERN_INFO "NAND ecc: Software \n");
#endif

	nexell_nand_timing_set(mtd);

	return ret;
}

