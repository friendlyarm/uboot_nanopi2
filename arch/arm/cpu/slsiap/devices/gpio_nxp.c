/*
 *  Copyright (C) 2013 NEXELL SOC Lab.
 *  BongKwan Kook <kook@nexell.co.kr>
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

#include <platform.h>
#include <common.h>
#include <asm/io.h>
#include <asm/gpio.h>

/* Common GPIO API */
int gpio_request(unsigned gpio, const char *label)
{
	return 0;
}

int gpio_free(unsigned gpio)
{
	return 0;
}

int gpio_direction_input(unsigned gpio)
{
	int grp = PAD_GET_GROUP(gpio);
	int bit = PAD_GET_BITNO(gpio);

	if (grp < PAD_GET_GROUP(PAD_GPIO_ALV))
		NX_GPIO_SetOutputEnable(grp, bit, CFALSE);
	else
		NX_ALIVE_SetOutputEnable(bit, CFALSE);

	return 0;
}

int gpio_direction_output(unsigned gpio, int value)
{
	int grp = PAD_GET_GROUP(gpio);
	int bit = PAD_GET_BITNO(gpio);

	if (grp < PAD_GET_GROUP(PAD_GPIO_ALV)) {
		NX_GPIO_SetOutputValue(grp, bit, (value ? CTRUE : CFALSE));
		NX_GPIO_SetOutputEnable(grp, bit, CTRUE);
	} else {
		NX_ALIVE_SetOutputValue(bit, (value ? CTRUE : CFALSE));
		NX_ALIVE_SetOutputEnable(bit, CTRUE);
	}
	return 0;
}

int gpio_get_value(unsigned gpio)
{
	int grp = PAD_GET_GROUP(gpio);
	int bit = PAD_GET_BITNO(gpio);

	if (grp < PAD_GET_GROUP(PAD_GPIO_ALV))
		return (int) NX_GPIO_GetInputValue(grp, bit);
	else
		return (int) NX_ALIVE_GetInputValue(bit);
}

int gpio_set_value(unsigned gpio, int value)
{
	int grp = PAD_GET_GROUP(gpio);
	int bit = PAD_GET_BITNO(gpio);

	if (grp < PAD_GET_GROUP(PAD_GPIO_ALV))
		NX_GPIO_SetOutputValue(grp, bit, (value ? CTRUE : CFALSE));
	else
		NX_ALIVE_SetOutputValue(bit, (value ? CTRUE : CFALSE));

	return 0;
}

/* Nexell GPIO API */
int gpio_get_int_pend(int gpio)
{
	int grp = PAD_GET_GROUP(gpio);
	int bit = PAD_GET_BITNO(gpio);

	if (grp < PAD_GET_GROUP(PAD_GPIO_ALV))
		return (int) NX_GPIO_GetInterruptPending(grp, bit);
	else
		return (int) NX_ALIVE_GetInterruptPending(bit);
}

void gpio_set_int_clear(int gpio)
{
	int grp = PAD_GET_GROUP(gpio);
	int bit = PAD_GET_BITNO(gpio);

	if (grp < PAD_GET_GROUP(PAD_GPIO_ALV))
		NX_GPIO_ClearInterruptPending(grp, bit);
	else
		NX_ALIVE_ClearInterruptPending(bit);
}

void gpio_set_alt(int gpio, int mode)
{
	int grp = PAD_GET_GROUP(gpio);
	int bit = PAD_GET_BITNO(gpio);

	if (grp > (PAD_GET_GROUP(PAD_GPIO_ALV) - 1) )
		return;

	NX_GPIO_SetPadFunction(grp, bit, (NX_GPIO_PADFUNC)mode);
}

int gpio_get_alt(int gpio)
{
	int grp = PAD_GET_GROUP(gpio);
	int bit = PAD_GET_BITNO(gpio);

	if (grp > (PAD_GET_GROUP(PAD_GPIO_ALV) - 1) )
		return -1;

	return (int) NX_GPIO_GetPadFunction(grp, bit);
}
