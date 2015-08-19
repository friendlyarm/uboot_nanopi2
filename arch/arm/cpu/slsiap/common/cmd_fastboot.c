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
#include <asm/byteorder.h>
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <mmc.h>
#include <fat.h>
#include <div64.h>
#include <decompress_ext4.h>

#ifdef CONFIG_NXP_DWC_OTG
#include <dwc_otg_hs.h>
#endif
#include "fastboot.h"
#include "usbid.h"

/* Board specific */
extern int board_mmc_bootdev(void);

/*
#define	debug	printf
*/

extern void CalUSBID(U16 *VID, U16 *PID, U32 ECID);
extern void GetUSBID(U16 *VID, U16 *PID);

#ifdef CONFIG_ENV_IS_NOWHERE
#define saveenv()		0
#endif

#ifndef FASTBOOT_PARTS_DEFAULT
#error "Not default FASTBOOT_PARTS_DEFAULT"
#endif

static const char *const f_parts_default = FASTBOOT_PARTS_DEFAULT;

#define	FASTBOOT_MMC_MAX		3
#define	FASTBOOT_EEPROM_MAX		1
#define	FASTBOOT_NAND_MAX		1
#define	FASTBOOT_MEM_MAX		1

#define	FASTBOOT_DEV_PART_MAX	(16)				/* each device max partition max num */

/* device types */
#define	FASTBOOT_DEV_EEPROM		(1<<0)	/*  name "eeprom" */
#define	FASTBOOT_DEV_NAND		(1<<1)	/*  name "nand" */
#define	FASTBOOT_DEV_MMC		(1<<2)	/*  name "mmc" */
#define	FASTBOOT_DEV_MEM		(1<<3)	/*  name "mem" */

/* filesystem types */
#define	FASTBOOT_FS_2NDBOOT		(1<<0)	/*  name "boot" <- bootable */
#define	FASTBOOT_FS_BOOT		(1<<1)	/*  name "boot" <- bootable */
#define	FASTBOOT_FS_RAW			(1<<2)	/*  name "raw" */
#define	FASTBOOT_FS_FAT			(1<<4)	/*  name "fat" */
#define	FASTBOOT_FS_EXT4		(1<<5)	/*  name "ext4" */
#define	FASTBOOT_FS_UBI			(1<<6)	/*  name "ubi" */
#define	FASTBOOT_FS_UBIFS		(1<<7)	/*  name "ubifs" */
#define	FASTBOOT_FS_RAW_PART	(1<<8)	/*  name "emmc" */
#define FASTBOOT_FS_FACTORY		(1<<9)	/*  name "factory" */

#define	FASTBOOT_FS_MASK		(FASTBOOT_FS_EXT4 | FASTBOOT_FS_FAT | FASTBOOT_FS_UBI | FASTBOOT_FS_UBIFS | FASTBOOT_FS_RAW_PART)

#define	TCLK_TICK_HZ			(1000000)

/* Use 65 instead of 64
 * null gets dropped
 * strcpy's need the extra byte */
#define	RESP_SIZE				(65)

/*
 *	f_devices[0,1,2..] : mmc / eeprom / nand
 *	|				|
 *	|				write_part
 *	|				|
 *	|				link -> fastboot_part -> fastboot_part  -> ...
 *	|							|
 *	|							.partition = bootloader, boot, system,...
 *	|							.start
 *	|							.length
 *	|							.....
 *	|
 */
struct fastboot_device;
struct fastboot_part {
	char partition[32];
	int dev_type;
	int dev_no;
	uint64_t start;
	uint64_t length;
	unsigned int fs_type;
	unsigned int flags;
	struct fastboot_device *fd;
	struct list_head link;
};

struct fastboot_device {
	char *device;
	int dev_max;
	unsigned int dev_type;
	unsigned int part_type;
	unsigned int fs_support;
	uint64_t parts[FASTBOOT_DEV_PART_MAX][2];	/* 0: start, 1: length */
	struct list_head link;
	int (*write_part)(struct fastboot_part *fpart, void *buf, uint64_t length);
	int (*capacity)(struct fastboot_device *fd, int devno, uint64_t *length);
	int (*create_part)(int dev, uint64_t (*parts)[2], int count);
};

struct fastboot_fs_type {
	char *name;
	unsigned int fs_type;
};

/* support fs type */
static struct fastboot_fs_type f_part_fs[] = {
	{ "2nd"			, FASTBOOT_FS_2NDBOOT	},
	{ "boot"		, FASTBOOT_FS_BOOT		},
	{ "factory" 	, FASTBOOT_FS_FACTORY	},
	{ "raw"			, FASTBOOT_FS_RAW		},
	{ "fat"			, FASTBOOT_FS_FAT		},
	{ "ext4"		, FASTBOOT_FS_EXT4		},
	{ "emmc"		, FASTBOOT_FS_RAW_PART	},
	{ "nand"		, FASTBOOT_FS_RAW_PART	},
	{ "ubi"			, FASTBOOT_FS_UBI		},
	{ "ubifs"		, FASTBOOT_FS_UBIFS		},
};

/* Reserved partition names
 *
 *  NOTE :
 * 		Each command must be ended with ";"
 *
 *	partmap :
 * 			flash= <device>,<devno> : <partition> : <fs type> : <start>,<length> ;
 *		EX> flash= nand,0:bootloader:boot:0x300000,0x400000;
 *
 *	env :
 * 			<command name> = " command arguments ";
 * 		EX> bootcmd	= "tftp 42000000 uImage";
 *
 *	cmd :
 * 			" command arguments ";
 * 		EX> "tftp 42000000 uImage";
 *
 */

static const char *f_reserve_part[] = {
	[0] = "partmap",			/* fastboot partition */
	[1] = "mem",				/* download only */
	[2] = "env",				/* u-boot environment setting */
	[3] = "cmd",				/* command run */
};

/*
 * device partition functions
 */
static int get_parts_from_lists(struct fastboot_part *fpart, uint64_t (*parts)[2], int *count);
static void part_dev_print(struct fastboot_device *fd);

#ifdef CONFIG_CMD_MMC
extern ulong mmc_bwrite(int dev_num, lbaint_t start, lbaint_t blkcnt, const void *src);
extern int mmc_get_part_table(block_dev_desc_t *desc, uint64_t (*parts)[2], int *count);

static int mmc_make_parts(int dev, uint64_t (*parts)[2], int count)
{
	char cmd[1024];
	int i = 0, l = 0, p = 0;

	l = sprintf(cmd, "fdisk %d %d:", dev, count);
	p = l;
	for (i= 0; count > i; i++) {
		l = sprintf(&cmd[p], " 0x%llx:0x%llx", parts[i][0], parts[i][1]);
		p += l;
	}
	
	if (p >= sizeof(cmd)) {
		printf("** %s: cmd stack overflow : stack %d, cmd %d **\n",
			__func__, sizeof(cmd), p);
		while(1);
	}	

	cmd[p] = 0;
	printf("%s\n", cmd);

	/* "fdisk <dev no> [part table counts] <start:length> <start:length> ...\n" */
	return run_command(cmd, 0);
}

static int mmc_check_part_table(block_dev_desc_t *desc, struct fastboot_part *fpart)
{
	uint64_t parts[FASTBOOT_DEV_PART_MAX][2] = { {0,0}, };
	int i = 0, num = 0;
	int ret = 1;

	if (0 > mmc_get_part_table(desc, parts, &num))
		return -1;

	for (i = 0; num > i; i++) {
		if (parts[i][0] == fpart->start &&
			parts[i][1] == fpart->length)
			return 0;
		/* when last partition set value is zero,
		   set avaliable length */
		if ((num-1) == i &&
			parts[i][0] == fpart->start &&
			0 == fpart->length) {
			fpart->length = parts[i][1];
			ret = 0;
			break;
		}
	}
	return ret;
}

