/******************************************************************************
 *
 * (C) COPYRIGHT 2008-2014 EASTWHO CO., LTD ALL RIGHTS RESERVED
 *
 * File name    : cmd_mio.c
 * Date         : 2014.08.30
 * Author       : TW.KIM (taewon@eastwho.com)
 * Abstraction  :
 * Revision     : V1.0 (2014.08.30 TW.KIM)
 *
 * Description  :
 *
 ******************************************************************************/
#include <common.h>
#include <command.h>

#include <nand_ftl.h>
#include <mio.uboot.h>

//#define DBG_CMD_MIO(fmt, args...) printf(fmt, ##args)
#define DBG_CMD_MIO(fmt, args...)


int get_number(void)
{
    int number = 0xFFFFFFFF;

    char c = 0;
    char p[16];
    unsigned char c_place = 0;

    memset((void *)p, 0, 16);

    do
    {
        if (0xFFFFFFFF != number) { break; }
        if (0xFFFFFFFE == number) { break; }

        while (!tstc()); // while no incoming data
        c = getc();

        switch (c)
        {
            case '\r':
            case '\n':
            {
                unsigned int val = 0;
                unsigned int digit = 0;
                char _c = 0;
                char * _p = p;

                puts("\r\n");

                // atoi
                while ((_c = *_p++) != '\0')
                {
                    if (('0' <= _c) && (_c <= '9'))
                    {
                        digit = _c - '0';
                    }
                    else
                    {
                        break;
                    }

                    val = (val * 10) + digit;
                }

                number = val;

            } break;

            case 0x03: // break
            {
                puts("\r\n");
                number = 0xFFFFFFFE;

            } break;

            case 0x08: // backspace
            case 0x7F: // backspace
            {
                if (c_place)
                {
                    putc('\b');
                    p[c_place] = '\0';
                    c_place -= 1;
                }

            } break;

            default:
            {
                if (('0' <= c) && (c <= '9'))
                {
                    if (c_place < 16)
                    {
                        putc(c);
                        p[c_place] = c;
                        c_place += 1;
                    }
                }

            } break;
        }

    } while (1);

    return number;
}

void do_mio_format(void)
{
    int format_type = 0xFFFFFFFF;

    printf("\n");
    printf("+------------------------------------------------------------------------------\n");
    printf("| Select NAND Format Type\n");
    printf("+------------------------------------------------------------------------------\n");
    printf("1. Intial (Erase All Blocks)\n");
    printf("2. EWS.FTL Format\n");
    printf("\n");
    printf("Select Number : ");
    format_type = get_number();

    /**************************************************************************
     * Do Format !
     **************************************************************************/
    if ((1 == format_type) || (2 == format_type))
    {
        switch (format_type)
        {
            case 1: { format_type = (1<<0); } break;
            case 2: { format_type = 0; } break;
        }

        mio_format(format_type);
    }
}

