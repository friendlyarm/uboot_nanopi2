/******************************************************************************
 *
 * (C) COPYRIGHT 2008-2014 EASTWHO CO., LTD ALL RIGHTS RESERVED
 *
 * File name    : mio.uboot.c
 * Date         : 2014.08.30
 * Author       : TW.KIM (taewon@eastwho.com)
 * Abstraction  :
 * Revision     : V1.0 (2014.08.30 TW.KIM)
 *
 * Description  :
 *
 ******************************************************************************/
#define __MIO_UBOOT_GLOBAL__

/*******************************************************************************
 *
 *******************************************************************************/
#include <mio.uboot.h>

#include "media/exchange.h"
#include "mio.definition.h"

#include <common.h>
#include <malloc.h>

#include "media/nfc/phy/nfc.phy.lowapi.h"
#include "media/nfc/phy/nfc.phy.h"
#include "media/nfc/phy/nfc.phy.scan.h"
#include "media/nfc/phy/nfc.phy.readretry.h"
#include <nand_ftl.h>

#if 0
#include "mio.uboot.rwtest.h"
#endif

/******************************************************************************
 * Optimize Option
 ******************************************************************************/
#if defined (__COMPILE_MODE_BEST_DEBUGGING__)
//#pragma GCC push_options
#pragma GCC optimize("O0")
#endif

/*******************************************************************************
 *
 *******************************************************************************/
#if defined (__BUILD_MODE_X86_LINUX_DEVICE_DRIVER__)
#define __MEDIA_ON_RAM__
#elif defined (__BUILD_MODE_ARM_LINUX_DEVICE_DRIVER__)
#define __MEDIA_ON_NAND__
#elif defined (__BUILD_MODE_ARM_UBOOT_DEVICE_DRIVER__)
#define __MEDIA_ON_NAND__
#endif

/*******************************************************************************
 *
 *******************************************************************************/
//#define MEDIA_READ_WRITE_TEST

#if defined (MEDIA_READ_WRITE_TEST)
    static struct
    {
        U32 uiDataSize;

        U8 *pucWData;
        U8 *pucRData;

} gstRW;

static int mio_init_rwtest_buffer(void);
static void mio_deinit_rwtest_buffer(void);
#endif

/*******************************************************************************
 *
 *******************************************************************************/
#define __SUPPORT_DEBUG_MIO_UBOOT_ERROR_STOP__

/*******************************************************************************
 * local functions
 *******************************************************************************/
static unsigned char is_mio_init = 0;
static unsigned char is_autosend_standbycmd = 1;

static S32 mio_cmd_to_ftl(U16 usCommand, U8 ucFeature, U32 uiAddress, U32 uiLength);
static void mio_fill_read_buffer(void *pvBuff, U32 uiSectors);
static void mio_fill_write_cache(const void *pvBuff, U32 uiSectors);

/*******************************************************************************
 * functions
 *******************************************************************************/

/*******************************************************************************
 *
 *******************************************************************************/
int mio_format(int _format_type)
{
#if defined (__MEDIA_ON_NAND__)

    int resp = -1;
    int capacity = -1;

    mio_deinit();

    /**************************************************************************
     * MIO Debug Options
     **************************************************************************/
  //Exchange.debug.misc.block_thread = 1;
  //Exchange.debug.misc.block_transaction = 1;
  //Exchange.debug.misc.block_background = 1;
  //Exchange.debug.misc.media_open = 1;
  //Exchange.debug.misc.media_format = 1;
  //Exchange.debug.misc.media_close = 1;
  //Exchange.debug.misc.media_rw_memcpy = 1;
  //Exchange.debug.misc.smart_store = 1;
    Exchange.debug.misc.uboot_format = 1;
  //Exchange.debug.misc.uboot_init = 1;

    Exchange.debug.ftl.format = 1;
    Exchange.debug.ftl.format_progress = 1;
    Exchange.debug.ftl.configurations = 1;
    Exchange.debug.ftl.open = 1;
    Exchange.debug.ftl.memory_usage = 1;
    Exchange.debug.ftl.boot = 1;
    Exchange.debug.ftl.block_summary = 1;
    Exchange.debug.ftl.license_detail = 0;
    Exchange.debug.ftl.warn = 1;
    Exchange.debug.ftl.error = 1;

  //Exchange.debug.nfc.sche.operation = 1;

  //Exchange.debug.nfc.phy.operation = 1;
  //Exchange.debug.nfc.phy.info_feature = 1;
  //Exchange.debug.nfc.phy.info_ecc = 1;
  //Exchange.debug.nfc.phy.info_ecc_correction = 1;
  //Exchange.debug.nfc.phy.info_ecc_corrected = 1;
  //Exchange.debug.nfc.phy.info_randomizer = 1;
  //Exchange.debug.nfc.phy.info_readretry = 1;
  //Exchange.debug.nfc.phy.info_readretry_table = 1;
  //Exchange.debug.nfc.phy.info_readretry_otp_table = 1;
  //Exchange.debug.nfc.phy.info_lowapi = 1;
    Exchange.debug.nfc.phy.warn_prohibited_block_access = 1;
  //Exchange.debug.nfc.phy.warn_ecc_uncorrectable = 1;
  //Exchange.debug.nfc.phy.warn_ecc_uncorrectable_show = 1;
    Exchange.debug.nfc.phy.err_ecc_uncorrectable = 1;

    /**************************************************************************
     * MIO Exchange Init
     **************************************************************************/
    if (Exchange.debug.misc.uboot_format) { printf("MIO.FORMAT: EXCHANGE_init()\n"); }
    EXCHANGE_init();

    /**************************************************************************
     * FTL Need Leaner Buffer
     **************************************************************************/
    if (Exchange.debug.misc.uboot_format) { Exchange.sys.fn.print("MIO.FORMAT: Memory Pool Pre-Allocation\n"); }

    Exchange.buffer.mpool_size  = 0;
    Exchange.buffer.mpool_size += 1 * 2 * (4<<20); // 1CH x 2WAY x 4MB (Page Map Table per Lun)
    Exchange.buffer.mpool_size += 1 * 2 * (1<<20); // 1CH x 2WAY x 1MB (Update Map Table per Lun)
    Exchange.buffer.mpool_size += (1<<20);         // 1MB (Misc)
    Exchange.buffer.mpool = (unsigned char *)malloc(Exchange.buffer.mpool_size);

    if (!Exchange.buffer.mpool)
    {
        Exchange.sys.fn.print("MIO.FORMAT: Memory Pool Pre-Allocation Fail\n");
        return -1;
    }

    /**************************************************************************
     * FTL Format
     **************************************************************************/
    if (Exchange.debug.misc.uboot_format) { Exchange.sys.fn.print("MIO.FORMAT: Exchange.ftl.fnFormat()\n"); }
    if ((resp = Exchange.ftl.fnFormat((unsigned char *)CHIP_NAME, CHIP_ID_PHY_BASE, (unsigned char)_format_type)) < 0)
    {
        Exchange.sys.fn.print("MIO.FORMAT: Exchange.ftl.fnFormat() Fail\n");
    }

    capacity = *Exchange.ftl.Capacity;
    Exchange.sys.fn.print("MIO.FORMAT: Capacity %xh(%d) Sectors = %d MB\n", capacity, capacity, ((capacity>>10)<<9)>>10);

    is_mio_init = 1;

    /**************************************************************************
     *
     **************************************************************************/
    mio_deinit();

    if (resp < 0)
    {
        return -1;
    }
#endif

    return 0;
}