static int mmc_part_write(struct fastboot_part *fpart, void *buf, uint64_t length)
{
	block_dev_desc_t *desc;
	struct fastboot_device *fd = fpart->fd;
	int dev = fpart->dev_no;
	lbaint_t blk, cnt;
	int blk_size = 512;
	char cmd[32];
	int ret = 0;

	sprintf(cmd, "mmc dev %d", dev);

	debug("** mmc.%d partition %s (%s)**\n",
		dev, fpart->partition, fpart->fs_type&FASTBOOT_FS_EXT4?"FS":"Image");

	/* set mmc devicee */
	if (0 > get_device("mmc", simple_itoa(dev), &desc)) {
    	if (0 > run_command(cmd, 0))
    		return -1;
    	if (0 > run_command("mmc rescan", 0))
    		return -1;
	}

	if (0 > run_command(cmd, 0))	/* mmc device */
		return -1;

	if (0 > get_device("mmc", simple_itoa(dev), &desc))
		return -1;

	if (fpart->fs_type == FASTBOOT_FS_2NDBOOT ||
		fpart->fs_type == FASTBOOT_FS_BOOT) {
		char args[64];
		int l = 0, p = 0;

		if (fpart->fs_type == FASTBOOT_FS_2NDBOOT)
			p = sprintf(args, "update_mmc %d 2ndboot", dev);
		else
			p = sprintf(args, "update_mmc %d boot", dev);

		l = sprintf(&args[p], " %p 0x%llx 0x%llx", buf, fpart->start, length);
		p += l;
		args[p] = 0;

		return run_command(args, 0); /* update_mmc [dev no] <type> 'mem' 'addr' 'length' [load addr] */
	}

	if (fpart->fs_type & FASTBOOT_FS_MASK) {

		ret = mmc_check_part_table(desc, fpart);
		if (0 > ret)
			return -1;

		if (ret) {	/* new partition */
			uint64_t parts[FASTBOOT_DEV_PART_MAX][2] = { {0,0}, };
			int num;

			printf("Warn  : [%s] make new partitions ....\n", fpart->partition);
			part_dev_print(fpart->fd);

			get_parts_from_lists(fpart, parts, &num);
			ret = mmc_make_parts(dev, parts, num);
			if (0 > ret) {
				printf("** Fail make partition : %s.%d %s**\n",
					fd->device, dev, fpart->partition);
				return -1;
			}
		}

		if (mmc_check_part_table(desc, fpart))
			return -1;
	}

 	if ((fpart->fs_type & FASTBOOT_FS_EXT4) &&
 		(0 == check_compress_ext4((char*)buf, fpart->length))) {
		debug("write compressed ext4 ...\n");
		return write_compressed_ext4((char*)buf, fpart->start/blk_size);
	}

	blk = fpart->start/blk_size ;
	cnt = (length/blk_size) + ((length & (blk_size-1)) ? 1 : 0);

	printf("write image to 0x%llx(0x%x), 0x%llx(0x%x)\n",
		fpart->start, (unsigned int)blk, length, (unsigned int)blk);

	ret = mmc_bwrite(dev, blk, cnt, buf);

	return (0 > ret ? ret : 0);
}

static int mmc_part_capacity(struct fastboot_device *fd, int devno, uint64_t *length)
{
	block_dev_desc_t *desc;
	char cmd[32];

	debug("** mmc.%d capacity **\n", devno);

	/* set mmc devicee */
	if (0 > get_device("mmc", simple_itoa(devno), &desc)) {
		sprintf(cmd, "mmc dev %d", devno);
    	if (0 > run_command(cmd, 0))
    		return -1;
    	if (0 > run_command("mmc rescan", 0))
    		return -1;
	}

	if (0 > get_device("mmc", simple_itoa(devno), &desc))
		return -1;

	*length = (uint64_t)desc->lba * (uint64_t)desc->blksz;

	debug("%u*%u = %llu\n", (uint)desc->lba, (uint)desc->blksz, *length);
	return 0;

}
#endif
#ifdef CONFIG_CMD_EEPROM
static int eeprom_part_write(struct fastboot_part *fpart, void *buf, uint64_t length)
{
	char args[64];
	int l = 0, p = 0;

	p = sprintf(args, "update_eeprom ");

	if (fpart->fs_type & FASTBOOT_FS_BOOT)
		l = sprintf(&args[p], "%s", "uboot");
	else if (fpart->fs_type & FASTBOOT_FS_2NDBOOT)
		l = sprintf(&args[p], "%s", "2ndboot");
	else
		l = sprintf(&args[p], "%s", "raw");

	p += l;
	l = sprintf(&args[p], " %p 0x%llx 0x%llx", buf, fpart->start, length);
	p += l;
	args[p] = 0;

	debug("%s\n", args);

	return run_command(args, 0);	/* "update_eeprom <type> <mem> <addr> <length> ...\n" */
}
#endif

#ifdef CONFIG_CMD_NAND

#ifdef CONFIG_NAND_MTD
static int nand_part_write(struct fastboot_part *fpart, void *buf, uint64_t length)
{
	char args1[64], args2[64];
	int l = 0, p = 0;


	/*
	 * nand standalone
	 *		2ndboot,3rdboot
	 *			"update_nand write         0x50000000 0x0        0x20000"
	 *
	 * normal
	 *		raw image
	 *			"nand        write         0x50000000 0x400000   0x100000"
	 *
	 *		ubi image
	 *			"nand        write.trimffs 0x50000000 0x20000000 0x20000000"
	 */
	if ((fpart->fs_type & FASTBOOT_FS_2NDBOOT) || (fpart->fs_type & FASTBOOT_FS_BOOT))
		p = sprintf(args1, "update_nand ");
	else
		p = sprintf(args1, "nand ");

	l = sprintf(&args1[p], "%s", "erase");
	p += l;
	l = sprintf(&args1[p], " 0x%llx 0x%llx", fpart->start, fpart->length);
	p += l;
	args1[p] = 0;

	run_command(args1, 0);


	l = 0, p = 0;
	if ((fpart->fs_type & FASTBOOT_FS_2NDBOOT) || (fpart->fs_type & FASTBOOT_FS_BOOT))
		p = sprintf(args2, "update_nand ");
	else
		p = sprintf(args2, "nand ");


	if (fpart->fs_type & FASTBOOT_FS_UBI)
		l = sprintf(&args2[p], "%s", "write.trimffs");
	else
		l = sprintf(&args2[p], "%s", "write");
	p += l;

	l = sprintf(&args2[p], " 0x%x 0x%llx 0x%llx", (unsigned int)buf, fpart->start, length);
	p += l;
	args2[p] = 0;

	debug("%s\n", args1);
	debug("%s\n", args2);

	return run_command(args2, 0);
}
#else /* CONFIG_NAND_FTL */

extern ulong nand_bwrite(int dev_num, lbaint_t start, lbaint_t blkcnt, const void *src);
extern int nand_get_part_table(block_dev_desc_t *desc, uint64_t (*parts)[2], int *count);
extern void mio_set_autosend_standbycmd(int enable);

