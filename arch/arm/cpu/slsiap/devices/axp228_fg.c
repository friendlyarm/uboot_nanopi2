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
#include <power/pmic.h>
#include <power/power_chrg.h>
#include <power/battery.h>
#include <power/axp228.h>

static int axp228_power_check_ibatt(struct pmic *p, struct pmic *bat)
{
	u32 tmp[2];
	u16 ichar_res;
	u16 idischar_res;
	u16 sum_ichar = 0;
	u16 sum_idischar = 0;
	int i;

	PMIC_DBGOUT("%s\n", __func__);

	for(i=0; i<5; i++)
	{
		mdelay(1);
		pmic_reg_read(p, AXP22_ICHGH_RES, &tmp[0]);
		pmic_reg_read(p, AXP22_ICHGL_RES, &tmp[1]);
		ichar_res = ((u16) tmp[0] << 8 )| tmp[1];

		sum_ichar += ichar_res;
	}
	sum_ichar = sum_ichar/5;


	for(i=0; i<5; i++)
	{
		mdelay(1);
		pmic_reg_read(p, AXP22_DISICHGH_RES, &tmp[0]);
		pmic_reg_read(p, AXP22_DISICHGL_RES, &tmp[1]);
		idischar_res = ((u16) tmp[0] << 8 )| tmp[1];

		sum_idischar += ichar_res;
	}
	sum_idischar = sum_idischar/5;

	return ABS(axp22_icharge_to_mA(sum_ichar)-axp22_ibat_to_mA(sum_idischar));
}

static int axp228_power_update_battery(struct pmic *p, struct pmic *bat)
{
	struct power_battery *pb = bat->pbat;
	u32 tmp[2];
	u16 cap_res = 0;
	int ret = 0;

	PMIC_DBGOUT("%s\n", __func__);

	pmic_reg_read(p, AXP22_VBATH_RES, &tmp[0]);
	pmic_reg_read(p, AXP22_VBATL_RES, &tmp[1]);
	pb->bat->voltage_uV = (axp22_vbat_to_mV(((u16) tmp[0] << 8 )| tmp[1])) * 1000;


	pmic_reg_read(p, AXP22_CAP, &tmp[0]);

	cap_res = tmp[0] & 0x7F;

#if 0 
	pmic_reg_read(p, AXP22_STATUS+0x1, &tmp[0]);

	if(tmp & AXP22_STATUS_BATEN)
        bat_det = 1;

	if((bat_det == 0) || (cap_res == 127))
	{
		cap_res = 100;
	}
#endif
	pb->bat->capacity = cap_res;

	return ret;
}

static int axp228_power_check_battery(struct pmic *p, struct pmic *bat)
{
	struct power_battery *pb = bat->pbat;
	u32 tmp[2];
	u16 vbat_res = 0;
	u16 cap_res = 0;
	u16 sum_tmp = 0;
	//u8 bat_det= 0;
	int i;
	int ret = 0;

	PMIC_DBGOUT("%s\n", __func__);

	for(i=0; i<5; i++)
	{
		mdelay(1);
		pmic_reg_read(p, AXP22_VBATH_RES, &tmp[0]);
		pmic_reg_read(p, AXP22_VBATL_RES, &tmp[1]);

		vbat_res = axp22_vbat_to_mV(((u16) tmp[0] << 8 )| tmp[1]);

		sum_tmp += vbat_res;
	}
	pb->bat->voltage_uV = (sum_tmp/5) * 1000;


	pmic_reg_read(p, AXP22_CAP, &tmp[0]);

	cap_res = tmp[0] & 0x7F;

#if 0 
	pmic_reg_read(p, AXP22_STATUS+0x1, &tmp[0]);

	if(tmp & AXP22_STATUS_BATEN)
        bat_det = 1;

	if((bat_det == 0) || (cap_res == 127))
	{
		cap_res = 100;
	}
#endif

	pb->bat->capacity = cap_res;

	return ret;
}

static struct power_fg power_fg_ops_axp228 = {
	.fg_battery_check = axp228_power_check_battery,
	.fg_battery_update = axp228_power_update_battery,
	.fg_ibatt = axp228_power_check_ibatt,
};

int power_fg_init(unsigned char bus)
{
	static const char name[] = "FG_AXP228";
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

	p->fg = &power_fg_ops_axp228;
	return 0;
}