int mio_init(void)
{
#if defined (__MEDIA_ON_NAND__)
    struct nand_ftl * nx_nand;

    int resp = -1;
    int capacity = -1;

    mio_deinit();

    /**************************************************************************
     * MIO Debug Options
     **************************************************************************/
  //Exchange.debug.misc.block_thread = 1;
  //Exchange.debug.misc.block_transaction = 1;
  //Exchange.debug.misc.block_background = 1;
  //Exchange.debug.misc.media_open = 1;
  //Exchange.debug.misc.media_format = 1;
  //Exchange.debug.misc.media_close = 1;
  //Exchange.debug.misc.media_rw_memcpy = 1;
  //Exchange.debug.misc.smart_store = 1;
  //Exchange.debug.misc.uboot_format = 1;
  //Exchange.debug.misc.uboot_init = 1;

    Exchange.debug.ftl.format = 1;
    Exchange.debug.ftl.format_progress = 1;
    Exchange.debug.ftl.configurations = 1;
    Exchange.debug.ftl.open = 1;
    Exchange.debug.ftl.memory_usage = 1;
    Exchange.debug.ftl.boot = 1;
    Exchange.debug.ftl.block_summary = 1;
    Exchange.debug.ftl.license_detail = 0;
    Exchange.debug.ftl.warn = 1;
    Exchange.debug.ftl.error = 1;

  //Exchange.debug.nfc.sche.operation = 1;

  //Exchange.debug.nfc.phy.operation = 1;
  //Exchange.debug.nfc.phy.info_feature = 1;
  //Exchange.debug.nfc.phy.info_ecc = 1;
  //Exchange.debug.nfc.phy.info_ecc_correction = 1;
  //Exchange.debug.nfc.phy.info_ecc_corrected = 1;
  //Exchange.debug.nfc.phy.info_randomizer = 1;
  //Exchange.debug.nfc.phy.info_readretry = 1;
  //Exchange.debug.nfc.phy.info_readretry_table = 1;
  //Exchange.debug.nfc.phy.info_readretry_otp_table = 1;
  //Exchange.debug.nfc.phy.info_lowapi = 1;
    Exchange.debug.nfc.phy.warn_prohibited_block_access = 1;
  //Exchange.debug.nfc.phy.warn_ecc_uncorrectable = 1;
  //Exchange.debug.nfc.phy.warn_ecc_uncorrectable_show = 1;
    Exchange.debug.nfc.phy.err_ecc_uncorrectable = 1;

    /**************************************************************************
     * MIO Exchange Init
     **************************************************************************/
    if (Exchange.debug.misc.uboot_init) { printf("MIO.INIT: EXCHANGE_init()\n"); }
    EXCHANGE_init();

    /**************************************************************************
     * FTL Need Leaner Buffer
     **************************************************************************/
    Exchange.buffer.mpool_size  = 0;
    Exchange.buffer.mpool_size += 1 * 2 * (4<<20); // 1CH x 4WAY x 4MB (Page Map Table per Lun)
    Exchange.buffer.mpool_size += 1 * 2 * (1<<20); // 1CH x 4WAY x 1MB (Update Map Table per Lun)
    Exchange.buffer.mpool_size += (1<<20);         // 1MB (Misc)
    Exchange.buffer.mpool = (unsigned char *)malloc(Exchange.buffer.mpool_size);

    if (!Exchange.buffer.mpool)
    {
        Exchange.sys.fn.print("MIO.INIT: Memory Pool Pre-Allocation Fail\n");
        return -1;
    }

    /**************************************************************************
     * FTL Open & Boot
     **************************************************************************/
    do 
    {
        if (Exchange.debug.misc.uboot_init) { Exchange.sys.fn.print("MIO.INIT: Exchange.ftl.fnOpen()\n"); }
        if ((resp = Exchange.ftl.fnOpen((unsigned char *)CHIP_NAME, CHIP_ID_PHY_BASE, 0)) < 0)
        {
            Exchange.sys.fn.print("MIO.INIT: Exchange.ftl.fnOpen() Fail\n");
            break;
        }

        if (Exchange.debug.misc.uboot_init) { Exchange.sys.fn.print("MIO.INIT: Exchange.ftl.fnBoot()\n"); }
        if ((resp = Exchange.ftl.fnBoot(0)) < 0)
        {
            Exchange.sys.fn.print("MIO.INIT: Exchange.ftl.fnBoot() Fail\n");
            break;
        }

    } while (0);

    if (resp < 0)
    {
        mio_deinit();
        return -1;
    }

    capacity = *Exchange.ftl.Capacity;
    Exchange.sys.fn.print("MIO.INIT: Capacity %xh(%d) Sectors = %d MB\n", capacity, capacity, ((capacity>>10)<<9)>>10);
    is_mio_init = 1;

    Exchange.sys.fn.print("MIO.INIT.WriteCacheBase: 0x%0lx, Sectors: 0x%0x\n", (unsigned long)(*Exchange.buffer.BaseOfWriteCache), (U32)(*Exchange.buffer.SectorsOfWriteCache));
    Exchange.sys.fn.print("MIO.INIT.ReadBufferBase: 0x%0lx, Sectors: 0x%0x\n", (unsigned long)(*Exchange.buffer.BaseOfReadBuffer), (U32)(*Exchange.buffer.SectorsOfReadBuffer));

    NFC_PHY_LOWAPI_init();

    /* change ftl status */
    nx_nand = find_nand_device(0);
    if (nx_nand)
        nx_nand->ftl_status = 0;

#if defined (MEDIA_READ_WRITE_TEST)
    /**************************************************************************
     * Memory Allocations : Read/Write Test
     **************************************************************************/
     mio_init_rwtest_buffer();
#endif
#endif
    return 0;
}

