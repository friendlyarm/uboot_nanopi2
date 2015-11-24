/******************************************************************************
 *
 * (C) COPYRIGHT 2008-2014 EASTWHO CO., LTD ALL RIGHTS RESERVED
 *
 * File name    : mio.uboot.rwtest.c
 * Date         : 2014.08.30
 * Author       : TW.KIM (taewon@eastwho.com)
 * Abstraction  :
 * Revision     : V1.0 (2014.08.30 TW.KIM)
 *
 * Description  :
 *
 ******************************************************************************/
#define __MIO_UBOOT_RWTEST_GLOBAL__

#include "mio.uboot.rwtest.h"
#include "media/exchange.h"

#include <malloc.h>

#include <mio.uboot.h>

#define __SUPPORT_DEBUG_PRINT_MIO_UBOOT_RWTEST__

#if defined (__SUPPORT_DEBUG_PRINT_MIO_UBOOT_RWTEST__)
    #define DBG_UBOOT_RWTEST(fmt, args...) printf(fmt, ##args)
#else
    #define DBG_UBOOT_RWTEST(fmt, args...)
#endif

/*******************************************************************************
 *
 *******************************************************************************/
#ifndef rand
    /*
     * Simple xorshift PRNG
     *   see http://www.jstatsoft.org/v08/i14/paper
     *
     * Copyright (c) 2012 Michael Walle
     * Michael Walle <michael@walle.cc>
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

    static unsigned int y = 1U;

    unsigned int rand_r(unsigned int *seedp)
    {
        *seedp ^= (*seedp << 13);
        *seedp ^= (*seedp >> 17);
        *seedp ^= (*seedp << 5);

        return *seedp;
    }

    unsigned int rand(void)
    {
        return rand_r(&y);
    }

    void srand(unsigned int seed)
    {
        y = seed;
    }
#endif

#if defined (__COMPILE_MODE_RW_TEST__)

/*******************************************************************************
 * local variables
 *******************************************************************************/
static struct
{
    ulong uiCapacity;

    ulong uiWriteRatio; // 0 ~ 100
    ulong uiSeqRatio;   // 0 ~ 100

    ulong uiMinTransferSectors;
    ulong uiMaxTransferSectors;

    ulong uiFlushCmdCycle;
    ulong uiStandbyCycle;
    ulong uiPowerdownCycle;
    ulong uiTrimCycle;

    ulong uiSectorsToTest;

    ulong uiBuffSize;
    unsigned char *pucBuff;
    unsigned int *puiIsWrittenLba;

} gstMioRwtestParam;

static struct 
{
    unsigned int uiCmdNo;

    unsigned int uiWrittenSectors;
    unsigned int uiReadSectors;
    unsigned int uiComparedSectors;
    unsigned int uiWriteCmdCnt[2];   // [0]: sequent, [1]: random
    unsigned int uiReadCmdCnt[2];    // [0]: sequent, [1]: random
    unsigned int uiFlushCmdCnt;
    unsigned int uiStandbyCmdCnt;
    unsigned int uiPowerdownCmdCnt;

    unsigned int uiError;

    struct 
    {
        unsigned int uiAddress;
        unsigned int uiSectors;
        unsigned int uiWrittenData;
        unsigned int uiReadData;
        unsigned int uiCmdNo;
        unsigned int uiCmdStartAddr;
        unsigned int uiCmdSectors;
        
    } stErrInfo;
    
} gstMioRwtestResult;

/*******************************************************************************
 * local functions
 *******************************************************************************/
static int mio_rwtest_init(ulong ulTestSectors, ulong ulCapacity, unsigned char ucWriteRatio, unsigned char ucSequentRatio, unsigned int uiRwBuffSize);
static void mio_rwtest_deinit(void);
static ulong mio_rwtest_write(ulong blknr, lbaint_t blkcnt);
static ulong mio_rwtest_read_verify(ulong blknr, lbaint_t blkcnt);
static void print_view_rwtest_result(void);

/*******************************************************************************
 *
 *******************************************************************************/