static int nand_make_parts(int dev, uint64_t (*parts)[2], int count)
{
	char cmd[128];
	int i = 0, l = 0, p = 0;

	l = sprintf(cmd, "ndisk %d %d:", dev, count);
	p = l;
	for (i= 0; count > i; i++) {
		l = sprintf(&cmd[p], " 0x%llx:0x%llx", parts[i][0], parts[i][1]);
		p += l;
	}
	cmd[p] = 0;
	printf("%s\n", cmd);

	/* "ndisk <dev no> [part table counts] <start:length> <start:length> ...\n" */
	return run_command(cmd, 0);
}

static int nand_check_part_table(block_dev_desc_t *desc, struct fastboot_part *fpart)
{
	uint64_t parts[FASTBOOT_DEV_PART_MAX][2] = { {0,0}, };
	int i = 0, num = 0;
	int ret = 1;

	if (0 > nand_get_part_table(desc, parts, &num))
		return -1;

	for (i = 0; num > i; i++) {
		if (parts[i][0] == fpart->start &&
			parts[i][1] == fpart->length)
			return 0;
		/* when last partition set value is zero,
		   set avaliable length */
		if ((num-1) == i &&
			parts[i][0] == fpart->start &&
			0 == fpart->length) {
			fpart->length = parts[i][1];
			ret = 0;
			break;
		}
	}
	return ret;
}

int ftl_write_raw_chunk(char* data, unsigned int sector, unsigned int sector_size) {
	char run_cmd[64];

	printf("write raw data in %d size %d (sector)\n", sector, sector_size);
	sprintf(run_cmd,"mio write 0x%x 0x%x 0x%x", (int)data, sector, sector_size);
	run_command(run_cmd, 0);

	return 0;
}

/* nand ftl */
enum {
	MIO_NAND_RAWREAD,
	MIO_NAND_RAWWRITE,
	MIO_NAND_ERASE,
};

static int nand_part_ftl(uint64_t start, uint64_t length, void *buf, int command)
{
	char args[64];
	int p = 0;

	p += sprintf(args, "mio ");

	switch (command) {
	case MIO_NAND_ERASE:
		p += sprintf(args+p, "nandrawerase ");
		break;
	case MIO_NAND_RAWWRITE:
		p += sprintf(args+p, "nandrawwrite 0x%x ", (unsigned int)buf);
		break;
	case MIO_NAND_RAWREAD:
	default:
		p += sprintf(args+p, "nandrawread 0x%x ", (unsigned int)buf);
		break;
	}
	p += sprintf(args+p, "%llx %llx", start, length);
	args[p] = 0;

	debug("%s\n", args);
	return run_command(args, 0);
}

extern int mio_standby(void);
static int nand_part_write(struct fastboot_part *fpart, void *buf, uint64_t length)
{
	block_dev_desc_t *desc;
	struct fastboot_device *fd = fpart->fd;
	int dev = fpart->dev_no;
	lbaint_t blk, cnt;
	int blk_size;
	int ret = 0;

	debug("** nand.%d partition %s (%s)**\n",
		dev, fpart->partition, fpart->fs_type&FASTBOOT_FS_EXT4?"FS":"Image");

	if (0 > get_device("nand", simple_itoa(dev), &desc))
		return -1;

	blk_size = desc->blksz;

	/*
	 * lowlevel : linear : no ecc, no randomize, no read-retry
	 *		2ndboot,3rdboot
	 *          "mio nandrawwrite 0x50000000 0x0        0x20000"
	 *
	 * lowlevel : linear : ecc, randomize, read-retry
	 *		raw data
	 *			"mio nandwrite    0x50000000 0x400000   0x100000"
	 *
	 * ftl : block :
	 *		ext4 image
	 *			"mio write        0x50000000 0x20000000 0x20000000"
	 */
	if (fpart->fs_type == FASTBOOT_FS_2NDBOOT ||
		fpart->fs_type == FASTBOOT_FS_BOOT ||
		fpart->fs_type == FASTBOOT_FS_FACTORY) {

		int i;
		uint64_t start = fpart->start;
		int offset = CFG_BOOTIMG_OFFSET;
		int repeat = CFG_BOOTIMG_REPEAT;

		/* erase */
		nand_part_ftl(start, length, buf, MIO_NAND_ERASE);

		start = fpart->start;

		/* write */
		for (i = 0; i < repeat; i++, start += offset) {
			nand_part_ftl(start, length, buf, MIO_NAND_RAWWRITE);
		}

		return 0;
	}

	if (fpart->fs_type & FASTBOOT_FS_MASK) {

		ret = nand_check_part_table(desc, fpart);
		if (0 > ret)
			return -1;

		if (ret) {	/* new partition */
			uint64_t parts[FASTBOOT_DEV_PART_MAX][2] = { {0,0}, };
			int num;

			printf("Warn  : [%s] make new partitions ....\n", fpart->partition);
			part_dev_print(fpart->fd);

			get_parts_from_lists(fpart, parts, &num);
			ret = nand_make_parts(dev, parts, num);
			if (0 > ret) {
				printf("** Fail make partition : %s.%d %s**\n",
					fd->device, dev, fpart->partition);
				return -1;
			}
		}

		if (nand_check_part_table(desc, fpart))
			return -1;
	}

	/* change write raw chunk method : mmc -> nand */
	set_write_raw_chunk_cb(ftl_write_raw_chunk);

 	if ((fpart->fs_type & FASTBOOT_FS_EXT4) &&
 		(0 == check_compress_ext4((char*)buf, fpart->length))) {
		debug("write compressed ext4 ...\n");

		mio_set_autosend_standbycmd(0);
		ret = write_compressed_ext4((char*)buf, fpart->start/blk_size);
		mio_set_autosend_standbycmd(1);
		mio_standby();
		return ret;
	}

	blk = fpart->start/blk_size ;
	cnt = (length/blk_size) + ((length & (blk_size-1)) ? 1 : 0);

	printf("write image to 0x%llx(0x%x), 0x%llx(0x%x)\n",
		fpart->start, (unsigned int)blk, length, (unsigned int)blk);

	mio_set_autosend_standbycmd(0);
	ret = nand_bwrite(dev, blk, cnt, buf);
	mio_set_autosend_standbycmd(1);
	mio_standby();

	return (0 > ret ? ret : 0);
}

static int nand_part_capacity(struct fastboot_device *fd, int devno, uint64_t *length)
{
	block_dev_desc_t *desc;

	debug("** nand.%d capacity **\n", devno);

	/* get nand device */
	if (0 > get_device("nand", simple_itoa(devno), &desc))
		return -1;

		//nand->block_dev.lba = nand->capacity;

	*length = (uint64_t)desc->lba * (uint64_t)desc->blksz;
	debug("%u*%u = %llu\n", (uint)desc->lba, (uint)desc->blksz, *length);

	return 0;
}
#endif /* CONFIG_NAND_FTL */

#endif /* CONFIG_CMD_NAND */

