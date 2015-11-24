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

#include <common.h>
#include <command.h>
#include <mmc.h>

/*
#define	debug	printf
*/

#define MMC_BLOCK_SIZE			(512)
#define	MAX_PART_TABLE			(8)
#define DOS_EBR_BLOCK				(0x100000/MMC_BLOCK_SIZE)

#define DOS_PART_DISKSIG_OFFSET		0x1b8
#define DOS_PART_TBL_OFFSET			0x1be
#define DOS_PART_MAGIC_OFFSET		0x1fe
#define DOS_PBR_FSTYPE_OFFSET		0x36
#define DOS_PBR32_FSTYPE_OFFSET		0x52
#define DOS_PBR_MEDIA_TYPE_OFFSET	0x15
#define DOS_MBR	0
#define DOS_PBR	1

typedef struct dos_partition {
	unsigned char boot_ind;		/* 0x80 - active			*/
	unsigned char head;		/* starting head			*/
	unsigned char sector;		/* starting sector			*/
	unsigned char cyl;		/* starting cylinder			*/
	unsigned char sys_ind;		/* What partition type			*/
	unsigned char end_head;		/* end head				*/
	unsigned char end_sector;	/* end sector				*/
	unsigned char end_cyl;		/* end cylinder				*/
	unsigned char start4[4];	/* starting sector counting from 0	*/
	unsigned char size4[4];		/* nr of sectors in partition		*/
} dos_partition_t;

/* Convert char[4] in little endian format to the host format integer
 */
static inline void int_to_le32(unsigned char *le32, unsigned int blocks)
{
	le32[3] = (blocks >> 24) & 0xff;
	le32[2] = (blocks >> 16) & 0xff;
	le32[1] = (blocks >>  8) & 0xff;
	le32[0] = (blocks >>  0) & 0xff;
}

static inline int le32_to_int(unsigned char *le32)
{
    return ((le32[3] << 24) +
	    (le32[2] << 16) +
	    (le32[1] << 8) +
	     le32[0]
	   );
}

static inline int is_extended(int part_type)
{
    return (part_type == 0x5 ||
        part_type == 0xf ||
        part_type == 0x85);
}

static inline void part_mmc_chs(dos_partition_t *pt, lbaint_t lba_start, lbaint_t lba_size)
{
	int head = 1, sector = 1, cys = 0;
	int end_head = 254, end_sector = 63, end_cys = 1023;

	/* default CHS */
	pt->head = (unsigned char)head;
	pt->sector = (unsigned char)(sector + ((cys & 0x00000300) >> 2) );
	pt->cyl = (unsigned char)(cys & 0x000000FF);
	pt->end_head = (unsigned char)end_head;
	pt->end_sector = (unsigned char)(end_sector + ((end_cys & 0x00000300) >> 2) );
	pt->end_cyl = (unsigned char)(end_cys & 0x000000FF);

	int_to_le32(pt->start4, lba_start);
	int_to_le32(pt->size4, lba_size);
}

static int mmc_get_part_table_extended(block_dev_desc_t *desc,
				lbaint_t lba_start, lbaint_t relative, uint64_t (*parts)[2], int *part_num)
{
	unsigned char buffer[512] = { 0, };
	dos_partition_t *pt;
	lbaint_t lba_s, lba_l;
	int i = 0;

	debug("--- LBA S= 0x%llx : 0x%llx ---\n", (uint64_t)lba_start*desc->blksz, (uint64_t)relative*desc->blksz);

	if (0 > desc->block_read(desc->dev, lba_start, 1, (void *)buffer)) {
		printf ("** Error read mmc.%d partition info **\n", desc->dev);
		return -1;
	}

	if (buffer[DOS_PART_MAGIC_OFFSET] != 0x55 ||
		buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xAA) {
		printf ("** Not find partition magic number **\n");
		return 0;
	}

	/* Print all primary/logical partitions */
	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	for (i = 0; 4 > i; i++, pt++) {

		lba_s = le32_to_int(pt->start4);
		lba_l = le32_to_int(pt->size4);

		if (is_extended(pt->sys_ind)) {
			debug("Extd p=\t0x%llx \t~ \t0x%llx (%ld:%ld) <%d>\n",
				(uint64_t)lba_s*desc->blksz, (uint64_t)lba_l*desc->blksz, lba_s, lba_l, i);
			continue;
		}

		if (lba_s && lba_l) {
			parts[*part_num][0] = (uint64_t)(lba_start + lba_s)*desc->blksz;
			parts[*part_num][1] = (uint64_t)lba_l*desc->blksz;
			debug("part.%d=\t0x%llx \t~ \t0x%llx (%ld:%ld) <%d>\n",
				*part_num, parts[*part_num][0], parts[*part_num][1], lba_s, lba_l, i);
			*part_num += 1;
		}
	}

	/* Follows the extended partitions */
	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	for (i = 0; 4 > i; i++, pt++) {
		if (is_extended(pt->sys_ind)) {
			lba_s = le32_to_int(pt->start4) + relative;
			mmc_get_part_table_extended(desc,
				lba_s,								/* next read block */
				lba_start == 0 ? lba_s : relative,	/* base offset, first offst */
				parts, part_num);
		}
	}
	return 0;
}