int mio_rwtest_run(ulong ulTestSectors, ulong ulCapacity, unsigned char ucWriteRatio, unsigned char ucSequentRatio)
{
    int MaxTestLoop=300000, CurrTestLoop=0;

    int siResp=0;
    //ulong ulStartSeconds=0;
    unsigned int uiAddr=0, uiNextSeqWriteAddr=0, uiNextSeqReadAddr=0;
    unsigned int uiSectors=0, uiTestSectors=0;
    unsigned int uiWriteSectors=0, uiReadSectors=0;
    unsigned char ucIsWrite=0, ucIsSequent=0;
    
    siResp = mio_rwtest_init(ulTestSectors, ulCapacity, ucWriteRatio, ucSequentRatio, 10*1024*1024);
    if (siResp < 0)
    {
        DBG_UBOOT_RWTEST("mio_rwtest_run: mio_rwtest_init() error\n");
    }
    else
    {
        memset(&gstMioRwtestResult, 0, sizeof(gstMioRwtestResult));

        srand(1234);

        DBG_UBOOT_RWTEST("mio_rwtest_run: gstMioRwtestParam.uiCapacity:%lu, uiMaxTransferSectors:%lu, uiMinTransferSectors:%lu\n",
            gstMioRwtestParam.uiCapacity, gstMioRwtestParam.uiMaxTransferSectors, gstMioRwtestParam.uiMinTransferSectors);
        do
        {
            if (gstMioRwtestParam.uiFlushCmdCycle && gstMioRwtestResult.uiCmdNo && !(gstMioRwtestResult.uiCmdNo % gstMioRwtestParam.uiFlushCmdCycle))
            {
                DBG_UBOOT_RWTEST("0x%08X mio_rwtest_run: Flush %u\n", CurrTestLoop, gstMioRwtestResult.uiCmdNo);
                mio_flush();
                gstMioRwtestResult.uiFlushCmdCnt += 1;
            }
            else if (gstMioRwtestParam.uiStandbyCycle && gstMioRwtestResult.uiCmdNo && !(gstMioRwtestResult.uiCmdNo % gstMioRwtestParam.uiStandbyCycle))
            {
                DBG_UBOOT_RWTEST("0x%08X mio_rwtest_run: Standby %u)\n", CurrTestLoop, gstMioRwtestResult.uiCmdNo);
                mio_standby();
                
                siResp = mio_init();
                if (siResp < 0)
                {
                    DBG_UBOOT_RWTEST("mio_rwtest_run: mio_rwtest_init() failed !!\n");
                    break;
                }
                gstMioRwtestResult.uiStandbyCmdCnt += 1;
            }
            else if (gstMioRwtestParam.uiPowerdownCycle && gstMioRwtestResult.uiCmdNo && !(gstMioRwtestResult.uiCmdNo % gstMioRwtestParam.uiPowerdownCycle))
            { 
                DBG_UBOOT_RWTEST("0x%08X mio_rwtest_run: Powerdown %u\n", CurrTestLoop, gstMioRwtestResult.uiCmdNo);
              //mio_powerdown();
                gstMioRwtestResult.uiPowerdownCmdCnt += 1;
                // ...
                // mio_init();
                // ...
            }
            else
            {
                ucIsWrite = ((((unsigned int)rand())%100) >= (100 - gstMioRwtestParam.uiWriteRatio))? 1: 0;
                ucIsSequent = ((((unsigned int)rand())%100) >= (100 - gstMioRwtestParam.uiSeqRatio))? 1: 0;

                if (ucIsWrite)
                {
                    uiAddr = (ucIsSequent)? uiNextSeqWriteAddr: (((unsigned int)rand()) % gstMioRwtestParam.uiCapacity);
                }
                else
                {
                    uiAddr = (ucIsSequent)? uiNextSeqReadAddr: (((unsigned int)rand()) % gstMioRwtestParam.uiCapacity);
                }

                uiSectors = ((unsigned int)rand()) % (gstMioRwtestParam.uiMaxTransferSectors - gstMioRwtestParam.uiMinTransferSectors);
                uiSectors += gstMioRwtestParam.uiMinTransferSectors;
                if (uiAddr + uiSectors > gstMioRwtestParam.uiCapacity)
                {
                    uiSectors = gstMioRwtestParam.uiCapacity - uiAddr;
                }

                if (ucIsWrite)
                {
                    DBG_UBOOT_RWTEST("0x%08X mio_rwtest_write(0x%08x, 0x%8x), \tnextSeqAddr:0x%08x\n", CurrTestLoop, uiAddr, uiSectors, uiAddr + uiSectors);
                    uiWriteSectors = mio_rwtest_write(uiAddr, uiSectors);
                    if (uiWriteSectors != uiSectors)
                    {
                        DBG_UBOOT_RWTEST("mio_rwtest_write: error %d, %d\n", uiWriteSectors, uiSectors);
                        break;
                    }
                    uiNextSeqWriteAddr = ((uiAddr + uiSectors) == gstMioRwtestParam.uiCapacity)? 0: uiAddr + uiSectors;
                    

                    if (ucIsSequent)    gstMioRwtestResult.uiWriteCmdCnt[0] += 1;
                    else                gstMioRwtestResult.uiWriteCmdCnt[1] += 1;
                }
                else
                {
                    DBG_UBOOT_RWTEST("0x%08X mio_rwtest_read (0x%08x, 0x%8x), \tnextSeqAddr:0x%08x\n", CurrTestLoop, uiAddr, uiSectors, uiAddr + uiSectors);
                    uiReadSectors = mio_rwtest_read_verify(uiAddr, uiSectors);
                    if (uiReadSectors != uiSectors)
                    {
                        DBG_UBOOT_RWTEST("mio_rwtest_read_verify: error %d, %d\n", uiReadSectors, uiSectors);
                        break;
                    }
                    uiNextSeqReadAddr = ((uiAddr + uiSectors) == gstMioRwtestParam.uiCapacity)? 0: uiAddr + uiSectors;

                    if (ucIsSequent)    gstMioRwtestResult.uiReadCmdCnt[0] += 1;
                    else                gstMioRwtestResult.uiReadCmdCnt[1] += 1;
                }
                uiTestSectors += uiSectors;
            }
            
            gstMioRwtestResult.uiCmdNo += 1;

            if (gstMioRwtestResult.uiError)
                break;

#if 1
            CurrTestLoop++;
            if (CurrTestLoop >= MaxTestLoop)
            {
                break;
            }
#endif
        } while(uiTestSectors < gstMioRwtestParam.uiSectorsToTest);

        DBG_UBOOT_RWTEST("mio_rwtest_run: Test end ! \n");
        
    }

    print_view_rwtest_result();
    mio_rwtest_deinit();

    return 0;
}