static struct fastboot_device f_devices[] = {
	{
		.device 	= "eeprom",
		.dev_max	= FASTBOOT_EEPROM_MAX,
		.dev_type	= FASTBOOT_DEV_EEPROM,
		.fs_support	= (FASTBOOT_FS_2NDBOOT | FASTBOOT_FS_BOOT | FASTBOOT_FS_RAW),
	#ifdef CONFIG_CMD_EEPROM
		.write_part	= eeprom_part_write,
	#endif
	},
#if defined(CONFIG_NAND_MTD)
	{
		.device 	= "nand",
		.dev_max	= FASTBOOT_NAND_MAX,
		.dev_type	= FASTBOOT_DEV_NAND,
		.fs_support	= (FASTBOOT_FS_2NDBOOT | FASTBOOT_FS_BOOT | FASTBOOT_FS_RAW | FASTBOOT_FS_UBI),
	#ifdef CONFIG_CMD_NAND
		.write_part	= nand_part_write,
	#endif
	},
#else /* CONFIG_NAND_FTL */
	{
		.device 	= "nand",
		.dev_max	= FASTBOOT_NAND_MAX,
		.dev_type	= FASTBOOT_DEV_NAND,
		.fs_support	= (FASTBOOT_FS_2NDBOOT | FASTBOOT_FS_BOOT | FASTBOOT_FS_RAW | FASTBOOT_FS_EXT4 |
						FASTBOOT_FS_RAW_PART | FASTBOOT_FS_FACTORY),
	#ifdef CONFIG_CMD_NAND
		.write_part	= nand_part_write,
		.capacity = nand_part_capacity,
		.create_part = nand_make_parts
	#endif
	},

#endif
	{
		.device 	= "mmc",
		.dev_max	= FASTBOOT_MMC_MAX,
		.dev_type	= FASTBOOT_DEV_MMC,
		.part_type	= PART_TYPE_DOS,
		.fs_support	= (FASTBOOT_FS_2NDBOOT | FASTBOOT_FS_BOOT | FASTBOOT_FS_RAW |
						FASTBOOT_FS_FAT | FASTBOOT_FS_EXT4 | FASTBOOT_FS_RAW_PART),
	#ifdef CONFIG_CMD_MMC
		.write_part	= mmc_part_write,
		.capacity = mmc_part_capacity,
		.create_part = mmc_make_parts,
	#endif
	},
};

#define	FASTBOOT_DEV_SIZE	ARRAY_SIZE(f_devices)

/*
 *
 * FASTBOOT COMMAND PARSE
 *
 */
static inline void parse_comment(const char *str, const char **ret)
{
	const char *p = str, *r;

	do {
		if (!(r = strchr(p, '#')))
			break;
		r++;

		if (!(p = strchr(r, '\n'))) {
			printf("---- not end comments '#' ----\n");
			break;
		}
		p++;
	} while (1);

	/* for next */
	*ret = p;
}

static inline int parse_string(const char *s, const char *e, char *b, int len)
{
	int l, a = 0;

	do { while (0x20 == *s || 0x09 == *s || 0x0a == *s) { s++; } } while(0);

	if (0x20 == *(e-1) || 0x09 == *(e-1))
		do { e--; while (0x20 == *e || 0x09 == *e) { e--; }; a = 1; } while(0);

	l = (e - s + a);
	if (l > len) {
		printf("-- Not enough buffer %d for string len %d [%s] --\n", len, l, s);
		return -1;
	}

	strncpy(b, s, l);
	b[l] = 0;

	return l;
}

static inline void sort_string(char *p, int len)
{
	int i, j;
	for (i = 0, j = 0; len > i; i++) {
		if (0x20 != p[i] && 0x09 != p[i] && 0x0A != p[i])
			p[j++] = p[i];
	}
	p[j] = 0;
}

static int parse_part_device(const char *parts, const char **ret,
			struct fastboot_device **fdev, struct fastboot_part *fpart)
{
	struct fastboot_device *fd = *fdev;
	const char *p, *id, *c;
	char str[32];
	int i = 0, id_len;

	if (ret)
		*ret = NULL;

	id = p = parts;
	if (!(p = strchr(id, ':'))) {
		printf("no <dev-id> identifier\n");
		return 1;
	}
	id_len = p - id;

	/* for next */
	p++, *ret = p;

	c = strchr(id, ',');
	parse_string(id, c, str, sizeof(str));

	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		if (strcmp(fd->device, str) == 0) {
			/* add to device */
			list_add_tail(&fpart->link, &fd->link);
			*fdev = fd;

			/* dev no */
			debug("device: %s", fd->device);
			if (!(p = strchr(id, ','))) {
				printf("no <dev-no> identifier\n");
				return -1;
			}
			p++;
			parse_string(p, p+id_len, str, sizeof(str));	/* dev no*/
			/* dev no */
			fpart->dev_no = simple_strtoul(str, NULL, 10);
			if (fpart->dev_no >= fd->dev_max) {
				printf("** Over dev-no max %s.%d : %d **\n",
					fd->device, fd->dev_max-1, fpart->dev_no);
				return -1;
			}

			debug(".%d\n", fpart->dev_no);
			fpart->fd = fd;
			return 0;
		}
	}

	/* to delete */
	fd = *fdev;
	strcpy(fpart->partition, "unknown");
	list_add_tail(&fpart->link, &fd->link);

	printf("** Can't device parse : %s **\n", parts);
	return -1;
}

static int parse_part_partition(const char *parts, const char **ret,
			struct fastboot_device **fdev, struct fastboot_part *fpart)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	const char *p, *id;
	char str[32] = { 0, };
	int i = 0;

	if (ret)
		*ret = NULL;

	id = p = parts;
	if (!(p = strchr(id, ':'))) {
		printf("no <name> identifier\n");
		return -1;
	}

	/* for next */
	p++, *ret = p;
	p--; parse_string(id, p, str, sizeof(str));

	for (i = 0; ARRAY_SIZE(f_reserve_part) > i; i++) {
		const char *r =f_reserve_part[i];
		if (!strcmp(r, str)) {
			printf("** Reserved partition name : %s  **\n", str);
			return -1;
		}
	}

	/* check partition */
	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;
		if (list_empty(head))
			continue;
		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			if (!strcmp(fp->partition, str)) {
				printf("** Exist partition : %s -> %s **\n",
					fd->device, fp->partition);
				strcpy(fpart->partition, str);
				fpart->partition[strlen(str)] = 0;
				return -1;
			}
		}
	}

	strcpy(fpart->partition, str);
	fpart->partition[strlen(str)] = 0;
	debug("part  : %s\n", fpart->partition);

	return 0;
}

static int parse_part_fs(const char *parts, const char **ret,
		struct fastboot_device **fdev, struct fastboot_part *fpart)
{
	struct fastboot_device *fd = *fdev;
	struct fastboot_fs_type *fs = f_part_fs;
	const char *p, *id;
	char str[16] = { 0, };
	int i = 0;

	if (ret)
		*ret = NULL;

	id = p = parts;
	if (!(p = strchr(id, ':'))) {
		printf("no <dev-id> identifier\n");
		return -1;
	}

	/* for next */
	p++, *ret = p;
	p--; parse_string(id, p, str, sizeof(str));

	for (; ARRAY_SIZE(f_part_fs) > i; i++, fs++) {
		if (strcmp(fs->name, str) == 0) {
			if (!(fd->fs_support & fs->fs_type)) {
				printf("** '%s' not support '%s' fs **\n", fd->device, fs->name);
				return -1;
			}

			fpart->fs_type = fs->fs_type;
			debug("fs    : %s\n", fs->name);
			return 0;
		}
	}

	printf("** Can't fs parse : %s **\n", str);
	return -1;
}

static int parse_part_address(const char *parts, const char **ret,
			struct fastboot_device **fdev, struct fastboot_part *fpart)
{
	const char *p, *id;
	char str[64] = { 0, };
	int id_len;

	if (ret)
		*ret = NULL;

	id = p = parts;
	if (!(p = strchr(id, ';')) && !(p = strchr(id, '\n'))) {
		printf("no <; or '\n'> identifier\n");
		return -1;
	}
	id_len = p - id;

	/* for next */
	p++, *ret = p;