int mmc_get_part_table(block_dev_desc_t *desc, uint64_t (*parts)[2], int *part_num)
{
	int partcnt = 0;
	int ret = mmc_get_part_table_extended(desc, 0, 0, parts, &partcnt);
	if (0 > ret)
		return ret;

	*part_num = partcnt;
	return 0;
}

int mmc_make_part_table_extended(block_dev_desc_t *desc, lbaint_t lba_start,
					lbaint_t relative, uint64_t (*parts)[2], int part_num)
{
	unsigned char buffer[512] = { 0, };
	dos_partition_t *pt;
	lbaint_t lba_s = 0, lba_l = 0, last_lba = lba_start;
	int i = 0, ret = 0;

	debug("--- LBA S= 0x%llx : 0x%llx ---\n",
		(uint64_t)lba_start*desc->blksz, (uint64_t)relative*desc->blksz);

	memset(buffer, 0x0, sizeof(buffer));
	buffer[DOS_PART_MAGIC_OFFSET] = 0x55;
	buffer[DOS_PART_MAGIC_OFFSET + 1] = 0xAA;

	pt = (dos_partition_t *)(buffer + DOS_PART_TBL_OFFSET);
	for (i = 0; 2 > i && part_num > i; i++, pt++) {
		lba_s = (lbaint_t)(parts[i][0] / desc->blksz);
		lba_l = (lbaint_t)(parts[i][1] / desc->blksz);

		if (0 == lba_s) {
			printf("-- Fail: invalid part.%d start 0x%llx --\n", i, parts[i][0]);
			return -1;
		}

		if (last_lba > lba_s) {
			printf ("** Overlapped primary part table 0x%llx previous 0x%llx (EBR) **\n",
				(uint64_t)(lba_s)*desc->blksz, (uint64_t)(last_lba)*desc->blksz);
			return -1;
		}

		if (0 == lba_l)
			lba_l = desc->lba - last_lba - DOS_EBR_BLOCK;

		if (lba_l > (desc->lba - last_lba)) {
			printf("-- Fail: part %d invalid length 0x%llx, avaliable (0x%llx)(EBR) --\n",
				i, (uint64_t)(lba_l)*desc->blksz, (uint64_t)(desc->lba-last_lba)*desc->blksz);
			return -1;
		}

		pt->boot_ind = 0x00;
		pt->sys_ind  = 0x83;
		if (i == 1) {
			pt->sys_ind = 0x05;
			lba_s -= relative + DOS_EBR_BLOCK;
			lba_l += DOS_EBR_BLOCK;
		} else {
			lba_s -= lba_start;
		}

		debug("%s=\t0x%llx \t~ \t0x%llx \n",
			i == 0 ? "Prim p" : "Extd p",
			i == 0 ? (uint64_t)(lba_s + lba_start)*desc->blksz : (uint64_t)(lba_s)*desc->blksz,
			(uint64_t)(lba_l)*desc->blksz);

		part_mmc_chs(pt, lba_s, lba_l);
		last_lba = lba_s + lba_l;
	}

	ret = desc->block_write(desc->dev, lba_start, 1, (const void *)buffer);
	if (0 > ret)
		return ret;

	if (part_num)
		return mmc_make_part_table_extended(desc,
				lba_s + relative, relative, &parts[1], part_num-1);

	return ret;
}