#if 0 // mio write with fixed pattern
typedef struct __RW_PATTERN__
{
	ulong blknr;
	lbaint_t blkcnt;
	const void *pvBuffer;
} RW_PATTERN;

RW_PATTERN w_pattern[] =
{
  // LBA      Sectors, data address 
	{0x1,     0x8,    0x00000000},
	{0x9,     0x8,    0x00000000},
	{0x11,    0x38,   0x00000000},
	{0x49,    0x10,   0x00000000},
	{0x59,    0x800,  0x00000000},
	{0x859,   0x2000, 0x00000000},
	{0x2859,  0x8,    0x00000000},
	{0x2861,  0x8,    0x00000000},
	{0x2869,  0x8,    0x00000000},
	{0x2871,  0x1C8,  0x00000000},
	{0x2A39,  0x1778, 0x00000000},
	{0x41B1,  0x710,  0x00000000},
	{0x48C1,  0x288,  0x00000000},
	{0x4B49,  0x20E0, 0x00000000},
	{0x6C29,  0x1E0,  0x00000000},
	{0x20001, 0x8,    0x00000000},
	{0x20009, 0x8,    0x00000000},
	{0x20011, 0x178,  0x00000000},
	{0x20189, 0x10,   0x00000000},
	{0x20199, 0xDA0,  0x00000000},
	{0x20F39, 0x51C0, 0x00000000},
	{0x260F9, 0x8,    0x00000000},
	{0x26101, 0x8,    0x00000000},
	{0x26109, 0x8,    0x00000000},
	{0x26111, 0x8,    0x00000000},
	{0x26119, 0x40,   0x00000000},
	{0x26159, 0x610,  0x00000000},
	{0x26769, 0x13D0, 0x00000000},
	{0x27B39, 0x298,  0x00000000},
	{0x27DD1, 0xE88,  0x00000000},
	{0x28C59, 0x1EA8, 0x00000000},
	{0x2AB01, 0x130,  0x00000000},
	{0x2AC31, 0x1438, 0x00000000},
	{0x2C069, 0x340,  0x00000000},
	{0x2C3A9, 0x1C8,  0x00000000},
	{0x2C571, 0x27F0, 0x00000000},
	{0x2ED61, 0x8D8,  0x00000000},
	{0x2F639, 0x210,  0x00000000},
	{0x2F849, 0x2880, 0x00000000},
	{0x320C9, 0x18,   0x00000000},
	{0x320E1, 0x78,   0x00000000},
	{0x32159, 0x90,   0x00000000},
	{0x321E9, 0x5ED8, 0x00000000},
	{0x380C1, 0xD60,  0x00000000},
	{0x38E21, 0x160,  0x00000000},
	{0x38F81, 0x1C8,  0x00000000},
	{0x39149, 0x588,  0x00000000},
	{0x396D1, 0xA8,   0x00000000},
	{0x39779, 0x1710, 0x00000000},
	{0x3AE89, 0x10,   0x00000000},
	{0x3AE99, 0x2D8,  0x00000000},
	{0x3B171, 0xA0,   0x00000000},
	{0x3B211, 0x3A0,  0x00000000},
	{0x3B5B1, 0x28,   0x00000000},
	{0x3B5D9, 0x3F98, 0x00000000},
	{0x3F571, 0x1C0,  0x00000000},
	{0x3F731, 0x8,    0x00000000},
	{0x3F739, 0x2D0,  0x00000000},
	{0x3FA09, 0x108,  0x00000000},
	{0x3FB11, 0xB0,   0x00000000},
	{0x3FBC1, 0x18,   0x00000000},
	{0x3FBD9, 0x28F8, 0x00000000},
	{0x424D1, 0x188,  0x00000000},
	{0x42659, 0x10,   0x00000000},
	{0x42669, 0x8,    0x00000000},
	{0x42671, 0xD8,   0x00000000}
};

