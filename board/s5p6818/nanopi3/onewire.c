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
#include <onewire.h>
#include <i2c.h>

#define SAMPLE_BPS		9600
#define SAMPLE_IN_US	104		/* (1000000 / BPS) */

#define REQ_INFO		0x60U
#define REQ_BL			0x80U

#define BUS_I2C			0x18
#define ONEWIRE_I2C_BUS		2
#define ONEWIRE_I2C_ADDR	0x2f

static int bus_type = -1;
static int lcd_id = -1;
static unsigned short lcd_fwrev = 0;
static int current_brightness = -1;

/* debug */
#if (0)
#define DBGOUT(msg...)	do { printf("onewire: " msg); } while (0)
#else
#define DBGOUT(msg...)	do {} while (0)
#endif

/* based on web page from http://lfh1986.blogspot.com */
static unsigned char crc8(unsigned v, unsigned len)
{
	unsigned char crc = 0xACU;
	while (len--) {
		if (( crc & 0x80U) != 0) {
			crc <<= 1;
			crc ^= 0x7U;
		} else {
			crc <<= 1;
		}
		if ( (v & (1U << 31)) != 0) {
			crc ^= 0x7U;
		}
		v <<= 1;
	}
	return crc;
}

/* GPIO helpers */
#define __IO_GRP		PAD_GET_GROUP(CFG_ONEWIRE_IO)
#define __IO_IDX		PAD_GET_BITNO(CFG_ONEWIRE_IO)

static inline void set_pin_as_input(void) {
	NX_GPIO_SetOutputEnable(__IO_GRP, __IO_IDX, 0);
}

static inline void set_pin_as_output(void) {
	NX_GPIO_SetOutputEnable(__IO_GRP, __IO_IDX, 1);
}

static inline void set_pin_value(int v) {
	NX_GPIO_SetOutputValue(__IO_GRP, __IO_IDX, !!v);
}

static inline int get_pin_value(void) {
	return NX_GPIO_GetInputValue(__IO_GRP, __IO_IDX);
}

/* Timer helpers */
static void wait_loops(int n) {
	while (--n)
		nop();

	asm volatile("" : : : "memory");
}

static inline void wait_one_tick(void) {
	__udelay(SAMPLE_IN_US);

	/* extra loop to get 9600 Hz */
	wait_loops(50);
}

/* Session handler */
static int onewire_session(unsigned char req, unsigned char res[])
{
	unsigned Req;
	unsigned *Res;
	int ints = disable_interrupts();
	int i;
	int ret;

	Req = (req << 24) | (crc8(req << 24, 8) << 16);
	Res = (unsigned *)res;

	set_pin_value(1);
	set_pin_as_output();
	for (i = 0; i < 60; i++) {
		wait_one_tick();
	}

	set_pin_value(0);
	for (i = 0; i < 2; i++) {
		wait_one_tick();
	}

	for (i = 0; i < 16; i++) {
		int v = !!(Req & (1U <<31));
		Req <<= 1;
		set_pin_value(v);
		wait_one_tick();
	}

	wait_one_tick();
	set_pin_as_input();
	wait_one_tick();
	for (i = 0; i < 32; i++) {
		(*Res) <<= 1;
		(*Res) |= get_pin_value();
		wait_one_tick();
	}
	set_pin_value(1);
	set_pin_as_output();

	if (ints) {
		enable_interrupts();
	}

	ret = crc8(*Res, 24) == res[0];
	DBGOUT("req = %02X, res = %02X%02X%02X%02X, ret = %d\n",
			req, res[3], res[2], res[1], res[0], ret);

	return ret;
}

static int onewire_i2c_do_request(unsigned char req, unsigned char *buf)
{
	unsigned char tx[4];
	int ret;

	tx[0] = req;
	tx[1] = crc8(req << 24, 8);
	if (i2c_write(ONEWIRE_I2C_ADDR, 0, 0, tx, 2))
		return -EIO;

	if (!buf) /* NO READ */
		return 0;

	if (i2c_read(ONEWIRE_I2C_ADDR, 0, 0, buf, 4))
		return -EIO;

	ret = crc8((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8), 24);
	DBGOUT("req = %02X, res = %02X%02X%02X%02X, ret = %02x\n",
			req, buf[0], buf[1], buf[2], buf[3], ret);

	return (ret == buf[3]) ? 0 : -EIO;
}

static void onewire_i2c_init(void)
{
	unsigned char buf[4];
	int ret;

	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	i2c_set_bus_num(ONEWIRE_I2C_BUS);

	if (i2c_probe(ONEWIRE_I2C_ADDR))
		return;

	ret = onewire_i2c_do_request(REQ_INFO, buf);
	if (!ret) {
		lcd_id = buf[0];
		lcd_fwrev = buf[1] * 0x100 + buf[2];
		bus_type = BUS_I2C;
	}
}

void onewire_init(void)
{
	/* See include/cfg_gpio.h */
	NX_GPIO_SetPadFunction(__IO_GRP, __IO_IDX, NX_GPIO_PADFUNC_1);
	NX_GPIO_SetPullEnable(__IO_GRP, __IO_IDX, NX_GPIO_PULL_OFF);

	onewire_i2c_init();
}

int onewire_get_info(unsigned char *lcd, unsigned short *fw_ver)
{
	unsigned char res[4];
	int i;

	if (bus_type == BUS_I2C && lcd_id > 0) {
		*lcd = lcd_id;
		*fw_ver = lcd_fwrev;
		return 0;
	}

	for (i = 0; i < 3; i++) {
		if (onewire_session(REQ_INFO, res)) {
			*lcd = res[3];
			*fw_ver = res[2] * 0x100 + res[1];
			lcd_id = *lcd;
			DBGOUT("lcd = %d, fw_ver = %x\n", *lcd, *fw_ver);
			return 0;
		}
	}

	/* LCD unknown or not connected */
	*lcd = 0;
	*fw_ver = -1;

	return -1;
}

int onewire_get_lcd_id(void)
{
	return lcd_id;
}

int onewire_set_backlight(int brightness)
{
	unsigned char res[4];
	int i;

	if (brightness == current_brightness)
		return 0;

	if (brightness > 127)
		brightness = 127;
	else if (brightness < 0)
		brightness = 0;

	if (bus_type == BUS_I2C) {
		onewire_i2c_do_request((REQ_BL | brightness), NULL);
		current_brightness = brightness;
		return 0;
	}

	for (i = 0; i < 3; i++) {
		if (onewire_session((REQ_BL | brightness), res)) {
			current_brightness = brightness;
			return 0;
		}
	}

	return -1;
}