/*******************************************************************************
 *
 *******************************************************************************/
int mio_deinit(void)
{
#if defined (__MEDIA_ON_NAND__)
    if (Exchange.ftl.fnClose)
    {
        Exchange.ftl.fnClose();
    }

    if (is_mio_init)
    {
        struct nand_ftl *nx_nand;

        is_mio_init = 0;

        NFC_PHY_LOWAPI_deinit();

        /* change ftl status */
        nx_nand = find_nand_device(0);
        if (nx_nand)
            nx_nand->ftl_status = 0;

#if defined (MEDIA_READ_WRITE_TEST)
        mio_deinit_rwtest_buffer();
#endif
    }

    if (Exchange.buffer.mpool)
    {
        free(Exchange.buffer.mpool);
        Exchange.buffer.mpool = (unsigned char *)0;
        Exchange.buffer.mpool_size = 0;
    }
#endif

    return 0;
}

#if defined (MEDIA_READ_WRITE_TEST)
/*******************************************************************************
 *
 *******************************************************************************/
int mio_init_rwtest_buffer(void)
{
    if (!gstRW.uiDataSize)
    {
        U32 i = 0;
        U32 randValue = 0;
        U32 ui_ofs = 0;

        gstRW.uiDataSize = 10 * 1024 * 1024;

        gstRW.pucWData = (U8 *)malloc(gstRW.uiDataSize);
        gstRW.pucRData = (U8 *)malloc(gstRW.uiDataSize);

        if (!gstRW.pucWData || !gstRW.pucRData)
        {
            Exchange.sys.fn.print("RW data buffer alloc failed!\n");

            if (gstRW.pucWData)
                free(gstRW.pucWData);

            if (gstRW.pucRData)
                free(gstRW.pucRData);

            return -1;
        }

        for (i = 0; i < (gstRW.uiDataSize / 4); i++)
        {
            ui_ofs = i % (512/4);

            if (!ui_ofs)
            {
                randValue = 0xAAAA5555;     //(U32)rand();
            }
        
            if (ui_ofs < 8)
            {
                ((U32 *)gstRW.pucWData)[i] = i/(512/4);
            }
            else
            {
                ((U32 *)gstRW.pucWData)[i] = randValue;
            }
        }

        printf("WriteBuff: 0x%0X, ReadBuff: 0x%0X\n", (U32)gstRW.pucWData, (U32)gstRW.pucRData);
    }

    return 0;
}

/*******************************************************************************
 *
 *******************************************************************************/
void mio_deinit_rwtest_buffer(void)
{
    if (gstRW.uiDataSize)
    {
        if (gstRW.pucWData)
            free(gstRW.pucWData);

        if (gstRW.pucRData)
            free(gstRW.pucRData);

        gstRW.uiDataSize = 0;
    }
}
#endif

/*******************************************************************************
 *
 *******************************************************************************/
