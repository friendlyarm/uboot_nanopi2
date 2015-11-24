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

#if defined(CONFIG_NXP_DWC_OTG_PHY)
#include <dwc_otg_phy.h>
#endif

static int axp228_muic_chrg_get_type(struct pmic *p, u32 ctrl_en)
{
	int ret = 0;
	u32 val = 0;

	PMIC_DBGOUT("%s\n", __func__);

	if (pmic_probe(p))
		return CHARGER_UNKNOWN;

	pmic_reg_read(p, AXP22_STATUS, &val);
	val &= 0xFF;
	PMIC_DBGOUT("read reg : 0x%02x, 0x%02x\n", AXP22_STATUS, val);

	if(val & AXP22_STATUS_ACEN)
	{
		ret = CHARGER_TA;
	}
	else if(val & AXP22_STATUS_USBEN)
	{
#if defined(CONFIG_FASTBOOT) && defined(CONFIG_SW_UBC_DETECT)
		if(otg_bind_check(500))
#else
		if(1)
#endif
		{
			axp228_usb_limit_set(p, AXP228_USB_LIMIT_500);
			ret = CHARGER_USB;
		}
		else
		{
			axp228_usb_limit_set(p, AXP228_USB_LIMIT_NO);
			ret = CHARGER_TA;
		}
	}
	else
	{
		axp228_usb_limit_set(p, AXP228_USB_LIMIT_500);
		ret = CHARGER_NO;
	}

	return ret;
}

static struct power_chrg power_muic_ops_axp228 = {
	.chrg_type = axp228_muic_chrg_get_type,
};

int power_muic_init(unsigned int bus)
{
	static const char name[] = "MUIC_AXP228";
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

	p->chrg = &power_muic_ops_axp228;
	return 0;
}