	if (!(p = strchr(id, ','))) {
		printf("no <start> identifier\n");
		return -1;
	}

	parse_string(id, p, str, sizeof(str));
	fpart->start = simple_strtoull(str, NULL, 16);
	debug("start : 0x%llx\n", fpart->start);

	p++;
	parse_string(p, p+id_len, str, sizeof(str));	/* dev no*/
	fpart->length = simple_strtoull(str, NULL, 16);
	debug("length: 0x%llx\n", fpart->length);

	return 0;
}

static int parse_part_head(const char *parts, const char **ret)
{
	const char *p = parts;
	int len = strlen("flash=");

	debug("\n");
	if (!(p = strstr(p, "flash=")))
		return -1;

	*ret = p + len;
	return 0;
}

typedef int (parse_fnc_t) (const char *parts, const char **ret,
						struct fastboot_device **fdev, struct fastboot_part *fpart);

parse_fnc_t *parse_part_seqs[] = {
	parse_part_device,
	parse_part_partition,
	parse_part_fs,
	parse_part_address,
	0,	/* end */
};

static inline void part_lists_init(int init)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	int i = 0;

	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;

		if (init) {
			INIT_LIST_HEAD(head);
			memset(fd->parts, 0x0, sizeof(FASTBOOT_DEV_PART_MAX*2));
			continue;
		}

		if (list_empty(head))
			continue;

		debug("delete [%s]:", fd->device);
		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			debug("%s ", fp->partition);
			list_del(entry);
			free(fp);
		}
		debug("\n");
		INIT_LIST_HEAD(head);
	}
}

static int part_lists_make(const char *ptable_str, int ptable_str_len)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	parse_fnc_t **p_fnc_ptr;
	const char *p = ptable_str;
	const char *env_flash, *env_p;
	int len = ptable_str_len;
	int err = -1;

	debug("\n---part_lists_make ---\n");
	part_lists_init(0);

	parse_comment(p, &p);
	sort_string((char*)p, len);

	env_flash = getenv("fastbootdev");
	if (env_flash) {
		env_flash  = strstr(env_flash, "flash=");
		if (env_flash)
			env_flash += strlen("flash=");
	}

	/* new parts table */
	while (1) {
		fd = f_devices;

		if (*p == '\0')
			break;

		fp = malloc(sizeof(*fp));
		if (!fp) {
			printf("** Can't malloc fastboot part table entry **\n");
			err = -1;
			break;
		}

		p_fnc_ptr = parse_part_seqs;

		if (env_flash) {
			if ((*p_fnc_ptr)(env_flash, &env_p, &fd, fp) != 0) {
				err = -1;
				goto fail_parse;
			}

			p_fnc_ptr++;

		} else if (parse_part_head(p, &p)) {
			if (err)
				printf("-- unknown parts head: [%s]\n", p);
			break;
		}

		for (; *p_fnc_ptr; ++p_fnc_ptr) {
			if ((*p_fnc_ptr)(p, &p, &fd, fp) != 0) {
				err = -1;
				goto fail_parse;
			}
		}

		err = 0;
	}

fail_parse:
	if (err)
		part_lists_init(0);

	return err;
}

static void part_lists_print(void)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	int i;

	printf("\nFastboot Partitions:\n");
	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;
		if (list_empty(head))
			continue;
		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			printf(" %s.%d: %s, %s : 0x%llx, 0x%llx\n",
				fd->device, fp->dev_no, fp->partition,
				FASTBOOT_FS_MASK&fp->fs_type?"fs":"img", fp->start, fp->length);
		}
	}

	printf("Support fstype :");
	for (i = 0; ARRAY_SIZE(f_part_fs) > i; i++)
		printf(" %s ", f_part_fs[i].name);
	printf("\n");

	printf("Reserved part  :");
	for (i = 0; ARRAY_SIZE(f_reserve_part) > i; i++)
		printf(" %s ", f_reserve_part[i]);
	printf("\n");
}

static int part_lists_check(const char *part)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	int i =0;

	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;
		if (list_empty(head))
			continue;
		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			if (!strcmp(fp->partition, part)) {
				return 0;
			}
		}
	}
	return -1;
}

static void part_dev_print(struct fastboot_device *fd)
{
	struct fastboot_part *fp;
	struct list_head *entry, *n;

	printf("Device: %s\n", fd->device);
	struct list_head *head = &fd->link;
	if (!list_empty(head)) {
		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			printf(" %s.%d: %s, %s : 0x%llx, 0x%llx\n",
				fd->device, fp->dev_no, fp->partition,
				FASTBOOT_FS_MASK&fp->fs_type?"fs":"img",
				fp->start, fp->length);
		}
	}
}

/* fastboot getvar capacity.<device>.<dev no> */
static int part_dev_capacity(const char *device, uint64_t *length)
{
	struct fastboot_device *fd = f_devices;
	const char *s = device, *c = device;
	char str[32] = {0,};
	uint64_t len = 0;
	int no = 0, i = 0;

	if ((c = strchr(s, '.'))) {
		strncpy(str, s, (c-s));
		str[c-s] = 0;
		c +=1;
		no = simple_strtoul(c, NULL, 10);
		for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
			if (strcmp(fd->device, str))
				continue;
			if (fd->capacity)
				fd->capacity(fd, no, &len);
			break;
		}
	}
#if defined(CONFIG_NAND_FTL)
	else {
		for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
			if (0 == strcmp(fd->device, "nand")) {
				if (fd->capacity)
					fd->capacity(fd, 0, &len);
				break;
			}
		}
	}
#endif /* CONFIG_NAND_FTL */

	*length = len;

	return !len ? -1 : 0;
}

static void part_mbr_update(void)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	int i = 0, j = 0;

	debug("%s:\n", __func__);
	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;
		uint64_t part_dev[FASTBOOT_DEV_PART_MAX][3] = { {0,0,0}, };
		int count = 0, dev = 0;
		int total = 0;

		if (list_empty(head))
			continue;

		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			if (FASTBOOT_FS_MASK & fp->fs_type) {
				part_dev[total][0] = fp->start;
				part_dev[total][1] = fp->length;
				part_dev[total][2] = fp->dev_no;
				debug("%s.%d 0x%llx, 0x%llx\n",
					fd->device, fp->dev_no, part_dev[total][0], part_dev[total][1]);
				total++;
			}
		}
		debug("total parts : %d\n", total);

		count = total;
		while (count > 0) {
			uint64_t parts[FASTBOOT_DEV_PART_MAX][2] = { {0,0 }, };
			volatile int mbrs = 0;

			if (dev > fd->dev_max) {
				printf("** Fail make to %s dev %d is over max %d **\n",
					fd->device, dev, fd->dev_max);
				break;
			}

			for (j = 0; total > j; j++) {
				if (dev == (int)part_dev[j][2]) {
					parts[mbrs][0] = part_dev[j][0];
					parts[mbrs][1] = part_dev[j][1];
					debug("MBR %s.%d 0x%llx, 0x%llx (%d:%d)\n",
						fd->device, dev, parts[mbrs][0], parts[mbrs][1], total, count);
					mbrs++;
				}
			}

			/* new MBR */
			if (mbrs && fd->create_part)
				fd->create_part(dev, parts, mbrs);

			count -= mbrs;
			debug("count %d, mbrs %d, dev %d\n", count, mbrs, dev);
			if (count)
				dev++;
		}
	}
}