int mio_info(void)
{
    NAND * nand = (NAND *)&phy_features.nand_config;

    printf("\n NAND INFORMATION");

    printf("\n");
    printf("*******************************************************************************\n");
    printf("* NAND Configuration Summary\n");
    printf("*\n");
    printf("* - Manufacturer : %s\n", nand->_f.manufacturer);
    printf("* - Model Name : %s\n", nand->_f.modelname);
    printf("* - Generation : %s\n", nand->_f.generation);

    printf("*\n");
    printf("* - Interfacetype : %d\n", nand->_f.interfacetype);
    printf("* - ONFI Detected : %d\n", nand->_f.onfi_detected);
    printf("* - ONFI Timing Mode : %d\n", nand->_f.onfi_timing_mode);

    printf("*\n");
    printf("* - tClk : %d\n", nand->_f.timing.async.tClk);
    printf("* - tRWC : %d\n", nand->_f.timing.async.tRWC);
    printf("* - tR : %d\n", nand->_f.timing.async.tR);
    printf("* - tWB : %d\n", nand->_f.timing.async.tWB);
    printf("* - tCCS : %d\n", nand->_f.timing.async.tCCS);
    printf("* - tADL : %d\n", nand->_f.timing.async.tADL);
    printf("* - tRHW : %d\n", nand->_f.timing.async.tRHW);
    printf("* - tWHR : %d\n", nand->_f.timing.async.tWHR);
    printf("* - tWW : %d\n", nand->_f.timing.async.tWW);

    printf("*\n");
    printf("* - tCS : %d\n", nand->_f.timing.async.tCS);
    printf("* - tCH : %d\n", nand->_f.timing.async.tCH);
    printf("* - tCLS : %d\n", nand->_f.timing.async.tCLS);
    printf("* - tALS : %d\n", nand->_f.timing.async.tALS);
    printf("* - tCLH : %d\n", nand->_f.timing.async.tCLH);
    printf("* - tALH : %d\n", nand->_f.timing.async.tALH);
    printf("* - tWP : %d\n", nand->_f.timing.async.tWP);
    printf("* - tWH : %d\n", nand->_f.timing.async.tWH);
    printf("* - tWC : %d\n", nand->_f.timing.async.tWC);
    printf("* - tDS : %d\n", nand->_f.timing.async.tDS);
    printf("* - tDH : %d\n", nand->_f.timing.async.tDH);
    printf("* - tCEA : %d\n", nand->_f.timing.async.tCEA);
    printf("* - tREA : %d\n", nand->_f.timing.async.tREA);
    printf("* - tRP : %d\n", nand->_f.timing.async.tRP);
    printf("* - tREH : %d\n", nand->_f.timing.async.tREH);
    printf("* - tRC : %d\n", nand->_f.timing.async.tRC);
    printf("* - tCOH : %d\n", nand->_f.timing.async.tCOH);

    printf("*\n");
    printf("* - Luns Per Ce : %d\n", nand->_f.luns_per_ce);
    printf("* - Databytes Per Page : %d\n", nand->_f.databytes_per_page);
    printf("* - Sparebytes Per Page : %d\n", nand->_f.sparebytes_per_page);
    printf("* - Number Of Planes : %d\n", nand->_f.number_of_planes);
    printf("* - Pages Per Block : %d\n", nand->_f.pages_per_block);
    printf("* - Mainblocks Per Lun : %d\n", nand->_f.mainblocks_per_lun);
    printf("* - Extendedblocks Per Lun : %d\n", nand->_f.extendedblocks_per_lun);
    printf("* - Next Lun Address : %d\n", nand->_f.next_lun_address);
    printf("* - Over Provisioning : %d\n", nand->_f.over_provisioning);
    printf("* - Bits Per Cell : %d\n", nand->_f.bits_per_cell);
    printf("* - Number Of Bits Ecc Correctability : %d\n", nand->_f.number_of_bits_ecc_correctability);
    printf("* - Maindatabytes Per Eccunit : %d\n", nand->_f.maindatabytes_per_eccunit);
    printf("* - Eccbits Per Maindata : %d\n", nand->_f.eccbits_per_maindata);
    printf("* - Eccbits Per Blockinformation : %d\n", nand->_f.eccbits_per_blockinformation);
    printf("* - Block Endurance : %d\n", nand->_f.block_endurance);
    printf("* - Factorybadblocks Per Nand : %d\n", nand->_f.factorybadblocks_per_nand);

    printf("*\n");
    printf("* - Randomize : %d\n", nand->_f.support_list.randomize);

    printf("*\n");
    printf("* - Multiplane Read %d\n", nand->_f.support_type.multiplane_read);
    printf("* - Multiplane Write %d\n", nand->_f.support_type.multiplane_write);
    printf("* - Cache Read %d\n", nand->_f.support_type.cache_read);
    printf("* - Cache Write %d\n", nand->_f.support_type.cache_write);
    printf("* - Interleave %d\n", nand->_f.support_type.interleave);
    printf("* - Paired Page Mapping %d\n", nand->_f.support_type.paired_page_mapping);
    printf("* - Block Indicator %d\n", nand->_f.support_type.block_indicator);
    printf("* - Paired Plane %d\n", nand->_f.support_type.paired_plane);
    printf("* - Multiplane Erase %d\n", nand->_f.support_type.multiplane_erase);
    printf("* - Read Retry %d\n", nand->_f.support_type.read_retry);
    printf("*\n");

    printf("*******************************************************************************\n");
    printf("\n");

    return 0;
}

/*******************************************************************************
 * get_mio_capacity()
 *******************************************************************************/
int get_mio_capacity(void)
{
    int resp = 0;
    NAND nand;
    struct nand_ftl *nx_nand;

    if (!is_mio_init)
    {
        Exchange.sys.fn.print("get_mio_capacity(): mio is not initialized!!\n");
        return 0;
    }

    resp = Exchange.ftl.fnGetNandInfo(&nand);

    if (resp < 0)
    {
        Exchange.sys.fn.print("get_mio_capacity(): failed to get NAND information.\n");
        return 0;
    }


    /* fill nand_ftl info : capacity */
    nx_nand = find_nand_device(0);
    if (nx_nand) {
        nx_nand->capacity = *Exchange.ftl.Capacity;
        printf("found device. Capacity is %x\n", nx_nand->capacity);
    }

    return 1;
}


/*******************************************************************************
 * mio_read()
 *******************************************************************************/
