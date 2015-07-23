
#include <common.h>
#include <command.h>
#include <mmc.h>

#define MMC 	0
#define SD 		1

#ifndef	CONFIG_UBOOT_SIZE
#define CONFIG_UBOOT_SIZE 512 * 1024 
#endif
#ifndef	CONFIG_NSIH_SIZE
#define CONFIG_NSIH_SIZE 512
#endif
struct BOOTINFO {
    int             loadsize;
    unsigned int    loadaddr;
    unsigned int    jumpaddr;
};

static int parse_nsih(char *addr, int size)
{
	char ch;
	int writesize, skipline = 0, line, bytesize, i;
	unsigned int writeval;

	struct BOOTINFO *pinfo = NULL;
	char *base = addr;
	char  buffer[512] = { 0, };

	bytesize  = 0;
	writeval  = 0;
	writesize = 0;
	skipline  = 0;
	line = 0;

	while (1) {

		ch = *addr++;
		if (0 >= size)
			break;

		if (skipline == 0) {
			if (ch >= '0' && ch <= '9') {
				writeval  = writeval * 16 + ch - '0';
				writesize += 4;
			} else if (ch >= 'a' && ch <= 'f') {
				writeval  = writeval * 16 + ch - 'a' + 10;
				writesize += 4;
			} else if (ch >= 'A' && ch <= 'F') {
				writeval  = writeval * 16 + ch - 'A' + 10;
				writesize += 4;
			} else {
				if (writesize == 8 || writesize == 16 || writesize == 32) {
					for (i=0 ; i<writesize/8 ; i++) {
						buffer[bytesize] = (unsigned char)(writeval & 0xFF);
						bytesize++;
						writeval >>= 8;
					}
				} else {
					if (writesize != 0)
						printf("parse nsih : Error at %d line.\n", line+1);
				}

				writesize = 0;
				skipline = 1;
			}
		}

		if (ch == '\n') {
			line++;
			skipline = 0;
			writeval = 0;
		}

		size--;
	}

	pinfo = (struct BOOTINFO *)&buffer[0x44];

	pinfo->loadsize	= (int)CONFIG_UBOOT_SIZE;
	pinfo->loadaddr	= (U32)_TEXT_BASE;
	pinfo->jumpaddr = (U32)_TEXT_BASE;

	memcpy(base, buffer, sizeof(buffer));

	printf(" parse nsih : %d line processed\n", line+1);
	printf(" parse nsih : %d bytes generated.\n\n", bytesize);

	return bytesize;
}




int do_bootwrite(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *argv2[6] = {NULL,NULL,NULL,NULL,NULL,NULL};
	char buffer[32];
	unsigned int addr = 0;
	int type = 0;
	int size = 0;
	int s_addr =0, s_size =0;
	
	U8 *buf = (uchar *)malloc(16*1024);

	if(!(strcmp(argv[1],"mmc")))
	{
		type = MMC ;
	}
	else if(!(strcmp(argv[1],"sd")))
	{
		type = SD;	
	}
	else 
	{
		printf("Not suport type secondboot Write \n");
		return -1;
	}

	addr = simple_strtoul(argv[2], NULL, 16);
	size = simple_strtoul(argv[3], NULL, 16);

	s_addr = simple_strtoul(argv[4], NULL, 16);
	s_size = simple_strtoul(argv[5], NULL, 16);

	size    = parse_nsih((char*)addr, size);
	if (512 != size) {
		printf(" fail nsih parse, invalid nsih headers ...\n");
		return 1;
	}
	
	memcpy((uchar *)buf,(uchar *)addr,CONFIG_NSIH_SIZE);
	memcpy(buf+CONFIG_NSIH_SIZE, (uchar *)s_addr, s_size);



	sprintf(buffer,"0x%x",buf);
	if (type == SD)
	{
		argv2[0] = "mmc";
		argv2[1] = "write";
		argv2[2] = buffer;
		argv2[3] = "1";
		argv2[4] = "21";
	} else {
		argv2[0] = "mmc";
		argv2[1] = "bootwrite";
		argv2[2] = buffer;
		argv2[3] = "1";
		argv2[4] = "400";
	}

	do_mmcops(NULL,0,5,argv2);
	//mmc_init(mmc);
 //n = mmc->block_dev.block_write(0, 1,100, buf);


	free (buf);
	return 0;
}

U_BOOT_CMD(
	bootwrite, 6, 1, do_bootwrite,
	"Write 2nd boot sd/mmc card ",
    "bootwrite card_type NSIH addr size , 2ndboot addr size\n"
    "ex> bootwrite mmc 41000000 2000 42000000 2000\n");