static int get_parts_from_lists(struct fastboot_part *fpart, uint64_t (*parts)[2], int *count)
{
	struct fastboot_part *fp = fpart;
	struct fastboot_device *fd = fpart->fd;
	struct list_head *head = &fd->link;
	struct list_head *entry, *n;
	int dev = fpart->dev_no;
	int i = 0;

	if (!parts || !count) {
		printf("-- No partition input params --\n");
		return -1;
	}
	*count = 0;

	if (list_empty(head))
		return 0;

	list_for_each_safe(entry, n, head) {
		fp = list_entry(entry, struct fastboot_part, link);
		if ((FASTBOOT_FS_MASK & fp->fs_type) &&
			(dev == fp->dev_no)) {
			parts[i][0] = fp->start;
			parts[i][1] = fp->length;
			i++;
			debug("%s.%d = %s\n", fd->device, dev, fp->partition);
		}
	}

	*count = i;	/* set part count */
	return 0;
}

/*
 * Display to LCD
 */
#define	ALIAS(fnc)	__attribute__((weak, alias(fnc)))

void fboot_lcd_start (void)				ALIAS("f_lcd_stop");	void f_lcd_start (void) {}
void fboot_lcd_stop  (void)				ALIAS("f_lcd_stop");	void f_lcd_stop  (void) {}
void fboot_lcd_part  (char *p, char *s)	ALIAS("f_lcd_part");	void f_lcd_part  (char *p, char *s){ }
void fboot_lcd_flash (char *p, char *s)	ALIAS("f_lcd_flash");	void f_lcd_flash (char *p, char *s){ }
void fboot_lcd_down  (int ps)			ALIAS("f_lcd_down");	void f_lcd_down  (int ps){ }
void fboot_lcd_status(char *s)			ALIAS("f_lcd_status");	void f_lcd_status(char *s){ }

/*
 *
 * FASTBOOT USB CONTROL
 *
 */
struct f_trans_stat {
	unsigned long long image_size;	/* Image size */
	unsigned long long down_bytes;	/* Downloaded size */
	int down_percent;
	unsigned int error;
};
static struct f_trans_stat f_status;

typedef struct cmd_fastboot_interface f_cmd_inf;
static int fboot_rx_handler(const unsigned char *buffer, unsigned int length);
static void fboot_reset_handler(void);

static f_cmd_inf f_interface = {
	.rx_handler = fboot_rx_handler,
	.reset_handler = fboot_reset_handler,
	.product_name = NULL,
	.serial_no = NULL,
	.transfer_buffer = (unsigned char *)CFG_FASTBOOT_TRANSFER_BUFFER,
	.transfer_buffer_size = CFG_FASTBOOT_TRANSFER_BUFFER_SIZE,
};

static int parse_env_head(const char *env, const char **ret, char *str, int len)
{
	const char *p = env, *r = p;

	parse_comment(p, &p);
	if (!(r = strchr(p, '=')))
		return -1;

	if (0 > parse_string(p, r, str, len))
		return -1;

	if (!(r = strchr(r, '"'))){
		printf("no <\"> identifier\n");
		return -1;
	}

	r++; *ret = r;
	return 0;
}

static int parse_env_end(const char *env, const char **ret, char *str, int len)
{
	const char *p = env;
	const char *r = p;

	if (!(r = strchr(p, '"'))) {
		printf("no <\"> end identifier\n");
		return -1;
	}

	if (0 > parse_string(p, r, str, len))
		return -1;

	r++;
	if (!(r = strchr(p, ';')) &&
		!(r = strchr(p, '\n'))) {
		printf("no <;> exit identifier\n");
		return -1;
	}

	/* for next */
	r++, *ret = r;
	return 0;
}

static int parse_cmd(const char *cmd, const char **ret, char *str, int len)
{
	const char *p = cmd, *r = p;

	parse_comment(p, &p);
	if (!(p = strchr(p, '"')))
		return -1;
	p++;

	if (!(r = strchr(p, '"')))
		return -1;

	if (0 > parse_string(p, r, str, len))
		return -1;
	r++;

	if (!(r = strchr(p, ';')) &&
		!(r = strchr(p, '\n'))) {
		printf("no <;> exit identifier\n");
		return -1;
	}

	/* for next */
	r++, *ret = r;
	return 0;
}

static inline void print_response(char *response, char *string)
{
	if (response)
		sprintf(response, "%s", string);
	printf("%s\n", string);
}

static int fboot_setenv(const char *str, int len)
{
	const char *p = str;
	char cmd[32];
	char arg[1024];
	int err = -1;

	debug("---fboot_setenv---\n");
	do {
		if (parse_env_head(p, &p, cmd, sizeof(cmd)))
			break;

		if (parse_env_end(p, &p, arg, sizeof(arg)))
			break;

		printf("%s=%s\n", cmd, arg);
		setenv(cmd, (char *)arg);
		saveenv();
		err = 0;
	} while (1);

	return err;
}

static int fboot_command(const char *str, int len)
{
	const char *p = str;
	char cmd[128];
	int err = -1;

	debug("---fboot_command---\n");
	do {
		if (parse_cmd(p, &p, cmd, sizeof(cmd)))
			break;

		printf("Run [%s]\n", cmd);
		err = run_command(cmd, 0);
		if (0 > err)
			break;
	} while (1);

	return err;
}

static int fboot_response(const char *resp, unsigned int len, unsigned int sync)
{
	debug("response -> %s\n", resp);
	fastboot_tx_status(resp, len, sync);
	return 0;
}

static int fboot_cmd_reboot(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	fboot_response("OKAY", strlen("OKAY"), FASTBOOT_TX_SYNC);
	return do_reset (NULL, 0, 0, NULL);
}

static int fboot_cmd_getvar(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	char resp[RESP_SIZE] = "OKAY";
	char *s = (char*)cmd;
	char *p = resp + strlen(resp);
	debug("getvar = %s\n", cmd);

	if (!strncmp(cmd, "partition-type:", strlen("partition-type:"))) {

		s += strlen("partition-type:");
		if (part_lists_check(s)) {
			strcpy(resp, "FAIL bad partition");
			fboot_lcd_part(s, "Bad Partition...");
			goto done_getvar;
		}
		printf("\nReady : ");
		print_response(p, s);
		fboot_lcd_part(s, "wait...");
		goto done_getvar;
	}

	if (!strncmp(cmd, "version", strlen("version"))) {
		strcpy(p, FASTBOOT_VERSION);
		goto done_getvar;
	}

	if (!strncmp(cmd, "product", strlen("product"))) {
		strcpy(p, CONFIG_SYS_BOARD);
		goto done_getvar;
	}

	if (!strncmp(cmd, "serialno", strlen("serialno"))) {
		if (inf->serial_no)
			strcpy(p, inf->serial_no);
		goto done_getvar;
	}

	if (!strncmp(cmd, "capacity", strlen("capacity"))) {
		const char *s = cmd;
		uint64_t length = 0;

		s += strlen("capacity");
		s += 1;	/* . */

		if (0 > part_dev_capacity(s, &length))
			strcpy(resp, "FAIL bad device");
		else
			sprintf(p, "%lld", length);

		goto done_getvar;
	}

	if (!strncmp(cmd, "max-download-size", strlen("max-download-size"))) {
		if (inf->transfer_buffer_size)
			sprintf(p, "%08x", inf->transfer_buffer_size);
		goto done_getvar;
	}

	if (!strncmp(cmd, "chip", strlen("chip"))) {
		strncpy(p, CONFIG_SYS_PROMPT, 7);
		goto done_getvar;
	}

	fastboot_getvar(cmd, p);

done_getvar:
	return fboot_response(resp, strlen(resp), FASTBOOT_TX_SYNC);
}