ulong mio_read(ulong blknr, lbaint_t blkcnt, void *pvBuffer)
{
    S32 siStartExtIndex = -1;
    U32 uiAddress = blknr;
    U8 *pucBuffer = (U8 *)pvBuffer;

    struct 
    {
        U32 uiSectorsLeft;
        U32 uiPartialSectors;
        U32 uiPartialAddr;
    } stFtl;

    struct
    {
        U32 uiSectorsLeft;
        U32 uiPartialSectors;
        U32 uiReadSectors;
    } stData;

    if (!is_mio_init)
    {
        Exchange.sys.fn.print("mio_read(): mio is not initialized!!\n");
        return 0;
    }

    stData.uiSectorsLeft = blkcnt;
    stFtl.uiSectorsLeft = blkcnt;
    stFtl.uiPartialAddr = uiAddress;

  //Exchange.sys.fn.print("mio_read(): 0x%0X, 0x%0X\n", (U32)blknr, (U32)blkcnt);

    while (stFtl.uiSectorsLeft || stData.uiSectorsLeft)
    {
        Exchange.ftl.fnMain();

        // put command to FTL 
        while(stFtl.uiSectorsLeft)
        {
            if (stFtl.uiSectorsLeft > IO_CMD_MAX_READ_SECTORS)
                stFtl.uiPartialSectors = IO_CMD_MAX_READ_SECTORS;
            else
                stFtl.uiPartialSectors = stFtl.uiSectorsLeft;

            siStartExtIndex = mio_cmd_to_ftl(IO_CMD_READ, 0, stFtl.uiPartialAddr, stFtl.uiPartialSectors);
            if (siStartExtIndex >= 0)
            {
              //Exchange.sys.fn.print("mio_cmd_to_ftl(READ): 0x%0X, 0x%0X\n", stFtl.uiPartialAddr, stFtl.uiPartialSectors);
            
                if (uiAddress == stFtl.uiPartialAddr)
                {
                    // set external index
                    *Exchange.buffer.ReadBlkIdx = (U32)siStartExtIndex;
                }

                stFtl.uiPartialAddr += stFtl.uiPartialSectors;
                stFtl.uiSectorsLeft -= stFtl.uiPartialSectors;
            }
            else
            {
                break;
            }
        }

        // copy data from read buffer
        if (uiAddress != stFtl.uiPartialAddr)
        {
            stData.uiReadSectors = Exchange.buffer.fnGetRequestReadSeccnt();
            if (stData.uiReadSectors && stData.uiSectorsLeft)
            {
                if (stData.uiSectorsLeft > stData.uiReadSectors)
                    stData.uiPartialSectors = stData.uiReadSectors;
                else
                    stData.uiPartialSectors = stData.uiSectorsLeft;

                mio_fill_read_buffer(pucBuffer, stData.uiPartialSectors);
              //Exchange.sys.fn.print("mio_fill_read_buffer: 0x%0X, 0x%0X\n", (U32)pucBuffer, stData.uiPartialSectors);

                pucBuffer += (stData.uiPartialSectors << 9);
                stData.uiSectorsLeft -= stData.uiPartialSectors;
                
                // increment external index
                *Exchange.buffer.ReadBlkIdx += stData.uiPartialSectors;
            }
        }
    }

    return (blkcnt - stData.uiSectorsLeft);
}

/*******************************************************************************
 * mio_write()
 *******************************************************************************/
ulong mio_write(ulong blknr, lbaint_t blkcnt, const void *pvBuffer)
{
    S32 siStartExtIndex = -1;
    U32 uiAddress = blknr;
    U8 *pucBuffer = (U8 *)pvBuffer;

    struct 
    {
        U32 uiSectorsLeft;
        U32 uiPartialSectors;
        U32 uiPartialAddr;
        U32 uiFeature;
    } stFtl;

    struct
    {
        U32 uiSectorsLeft;
        U32 uiPartialSectors;
        U32 uiAvaliableSectors;
    } stData;

    if (!is_mio_init)
    {
        Exchange.sys.fn.print("mio_write(): mio is not initialized!!\n");
        return 0;
    }

  //Exchange.sys.fn.print("mio_write(): 0x%lX, 0x%lX\n", blknr, blkcnt);

    stData.uiSectorsLeft = blkcnt;
    stFtl.uiSectorsLeft = blkcnt;
    stFtl.uiPartialAddr = uiAddress;

    while (stFtl.uiSectorsLeft || stData.uiSectorsLeft)
    {
        Exchange.ftl.fnMain();

        // put command to FTL
        while(stFtl.uiSectorsLeft)
        {
            if (stFtl.uiSectorsLeft > IO_CMD_MAX_WRITE_SECTORS)
            {
                stFtl.uiPartialSectors = IO_CMD_MAX_WRITE_SECTORS;
                stFtl.uiFeature = IO_CMD_FEATURE_WRITE_CONTINUE;
            }            
            else
            {
                stFtl.uiPartialSectors = stFtl.uiSectorsLeft;
                stFtl.uiFeature = 0;
            }

            siStartExtIndex = mio_cmd_to_ftl(IO_CMD_WRITE, stFtl.uiFeature, stFtl.uiPartialAddr, stFtl.uiPartialSectors);
            if (siStartExtIndex >= 0)
            {
              //Exchange.sys.fn.print("mio_cmd_to_ftl(WRITE): 0x%0X, 0x%0X\n", stFtl.uiPartialAddr, stFtl.uiPartialSectors);
                if (uiAddress == stFtl.uiPartialAddr)
                {
                    // set external index
                    *Exchange.buffer.WriteBlkIdx = (U32)siStartExtIndex;
                }

                stFtl.uiPartialAddr += stFtl.uiPartialSectors;
                stFtl.uiSectorsLeft -= stFtl.uiPartialSectors;
            }
            else
            {
                break;
            }
        }

        // copy data to write cache
        if ((uiAddress != stFtl.uiPartialAddr) && stData.uiSectorsLeft)
        {
            stData.uiAvaliableSectors = (*Exchange.buffer.WriteBlkIdx - *Exchange.buffer.WriteNfcIdx) & (*Exchange.buffer.SectorsOfWriteCache - 1);
            stData.uiAvaliableSectors += 1;
            stData.uiAvaliableSectors = *Exchange.buffer.SectorsOfWriteCache - stData.uiAvaliableSectors;

          //Exchange.sys.fn.print("Index: EXT(0x%0X), NAND(0x%0X), Aval:0x%0X\n", *Exchange.buffer.WriteBlkIdx, *Exchange.buffer.WriteNfcIdx, stData.uiAvaliableSectors);
            if (stData.uiAvaliableSectors)
            {
                if (stData.uiSectorsLeft > stData.uiAvaliableSectors)
                    stData.uiPartialSectors = stData.uiAvaliableSectors;
                else
                    stData.uiPartialSectors = stData.uiSectorsLeft;

                mio_fill_write_cache(pucBuffer, stData.uiPartialSectors);
              //Exchange.sys.fn.print("mio_fill_write_cache: 0x%0X, 0x%0X\n", (U32)pucBuffer, stData.uiPartialSectors);

                pucBuffer += (stData.uiPartialSectors << 9);
                stData.uiSectorsLeft -= stData.uiPartialSectors;

                // increment external index
                *Exchange.buffer.WriteBlkIdx += stData.uiPartialSectors;
            }
        }
    }

    if (is_autosend_standbycmd)
    {
        mio_standby();
    }

    return (stFtl.uiPartialAddr - uiAddress);
}

