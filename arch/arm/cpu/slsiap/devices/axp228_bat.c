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
#include <power/pmic.h>
#include <power/battery.h>
#include <power/axp228.h>

static struct battery battery_axp228;

static int axp228_power_battery_charge(struct pmic *bat)
{
	int ret = 0;
	PMIC_DBGOUT("%s\n", __func__);

	return ret;
}

static int axp228_power_battery_init(struct pmic *bat_,
				    struct pmic *fg_,
				    struct pmic *chrg_,
				    struct pmic *muic_)
{
	int ret = 0;

	PMIC_DBGOUT("%s\n", __func__);

	bat_->pbat->fg = fg_;
	bat_->pbat->chrg = chrg_;
	bat_->pbat->muic = muic_;

	bat_->fg = fg_->fg;
	bat_->chrg = chrg_->chrg;
	bat_->chrg->chrg_type = muic_->chrg->chrg_type;

	return ret;
}

static struct power_battery power_bat_ops_axp228 = {
	.bat = &battery_axp228,
	.battery_init = axp228_power_battery_init,
	.battery_charge = axp228_power_battery_charge,
};

int power_bat_init(unsigned char bus)
{
	static const char name[] = "BAT_AXP228";
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

	p->pbat = &power_bat_ops_axp228;
	return 0;
}