static int fboot_cmd_download(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	char resp[RESP_SIZE] = "OKAY";
	unsigned int clear = (uintptr_t)(inf->transfer_buffer);

	fst->image_size = simple_strtoull (cmd, NULL, 16);
	fst->down_bytes = 0;
	fst->error = 0;
	fst->down_percent = -1;

	clear += fst->image_size;
	clear &= ~0x3;

	memset((void*)((ulong)clear), 0x0, 16);	/* clear buffer for string parsing */

	printf("Starting download of %lld bytes\n", fst->image_size);

	if (0 == fst->image_size) {
		print_response(resp, "FAIL data invalid size");	/* bad user input */
	} else if (fst->image_size > inf->transfer_buffer_size) {
		print_response(resp, "FAIL data too large");
		fst->image_size = 0;
	} else {
		sprintf(resp, "DATA%08llx", fst->image_size);
	}

	return fboot_response(resp, strlen(resp), FASTBOOT_TX_SYNC);
}

static int fboot_cmd_flash(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	struct fastboot_device *fd = f_devices;
	struct fastboot_part *fp;
	struct list_head *entry, *n;
	char resp[RESP_SIZE] = "OKAY";
	int i = 0, err = 0;

	printf("Flash : %s\n", cmd);
	fboot_lcd_flash((char*)cmd, "flashing");

	if (fst->down_bytes == 0) {
		print_response(resp, "FAIL no image downloaded");
		return fboot_response(resp, strlen(resp), FASTBOOT_TX_SYNC);
	}

	/* new partition map */
	if (!strcmp("partmap", cmd)) {
		const char *p = (const char *)inf->transfer_buffer;

		if (0 > part_lists_make(p, strlen(p))) {
			print_response(resp, "FAIL partition map parse");
			goto err_flash;
		}

		part_lists_print();
		part_mbr_update();
		parse_comment(p, &p);

		if (0 == setenv("fastboot", (char *)p) &&
			0 == saveenv());
			goto done_flash;

	/* set environments */
	} else if (!strcmp("env", cmd)) {
		char *p = (char *)inf->transfer_buffer;

		if(0 > fboot_setenv(p, fst->down_bytes)){
			print_response(resp, "FAIL environment parse");
			goto err_flash;
		}
		goto done_flash;

	/* run command */
	} else if (!strcmp("cmd", cmd)) {
		char *p = (char *)inf->transfer_buffer;

		if(0 > fboot_command(p, fst->down_bytes)){
			print_response(resp, "FAIL cmd parse");
			goto err_flash;
		}
		goto done_flash;

	/* memory partition : do nothing */
	} else if (0 == strcmp("mem", cmd)) {
		goto done_flash;
	}

	/* flash to partition */
	for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
		struct list_head *head = &fd->link;
		if (list_empty(head))
			continue;

		list_for_each_safe(entry, n, head) {
			fp = list_entry(entry, struct fastboot_part, link);
			if (!strcmp(fp->partition, cmd)) {

				if ((fst->down_bytes > fp->length) && (fp->length != 0)) {
					print_response(resp, "FAIL image too large for partition");
					goto err_flash;
				}

				if ((fd->dev_type != FASTBOOT_DEV_MEM) &&
					fd->write_part) {
					char *p = (char *)inf->transfer_buffer;
					if (0 > fd->write_part(fp, p, fst->down_bytes))
						print_response(resp, "FAIL to flash partition");
				}

				goto done_flash;
			}
		}
	}

err_flash:
	err = -1;
	print_response(resp, "FAIL partition does not exist");

done_flash:
	printf("Flash : %s - %s\n", cmd, 0 > err ? "FAIL":"DONE");
	fboot_lcd_flash((char*)cmd, 0 > err ? "fail":"done");

	return fboot_response(resp, strlen(resp), FASTBOOT_TX_SYNC);
}

static int fboot_cmd_boot(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	char resp[RESP_SIZE] = "FAIL";
	printf("*** Not IMPLEMENT ***\n");
	return fboot_response(resp, strlen(resp), FASTBOOT_TX_SYNC);
}

static int fboot_cmd_format(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	char resp[RESP_SIZE] = "FAIL";
	printf("*** Not IMPLEMENT ***\n");
	return fboot_response(resp, strlen(resp), FASTBOOT_TX_ASYNC);
}

static int fboot_cmd_erase(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	char resp[RESP_SIZE] = "FAIL";
	printf("*** Not IMPLEMENT ***\n");
	return fboot_response(resp, strlen(resp), FASTBOOT_TX_ASYNC);
}

static int fboot_cmd_oem(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst)
{
	char resp[RESP_SIZE] = "INFO unknown OEM command";
	fboot_response(resp, strlen(resp), FASTBOOT_TX_ASYNC);
	return 0;
}

static void fboot_reset_handler(void)
{
	struct f_trans_stat *fst = &f_status;
	fst->image_size = 0;
	fst->down_bytes = 0;
	fst->error = 0;
	fst->down_percent = -1;
}

struct f_cmd_fnc_t {
	char *command;
	int (*fnc_t)(const char *cmd, f_cmd_inf *inf, struct f_trans_stat *fst);
};

static struct f_cmd_fnc_t f_cmd_seqs [] = {
	{ "reboot"		, fboot_cmd_reboot		},
	{ "getvar:"		, fboot_cmd_getvar		},
	{ "download:"	, fboot_cmd_download	},
	{ "flash:"		, fboot_cmd_flash		},
	{ "boot"		, fboot_cmd_boot		},
	{ "format:"		, fboot_cmd_format		},
	{ "erase:"		, fboot_cmd_erase		},
	{ "oem "		, fboot_cmd_oem			},
};
#define	FASTBOOT_CMD_SIZE	ARRAY_SIZE(f_cmd_seqs)

static int fboot_rx_handler(const unsigned char *buffer, unsigned int length)
{
	f_cmd_inf *inf = &f_interface;
	struct f_trans_stat *fst = &f_status;
	int i = 0, ret = 0;

	/* Command */
	if (!fst->image_size) {
		const char *cmd = (char *)buffer;
		debug("[CMD = %s]\n", cmd);
		for (i = 0; FASTBOOT_CMD_SIZE > i; i++) {
			struct f_cmd_fnc_t *fptr = &f_cmd_seqs[i];
			const char *str = fptr->command;
			int len = strlen(str);
			if (!strncmp(cmd, str, len) &&
				fptr->fnc_t)
			{
				flush_dcache_all();
				fptr->fnc_t(cmd+len, inf, fst);
				break;
			}
		}
		memset((void*)buffer, 0, length);	/* clear buffer */

		if (FASTBOOT_CMD_SIZE == i) {
			printf("-- unknown fastboot cmd [%s] --\n", cmd);
			fboot_response("ERROR", strlen("ERROR"), FASTBOOT_TX_ASYNC);
		}

	/* Download */
	} else {
		if (length) {
			char resp[RESP_SIZE];
			unsigned long long len = (fst->image_size - fst->down_bytes);
			unsigned long long n = fst->down_bytes * 100ULL;
			int percent;

			do_div(n, fst->image_size);
			percent = (int)n;

			if (len > length)
				len = length;

			memcpy(inf->transfer_buffer+fst->down_bytes, buffer, len);
			fst->down_bytes += len;

			/* transfer is done */
			if (fst->down_bytes >= fst->image_size) {

				(fst->down_percent > 0) ? printf ("\n"): 0;
				printf ("downloading of %lld bytes to %p (0x%x) finished\n",
					fst->down_bytes, inf->transfer_buffer, inf->transfer_buffer_size);

				if (fst->error)
					sprintf(resp, "ERROR");
				else
					sprintf(resp, "OKAY");

				fst->image_size = 0;
				fboot_response(resp, strlen(resp), FASTBOOT_TX_SYNC);

				fboot_lcd_down(100);
			} else {
				if (percent != fst->down_percent) {
					fst->down_percent = percent;
					printf("\rdownloading %lld -- %3d%% complete.", fst->down_bytes, percent);
					fboot_lcd_down(percent);
				}
			}

		} else {
			/* Ignore empty buffers */
			printf ("Warning empty download buffer\n");
			printf ("Ignoring\n");
		}
	}

	return ret;
}

