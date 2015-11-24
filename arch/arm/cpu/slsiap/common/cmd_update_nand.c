/*
 * (C) Copyright Nexell
 */

#include <common.h>
#include <linux/mtd/mtd.h>
#include <command.h>
#include <watchdog.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <jffs2/jffs2.h>
#include <nand.h>

#define __round_mask(x, y) ((typeof(x))((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
#define round_down(x, y) ((x) & ~__round_mask(x, y))


#if (0)
#define DBGOUT(msg...)		{ printf("NANDUP: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif


#define CFG_NANDCMD		do_nand

extern int do_nand(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]);

/* ------------------------------------------------------------------------- */

#ifdef CONFIG_CMD_MTDPARTS
static int set_dev(int dev)
{
	if (dev < 0 || dev >= CONFIG_SYS_MAX_NAND_DEVICE ||
	    !nand_info[dev].name) {
		puts("No such device\n");
		return -1;
	}

	if (nand_curr_device == dev)
		return 0;

	printf("Device %d: %s", dev, nand_info[dev].name);
	puts("... is now current device\n");
	nand_curr_device = dev;

#ifdef CONFIG_SYS_NAND_SELECT_DEVICE
	board_nand_select_device(nand_info[dev].priv, dev);
#endif

	return 0;
}
#endif

static inline int str2off(const char *p, loff_t *num)
{
	char *endptr;

	*num = simple_strtoull(p, &endptr, 16);
	return *p != '\0' && *endptr == '\0';
}

static inline int str2long(const char *p, ulong *num)
{
	char *endptr;

	*num = simple_strtoul(p, &endptr, 16);
	return *p != '\0' && *endptr == '\0';
}

static int get_part(const char *partname, int *idx, loff_t *off, loff_t *size)
{
#ifdef CONFIG_CMD_MTDPARTS
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
	int ret;

	ret = mtdparts_init();
	if (ret)
		return ret;

	ret = find_dev_and_part(partname, &dev, &pnum, &part);
	if (ret)
		return ret;

	if (dev->id->type != MTD_DEV_TYPE_NAND) {
		puts("not a NAND device\n");
		return -1;
	}

	*off = part->offset;
	*size = part->size;
	*idx = dev->id->num;

	ret = set_dev(*idx);
	if (ret)
		return ret;

	return 0;
#else
	puts("offset is not a number\n");
	return -1;
#endif
}

static int arg_off(const char *arg, int *idx, loff_t *off, loff_t *maxsize)
{
	if (!str2off(arg, off))
		return get_part(arg, idx, off, maxsize);

	if (*off >= nand_info[*idx].size) {
		puts("Offset exceeds device limit\n");
		return -1;
	}

	*maxsize = nand_info[*idx].size - *off;
	return 0;
}

static int arg_off_size(int argc, char *const argv[], int *idx,
			loff_t *off, loff_t *size)
{
	int ret;
	loff_t maxsize = 0;

	if (argc == 0) {
		printf("Can't update entire chip.\n");
		return -1;
	}

	if (str2off(argv[0], off) && argc == 1) {
		*size = 0;
		goto print;
	}

	ret = arg_off(argv[0], idx, off, &maxsize);
	if (ret)
		return ret;

	if (argc == 1) {
		*size = maxsize;
		goto print;
	}

	if (!str2off(argv[1], size)) {
		printf("'%s' is not a number\n", argv[1]);
		return -1;
	}

	if (*size > maxsize) {
		puts("Size exceeds partition or device limit\n");
		return -1;
	}

print:
	printf("device %d ", *idx);
	if (*size == nand_info[*idx].size)
		puts("whole chip\n");
	else
		printf("offset 0x%llx, size 0x%llx\n",
		       (unsigned long long)*off, (unsigned long long)*size);
	return 0;
}

