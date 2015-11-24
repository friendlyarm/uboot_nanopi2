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

#ifndef _NAND_FTL_H_
#define _NAND_FTL_H_

#include <linux/list.h>

/*
 * nand info : nand chip info. (block/page/block_cnt/ecc)
 *
 * Layout:
 * ===========================
 *
 *       F T L
 *
 * ===========================     <-- nxp_ftl_start_block
 *       linear:normal
 * ===========================     <-- linear_block_start
 *       linear:raw
 * ===========================
 *
 * -> FTL region
 *    FTL managed region. Must be accessible after the function mio_init() is called.
 *
 * -> linear:normal region
 *    Linear access region. Must be accessible after the function mio_init() is called.
 *
 * -> linear:raw region
 *    no ecc, no read-retry, no randomize region.
 *
 * API:
 *   mio.uboot.h
 */ 
struct nand_ftl {
	struct list_head link;

	/* ftl driver info. */
	block_dev_desc_t block_dev;
	char part_num;
	uint32_t version;
	uint32_t ftl_status;


	/* ftl block info. */
	uint32_t nxp_ftl_start_block;
	/* linear block info. */
	uint32_t linear_block_start;


	/* chip info */
	uint32_t capacity;
	// manufacture, ...
	void *priv;
};


/* block driver */
struct nand_ftl *find_nand_device(int dev_num);
block_dev_desc_t *nand_get_dev(int dev);
int nand_switch_part(int dev_num, unsigned int part_num);

unsigned long nand_bread(int dev, lbaint_t start, lbaint_t blkcnt, void *buffer);
unsigned long nand_bwrite(int dev, lbaint_t start, lbaint_t blkcnt, const void *buffer);
unsigned long nand_berase(int dev, lbaint_t start, lbaint_t blkcnt);


/* ftl */
int nand_ftl_init(void);
int nand_startup(struct nand_ftl *nand);

#endif /* _NAND_FTL_H_ */