extern int mio_standby(void);
static int do_mio(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    switch (argc)
    {
        case 0:
        case 1:
        {
            return CMD_RET_USAGE;

        } break;

        case 2:
        {
            if (strncmp(argv[1], "init", 4) == 0)
            {
                struct nand_ftl *nand;
                nand = find_nand_device(0);
                nand_startup(nand);            //mio_init();
                return 0;
            }
            if (strncmp(argv[1], "info", 4) == 0)
            {
                mio_info();
                return 0;
            }
            if (strncmp(argv[1], "format", 6) == 0)
            {
                do_mio_format();
                return 0;
            }
            if (strncmp(argv[1], "standby_autosend", 16) == 0)
            {
                int enable = mio_get_autosend_standbycmd();
                printf("MIO standby_autosend is %s\n", ((enable)? "enabled.": "disabled.") );
                return 0;
            }
            if (strncmp(argv[1], "standby", 7) == 0)
            {
                DBG_CMD_MIO("MIO standby ...");
                mio_standby();
                DBG_CMD_MIO("done\n");
                return 0;
            }

        } return CMD_RET_USAGE;

        case 3:
        {
            if (strncmp(argv[1], "init", 4) == 0)
            {
                if (strncmp(argv[2], "noftl", 5) == 0)
                {
                    mio_init_without_ftl();
                }
                return 0;
            }

            if (strncmp(argv[1], "standby_autosend", 16) == 0)
            {
                int enable = (int)simple_strtoul(argv[2], NULL, 16);

                mio_set_autosend_standbycmd(enable);

                return 0;
            }

            return CMD_RET_USAGE;

        } break;

        case 4:
        {
            if (strcmp(argv[1], "nanderase") == 0)
            {
                loff_t ofs = (loff_t)simple_strtoull(argv[2], NULL, 16);
                size_t size = (size_t)simple_strtoul(argv[3], NULL, 16);
                ulong n = 0;

                DBG_CMD_MIO("MIO nanderase:ofs %llu size %u ...", ofs, size);
                n = mio_nand_erase(ofs, size);
                DBG_CMD_MIO("last block: %ld\n", n);
                return 0;
            }

            if (strcmp(argv[1], "nandrawerase") == 0)
            {
                loff_t ofs = (loff_t)simple_strtoull(argv[2], NULL, 16);
                size_t size = (size_t)simple_strtoul(argv[3], NULL, 16);
                ulong n = 0;

                DBG_CMD_MIO("MIO nandrawerase:ofs %llu size %u ...", ofs, size);
                n = mio_nand_raw_erase(ofs, size);
                DBG_CMD_MIO("last block: %ld\n", n);
                return 0;
            }

            return CMD_RET_USAGE;

        } break;

        case 5:
        {
            if (strcmp(argv[1], "read") == 0)
            {
                ulong addr = simple_strtoul(argv[2], NULL, 16);
                ulong blk = simple_strtoul(argv[3], NULL, 16);
                ulong seccnt = simple_strtoul(argv[4], NULL, 16);
                ulong n = 0;

                DBG_CMD_MIO("MIO read: block# %lu seccnt %lu addr %lXh ...", blk, seccnt, addr);
                n = mio_read(blk, seccnt, (ulong *)addr);
                DBG_CMD_MIO("%ld sectors read: %s\n", n, (n == seccnt) ? "OK" : "ERROR");
                return 0;
            }
            else if (strcmp(argv[1], "write") == 0)
            {
                ulong addr = simple_strtoul(argv[2], NULL, 16);
                ulong blk = simple_strtoul(argv[3], NULL, 16);
                ulong seccnt = simple_strtoul(argv[4], NULL, 16);
                ulong n = 0;
                DBG_CMD_MIO("MIO write: block# %lu seccnt %lu addr %lX ...", blk, seccnt, addr);
                n = mio_write(blk, seccnt, (ulong *)addr);
                DBG_CMD_MIO("%ld sectors write: %s\n", n, (n == seccnt) ? "OK" : "ERROR");
                return 0;
            }
            else if (strcmp(argv[1], "nandread") == 0)
            {
                ulong addr = simple_strtoul(argv[2], NULL, 16);
                loff_t ofs = (loff_t)simple_strtoull(argv[3], NULL, 16);
                size_t len = (size_t)simple_strtoul(argv[4], NULL, 16);
                ulong n = 0;
                DBG_CMD_MIO("MIO nandread: ofs %llu len %u addr %lX ...", ofs, len, addr);
                n = mio_nand_read(ofs, &len, (u_char *)addr);
                DBG_CMD_MIO("last block: %ld\n", n);
                return 0;
            }
            else if (strcmp(argv[1], "nandwrite") == 0)
            {
                ulong addr = simple_strtoul(argv[2], NULL, 16);
                loff_t ofs = (loff_t)simple_strtoull(argv[3], NULL, 16);
                size_t len = (size_t)simple_strtoul(argv[4], NULL, 16);
                ulong n = 0;
                DBG_CMD_MIO("MIO nandwrite: ofs %llu len %u addr %lX ...", ofs, len, addr);
                n = mio_nand_write(ofs, &len, (u_char *)addr);
                DBG_CMD_MIO("last block: %ld\n", n);
                return 0;
            }
            else if (strcmp(argv[1], "nandrawread") == 0)
            {
                ulong addr = simple_strtoul(argv[2], NULL, 16);
                loff_t ofs = (loff_t)simple_strtoull(argv[3], NULL, 16);
                size_t len = (size_t)simple_strtoul(argv[4], NULL, 16);
                ulong n = 0;
                DBG_CMD_MIO("MIO nandrawread: ofs %llu len %u addr %lX ...", ofs, len, addr);
                n = mio_nand_raw_read(ofs, &len, (u_char *)addr);
                DBG_CMD_MIO("last block: %ld\n", n);
                return 0;
            }
            else if (strcmp(argv[1], "nandrawwrite") == 0)
            {
                ulong addr = simple_strtoul(argv[2], NULL, 16);
                loff_t ofs = (loff_t)simple_strtoull(argv[3], NULL, 16);
                size_t len = (size_t)simple_strtoul(argv[4], NULL, 16);
                ulong n = 0;
                DBG_CMD_MIO("MIO nandrawwrite: ofs %llu len %u addr %lX ...", ofs, len, addr);
                n = mio_nand_raw_write(ofs, &len, (u_char *)addr);
                DBG_CMD_MIO("last block: %ld\n", n);
                return 0;
            }

            return CMD_RET_USAGE;

        } break;

        case 6:
        {
            if (strcmp(argv[1], "rwtest") == 0)
            {
                ulong sectors_to_test = simple_strtoul(argv[2], NULL, 16);
                ulong capacity = simple_strtoul(argv[3], NULL, 16);
                ulong write_ratio = simple_strtoul(argv[4], NULL, 10);
                ulong sequent_ratio = simple_strtoul(argv[5], NULL, 10);

                if (write_ratio > 100)
                    write_ratio = 100;
                if (sequent_ratio > 100)
                    sequent_ratio = 100;

                printf("MIO rwtest: %lu testsectors(%lu MB), capacity: %lu sectors(%lu MB), write_ratio:%lu%%, sequent_ratio:%lu%%\n",
                       sectors_to_test, sectors_to_test / (2 * 1024), capacity, capacity / (2 * 1024), write_ratio, sequent_ratio);

                mio_rwtest(sectors_to_test, capacity, (unsigned char)write_ratio, (unsigned char)sequent_ratio);
                return 0;
            }

            return CMD_RET_USAGE;

        } break;

        default:
        {
            return CMD_RET_USAGE;

        } break;
    }
}

U_BOOT_CMD(mio, 9, 0, do_mio,
           "MIO sub system",
           "init - init MIO sub system\n"
           "mio init noftl - init MIO sub system without FTL\n"
           "mio format - FTL format \n"
           "mio info - show available FTL\n"
           "mio rwtest testsectors capacity(sectors) write_ratio sequent_ratio\n"
           "mio read addr blk# cnt\n"
           "mio write addr blk# cnt\n"
           "mio nandwrite addr ofs len\n"
           "mio nandread addr ofs len\n"
           "mio nanderase ofs size\n"
           "mio nandrawwrite addr ofs len\n"
           "mio nandrawread addr ofs len\n"
           "mio nandrawerase ofs size\n"
           "mio standby - send a standby command to the FTL.\n"
           "mio standby_autosend <0 | 1> - enable(1)/disable(0) to auto-send standby command after every mio write.\n"
           "                               the default is 1 (enabled).")


