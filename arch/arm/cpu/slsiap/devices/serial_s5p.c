/*
 * (C) Copyright 2009 SAMSUNG Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Heungjun Kim <riverful.kim@samsung.com>
 *
 * based on drivers/serial/s3c64xx.c
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/compiler.h>
#include <asm/io.h>
#include <asm/arch/clk.h>
#include <serial.h>
#include "serial_s5p.h"

DECLARE_GLOBAL_DATA_PTR;

#define CONSOLE_PORT CONFIG_S5P_SERIAL_INDEX
static volatile unsigned char *const port = CONFIG_S5P_SERIAL_PORT;
static volatile unsigned int   const clock_in = CONFIG_S5P_SERIAL_CLOCK;

#define RX_FIFO_COUNT_MASK	0xff
#define RX_FIFO_FULL_MASK	(1 << 8)
#define TX_FIFO_FULL_MASK	(1 << 24)

static void __serial_device_init(void)
{
}
void serial_device_init(void)	__attribute__((weak, alias("__serial_device_init")));

static inline struct s5p_uart *s5p_get_base_uart(int dev_index)
{
	return (struct s5p_uart *)port;
}

/*
 * The coefficient, used to calculate the baudrate on S5P UARTs is
 * calculated as
 * C = UBRDIV * 16 + number_of_set_bits_in_UDIVSLOT
 * however, section 31.6.11 of the datasheet doesn't recomment using 1 for 1,
 * 3 for 2, ... (2^n - 1) for n, instead, they suggest using these constants:
 */
static const int udivslot[] = {
	0,
	0x0080,
	0x0808,
	0x0888,
	0x2222,
	0x4924,
	0x4a52,
	0x54aa,
	0x5555,
	0xd555,
	0xd5d5,
	0xddd5,
	0xdddd,
	0xdfdd,
	0xdfdf,
	0xffdf,
};

static void serial_setbrg_dev(const int dev_index)
{
	struct s5p_uart *const uart = s5p_get_base_uart(dev_index);
	u32 uclk = clock_in;
	u32 baudrate = gd->baudrate;
	u32 val;

	val = uclk / baudrate;

	writel(val / 16 - 1, &uart->ubrdiv);

	if (s5p_uart_divslot())
		writew(udivslot[val % 16], &uart->rest.slot);
	else
		writeb(val % 16, &uart->rest.value);
}

/*
 * Initialise the serial port with the given baudrate. The settings
 * are always 8 data bits, no parity, 1 stop bit, no start bits.
 */
static int serial_init_dev(const int dev_index)
{
	struct s5p_uart *const uart = s5p_get_base_uart(dev_index);

	serial_device_init();

	/* enable FIFOs, auto clear Rx FIFO */
	writel(0x3, &uart->ufcon);
	writel(0, &uart->umcon);
	/* 8N1 */
	writel(0x3, &uart->ulcon);
	/* No interrupts, no DMA, pure polling */
	writel(0x245, &uart->ucon);

	serial_setbrg_dev(dev_index);

	return 0;
}

static int serial_err_check(const int dev_index, int op)
{
	struct s5p_uart *const uart = s5p_get_base_uart(dev_index);
	unsigned int mask;

	/*
	 * UERSTAT
	 * Break Detect	[3]
	 * Frame Err	[2] : receive operation
	 * Parity Err	[1] : receive operation
	 * Overrun Err	[0] : receive operation
	 */
	if (op)
		mask = 0x8;
	else
		mask = 0xf;

	return readl(&uart->uerstat) & mask;
}

/*
 * Read a single byte from the serial port. Returns 1 on success, 0
 * otherwise. When the function is succesfull, the character read is
 * written into its argument c.
 */
static int serial_getc_dev(const int dev_index)
{
	struct s5p_uart *const uart = s5p_get_base_uart(dev_index);

	/* wait for character to arrive */
	while (!(readl(&uart->ufstat) & (RX_FIFO_COUNT_MASK |
					 RX_FIFO_FULL_MASK))) {
		if (serial_err_check(dev_index, 0))
			return 0;
	}

	return (int)(readb(&uart->urxh) & 0xff);
}

/*
 * Output a single byte to the serial port.
 */
static void serial_putc_dev(const char c, const int dev_index)
{
	struct s5p_uart *const uart = s5p_get_base_uart(dev_index);

	/* wait for room in the tx FIFO */
	while ((readl(&uart->ufstat) & TX_FIFO_FULL_MASK)) {
		if (serial_err_check(dev_index, 1))
			return;
	}

	writeb(c, &uart->utxh);

	/* If \n, also do \r */
	if (c == '\n')
		serial_putc('\r');
}

/*
 * Test whether a character is in the RX buffer
 */
static int serial_tstc_dev(const int dev_index)
{
	struct s5p_uart *const uart = s5p_get_base_uart(dev_index);
	return (int)(readl(&uart->utrstat) & 0x1);
}

static void serial_puts_dev(const char *s, const int dev_index)
{
	while (*s)
		serial_putc_dev(*s++, dev_index);
}

static int s5p_serial_init(void) 			{ return serial_init_dev(CONSOLE_PORT); }
static void s5p_serial_setbrg(void) 		{ serial_setbrg_dev(CONSOLE_PORT); }
static int s5p_serial_getc(void) 			{ return serial_getc_dev(CONSOLE_PORT); }
static int s5p_serial_tstc(void) 			{ return serial_tstc_dev(CONSOLE_PORT); }
static void s5p_serial_putc(const char c) 	{ serial_putc_dev(c, CONSOLE_PORT); }
static void s5p_serial_puts(const char *s) 	{ serial_puts_dev(s, CONSOLE_PORT); }

static struct serial_device s5p_serial_device = {
	.name	= "serial_s5p",
	.start	= s5p_serial_init,
	.stop	= NULL,
	.setbrg	= s5p_serial_setbrg,
	.getc	= s5p_serial_getc,
	.tstc	= s5p_serial_tstc,
	.putc	= s5p_serial_putc,
	.puts	= s5p_serial_puts,
};

__weak struct serial_device *default_serial_console(void)
{
	return &s5p_serial_device;
}

void s5p_serial_initialize(void)
{
	serial_register(&s5p_serial_device);
}