int mio_rwtest_pattern_run(ulong ulTestSectors, ulong ulCapacity, unsigned char ucWriteRatio, unsigned char ucSequentRatio)
{
    int MaxTestLoop=300000, CurrTestLoop=0;

    int siResp=0;
    //ulong ulStartSeconds=0;
    unsigned int uiAddr=0, uiNextSeqWriteAddr=0, uiNextSeqReadAddr=0;
    unsigned int uiSectors=0, uiTestSectors=0;
    unsigned int uiWriteSectors=0, uiReadSectors=0;
    unsigned char ucIsWrite=0, ucIsSequent=0;
    int uiPatternIdx=0;

    siResp = mio_rwtest_init(ulTestSectors, ulCapacity, ucWriteRatio, ucSequentRatio, 15*1024*1024);
    if (siResp < 0)
    {
        DBG_UBOOT_RWTEST("mio_rwtest_pattern_run: mio_rwtest_init() error\n");
    }
    else
    {
        memset(&gstMioRwtestResult, 0, sizeof(gstMioRwtestResult));

        srand(1234);

        DBG_UBOOT_RWTEST("mio_rwtest_pattern_run: gstMioRwtestParam.uiCapacity:%lu, uiMaxTransferSectors:%lu, uiMinTransferSectors:%lu\n",
            gstMioRwtestParam.uiCapacity, gstMioRwtestParam.uiMaxTransferSectors, gstMioRwtestParam.uiMinTransferSectors);

        for (uiPatternIdx=0; uiPatternIdx < (sizeof(w_pattern)/sizeof(w_pattern[0])); uiPatternIdx++)
        {
            uiAddr = (unsigned int)w_pattern[uiPatternIdx].blknr;
            uiSectors = (unsigned int)w_pattern[uiPatternIdx].blkcnt;

            DBG_UBOOT_RWTEST("0x%08X mio_rwtest_write(0x%08x, 0x%8x), \tnextSeqAddr:0x%08x\n", CurrTestLoop, uiAddr, uiSectors, uiAddr + uiSectors);
            uiWriteSectors = mio_rwtest_write(uiAddr, uiSectors);
            if (uiWriteSectors != uiSectors)
            {
                DBG_UBOOT_RWTEST("mio_rwtest_write: error %d, %d\n", uiWriteSectors, uiSectors);
                break;
            }

            gstMioRwtestResult.uiCmdNo += 1;

            if (gstMioRwtestResult.uiError)
                break;

            CurrTestLoop++;
            if (CurrTestLoop >= MaxTestLoop)
            {
                break;
            }
        }

        DBG_UBOOT_RWTEST("mio_rwtest_pattern_run: Test end ! \n");

    }

  //print_view_rwtest_result();
    mio_rwtest_deinit();

    return 0;
}
#endif