/*******************************************************************************
 * enable/disable to autosend standby command after each mio write.
 *******************************************************************************/
void mio_set_autosend_standbycmd(int enable)
{
    is_autosend_standbycmd = (enable)? 1: 0;
}

int mio_get_autosend_standbycmd(void)
{
    return is_autosend_standbycmd;
}

/*******************************************************************************
 * flush, standby, powerdown
 *******************************************************************************/
int mio_flush(void)
{
    if (mio_cmd_to_ftl(IO_CMD_FLUSH, 0, 0, 0) < 0)
    {
        return -1;
    }
    return 0;
}

int mio_standby(void)
{
    if (mio_cmd_to_ftl(IO_CMD_STANDBY, 0, 0, 0) < 0)
    {
        return -1;
    }
    return 0;
}

int mio_powerdown(void)
{
    if (mio_cmd_to_ftl(IO_CMD_POWER_DOWN, 0, 0, 0) < 0)
    {
        return -1;
    }
    return 0;
}

/*******************************************************************************
 * low level api.
 *******************************************************************************/
int mio_boost_time_regval(ulong tacs, ulong tcos, ulong tacc, ulong tcoh, ulong tcah)
{
	NF_TIME_REGS _t = { tacs, tcos, tacc, tcoh, tcah };

	NFC_PHY_Boost_time_regval(_t);

	return 0;
}

int mio_force_origin_time_regval(ulong tacs, ulong tcos, ulong tacc, ulong tcoh, ulong tcah)
{
	NF_TIME_REGS _t = { tacs, tcos, tacc, tcoh, tcah };

	NFC_PHY_Origin_time_regval(_t);
	NFC_PHY_ForceSet_Nftime(_t);

	return 0;
}

int mio_init_without_ftl(void)
{
    int ret = 0;

    /**************************************************************************
     * MIO Debug Options
     **************************************************************************/
  //Exchange.debug.misc.block_thread = 1;
  //Exchange.debug.misc.block_transaction = 1;
  //Exchange.debug.misc.block_background = 1;
  //Exchange.debug.misc.media_open = 1;
  //Exchange.debug.misc.media_format = 1;
  //Exchange.debug.misc.media_close = 1;
  //Exchange.debug.misc.smart_store = 1;
  //Exchange.debug.misc.uboot_format = 1;
    Exchange.debug.misc.uboot_init = 1;

    Exchange.debug.ftl.format = 1;
    Exchange.debug.ftl.format_progress = 1;
    Exchange.debug.ftl.configurations = 1;
    Exchange.debug.ftl.open = 1;
    Exchange.debug.ftl.memory_usage = 1;
    Exchange.debug.ftl.boot = 1;
    Exchange.debug.ftl.block_summary = 1;
    Exchange.debug.ftl.warn = 1;
    Exchange.debug.ftl.error = 1;

  //Exchange.debug.nfc.sche.operation = 1;

  //Exchange.debug.nfc.phy.operation = 1;
    Exchange.debug.nfc.phy.info_feature = 1;
  //Exchange.debug.nfc.phy.info_ecc = 1;
  //Exchange.debug.nfc.phy.info_ecc_correction = 1;
  //Exchange.debug.nfc.phy.info_ecc_corrected = 1;
  //Exchange.debug.nfc.phy.info_randomizer = 1;
  //Exchange.debug.nfc.phy.info_readretry = 1;
  //Exchange.debug.nfc.phy.info_readretry_table = 1;
  //Exchange.debug.nfc.phy.info_readretry_otp_table = 1;
    Exchange.debug.nfc.phy.warn_prohibited_block_access = 1;
  //Exchange.debug.nfc.phy.warn_ecc_uncorrectable = 1;
  //Exchange.debug.nfc.phy.warn_ecc_uncorrectable_show = 1;
    Exchange.debug.nfc.phy.err_ecc_uncorrectable = 1;

    ret = NFC_PHY_LOWAPI_init();

#if defined (MEDIA_READ_WRITE_TEST)
    /**************************************************************************
     * Memory Allocations : Read/Write Test
     **************************************************************************/
     mio_init_rwtest_buffer();
#endif

    return ret;
}

int mio_deinit_without_ftl(void)
{
    NFC_PHY_LOWAPI_deinit();

#if defined (MEDIA_READ_WRITE_TEST)
    mio_deinit_rwtest_buffer();
#endif

    return 0;
}

int mio_nand_write(loff_t ofs, size_t *len, u_char *buf)
{
    int ret = 0;
    unsigned char enable_ecc = 1;

    if (!NFC_PHY_LOWAPI_is_init())
    {
        Exchange.sys.fn.print("error! NFC_PHY_LOWAPI is not initialized!\n");
        return -1;
    }

    ret = NFC_PHY_LOWAPI_nand_write(ofs, len, buf, enable_ecc);

    return ret;
}

int mio_nand_read(loff_t ofs, size_t *len, u_char *buf)
{
    int ret = 0;
    unsigned char enable_ecc = 1;

    if (!NFC_PHY_LOWAPI_is_init())
    {
        Exchange.sys.fn.print("error! NFC_PHY_LOWAPI is not initialized!\n");
        return -1;
    }

    ret = NFC_PHY_LOWAPI_nand_read(ofs, len, buf, enable_ecc);

    return ret;
}

