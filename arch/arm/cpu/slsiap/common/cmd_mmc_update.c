/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

/*
 * Misc boot support
 */
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <mmc.h>
#include <decompress_ext4.h>
#include <asm/sections.h>

#if defined (CONFIG_MACH_S5P4418)
#include <asm/arch/s5p4418_boot.h>
#elif defined (CONFIG_MACH_S5P6818)
#include <asm/arch/s5p6818_boot.h>
#endif

#define MMC_BLOCK_SIZE		(512)

extern ulong mmc_bwrite(int dev_num, lbaint_t start, lbaint_t blkcnt, const void *src);
extern int mmc_get_part_table(block_dev_desc_t *desc, uint64_t (*parts)[2], int *count);

/* update_mmc [dev no] <type> 'mem' 'addr' 'length' [load addr] */
int do_update_mmc(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	block_dev_desc_t *desc;
	uint64_t dst_addr = 0, mem_len = 0;
	unsigned int mem_addr = 0;
	unsigned char *p;
	char cmd[32];
	lbaint_t blk, cnt;
	int ret, dev;

	if (6 > argc)
		goto usage;

	ret = get_device("mmc", argv[1], &desc);
	if (0 > ret) {
		printf ("** Not find device mmc.%s **\n", argv[1]);
		return 1;
	}

	dev = simple_strtoul (argv[1], NULL, 10);
	sprintf(cmd, "mmc dev %d", dev);
	if (0 > run_command(cmd, 0))	/* mmc device */
		return -1;

	if (0 != strcmp(argv[2], "2ndboot")  &&
		0 != strcmp(argv[2], "boot") &&
		0 != strcmp(argv[2], "raw")  &&
		0 != strcmp(argv[2], "part"))
		goto usage;

	mem_addr = simple_strtoul (argv[3], NULL, 16);
	dst_addr = simple_strtoull(argv[4], NULL, 16);
	mem_len  = simple_strtoull(argv[5], NULL, 16);

	p   = (unsigned char *)((ulong)mem_addr);
	blk = (dst_addr/MMC_BLOCK_SIZE);
	cnt = (mem_len/MMC_BLOCK_SIZE) + ((mem_len & (MMC_BLOCK_SIZE-1)) ? 1 : 0);

	flush_dcache_all();

	if (! strcmp(argv[2], "2ndboot")) {
		struct boot_dev_head *bh = (struct boot_dev_head *)((ulong)mem_addr);
		struct boot_dev_mmc  *bd = (struct boot_dev_mmc *)&bh->bdi;

		bd->port_no = dev; /* set u-boot device port num */
		printf("head boot dev  = %d\n", bd->port_no);

		goto do_write;
	}

	if (! strcmp(argv[2], "boot")) {
		struct boot_dev_head head;
		struct boot_dev_head *bh = &head;
		struct boot_dev_mmc *bd = (struct boot_dev_mmc *)&bh->bdi;
		int len = sizeof(head);
		unsigned int load = CONFIG_SYS_TEXT_BASE;

		if (argc == 7)
			load = simple_strtoul (argv[6], NULL, 16);

		memset((void*)&head, 0x00, len);

		bh->load_addr = (unsigned int)load;
		bh->jump_addr = bh->load_addr;
		bh->load_size = (unsigned int)mem_len;
		bh->signature = SIGNATURE_ID;
		bd->port_no	  = dev;

		printf("head boot dev  = %d\n", bd->port_no);
		printf("head load addr = 0x%08x\n", bh->load_addr);
		printf("head load size = 0x%08x\n", bh->load_size);
		printf("head gignature = 0x%08x\n", bh->signature);

		p -= len;
		memcpy(p, bh, len);

		mem_len += MMC_BLOCK_SIZE;
		cnt = (mem_len/MMC_BLOCK_SIZE) + ((mem_len & (MMC_BLOCK_SIZE-1)) ? 1 : 0);

		goto do_write;
	}

	if (strcmp(argv[2], "part") == 0) {
		uint64_t parts[4][2] = { {0,0}, };
		uint64_t part_len = 0;
		int partno = (int)dst_addr;
		int num = 0;

		if (0 > mmc_get_part_table(desc, parts, &num))
			return 1;

		if (partno > num || 1 > partno)  {
			printf ("** Invalid mmc.%d partition number %d (1 ~ %d) **\n",
				dev, partno, num);
			return 1;
		}

		dst_addr = parts[partno-1][0];	/* set write addr from part table */
		part_len = parts[partno-1][1];
		blk = (dst_addr/MMC_BLOCK_SIZE);

 		if (0 == check_compress_ext4((char*)p, part_len)) {
			printf("update mmc.%d compressed ext4 = 0x%llx(%d) ~ 0x%llx(%d): ",
				dev, dst_addr, (unsigned int)blk, mem_len, (unsigned int)cnt);

			ret = write_compressed_ext4((char*)p, blk);
			printf("%s\n", ret?"Fail":"Done");
			return 1;
		}
		goto do_write;
	}

do_write:
	if (! blk) {
		printf("-- Fail: start %d block(0x%llx) is in MBR zone (0x200) --\n", (int)blk, dst_addr);
		return -1;
	}

	printf("update mmc.%d type %s = 0x%llx(0x%x) ~ 0x%llx(0x%x): ",
		dev, argv[2], dst_addr, (unsigned int)blk, mem_len, (unsigned int)cnt);

	ret = mmc_bwrite(dev, blk, cnt, (void const*)p);

	printf("%s\n", ret?"Done":"Fail");
	return ret;

usage:
	cmd_usage(cmdtp);
	return 1;
}

U_BOOT_CMD(
	update_mmc, CONFIG_SYS_MAXARGS, 1,	do_update_mmc,
	"update mmc data\n",
	"<dev no> <type> <mem> <addr> <length>\n"
	"    - type :  2ndboot | boot | raw | part \n\n"
	"update_mmc <dev no> boot 'mem' 'addr' 'length' [load addr]\n"
	"    - update  data 'length' add boot header(512) on 'mem' to device addr, \n"
	"      and set jump addr with 'load addr'\n"
	"      if no [load addr], set jump addr default u-boot _TEXT_BASE_\n\n"
	"update_mmc <dev no> raw 'mem' 'addr' 'length'\n"
	"    - update data 'length' on 'mem' to device addr.\n\n"
	"update_mmc <dev no> part 'mem' 'part no' 'length'\n"
	"    - update partition image 'length' on 'mem' to mmc 'part no'.\n\n"
	"Note.\n"
	"    - All numeric parameters are assumed to be hex.\n"
);