/*******************************************************************************
 * local functions
 *******************************************************************************/
static int mio_rwtest_init(ulong ulSectorsToTest, ulong ulCapacity, unsigned char ucWriteRatio, unsigned char ucSequentRatio, unsigned int uiRwBuffSize)
{
    int siResp=-1;
    unsigned int uiBuffSize=0;

    memset(&gstMioRwtestParam, 0, sizeof(gstMioRwtestParam));
    
    // set parameter for Read/Write Test 
    gstMioRwtestParam.uiCapacity = ulCapacity;

    gstMioRwtestParam.uiWriteRatio = ucWriteRatio; // 0 ~ 100 %
    gstMioRwtestParam.uiSeqRatio = ucSequentRatio; // 0 ~ 100 %

    gstMioRwtestParam.uiFlushCmdCycle =  100;
  //gstMioRwtestParam.uiStandbyCycle =   200;
  //gstMioRwtestParam.uiPowerdownCycle = 100000;
  //gstMioRwtestParam.uiTrimCycle = 1000;

    gstMioRwtestParam.uiSectorsToTest = (unsigned int)ulSectorsToTest;

    // allocate buffer
    gstMioRwtestParam.uiBuffSize = uiRwBuffSize;
    gstMioRwtestParam.pucBuff = (unsigned char *)malloc(gstMioRwtestParam.uiBuffSize);
    if (!gstMioRwtestParam.pucBuff)
        siResp = -1;

  //gstMioRwtestParam.uiMinTransferSectors = (4*1024)/512;       //  4 KB
  //gstMioRwtestParam.uiMaxTransferSectors = (32*1024*1024)/512; // 32 MB

  //gstMioRwtestParam.uiMinTransferSectors = (4*1024)/512;       //  4 KB
  //gstMioRwtestParam.uiMaxTransferSectors = (4*1024*1024)/512;  //  4 MB

    gstMioRwtestParam.uiMinTransferSectors = (4*1024)/512;       //  4 KB
    gstMioRwtestParam.uiMaxTransferSectors = (4*1024*1024)/512;  //  4 MB

    if (gstMioRwtestParam.uiMaxTransferSectors > (gstMioRwtestParam.uiBuffSize / 512))
    {
        gstMioRwtestParam.uiMaxTransferSectors = gstMioRwtestParam.uiBuffSize / 512;
    }

    uiBuffSize = (gstMioRwtestParam.uiCapacity + 7) / 8;
    gstMioRwtestParam.puiIsWrittenLba = (unsigned int *)malloc(uiBuffSize);
    if (gstMioRwtestParam.puiIsWrittenLba)
        memset(gstMioRwtestParam.puiIsWrittenLba, 0, uiBuffSize);
    else
        siResp = -1;

    memset(&gstMioRwtestResult, 0, sizeof(gstMioRwtestResult));

    siResp = 0;

    if (siResp < 0)
    {
        printf("mio_rwtest_init: error\n");
        mio_rwtest_deinit();
    }

    return siResp;
}