int mmc_make_part_table(block_dev_desc_t *desc,
				uint64_t (*parts)[2], int part_num, unsigned int part_type)
{
	unsigned char buffer[MMC_BLOCK_SIZE];
	dos_partition_t *pt;
	uint64_t lba_s, lba_l, last_lba = 0;
	uint64_t avalible = desc->lba;
	int part_tables = part_num, part_EBR = 0;
	int i = 0, ret = 0;

	debug("--- Create mmc.%d partitions %d ---\n", desc->dev, part_num);
	if (part_type != PART_TYPE_DOS) {
		printf ("** Support only DOS PARTITION **\n");
		return -1;
	}

	if (1 > part_num || part_num > MAX_PART_TABLE) {
		printf ("** Can't make partition tables %d (1 ~ %d) **\n", part_num, MAX_PART_TABLE);
		return -1;
	}

	debug("Total = %lld * %d :0x%llx (%d.%d G) \n", avalible,
		(int)desc->blksz, (uint64_t)(avalible)*desc->blksz,
		(int)((avalible*desc->blksz)/(1024*1024*1024)),
		(int)((avalible*desc->blksz)%(1024*1024*1024)));

	memset(buffer, 0x0, sizeof(buffer));
	buffer[DOS_PART_MAGIC_OFFSET] = 0x55;
	buffer[DOS_PART_MAGIC_OFFSET + 1] = 0xAA;

	if (part_num > 4) {
		part_tables = 3;
		part_EBR = 1;
	}

	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	last_lba = (int)(parts[0][0] / (uint64_t)desc->blksz);

	for (i = 0; part_tables > i; i++, pt++) {
		lba_s = (lbaint_t)(parts[i][0] / desc->blksz);
		lba_l = parts[i][1] ? (lbaint_t)(parts[i][1] / desc->blksz) : (desc->lba - last_lba);

		if (0 == lba_s) {
			printf("-- Fail: invalid part.%d start 0x%llx --\n", i, parts[i][0]);
			return -1;
		}

		if (last_lba > lba_s) {
			printf ("** Overlapped primary part table 0x%llx previous 0x%llx **\n",
				(uint64_t)(lba_s)*desc->blksz, (uint64_t)(last_lba)*desc->blksz);
			return -1;
		}

		if (lba_l > (desc->lba - last_lba)) {
			printf("-- Fail: part %d invalid length 0x%llx, avaliable (0x%llx) --\n",
				i, (uint64_t)(lba_l)*desc->blksz, (uint64_t)(desc->lba-last_lba)*desc->blksz);
			return -1;
		}

		/* Linux partition */
		pt->boot_ind = 0x00;
		pt->sys_ind  = 0x83;
		part_mmc_chs(pt, lba_s, lba_l);
		last_lba = lba_s + lba_l;

		debug("part.%d=\t0x%llx \t~ \t0x%llx \n",
			i, (uint64_t)(lba_s)*desc->blksz, (uint64_t)(lba_l)*desc->blksz);
	}

	if (part_EBR) {
		lba_s = (lbaint_t)(parts[3][0] / desc->blksz) - DOS_EBR_BLOCK;
		lba_l = (lbaint_t)(desc->lba - last_lba);

		if (last_lba > lba_s) {
			printf ("** Overlapped extended part table 0x%llx (with offset -0x%llx) previous 0x%llx **\n",
				(uint64_t)(lba_s)*desc->blksz, (uint64_t)DOS_EBR_BLOCK*desc->blksz,
				(uint64_t)(last_lba)*desc->blksz);
			return -1;
		}

		/* Extended partition */
		pt->boot_ind = 0x00;
		pt->sys_ind  = 0x05;
		part_mmc_chs(pt, lba_s, lba_l);

	}

	ret = desc->block_write(desc->dev, 0, 1, (const void *)buffer);
	if (0 > ret)
		return ret;

	if (part_EBR)
		return mmc_make_part_table_extended(desc, lba_s, lba_s, &parts[part_tables], part_num - part_tables);

	return ret;

}

/*
 * cmd : fdisk 0 n, s1:size, s2:size, s3:size, s4:size
 */
static int do_fdisk(cmd_tbl_t *cmdtp, int flag, int argc, char* const argv[])
{
	block_dev_desc_t *desc;
	int i = 0, ret;

	if (2 > argc)
		return CMD_RET_USAGE;

	ret = get_device("mmc", argv[1], &desc);
	if (ret < 0) {
		printf ("** Not find device mmc.%s **\n", argv[1]);
		return 1;
	}

	if (2 == argc) {
		print_part(desc);
		dev_print(desc);
	} else {
		uint64_t parts[MAX_PART_TABLE][2];
		int count = 1;

		count = simple_strtoul(argv[2], NULL, 10);
		if (1 > count || count > MAX_PART_TABLE) {
			printf ("** Invalid partition table count %d (1 ~ %d) **\n", count, MAX_PART_TABLE);
			return -1;
		}

		for (i = 0; count > i; i++) {
			const char *p = argv[i+3];
			parts[i][0] = simple_strtoull(p, NULL, 16);	/* start */
			if (!(p = strchr(p, ':'))) {
				printf("no <0x%llx:length> identifier\n", parts[i][0]);
				return -1;
			}
			p++;
			parts[i][1] = simple_strtoull(p, NULL, 16);		/* end */
			debug("part[%d] 0x%llx:0x%llx\n", i, parts[i][0], parts[i][1]);
		}

		mmc_make_part_table(desc, parts, count, PART_TYPE_DOS);
	}
	return 0;
}

U_BOOT_CMD(
	fdisk, 16, 1, do_fdisk,
	"mmc list or create ms-dos partition tables (MAX TABLE 7)",
	"<dev no>\n"
	"	- list partition table info\n"
	"fdisk <dev no> [part table counts] <start:length> <start:length> ...\n"
	"	- Note. each arguments seperated with space"
	"	- Create partition table info\n"
	"	- All numeric parameters are assumed to be hex.\n"
	"	- start and length is offset.\n"
	"	- If the length is zero, uses the remaining.\n"
);