int mio_nand_erase(loff_t ofs, size_t size)
{
    int ret = 0;

    if (!NFC_PHY_LOWAPI_is_init())
    {
        Exchange.sys.fn.print("error! NFC_PHY_LOWAPI is not initialized!\n");
        return -1;
    }

    ret = NFC_PHY_LOWAPI_nand_erase(ofs, size);

    return ret;
}

int mio_nand_raw_write(loff_t ofs, size_t *len, u_char *buf)
{
    int ret = 0;
    unsigned char enable_ecc = 0;

    if (!NFC_PHY_LOWAPI_is_init())
    {
        Exchange.sys.fn.print("error! NFC_PHY_LOWAPI is not initialized!\n");
        return -1;
    }

    ret = NFC_PHY_LOWAPI_nand_write(ofs, len, buf, enable_ecc);

    return ret;
}

#if 0
int mio_nand_raw_read(loff_t ofs, size_t *len, u_char *buf)
{
    int ret = 0;
    MIO_NAND_RAW_INFO info;

	info.channel = 0;
	info.phyway = 0;
	info.pages_per_block = 256;
	info.bytes_per_page = 8192;
	info.blocks_per_lun = 2048;

    /*******************************************************************************
     * NFC_PHY_LOWAPI_nand_raw_read() function has no prerequisite including 
     * the NFC_PHY_LOWAPI_init() function.
     *******************************************************************************/
    ret = NFC_PHY_LOWAPI_nand_raw_read(&info, ofs, len, buf);

    return ret;
}
#else
int mio_nand_raw_read(loff_t ofs, size_t *len, u_char *buf)
{
    int ret = 0;
    unsigned char enable_ecc = 0;

    if (!NFC_PHY_LOWAPI_is_init())
    {
        Exchange.sys.fn.print("error! NFC_PHY_LOWAPI is not initialized!\n");
        return -1;
    }

    ret = NFC_PHY_LOWAPI_nand_read(ofs, len, buf, enable_ecc);

    return ret;
}
#endif

int mio_nand_raw_erase(loff_t ofs, size_t size)
{
    return mio_nand_erase(ofs, size);
}

/*******************************************************************************
 * local functions
 *******************************************************************************/
static S32 mio_cmd_to_ftl(U16 usCommand, U8 ucFeature, U32 uiAddress, U32 uiLength)
{
    S32 siResp = -1;
    S32 siExtIndex = -1;
    U8 ucIsNeedRetry = 0;
    U8 ucIsWaitForDone = 0;

    switch (usCommand)
    {
        case IO_CMD_READ_DIRECT:
        case IO_CMD_WRITE_DIRECT:
        case IO_CMD_DATA_SET_MANAGEMENT:
        case IO_CMD_FLUSH:
        case IO_CMD_STANDBY:
        case IO_CMD_SWITCH_PARTITION:
        case IO_CMD_POWER_DOWN:
        {
            ucIsNeedRetry = 1;
            ucIsWaitForDone = 1;
        } break;
    }

    do
    {
        siResp = Exchange.ftl.fnPrePutCommand(usCommand, ucFeature, uiAddress, uiLength);
        if (siResp >= 0)
        {   siExtIndex = siResp;
            siResp = Exchange.ftl.fnPutCommand(usCommand, ucFeature, uiAddress, uiLength);
            if (siResp >= 0)
            {
                ucIsNeedRetry = 0;
            }
        }

        if (ucIsNeedRetry)
        {
            Exchange.ftl.fnMain();
        }
    } while (ucIsNeedRetry);

    // wait for done
    if ((siResp >= 0) && ucIsWaitForDone)
    {
        while(!Exchange.ftl.fnIsIdle())
        {
            Exchange.ftl.fnMain();
        }
    }

    return siExtIndex;
}