static void mio_rwtest_deinit(void)
{
    if (gstMioRwtestParam.pucBuff)
        free(gstMioRwtestParam.pucBuff);

    if (gstMioRwtestParam.puiIsWrittenLba)
        free(gstMioRwtestParam.puiIsWrittenLba);
}

static ulong mio_rwtest_write(ulong blknr, lbaint_t blkcnt)
{
    ulong ulSize=0;

    unsigned int uiSectorsLeft = blkcnt;
    unsigned int uiByteIndex=0, uiBitIndex=0;
    unsigned int uiAddr=blknr, uiSectors=0, uiBlkIndex=0, uiBuffOffset=0;
    const unsigned int uiMaxSectorsInCmd = (gstMioRwtestParam.uiBuffSize/512);
    unsigned int *puiData=0;
    unsigned int uiDataIdx=0, uiRandData=0;

    while(uiSectorsLeft)
    {
        uiSectors = (uiSectorsLeft > uiMaxSectorsInCmd)? uiMaxSectorsInCmd: uiSectorsLeft;

        for (uiBlkIndex=0; uiBlkIndex < uiSectors; uiBlkIndex++)
        {
            uiBuffOffset = uiBlkIndex % uiMaxSectorsInCmd;

            puiData = (unsigned int *)(gstMioRwtestParam.pucBuff + (uiBuffOffset * 512));

            memset(puiData, 0, 512);
            uiDataIdx = 0;
            puiData[uiDataIdx++] = (unsigned int)(uiAddr + uiBlkIndex);      // 1st 4B: blknr
            puiData[uiDataIdx++] = (unsigned int)gstMioRwtestResult.uiCmdNo; // 2nd 4B: write command number
            puiData[uiDataIdx++] = (unsigned int)uiAddr;                     // 3rd 4B: start blknr of write command
            puiData[uiDataIdx++] = (unsigned int)uiSectors;                  // 4th 4B: blkcnt of write command

            puiData[uiDataIdx++] = (unsigned int)(uiAddr + uiBlkIndex);      // 1st 4B: blknr
            puiData[uiDataIdx++] = (unsigned int)(uiAddr + uiBlkIndex);      // 1st 4B: blknr
            puiData[uiDataIdx++] = (unsigned int)(uiAddr + uiBlkIndex);      // 1st 4B: blknr
            puiData[uiDataIdx++] = (unsigned int)(uiAddr + uiBlkIndex);      // 1st 4B: blknr
            puiData[uiDataIdx++] = (unsigned int)(uiAddr + uiBlkIndex);      // 1st 4B: blknr
            puiData[uiDataIdx++] = (unsigned int)(uiAddr + uiBlkIndex);      // 1st 4B: blknr
            puiData[uiDataIdx++] = (unsigned int)(uiAddr + uiBlkIndex);      // 1st 4B: blknr
            puiData[uiDataIdx++] = (unsigned int)(uiAddr + uiBlkIndex);      // 1st 4B: blknr

            uiRandData = rand();
            for (; uiDataIdx < (512 / sizeof(puiData[0])); uiDataIdx++)
            {
                puiData[uiDataIdx] = uiRandData;
            }

            uiByteIndex = puiData[0] / 32;
            uiBitIndex = puiData[0] % 32;

            gstMioRwtestParam.puiIsWrittenLba[uiByteIndex] |= (1 << uiBitIndex);
        }
        
        ulSize = mio_write(uiAddr, uiSectors, gstMioRwtestParam.pucBuff);
        if (ulSize != uiSectors)
        {
            gstMioRwtestResult.uiError = 1;
            gstMioRwtestResult.stErrInfo.uiAddress = uiAddr;
            gstMioRwtestResult.stErrInfo.uiSectors = uiSectors;
          //gstMioRwtestResult.stErrInfo.uiWrittenData = 0;
          //gstMioRwtestResult.stErrInfo.uiReadData = 0;
            gstMioRwtestResult.stErrInfo.uiCmdNo = gstMioRwtestResult.uiCmdNo;
            gstMioRwtestResult.stErrInfo.uiCmdStartAddr = uiAddr;
            gstMioRwtestResult.stErrInfo.uiCmdSectors = uiSectors;
        }

        uiAddr += ulSize;
        uiSectorsLeft -= ulSize;

        if (gstMioRwtestResult.uiError)
            break;

    }

    gstMioRwtestResult.uiWrittenSectors += (blkcnt - uiSectorsLeft);

    if (uiSectorsLeft)
    {
        printf("mio_rwtest_write() failed!\n");
    }

    return (blkcnt - uiSectorsLeft);
}

