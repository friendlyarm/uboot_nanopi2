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
#include <common.h>
#include <command.h>
#include <config.h>
#include <asm/gpio.h>
#include <i2c.h>
#include "i2c_dev.h"

//#define	I2C_DBG
#if defined(I2C_DBG)
#define	pr_debug(m...)		printf(m)
#else
#define	pr_debug(m...)
#endif

/*
 * I2C control macro
 */
#define I2C_SCL_H(io)		gpio_direction_output(io, 1)
#define I2C_SCL_L(io)		gpio_direction_output(io, 0)
#define I2C_SCL_I(io)		gpio_direction_input(io)
#define I2C_SCL_R(io)		gpio_get_value(io)
#define I2C_SDA_H(io)		gpio_direction_output(io, 1)
#define I2C_SDA_L(io)		gpio_direction_output(io, 0)
#define I2C_SDA_I(io)		gpio_direction_input(io)
#define I2C_SDA_R(io)		gpio_get_value(io)

/* 			________		 ___________		______
 *	<SDA>	 		|_______|			|_______|
 *			 <1>|<1>|SHT|DHT|DST|CHT|DHT|DST|EST|<1>
 *			____________		 ___		 __________
 *	<SCL>		 		|_______|	|_______|
 *			 			    <DST|CHT|DHT> : 1cycle
 */
#define	I2C_DELAY_HZ		100000

#define	_SHT_		(1)		/* start  hold  time */
#define	_EST_		(1)		/* Stop   setup time */
#define	_DHT_		(1)		/* data   hold  time */
#define	_DST_		(1)		/* data   setup time */
#define	_CHT_		(1)		/* clock  hold  time */

#define I2C_DELAY(n, t)	udelay(n * t);

static struct i2c_dev * i2c_ptr __attribute__ ((section(".data"))) = NULL;
static int i2c_bus __attribute__ ((section(".data"))) = 0;
static int __i2c_gpio_init(int bus) { return -1; }
int i2c_gpio_init(int bus) __attribute__((weak, alias("__i2c_gpio_init")));

static void send_start(struct i2c_dev *i2c)
{
	int scl = i2c->scl, sda = i2c->sda;
	int d = i2c->delay;

	/* SCL/SDA High */
	I2C_SDA_H(sda);
	I2C_DELAY(1, d);

	/* START signal */
	I2C_SCL_H(scl);
	I2C_DELAY(1, d);
	I2C_SDA_L(sda);
	I2C_DELAY(_SHT_, d);	/* Start hold */

	I2C_SCL_L(scl);
	I2C_DELAY(_DHT_, d);	/* Data  hold */
}

static void send_stop(struct i2c_dev *i2c)
{
	int scl = i2c->scl, sda = i2c->sda;
	int d = i2c->delay;

	/* STOP signal */
	I2C_SDA_L(sda);
	I2C_DELAY(_DST_, d);
	I2C_SCL_H(scl);
	I2C_DELAY(_EST_, d);
	I2C_SDA_H(sda);
	I2C_DELAY(1, d);

	I2C_SCL_I(scl);
	I2C_SDA_I(sda);
}

static int write_byte(struct i2c_dev *i2c, unsigned char data)
{
	int scl = i2c->scl, sda = i2c->sda;
	int d = i2c->delay;
	int i, nack = 0;

	I2C_SDA_H(sda);

	for (i=7 ; i >= 0 ; i--) {
		if (data & (1<<i))
			I2C_SDA_H(sda);
		else
			I2C_SDA_L(sda);

		I2C_DELAY(_DST_, d);
		I2C_SCL_H(scl);
		I2C_DELAY(_CHT_, d);
		I2C_SCL_L(scl);
		I2C_DELAY(_DHT_, d);
	}

	I2C_SDA_I(sda);
	I2C_DELAY(_DST_, d);
	I2C_SCL_H(scl);
	I2C_DELAY(_CHT_, d);

	nack = I2C_SDA_R(sda);

	I2C_SCL_L(scl);
	I2C_DELAY(_DHT_, d);
	I2C_SDA_I(sda);	/* END */

	return (nack ? -1 : 0);
}

