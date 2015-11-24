/*
 * (C) Copyright 2014
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
#include <malloc.h>
#include <errno.h>
#include <part.h>
#include <mio.uboot.h>
#include <nand_ftl.h>
#include "loglevel.h"

static struct list_head nand_devices;
unsigned long nxp_ftl_start_block = 0;

static int nand_register(struct nand_ftl *nand);
static int nand_drv_register(struct nand_ftl *nand);

struct nand_ftl *find_nand_device(int dev_num)
{
	struct nand_ftl *f;
	struct list_head *entry;

	list_for_each(entry, &nand_devices) {
		f = list_entry(entry, struct nand_ftl, link);
		if (f->block_dev.dev == dev_num)
			return f;
	}

	return NULL;
}


/* u-boot block driver */
block_dev_desc_t *nand_get_dev(int dev)
{
	struct nand_ftl *nand = find_nand_device(dev);

	if (!nand)
		return NULL;

	nand_drv_register(nand);
	nx_debug("get nand. (lba: 0x%lx)\n", nand->block_dev.lba);

	return &nand->block_dev;
}

int nand_select_hwpart(int dev_num, int hwpart)
{
	struct nand_ftl *nand = find_nand_device(dev_num);
	int ret;

	if (!nand)
		return -ENODEV;

	if (nand->part_num == hwpart)
		return 0;

	ret = nand_switch_part(dev_num, hwpart);
	if (ret)
		return ret;

	nand->part_num = hwpart;

	return 0;
}

int nand_switch_part(int dev_num, unsigned int part_num)
{
	struct nand_ftl *nand = find_nand_device(dev_num);

	if (!nand)
		return -1;

	return 0;
	//return nand_set_capacity(mmc, part_num);
}

static int nand_drv_register(struct nand_ftl *nand)
{
	nand->block_dev.blksz = 512;
	nand->block_dev.log2blksz = LOG2(nand->block_dev.blksz);
	//nand->block_dev.lba = lldiv(nand->capacity, nand->block_dev.blksz);
	nand->block_dev.vendor[0] = 0;
	nand->block_dev.product[0] = 0;
	nand->block_dev.revision[0] = 0;

	if (nand->ftl_status)
	{
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBDISK_SUPPORT)
		init_part(&nand->block_dev);
#endif

		/* get NAND info */
		get_mio_capacity();		// fill nand->capacity;
		nand->block_dev.lba = nand->capacity;
	}

	return 0;
}

int nand_startup(struct nand_ftl *nand)
{
	/* ftl start */
	if (mio_init() >= 0) {
		nand->ftl_status = 1;
		nx_info("mio_init success.\n");
	}
	else {
		nand->ftl_status = 0;
		nx_info("mio_init failure.\n");
	}

	return 0;
}


unsigned long nand_bread(int dev, lbaint_t start, lbaint_t blkcnt, void *buffer)
{
	struct nand_ftl *nand = find_nand_device(dev);
	if (!nand)
		return 0;

	return mio_read((long)start, blkcnt, buffer);
}
unsigned long nand_bwrite(int dev, lbaint_t start, lbaint_t blkcnt, const void *buffer)
{
	struct nand_ftl *nand = find_nand_device(dev);
	if (!nand)
		return 0;

	return mio_write((long)start, blkcnt, (ulong *)buffer);
}
unsigned long nand_berase(int dev, lbaint_t start, lbaint_t blkcnt)
{
	// buffer ...
	// while (blkcnt--)
	//	  mio_write((long)start, ...);
	// flush...
	return 0;
}


static int nand_register(struct nand_ftl *nand)
{
	/* FTL start block - Physical Block Aligned. */
	nxp_ftl_start_block = CFG_NAND_FTL_START_BLOCK;

	/* fill in device description */
	nand->block_dev.if_type = IF_TYPE_NAND;
	nand->block_dev.dev = 0;
	nand->block_dev.type = 0;
	nand->block_dev.lun = 0;
	nand->block_dev.removable = 0;
	nand->block_dev.block_read = nand_bread;
	nand->block_dev.block_write = nand_bwrite;
	nand->block_dev.block_erase = nand_berase;

	INIT_LIST_HEAD (&nand->link);
	list_add_tail (&nand->link, &nand_devices);

	return 0;
}


int nand_ftl_init(void)
{
	struct nand_ftl *nand;

	INIT_LIST_HEAD (&nand_devices);
	nand = malloc(sizeof(struct nand_ftl));
	if (!nand) {
		nx_error("nand ftl alloc failed!\n");
		return -1;
	}

	/* register nand */
	nand_register(nand);

	/* start ftl */
	nand_startup(nand);

	/* register nand driver */
	nand_drv_register(nand);

	return 0;
}
