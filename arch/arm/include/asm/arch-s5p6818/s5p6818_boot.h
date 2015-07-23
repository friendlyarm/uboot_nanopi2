/*
 * (C) Copyright 2009
 * jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
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

#ifndef __S5P6818_BOOT_H__
#define __S5P6818_BOOT_H__

struct boot_dev_eeprom {
	char	addr_step;
	char	resv0[3];
	unsigned int resv1;
	unsigned int crc32;
};

struct boot_dev_mmc {
	char port_no;
	char resv0[3];
	char rese1;
	unsigned int crc32;		/* not use : s5p6818 */
};

union boot_dev_data {
	struct boot_dev_eeprom spirom;
	struct boot_dev_mmc mmc;
};

#define SIGNATURE_ID		((((U32)'N')<< 0) | (((U32)'S')<< 8) | (((U32)'I')<<16) | (((U32)'H')<<24))

struct boot_dev_head {
	unsigned int vector[8];			// 0x000 ~ 0x01C
	unsigned int vector_rel[8];		// 0x020 ~ 0x03C
	unsigned int dev_addr;			// 0x040

	/* boot device info */
	unsigned int  load_size;		// 0x044
	unsigned int  load_addr;		// 0x048
	unsigned int  jump_addr;		// 0x04C
	union boot_dev_data bdi;		// 0x050~0x058

	unsigned int  resv_pll[4];				// 0x05C ~ 0x068
	unsigned int  resv_pll_spread[2];		// 0x06C ~ 0x070
	unsigned int  resv_dvo[5];				// 0x074 ~ 0x084
	char resv_ddr[36];				// 0x088 ~ 0x0A8
	unsigned int  resv_axi_b[32];		// 0x0AC ~ 0x128
	unsigned int  resv_axi_d[32];		// 0x12C ~ 0x1A8
	unsigned int  resv_stub[(0x1F8-0x1A8)/4];	// 0x1AC ~ 0x1F8
	unsigned int  signature;			// 0x1FC	"NSIH"
};

#endif /*	__S5P6818_BOOT_H__ */

