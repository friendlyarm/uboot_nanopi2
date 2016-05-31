/*
 * Copyright (C) Guangzhou FriendlyARM Computer Tech. Co., Ltd.
 * (http://www.friendlyarm.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <platform.h>
#include <mach-api.h>
#include <i2c.h>

/* Board revision list: <PCB3 | PCB2 | PCB1>
 *  0b000 - NanoPi 2
 *  0b001 - NanoPC-T2
 *  0b010 - NanoPi S2
 *  0b011 - Smart4418
 *  0b100 - NanoPi 2 Fire
 *  0b101 - NanoPi M2
 *
 * Extented revision:
 *  0b001 - Smart4418-SDK
 */
#define __IO_GRP		PAD_GET_GROUP(PAD_GPIO_C)
#define __IO_PCB1			26
#define __IO_PCB2			27
#define __IO_PCB3			25

static int pcb_rev	= 0;
static int base_rev	= 0;


static void smart4418_rev_init(void)
{
	u8 val = 0;

#define PCA9536_I2C_BUS		2
#define PCA9636_I2C_ADDR	0x41
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	i2c_set_bus_num(PCA9536_I2C_BUS);

	if (i2c_probe(PCA9636_I2C_ADDR))
		return;

	if (!i2c_read(PCA9636_I2C_ADDR, 0, 1, &val, 1))
		base_rev = (val & 0xf);
}

void board_rev_init(void)
{
	/* Get core revision */
	pcb_rev  = NX_GPIO_GetInputValue(__IO_GRP, __IO_PCB1);
	pcb_rev |= NX_GPIO_GetInputValue(__IO_GRP, __IO_PCB2) << 1;
	pcb_rev |= NX_GPIO_GetInputValue(__IO_GRP, __IO_PCB3) << 2;

	/* Get extended revision for SmartXX18 */
	if (pcb_rev == 0x3)
		smart4418_rev_init();
}

void board_show_rev(void)
{
	printf("REV  = %d.%d\n", pcb_rev, base_rev);
}

/* To override __weak symbols */
u32 get_board_rev(void)
{
	return (base_rev << 8) | pcb_rev;
}

const char *get_board_name(void)
{
	switch (pcb_rev) {
	case 0:
		return "NanoPi 2";
	case 1:
		return "NanoPC-T2";
	case 2:
		return "NanoPi S2";
	case 3:
		if (base_rev == 0)
			return "Smart4418-core";
		else
			return "Smart4418-SDK";
	case 4:
		return "NanoPi 2 Fire";
	case 5:
		return "NanoPi M2";
	default:
		return "s5p4418-X";
	}
}