#define ANDROID_VENDOR_ID 				0x18D1
#define ANDROID_PRODUCT_ID				0x0002

#define	USB_STRING_MANUFACTURER			"SLSIAP"

#if	defined (CONFIG_MACH_S5P4418)
#define USB_STRING_PRODUCT				"S5P4418"
#define USB_STRING_SERIAL				"S5P4418"
#elif defined (CONFIG_MACH_S5P6818)
#define USB_STRING_PRODUCT				"S5P6818"
#define USB_STRING_SERIAL				"S5P6818"
#else
#define USB_STRING_SERIAL				"Unknown"
#endif

#define USB_STRING_MANUFACTURER_INDEX  	1
#define USB_STRING_PRODUCT_INDEX       	2
#define USB_STRING_SERIAL_INDEX 		3
#define USB_STRING_CONFIG_INDEX        	4
#define USB_STRING_INF_INDEX   		 	5
#define USB_STRING_MAX_INDEX   			USB_STRING_INF_INDEX

static char *usb_dev_descript[USB_STRING_MAX_INDEX+1];
static int	android_drvier = 1;

static int fboot_interface_init(void)
{
	f_cmd_inf *inf = &f_interface;
	int ret;

	ret = fastboot_init(inf);
	if (ret)
		return ret;

	if (android_drvier)
		return 0;

	usb_dev_descript[USB_STRING_MANUFACTURER_INDEX] = USB_STRING_MANUFACTURER;
	usb_dev_descript[USB_STRING_PRODUCT_INDEX]   	= USB_STRING_PRODUCT;
	usb_dev_descript[USB_STRING_SERIAL_INDEX] 		= USB_STRING_SERIAL;
	usb_dev_descript[USB_STRING_CONFIG_INDEX]    	= "Android Fastboot";
	usb_dev_descript[USB_STRING_INF_INDEX]  		= "Android Fastboot";

	inf->product_name = usb_dev_descript[USB_STRING_PRODUCT_INDEX];
	inf->serial_no = usb_dev_descript[USB_STRING_SERIAL_INDEX];

	return 0;
}

void fboot_usb_descriptor(descriptors_t *desc)
{
	U16	VID = USBD_VID, PID = USBD_PID;

	if (android_drvier) {
		desc->dev.idVendorL  = ANDROID_VENDOR_ID	& 0xff;	//0xB4;	/**/
		desc->dev.idVendorH  = ANDROID_VENDOR_ID	>>8;	//0x0B;	/**/
		desc->dev.idProductL = ANDROID_PRODUCT_ID	& 0xff;	//0xFF; /**/
		desc->dev.idProductH = ANDROID_PRODUCT_ID	>>8;	//0x0F; /**/
	} else {
		GetUSBID(&VID, &PID);
		debug("%s %x %x\n", __func__, VID, PID);

		desc->dev.idVendorL  = VID & 0xff;	//0xB4;	/**/
		desc->dev.idVendorH  = VID >> 8;	//0x0B;	/**/
		desc->dev.idProductL = PID & 0xff;	//0xFF; /**/
		desc->dev.idProductH = PID >> 8;	//0x0F; /**/
	}
}

static int do_fastboot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	static int init_parts = 0;
	const char *p;
	unsigned int tclk = TCLK_TICK_HZ;
	int timeout = 0, f_connect = 0;
	int err;

#ifdef FASTBOOT_DEV_DEFAULT
	p = getenv("fastbootdev");
	if (NULL == p) {
		char buf[64];
		sprintf(buf, "flash=mmc,%d:parts", board_mmc_bootdev());
		setenv("fastbootdev", buf);
		saveenv();
	}
#endif

	p = getenv("fastboot");
	if (NULL == p) {
		printf("*** Warning: use default fastboot commands ***\n");
		p = f_parts_default;
		init_parts = 0;

		sort_string((char*)p, strlen(p));
		setenv("fastboot", (char *)p);
		saveenv();
	}

	if (!init_parts) {
		part_lists_init(1);
		err = part_lists_make(p, strlen(p));
		if (0 > err)
			return err;

		init_parts = 1;
	}
	android_drvier = 1;

	if (argc > 1) {
		if (!strcmp(argv[1], "-l")) {
			part_lists_print();
			return 0;
		}

		if (!strcmp(argv[1], "-n"))
			timeout = simple_strtol(argv[1], NULL, 10);

		if (!strcmp(argv[1], "nexell"))
			android_drvier = 0;
	}

	part_lists_print();
	fboot_lcd_start();
	printf("Load USB Driver: %s\n", android_drvier?"android":"nexell");

	do {
		/* reset */
		f_connect = 0;
		if (0 == fboot_interface_init()) {
			unsigned int curr_time = (get_ticks()/tclk);
			unsigned int end_time = curr_time + timeout;

			printf("------------------------------------------\n");
			while (1) {
				int status = fastboot_poll();
				if (timeout)
					curr_time = (get_ticks()/tclk);

				if (status != FASTBOOT_OK) {
					if (ctrlc()) {
						printf("Fastboot ended by user\n");
						fboot_lcd_status("exit");
						f_connect = 0;
						break;
					}
				}

				if (FASTBOOT_ERROR == status) {
					printf("Fastboot error \n");
					fboot_lcd_status("error!!!");
					break;
				}
				else if (FASTBOOT_DISCONNECT == status) {
					f_connect = 1;
					printf("Fastboot disconnect detected\n");
					fboot_lcd_status("disconnect...");
					break;
				}
				else if ((FASTBOOT_INACTIVE == status) && timeout) {
					if (curr_time >= end_time) {
						printf("Fastboot inactivity detected\n");
						fboot_lcd_status("inactivity...");
						break;
					}
				}
				else {
					/* Something happened */
					/* Update the timeout endtime */
					if (timeout &&
						end_time != (curr_time+timeout)) {
						debug("Fastboot update inactive timeout (%d->", end_time);
						end_time = curr_time;
						end_time += timeout;
						debug("%d)\n", end_time);
					}
				}
			} /* while (1) */
		}

		fastboot_shutdown();
		mdelay(10);

	} while (f_connect);

	fboot_lcd_stop();
	return 0;
}


U_BOOT_CMD(
	fastboot,	4,	1,	do_fastboot,
	"fastboot- use USB Fastboot protocol\n",
	"[inactive timeout]\n"
	"    - Run as a fastboot usb device.\n"
	"    - The optional inactive timeout is the decimal seconds before\n"
	"    - the normal console resumes\n"
	"fastboot -n [inactive timeout]\n"
	"    - reflect new partition environments and Run.\n"
	"fastboot -l \n"
	"    - Print current fastboot partition map table.\n"
	"fastboot nexell \n"
	"    - connect to nexell usb device driver\n"
);