static uchar read_byte(struct i2c_dev *i2c, bool ack)
{
	int scl = i2c->scl, sda = i2c->sda;
	int d = i2c->delay;
	uchar data = 0;
	int i;

	I2C_SDA_I(sda);

	for (i=7; i >= 0; i--) {
		I2C_DELAY(_DST_, d);
		I2C_SCL_H(scl);
		I2C_DELAY(_CHT_, d);

		/* Falling Edge */
		if (I2C_SDA_R(sda))
			data = (unsigned char)(data | (1<<i));
		else
			data = (unsigned char)(data | (0<<i));

		I2C_SCL_L(scl);
		I2C_DELAY(_DHT_, d);
	}
	I2C_SDA_H(sda);

	if (ack)
		I2C_SDA_L(sda);
	else
		I2C_SDA_H(sda);

	I2C_DELAY(_DST_, d);
	I2C_SCL_H(scl);
	I2C_DELAY(_CHT_, d);
	I2C_SCL_L(scl);
	I2C_DELAY(_DHT_, d);
	I2C_SDA_I(sda);	/* END */

	return data;
}

/*
 * I2C API
 */
int i2c_write(u8 chip, u32 addr, int alen, u8 *buffer, int len)
{
	struct i2c_dev *i2c = i2c_ptr;
	int nostop = i2c->nostop;
	u8 addr_bytes[3]; /* lowest...highest byte of data address */
	u8 data;
	int ret  = -1;

	chip = (chip<<1);

#if defined(I2C_DBG)
	int i = 0;
	printf("i2c.%d: W chip=0x%2x, addr=0x%x, alen=%d, wlen=%d ",
		i2c_bus, chip, addr, alen, len);
	for (; len > i; i++)
		printf("i2c.%d:0x%2x ", i, buffer[i]);
	printf("\n");
#endif

	if (NULL == i2c_ptr) {
		i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
		i2c = i2c_ptr;
	}

	/*
	 * send memory address bytes;
	 * alen defines how much bytes we have to send.
	 */
	addr_bytes[0] = (u8)((addr >>  0) & 0x000000FF);
	addr_bytes[1] = (u8)((addr >>  8) & 0x000000FF);
	addr_bytes[2] = (u8)((addr >> 16) & 0x000000FF);

	/* transfer : slave addresss */
	send_start(i2c);
	data = chip;
	ret  = write_byte(i2c, data);
	if (ret) {
		printf("Fail, i2c.%d start wait ack, addr:0x%02x \n", i2c->bus, data);
		goto exit_w;
	}

	/* transfer : regsiter addr */
	while (--alen >= 0) {
		data = addr_bytes[alen];
		ret  = write_byte(i2c, data);
		if (ret) {
			printf("Fail, i2c.%d no ack data [0x%2x] \n", i2c->bus, data);
			goto exit_w;
		}
	}

	/* transfer : data */
	while (len--) {
		data = *(buffer++);
		ret  = write_byte(i2c, data);
		if (ret) {
			printf("Fail, i2c.%d no ack data [0x%2x] \n", i2c->bus, data);
			goto exit_w;
		}
	}

#if defined(I2C_DBG)
	printf("i2c.%d: W done chip=0x%2x \n", i2c->bus, chip);
#endif

exit_w:
	/* transfer : end */
	if (ret || 0 == nostop)
		send_stop(i2c);

	return ret;
}

int i2c_read (u8 chip, uint addr, int alen, u8 *buffer, int len)
{
	struct i2c_dev *i2c = i2c_ptr;
	u8  data;
	int ret  = -1;

#if defined(I2C_DBG)
	printf("i2c.%d: R chip=0x%2x, addr=0x%x, alen=%d, rlen=%d\n",
		i2c->bus, (chip<<1)|0x1, addr, alen, len);
#endif

	/* transfer : register addr */
	if (0 > i2c_write(chip, addr, alen, NULL, 0))
		return ret;

	/* transfer : slave addresss */
	send_start(i2c);
	data = (chip<<1) | 0x01;
	ret  = write_byte(i2c, data);
	if (ret) {
		printf("Fail, i2c.%d start wait ack, addr:0x%02x \n", i2c->bus, data);
		goto exit_r;
	}

	/* transfer : read data */
	while (len--) {
		int ack = (len == 0) ? 0: 1;
		*buffer = read_byte(i2c, ack);
		 buffer++;
	}

exit_r:
	send_stop(i2c);

	return ret;
}

