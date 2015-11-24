/*
 * (C) Copyright 2015
 *  Jongshin Park, Nexell Co, <pjsin865@nexell.co.kr>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <errno.h>
#include <i2c.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <power/pmic.h>
#include <power/power_chrg.h>
#include <power/axp228.h>

static u32 axp228_regu_get_vol_step(int want_vol, int step, int min, int max)
{
	u32	vol_step = 0;
	//u32	temp = 0;

	if (want_vol < min)
	{
		want_vol = min;
	}
	else if (want_vol > max)
	{
		want_vol = max;
	}

	//temp		= (want_vol - min);
	//vol_step	= (temp / step);

	vol_step = (want_vol - min + step - 1) / step;

	return	(u32)(vol_step & 0xFF);
}


static int axp228_regu_get_voltate(struct pmic *p, int type)
{
	int ret = -1;
	//u32 val = 0;

	PMIC_DBGOUT("%s\n", __func__);

	switch(type)
	{
		case REGULATOR_ARM:
			if(p->regu->arm_vol > 0)
				ret = p->regu->arm_vol;
			else
				ret = -1;
			break;

		case REGULATOR_CORE:
			if(p->regu->core_vol > 0)
				ret = p->regu->core_vol;
			else
				ret = -1;
			break;
	}

	return ret;
}

static int axp228_regu_set_voltage(struct pmic *p, int type, int vol)
{
	int ret = 0;
	u32 val = 0;

	PMIC_DBGOUT("%s\n", __func__);

	if (pmic_probe(p))
		return -1;

	switch(type)
	{
		case REGULATOR_ARM:
			/* REG 22H:DCDC2 Output Voltage Set */
			val = axp228_regu_get_vol_step(vol, AXP22_DCDC2_STEP, AXP22_DCDC2_MIN, AXP22_DCDC2_MAX);
			pmic_reg_write(p, AXP22_DC2OUT_VOL, val);
			p->regu->arm_vol = vol;
			break;

		case REGULATOR_CORE:
			/* REG 23H:DCDC3 Output Voltage Set */
			val = axp228_regu_get_vol_step(vol, AXP22_DCDC3_STEP, AXP22_DCDC3_MIN, AXP22_DCDC3_MAX);
			pmic_reg_write(p, AXP22_DC3OUT_VOL, val);
			p->regu->core_vol = vol;
			break;
	}

	return ret;
}

static struct power_regulator power_regu_ops_axp228 = {
	.arm_vol = 0,
	.core_vol= 0,
	.reg_set_voltage = axp228_regu_set_voltage,
	.reg_get_voltate = axp228_regu_get_voltate,

};

int power_regu_init(unsigned int bus)
{
	static const char name[] = "REGU_AXP228";
	struct pmic *p = pmic_alloc();

	PMIC_DBGOUT("%s\n", __func__);

	if (!p) {
		printf("%s: POWER allocation error!\n", __func__);
		return -ENOMEM;
	}

	p->name = name;
	p->interface = PMIC_I2C;
	p->number_of_regs = AXP228_NUM_OF_REGS;
	p->hw.i2c.addr = AXP228_I2C_ADDR;
	p->hw.i2c.tx_num = 1;
	p->bus = bus;

	p->regu = &power_regu_ops_axp228;
	return 0;
}