static void mio_fill_read_buffer(void *pvBuff, U32 uiSectors)
{
    U8 *pucSrcBuff = 0;
    U8 *pucDestBuff = (U8 *)pvBuff;
    U32 uiCpyBytes = 0;
    U32 uiCurrExtIndex = *Exchange.buffer.ReadBlkIdx & (*Exchange.buffer.SectorsOfReadBuffer - 1);

    if ((uiCurrExtIndex + uiSectors) > *Exchange.buffer.SectorsOfReadBuffer)
    { // is rollover
        pucSrcBuff = (U8 *)(*Exchange.buffer.BaseOfReadBuffer + (uiCurrExtIndex << 9));
        uiCpyBytes = (*Exchange.buffer.SectorsOfReadBuffer - uiCurrExtIndex) << 9;
        memcpy((void *)pucDestBuff, (const void *)pucSrcBuff, uiCpyBytes);
        if (Exchange.debug.misc.uboot_rw_memcpy)
        {
            if (sizeof(unsigned long) == 8) { Exchange.sys.fn.print("memcpy(Dest:%016lx Src:%016lx bytes:%08x)\n", pucDestBuff, pucSrcBuff, (U32)uiCpyBytes); }
            else                            { Exchange.sys.fn.print("memcpy(Dest:%08x Src:%08x bytes:%08x)\n", pucDestBuff, pucSrcBuff, (U32)uiCpyBytes); }
        }
#if defined (__SUPPORT_DEBUG_MIO_UBOOT_ERROR_STOP__)
        if (!uiCpyBytes) { Exchange.sys.fn.print("mio_fill_read_buffer: memcpy error (copybytes 0)\n"); while(1); }
#endif

        pucDestBuff += uiCpyBytes;
        pucSrcBuff = (U8 *)(*Exchange.buffer.BaseOfReadBuffer);
        uiCpyBytes = (uiSectors << 9) - uiCpyBytes;
        memcpy((void *)pucDestBuff, (const void *)pucSrcBuff, uiCpyBytes);
        if (Exchange.debug.misc.uboot_rw_memcpy)
        {
            if (sizeof(unsigned long) == 8) { Exchange.sys.fn.print("memcpy(Dest:%016lx Src:%016lx bytes:%08x)\n", pucDestBuff, pucSrcBuff, (U32)uiCpyBytes); }
            else                            { Exchange.sys.fn.print("memcpy(Dest:%08x Src:%08x bytes:%08x)\n", pucDestBuff, pucSrcBuff, (U32)uiCpyBytes); }
        }
#if defined (__SUPPORT_DEBUG_MIO_UBOOT_ERROR_STOP__)
        if (!uiCpyBytes) { Exchange.sys.fn.print("mio_fill_read_buffer: memcpy error (copybytes 0)\n"); while(1); }
#endif
    }
    else
    {
        pucSrcBuff = (U8*)(*Exchange.buffer.BaseOfReadBuffer + (uiCurrExtIndex << 9));
        uiCpyBytes = uiSectors << 9;
        memcpy((void *)pucDestBuff, (const void *)pucSrcBuff, uiCpyBytes);
        if (Exchange.debug.misc.uboot_rw_memcpy)
        {
            if (sizeof(unsigned long) == 8) { Exchange.sys.fn.print("memcpy(Dest:%016lx Src:%016lx bytes:%08x)\n", pucDestBuff, pucSrcBuff, (U32)uiCpyBytes); }
            else                            { Exchange.sys.fn.print("memcpy(Dest:%08x Src:%08x bytes:%08x)\n", pucDestBuff, pucSrcBuff, (U32)uiCpyBytes); }
        }
#if defined (__SUPPORT_DEBUG_MIO_UBOOT_ERROR_STOP__)
        if (!uiCpyBytes) { Exchange.sys.fn.print("mio_fill_read_buffer: memcpy error (copybytes 0)\n"); while(1); }
#endif
    }
}

static void mio_fill_write_cache(const void *pvBuff, U32 uiSectors)
{
    U8 *pucSrcBuff = (U8 *)pvBuff;
    U8 *pucDestBuff = 0;
    U32 uiCpyBytes = 0;
    U32 uiCurrExtIndex = *Exchange.buffer.WriteBlkIdx & (*Exchange.buffer.SectorsOfWriteCache - 1);

    if ((uiCurrExtIndex + uiSectors) > *Exchange.buffer.SectorsOfWriteCache)
    { // is rollover
        pucDestBuff = (U8 *)(*Exchange.buffer.BaseOfWriteCache + (uiCurrExtIndex << 9));
        uiCpyBytes = (*Exchange.buffer.SectorsOfWriteCache - uiCurrExtIndex) << 9;
        memcpy((void *)pucDestBuff, (const void *)pucSrcBuff, uiCpyBytes);
#if defined (__SUPPORT_DEBUG_MIO_UBOOT_ERROR_STOP__)
        if (!uiCpyBytes) { Exchange.sys.fn.print("mio_fill_write_cache: memcpy error (copybytes 0)\n"); while(1); }
#endif

        pucDestBuff = (U8 *)(*Exchange.buffer.BaseOfWriteCache);
        pucSrcBuff += uiCpyBytes;
        uiCpyBytes = (uiSectors << 9) - uiCpyBytes;
        memcpy((void *)pucDestBuff, (const void *)pucSrcBuff, uiCpyBytes);
#if defined (__SUPPORT_DEBUG_MIO_UBOOT_ERROR_STOP__)
        if (!uiCpyBytes) { Exchange.sys.fn.print("mio_fill_write_cache: memcpy error (copybytes 0)\n"); while(1); }
#endif
    }
    else
    {
        pucDestBuff = (U8 *)(*Exchange.buffer.BaseOfWriteCache + (uiCurrExtIndex << 9));
        uiCpyBytes = uiSectors << 9;
        memcpy((void *)pucDestBuff, (const void *)pucSrcBuff, uiCpyBytes);
#if defined (__SUPPORT_DEBUG_MIO_UBOOT_ERROR_STOP__)
        if (!uiCpyBytes) { Exchange.sys.fn.print("mio_fill_write_cache: memcpy error (copybytes 0)\n"); while(1); }
#endif
    }
}
        
/*******************************************************************************
 *
 *******************************************************************************/
int mio_rwtest(ulong ulTestSectors, ulong ulCapacity, unsigned char ucWriteRatio, unsigned char ucSequentRatio)
{
#if defined (__COMPILE_MODE_RW_TEST__)
    if (!is_mio_init)
    {
        Exchange.sys.fn.print("mio_nand_write(): mio is not initialized!!\n");
        return -1;
    }

    if (ulCapacity < 4*1024)
    {
        Exchange.sys.fn.print("mio_rwtest(): wrong capacity: %lu\n", ulCapacity);
        return -1;
    }

    //ulTestSectors = 1000*1024*1024 / 512;
    //ulCapacity = 100*1024*1024 / 512;
    //ucWriteRatio = 50;
    //ucSequentRatio = 50;

    if (ulCapacity > *Exchange.ftl.Capacity)
        ulCapacity = *Exchange.ftl.Capacity;

    if (ucWriteRatio > 100)
        ucWriteRatio = 100;
    if (ucSequentRatio > 100)
        ucSequentRatio = 100;

    Exchange.sys.fn.print("mio_rwtest(): %lu sectors(%lu MB), Capacity: %lu sectors(%lu MB), writeRatio:%d%%, sequentRatio:%d%%\n", ulTestSectors, ulTestSectors/(2*1024), ulCapacity, ulCapacity/(2*1024), ucWriteRatio, ucSequentRatio);
    mio_rwtest_run(ulTestSectors, ulCapacity, ucWriteRatio, ucSequentRatio);
#endif
    return 0;
}