/*
 * params in:
 * 		chip = slave addres
 * return:
 *		0 = detect else not detect
 */
int i2c_probe(u8 chip)
{
	struct i2c_dev *i2c = i2c_ptr;
	u8 data;
	int ret = -1;

	if (NULL == i2c_ptr){
		i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
		i2c = i2c_ptr;
	}

	/* test with tx: probe ragne 0 ~ 128 */
	chip <<= 1;
	pr_debug("i2c.%d: chip=0x%02x, speed=%d\n", i2c->bus, chip, i2c->speed);

	/* transfer : slave addresss */
	data = chip;
	send_start(i2c);
	ret = write_byte(i2c, data);
	send_stop(i2c);

	return ret;
}

#if defined(CONFIG_I2C_MULTI_BUS)
int i2c_set_bus_num (unsigned int bus)
{
	struct i2c_dev *i2c;

	if (0 > i2c_gpio_init(bus))
		return -1;

	i2c = &i2c_devices[bus];

	pr_debug("i2c.%d set bus=%d, speed=%d (%s)[%d,%d]\n",
		i2c_bus, bus, i2c->speed, i2c->nostop?"nostop":"stop", i2c->scl, i2c->sda);

	i2c_ptr = i2c;
	i2c_bus = bus;

	i2c_set_bus_speed(i2c->speed);

	return 0;
}

unsigned int i2c_get_bus_num(void)
{
	pr_debug("i2c.%d: get bus=%d\n", i2c_bus, i2c_bus);
	return i2c_bus;
}
#endif

int i2c_set_bus_speed(unsigned int speed)
{
	struct i2c_dev *i2c;
	int bus = i2c_bus;

	if (0 > i2c_gpio_init(i2c_bus))
		return -1;

	i2c = &i2c_devices[bus];
	i2c->speed = speed;

#ifdef CONFIG_MMU_ENABLE
	speed = (speed > I2C_DELAY_HZ) ? speed : I2C_DELAY_HZ;
	i2c->delay = 1000000/(3 * speed);
#else
	speed = (speed > I2C_DELAY_HZ) ? speed : I2C_DELAY_HZ;
	i2c->delay = ((1000000/(speed) - 7) / 3) ;
#endif

	if(1 > i2c->delay)
  	 	i2c->delay = 1;

	pr_debug("i2c.%d set speed=%d, delay=%d (%s)\n",
		bus, i2c->speed, i2c->delay, i2c->nostop?"nostop":"stop");
	return 0;
}

unsigned int i2c_get_bus_speed(void)
{
	struct i2c_dev *i2c = i2c_ptr;
	if (NULL == i2c_ptr)
		return 0;
	return i2c->speed;
}

void i2c_init(int speed, int slaveaddr)
{
	int bus = i2c_bus;

    i2c_set_bus_num(bus);
	i2c_set_bus_speed(speed);
}

/*
 *  To support non stop mode
 */
int do_i2c_mode (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *cmd;

	cmd = argv[1];
	if (strcmp(cmd, "stop") == 0) {
		printf("Set i2c.%d stop mode  \n", i2c_bus);
		i2c_devices[i2c_bus].nostop = 0;
		return 0;
	} else if (strcmp(cmd, "nostop") == 0) {
		printf("Set i2c.%d nostop mode \n", i2c_bus);
		i2c_devices[i2c_bus].nostop = 1;
		return 0;
	} else {
		printf("Current i2c.%d %s mode  \n",
			i2c_bus, i2c_devices[i2c_bus].nostop?"nostop":"stop");
	}
	return 1;
}

U_BOOT_CMD(
	i2cmod, 3, 1,	do_i2c_mode,
	"set I2C mode",
	"stop\n"
	"    - generate stop signal, when tx end (normal)\n"
	"i2cmod nostop\n"
	"    - skip stop signal, when tx end (restart)\n"
);

