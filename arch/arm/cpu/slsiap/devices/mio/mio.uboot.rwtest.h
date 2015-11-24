/******************************************************************************
 *
 * (C) COPYRIGHT 2008-2014 EASTWHO CO., LTD ALL RIGHTS RESERVED
 *
 * File name    : mio.uboot.rwtest.h
 * Date         : 2014.08.30
 * Author       : TW.KIM (taewon@eastwho.com)
 * Abstraction  :
 * Revision     : V1.0 (2014.08.30 TW.KIM)
 *
 * Description  :
 *
 ******************************************************************************/
#ifndef __MIO_UBOOT_RWTEST_H__
#define __MIO_UBOOT_RWTEST_H__


#ifdef __MIO_UBOOT_RWTEST_GLOBAL__
#define MIO_UBOOT_RWTEST_EXT
#else
#define MIO_UBOOT_RWTEST_EXT extern
#endif

#include <common.h>
#include <linux/types.h>    // lbainit_t
#include <ide.h>            // lbainit_t


MIO_UBOOT_RWTEST_EXT int mio_rwtest_run(ulong ulSec, ulong ulCapacity, unsigned char ucWriteRatio, unsigned char ucSequentRatio);


#endif
