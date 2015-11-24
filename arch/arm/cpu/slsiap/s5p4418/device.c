/*
 * (C) Copyright 2009 Nexell Co.,
 * jung hyun kim<jhkim@nexell.co.kr>
 *
 * Configuation settings for the Nexell board.
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
#include <common.h>
#include <config.h>
#include <platform.h>
#include <asm/io.h>
#include "../devices/i2c_dev.h"

#if defined (CONFIG_CMD_I2C)

/* gpio i2c 0 */
#ifdef CFG_IO_I2C0_SCL
	#define	I2C0_SCL	CFG_IO_I2C0_SCL
#else
	#define	I2C0_SCL	((PAD_GPIO_D + 2) | PAD_FUNC_ALT0)
#endif
#ifdef CFG_IO_I2C0_SDA
	#define	I2C0_SDA	CFG_IO_I2C0_SDA
#else
	#define	I2C0_SDA	((PAD_GPIO_D + 3) | PAD_FUNC_ALT0)
#endif
/* gpio i2c 1 */
#ifdef CFG_IO_I2C1_SCL
	#define	I2C1_SCL	CFG_IO_I2C1_SCL
#else
	#define	I2C1_SCL	((PAD_GPIO_D + 4) | PAD_FUNC_ALT0)
#endif
#ifdef CFG_IO_I2C1_SDA
	#define	I2C1_SDA	CFG_IO_I2C1_SDA
#else
	#define	I2C1_SDA	((PAD_GPIO_D + 5) | PAD_FUNC_ALT0)
#endif
/* gpio i2c 2 */
#ifdef CFG_IO_I2C2_SCL
	#define	I2C2_SCL	CFG_IO_I2C2_SCL
#else
	#define	I2C2_SCL	((PAD_GPIO_D + 6) | PAD_FUNC_ALT0)
#endif
#ifdef CFG_IO_I2C2_SDA
	#define	I2C2_SDA	CFG_IO_I2C2_SDA
#else
	#define	I2C2_SDA	((PAD_GPIO_D + 7) | PAD_FUNC_ALT0)
#endif
#ifdef CFG_IO_I2C3_SCL
	#define	I2C3_SCL	CFG_IO_I2C3_SCL
#endif
#ifdef CFG_IO_I2C3_SDA
	#define	I2C3_SDA	CFG_IO_I2C3_SDA
#endif

struct i2c_dev i2c_devices[] = {
	{ .bus = 0, .scl = I2C0_SCL, .sda = I2C0_SDA, .speed = CONFIG_SYS_I2C_SPEED, .nostop = CONFIG_I2C0_NO_STOP, },
	{ .bus = 1, .scl = I2C1_SCL, .sda = I2C1_SDA, .speed = CONFIG_SYS_I2C_SPEED, .nostop = CONFIG_I2C1_NO_STOP, },
	{ .bus = 2, .scl = I2C2_SCL, .sda = I2C2_SDA, .speed = CONFIG_SYS_I2C_SPEED, .nostop = CONFIG_I2C2_NO_STOP, },
#if defined (CFG_IO_I2C3_SCL) || defined (CFG_IO_I2C3_SDA)
	{ .bus = 3, .scl = I2C3_SCL, .sda = I2C3_SDA, .speed = CONFIG_SYS_I2C_SPEED, .nostop = CONFIG_I2C3_NO_STOP, },
#endif
};

int i2c_gpio_init(int bus)
{
	int scl, sda;

	if (bus > ARRAY_SIZE(i2c_devices) - 1) {
		printf("i2c bus %d is not exist (max bus %d)\n", bus, ARRAY_SIZE(i2c_devices)-1);
		return -1;
	}

	scl = i2c_devices[bus].scl;
	sda = i2c_devices[bus].sda;

	NX_GPIO_SetPadFunction(PAD_GET_GROUP(scl), PAD_GET_BITNO(scl), PAD_GET_FUNC(scl));
	NX_GPIO_SetPadFunction(PAD_GET_GROUP(sda), PAD_GET_BITNO(sda), PAD_GET_FUNC(sda));

	return 0;
}
#endif
