/******************************************************************************
 *
 * (C) COPYRIGHT 2008-2014 EASTWHO CO., LTD ALL RIGHTS RESERVED
 *
 * File name    : mio.uboot.h
 * Date         : 2014.08.30
 * Author       : TW.KIM (taewon@eastwho.com)
 * Abstraction  :
 * Revision     : V1.0 (2014.08.30 TW.KIM)
 *
 * Description  :
 *
 ******************************************************************************/
#ifndef __MIO_UBOOT_H__
#define __MIO_UBOOT_H__

#ifdef __MIO_UBOOT_GLOBAL__
#define MIO_UBOOT_EXT
#else
#define MIO_UBOOT_EXT extern
#endif

#include <linux/types.h>    // lbainit_t 
#include <ide.h>            // lbainit_t

/******************************************************************************
 *
 * NAND Configurations
 *
 ******************************************************************************/

/******************************************************************************
 *
 ******************************************************************************/
MIO_UBOOT_EXT int mio_format(int _format_type);
MIO_UBOOT_EXT int mio_init(void);
MIO_UBOOT_EXT int mio_deinit(void);
MIO_UBOOT_EXT int mio_info(void);
MIO_UBOOT_EXT int get_mio_capacity(void);

/******************************************************************************
 * block device interface
 ******************************************************************************/
MIO_UBOOT_EXT ulong mio_read(ulong blknr, lbaint_t blkcnt, void *buffer);
MIO_UBOOT_EXT ulong mio_write(ulong blknr, lbaint_t blkcnt, const void *buffer);
MIO_UBOOT_EXT void mio_set_autosend_standbycmd(int enable);
MIO_UBOOT_EXT int mio_get_autosend_standbycmd(void);
MIO_UBOOT_EXT int mio_flush(void);
MIO_UBOOT_EXT int mio_standby(void);
MIO_UBOOT_EXT int mio_powerdown(void);

/******************************************************************************
 * low level interface
 ******************************************************************************/
MIO_UBOOT_EXT int mio_init_without_ftl(void);
MIO_UBOOT_EXT int mio_deinit_without_ftl(void);
 
MIO_UBOOT_EXT int mio_nand_write(loff_t ofs, size_t *len, u_char *buf);
MIO_UBOOT_EXT int mio_nand_read(loff_t ofs, size_t *len, u_char *buf);
MIO_UBOOT_EXT int mio_nand_erase(loff_t ofs, size_t size);

/******************************************************************************
 * low level interface (no ecc + no randomize + no readretry)
 ******************************************************************************/
MIO_UBOOT_EXT int mio_nand_raw_write(loff_t ofs, size_t *len, u_char *buf);
MIO_UBOOT_EXT int mio_nand_raw_read(loff_t ofs, size_t *len, u_char *buf);
MIO_UBOOT_EXT int mio_nand_raw_erase(loff_t ofs, size_t size);

/******************************************************************************
 * etc
 ******************************************************************************/
//int get_nand_info(total/block/page/oob size...); ==> phy_features.nand_config

/******************************************************************************
 * for test
 ******************************************************************************/
MIO_UBOOT_EXT int mio_rwtest(ulong ulTestSectors, ulong ulCapacity, unsigned char ucWriteRatio, unsigned char ucSequentRatio);

#endif