static ulong mio_rwtest_read_verify(ulong blknr, lbaint_t blkcnt)
{
    ulong ulSize=0, ulComparedSectors=0;

    unsigned int uiSectorsLeft = blkcnt;
    unsigned int uiByteIndex=0, uiBitIndex=0;
    unsigned int uiAddr=blknr, uiSectors=0, uiBlkIndex=0;
    const unsigned int uiMaxSectorsInCmd = (gstMioRwtestParam.uiBuffSize/512);
    unsigned int *puiData = (unsigned int *)gstMioRwtestParam.pucBuff;

    while(uiSectorsLeft)
    {
        uiSectors = (uiSectorsLeft > uiMaxSectorsInCmd)? uiMaxSectorsInCmd: uiSectorsLeft;

        ulSize = mio_read(uiAddr, uiSectors, puiData);
        if (ulSize == uiSectors)
        {
            for (uiBlkIndex=0; uiBlkIndex < ulSize; uiBlkIndex++)
            {
                puiData = (unsigned int *)(gstMioRwtestParam.pucBuff + (uiBlkIndex * 512));

                uiByteIndex = (uiAddr + uiBlkIndex) / 32;
                uiBitIndex = (uiAddr + uiBlkIndex) % 32;

              //printf("uiSectorsLeft %d, uiBlkIndex %d, ulSize %d, Byte %d Bit %d [0x%0x]\n",
              //    uiSectorsLeft, uiBlkIndex, ulSize, uiByteIndex, uiBitIndex, gstMioRwtestParam.puiIsWrittenLba[uiByteIndex]);

                if (gstMioRwtestParam.puiIsWrittenLba[uiByteIndex] & (1 << uiBitIndex))
                {
                    if ((puiData[0] == (unsigned int)(uiAddr + uiBlkIndex)))
                    {
                        ulComparedSectors++;
                        gstMioRwtestResult.uiComparedSectors++;
                    }
                    else
                    {   // Verify failed!
                        gstMioRwtestResult.uiError = (1<<2);

                        gstMioRwtestResult.stErrInfo.uiAddress = uiAddr + uiBlkIndex;
                      //gstMioRwtestReslut.stErrInfo.uiSectors = 0;
                        gstMioRwtestResult.stErrInfo.uiWrittenData = uiAddr + uiBlkIndex;
                        gstMioRwtestResult.stErrInfo.uiReadData = puiData[0];
                        gstMioRwtestResult.stErrInfo.uiCmdNo = puiData[1];
                        gstMioRwtestResult.stErrInfo.uiCmdStartAddr = puiData[2];
                        gstMioRwtestResult.stErrInfo.uiCmdSectors = puiData[3];
                    }
                }
                else // not written yet.
                {
                }
            }
        }
        else
        {
            gstMioRwtestResult.uiError = (1<<1);
            gstMioRwtestResult.stErrInfo.uiAddress = uiAddr;
            gstMioRwtestResult.stErrInfo.uiSectors = uiSectors;
          //gstMioRwtestResult.stErrInfo.uiWrittenData = uiAddr;
          //gstMioRwtestResult.stErrInfo.uiReadData = puiData[0];
          //gstMioRwtestResult.stErrInfo.uiCmdNo = gstMioRwtestResult.uiCmdNo;
          //gstMioRwtestResult.stErrInfo.uiCmdStartAddr = puiData[2];
          //gstMioRwtestResult.stErrInfo.uiCmdSectors = puiData[3];

        }

        uiAddr += ulSize;
        uiSectorsLeft -= ulSize;

        if (gstMioRwtestResult.uiError)
            break;

    }

    gstMioRwtestResult.uiReadSectors += (blkcnt - uiSectorsLeft);

    return (blkcnt - uiSectorsLeft);
}