int do_update_nand(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i, ret = 0;
	ulong addr, mem_pos;
	loff_t off, size;
	char *cmd, *s;
	nand_info_t *nand;
#ifdef CONFIG_SYS_NAND_QUIET
	int quiet = CONFIG_SYS_NAND_QUIET;
#else
	int quiet = 0;
#endif
	const char *quiet_str = getenv("quiet");
	int dev = nand_curr_device;
 

	loff_t start, end;				// data start, data end
	loff_t nstart, nend;			// rw start, rw end in block
	loff_t bstart, bend;			// start of the block, end of the block

	uint64_t remain = 0;
	uint64_t update_size = 0;		// update size
	ulong blk_size = 0;				// erase block size
	u_char *datbuf = NULL;			// nand data buffer


	/* for commands */
	char *argv_erase[5]  = { NULL, "erase",  NULL, NULL, NULL, };
	char *argv_read[6]   = { NULL, NULL,  NULL, NULL, NULL, NULL, };
	char *argv_write[6]  = { NULL, NULL,  NULL, NULL, NULL, NULL, };

	/* command buffer */
	char rw_start[32] = { 0, }, rw_length[32] = { 0, };
	char wr_buffer[32] = { 0, };
	char wr_type[32] = { 0, };
	char rd_type[32] = { 0, };


	/* at least 3 arguments please */
	if (argc < 3)
		goto usage;

	if (quiet_str)
		quiet = simple_strtoul(quiet_str, NULL, 0) != 0;

	cmd = argv[1];

	/* The following commands operate on the current device, unless
	 * overridden by a partition specifier.  Note that if somehow the
	 * current device is invalid, it will have to be changed to a valid
	 * one before these commands can run, even if a partition specifier
	 * for another device is to be used.
	 */
	if (dev < 0 || dev >= CONFIG_SYS_MAX_NAND_DEVICE ||
	    !nand_info[dev].name) {
		puts("\nno devices available\n");
		return 1;
	}

	for (i = 0; i < argc; i++)
		DBGOUT ("argv[%d]: %s\n", i, argv[i]);


	/*
	 * Syntax is:
	 *   0           1     2
	 *   update_nand erase [off size]
	 * 
	 *  +---------+--------+---------+
	 *  | partial | full   | partial |
	 *  +---------+--------+---------+
	 *        |_____________|
	 *          erase length
	 *
	 *   - partial block
	 *       1) Read the block
	 *       2) Erase the block
	 *       3) Corresponding to the deleted region 'ff' is filled.
	 *       4) Write the block
     *
	 *   - full block
	 *       1) erase block
	 */

	if (0 == strncmp (cmd, "erase", 5)) {
		
		if (argc < 3)
			goto usage;

		/* skip first two or three arguments, look for offset and size */
		if (arg_off_size(argc - 2, argv + 2, &dev, &off, &size) != 0)
			return 1;


		addr = (ulong)simple_strtoul(argv[2], NULL, 16);
		mem_pos = addr;

		nand = &nand_info[dev];
		blk_size = nand->erasesize;
		DBGOUT (" erasesize: 0x%lx\n",  blk_size);

		remain = (uint64_t)size;
		DBGOUT (" remain: %llx off: 0x%llx\n", remain, (unsigned long long)off);

		datbuf = memalign (ARCH_DMA_MINALIGN, blk_size);
		if (!datbuf) {
			puts("No memory for block buffer\n");
			return 1;
		}


		start = off;
		end = off + size;

		bstart  = round_down (start, blk_size);
		bend    = bstart + blk_size;


		/* make command */
		argv_erase[2] = rw_start;
		argv_erase[3] = rw_length;

		argv_read[1] = rd_type;
		argv_read[2] = wr_buffer;
		argv_read[3] = rw_start;
		argv_read[4] = rw_length;

		argv_write[1] = wr_type;
		argv_write[2] = wr_buffer;
		argv_write[3] = rw_start;
		argv_write[4] = rw_length;

#ifdef CONFIG_NAND_RANDOMIZER
		no_nand_randomize	= 1;
#endif

		while (remain > 0) {
			ulong offset = 0;

			memset (datbuf, 0x00, blk_size);

			nstart = max (bstart, start);
			nend = min (bend, end);

			update_size = (uint64_t)(nend - nstart);
			DBGOUT ("nstart: 0x%llx nend: 0x%llx\n", nstart, nend);
			offset = nstart - bstart;

			sprintf (wr_type, "write");							/* build write command */
			sprintf (rd_type, "read");							/* build read command */

			/* build commands */
			sprintf (wr_buffer, "0x%lx", (ulong)datbuf);
			sprintf (rw_start, "0x%llx", bstart);
			sprintf (rw_length, "0x%lx", blk_size);

			if (update_size < blk_size)								/* partial block */
			{
				ret = CFG_NANDCMD (NULL, 0, 5, argv_read);			/* read a block */
				if (ret)
				{
					printk ("  -> read failed\n");
					goto out;
				}
				CFG_NANDCMD (NULL, 0, 4, argv_erase);				/* erase a block */
				memset (datbuf + offset, 0xff, update_size);		/* erase update region */
				ret = CFG_NANDCMD (NULL, 0, 5, argv_write);			/* write a block */
				DBGOUT ("offset: %lx, update_size: %llx, mem_pos: %lx\n", offset, update_size, mem_pos);
			}
			else													/* full block */
			{
				CFG_NANDCMD (NULL, 0, 4, argv_erase);				/* erase a block */
			}

			remain -= update_size;
			bstart += blk_size;
			bend = bstart + blk_size;
			mem_pos += update_size;
		}

		goto done;
	}


	/*
	 * Syntax is:
	 *   0           1     2         3
	 *   update_nand write [address] [off size]
	 *
	 *  +---------+--------+---------+
	 *  | partial | full   | partial |
	 *  +---------+--------+---------+
	 *        |_____________|
	 *          update length
	 *
	 *
	 *  from start to end block
	 *
	 *   if) partial block
	 *       1) Read the block
	 *       2) Erase the block
	 *       3) Copy the length of the buffer
	 *       4) Write the block
     *
	 *   if) full block
	 *       1) Erase the block
	 *       2) Copy the length of the buffer
	 *       3) Write the block
	 */

	if (0 == strncmp (cmd, "write", 5)) {

		ulong pagecount = 1;
		int raw;

		if (argc < 4)
			goto usage;


		addr = (ulong)simple_strtoul(argv[2], NULL, 16);
		mem_pos = addr;

		nand = &nand_info[dev];

		s = strchr(cmd, '.');

		if (s && !strcmp(s, ".raw")) {
			raw = 1;

			if (arg_off(argv[3], &dev, &off, &size))
				return 1;

			if (argc > 4 && !str2long(argv[4], &pagecount)) {
				printf("'%s' is not a number\n", argv[4]);
				return 1;
			}

			if (pagecount * nand->writesize > size) {
				puts("Size exceeds partition or device limit\n");
				return -1;
			}

			remain = (uint64_t)(pagecount * (nand->writesize + nand->oobsize));
		} else {
			if (arg_off_size(argc - 3, argv + 3, &dev, &off, &size) != 0)
				return 1;

			remain = (uint64_t)size;
		}

		blk_size = nand->erasesize;
		DBGOUT (" erasesize: 0x%lx\n",  blk_size);

		DBGOUT (" remain: %llx off: 0x%llx\n", remain, (unsigned long long)off);

		datbuf = memalign (ARCH_DMA_MINALIGN, blk_size);
		if (!datbuf) {
			puts("No memory for block buffer\n");
			return 1;
		}


		start = off;
		end = off + size;

		bstart  = round_down (start, blk_size);
		bend    = bstart + blk_size;


		/* make command */
		argv_erase[2] = rw_start;
		argv_erase[3] = rw_length;

		argv_read[1] = rd_type;
		argv_read[2] = wr_buffer;
		argv_read[3] = rw_start;
		argv_read[4] = rw_length;

		argv_write[1] = wr_type;
		argv_write[2] = wr_buffer;
		argv_write[3] = rw_start;
		argv_write[4] = rw_length;

#ifdef CONFIG_NAND_RANDOMIZER
		no_nand_randomize	= 1;
#endif

		while (remain > 0) {
			ulong offset = 0;

			memset (datbuf, 0x00, blk_size);

			nstart = max (bstart, start);
			nend = min (bend, end);

			update_size = (uint64_t)(nend - nstart);
			DBGOUT ("nstart: 0x%llx nend: 0x%llx\n", nstart, nend);
			offset = nstart - bstart;

			sprintf (wr_type, "write");							/* build write command */
			sprintf (rd_type, "read");							/* build read command */

			/* build commands */
			sprintf (wr_buffer, "0x%lx", (ulong)datbuf);
			sprintf (rw_start, "0x%llx", bstart);
			sprintf (rw_length, "0x%lx", blk_size);

			if (update_size < blk_size)							/* partial block */
			{
				ret = CFG_NANDCMD (NULL, 0, 5, argv_read);		/* read a block */
				if (ret)
				{
					printk ("  -> write failed\n");
					goto out;
				}
			}

			CFG_NANDCMD (NULL, 0, 4, argv_erase);				/* erase a block */
			memcpy (datbuf + offset, (void *)mem_pos, update_size);	/* copy update region */
			ret = CFG_NANDCMD (NULL, 0, 5, argv_write);			/* write a block */
			DBGOUT ("offset: %lx, update_size: %llx, mem_pos: %lx\n", offset, update_size, mem_pos);

			remain -= update_size;
			bstart += blk_size;
			bend = bstart + blk_size;
			mem_pos += update_size;
		}

		goto done;
	}

usage:
	return CMD_RET_USAGE;

out:
	free (datbuf);
done:
#ifdef CONFIG_NAND_RANDOMIZER
	no_nand_randomize = 0;
#endif
	return ret == 0 ? 0 : 1;
}

#ifdef CONFIG_SYS_LONGHELP
static char nand_help_text[] =
	"update_nand write - addr off|partition size\n"
	"    update 'size' bytes starting at offset 'off'\n"
	"    from memory address 'addr', skipping bad blocks.\n"
	"update_nand erase off size - erase 'size' bytes\n"
	"";
#endif

U_BOOT_CMD(
	update_nand, CONFIG_SYS_MAXARGS, 1,	do_update_nand,
	"NAND update sub-system", nand_help_text
);