static void print_view_rwtest_result(void)
{
    printf("##########################\n");
    printf("READ/WRITE TEST SUMMARY!!!\n");
    printf(" Total written  sectors: %u (%u MB)\n", gstMioRwtestResult.uiWrittenSectors, gstMioRwtestResult.uiWrittenSectors/(2*1024));
    printf(" Total read     sectors: %u (%u MB)\n", gstMioRwtestResult.uiReadSectors, gstMioRwtestResult.uiReadSectors/(2*1024));
    printf(" Total compared sectors: %u (%u MB)\n", gstMioRwtestResult.uiComparedSectors, gstMioRwtestResult.uiComparedSectors/(2*1024));
    printf("\n");
    printf(" write     command count: %u (Seq:%u, Rand:%u)\n", gstMioRwtestResult.uiWriteCmdCnt[0]+gstMioRwtestResult.uiWriteCmdCnt[1], gstMioRwtestResult.uiWriteCmdCnt[0], gstMioRwtestResult.uiWriteCmdCnt[1]);
    printf(" read      command count: %u (Seq:%u, Rand:%u)\n", gstMioRwtestResult.uiReadCmdCnt[0]+gstMioRwtestResult.uiReadCmdCnt[1], gstMioRwtestResult.uiReadCmdCnt[0], gstMioRwtestResult.uiReadCmdCnt[1]);
    printf(" flush     command count: %u\n", gstMioRwtestResult.uiFlushCmdCnt);
    printf(" standby   command count: %u\n", gstMioRwtestResult.uiStandbyCmdCnt);
    printf(" powerdown command count: %u\n", gstMioRwtestResult.uiPowerdownCmdCnt);
  //printf(" Total test time: %u secs\n", gstMioRwtestResult....);
  //printf("\n");

    if (gstMioRwtestResult.uiError & (1<<0))
    {
        printf("\n");
        printf(" write command failed!\n");
        printf("  Address: %xh, Sectors: %xh\n", gstMioRwtestResult.stErrInfo.uiAddress, gstMioRwtestResult.stErrInfo.uiSectors);
        printf("  write command number       : %xh\n", gstMioRwtestResult.stErrInfo.uiCmdNo);
        printf("  write command start address: %xh\n", gstMioRwtestResult.stErrInfo.uiCmdStartAddr);
        printf("  write command sectors      : %xh\n", gstMioRwtestResult.stErrInfo.uiCmdSectors);
    }

    if (gstMioRwtestResult.uiError & (1<<1))
    {
        printf("\n");
        printf(" read command failed!\n");
        printf("  address: %x, sectors: %x\n", gstMioRwtestResult.stErrInfo.uiAddress, gstMioRwtestResult.stErrInfo.uiSectors);
    }

    if (gstMioRwtestResult.uiError & (1<<2))
    {
        printf("\n");
        printf(" Verify failed!\n");
        printf("  address:%d\n",  gstMioRwtestResult.stErrInfo.uiAddress);
        printf("  written data[0]: %08xh\n", gstMioRwtestResult.stErrInfo.uiWrittenData);
        printf("  read    data[0]: %08xh\n", gstMioRwtestResult.stErrInfo.uiReadData);
        printf("  This sector is written by the following write command\n");
        printf("   write command number        : %xh\n", gstMioRwtestResult.stErrInfo.uiCmdNo);
        printf("   write command start address : %xh\n", gstMioRwtestResult.stErrInfo.uiCmdStartAddr);
        printf("   write command sectors       : %xh\n", gstMioRwtestResult.stErrInfo.uiCmdSectors);
    }
    printf("##########################\n");
}
#endif
