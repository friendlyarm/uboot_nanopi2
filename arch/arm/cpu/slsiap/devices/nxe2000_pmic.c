/*
 *  Copyright (C) 2013 NEXELL SOC Lab.
 *  BongKwan Kook <kook@nexell.co.kr>
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

#include "nxe2000-private.h"
#include <nxe2000_power.h>

static int nxe2000_get_enable_reg(
		int pwr_src, u32 *reg, u32 *mask)
{
	switch (pwr_src) {
	case NXE2000_BUCK1 ... NXE2000_BUCK5:
		*reg = NXE2000_REG_DC1CTL + ((pwr_src - NXE2000_BUCK1) * 2);
		*mask = 1;
		break;
	case NXE2000_LDO1 ... NXE2000_LDO8:
		*reg = NXE2000_REG_LDOEN1;
		*mask = 1 << (pwr_src - NXE2000_LDO1);
		break;
    case NXE2000_LDO9 ... NXE2000_LDO10:
        *reg = NXE2000_REG_LDOEN2;
        *mask = 1 << (pwr_src - NXE2000_LDO9);
        break;
    case NXE2000_LDORTC1 ... NXE2000_LDORTC2:
        *reg = NXE2000_REG_LDOEN2;
        *mask = 1 << ((pwr_src - NXE2000_LDORTC1) + 4);
        break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int nxe2000_get_vol_reg(
		int pwr_src, u32 *_reg, u32 *_shift, u32 *_mask)
{
	u32 reg, shift = 0, mask = 0xff;

	switch (pwr_src) {
	case NXE2000_BUCK1 ... NXE2000_BUCK5:
		reg = NXE2000_REG_DC1VOL + (pwr_src - NXE2000_BUCK1);
		mask = 0xff;
		break;
	case NXE2000_LDO1 ... NXE2000_LDORTC2:
		reg = NXE2000_REG_LDO1VOL + (pwr_src - NXE2000_LDO1);
		mask = 0x7f;
		break;
	default:
		return -EINVAL;
	}

	*_reg = reg;
	*_shift = shift;
	*_mask = mask;

	return 0;
}

static u32 nxe2000_get_vol_val(
		int pwr_src, int set_uV)
{
	int min_uV, max_uV, step_uV;
    u32 val;

	val = 0;

	switch (pwr_src) {
	case NXE2000_BUCK1 ... NXE2000_BUCK5:
        min_uV  = 600000;
        max_uV  = 3500000;
        step_uV = 12500;
		break;
    case NXE2000_LDO5 ... NXE2000_LDO6:
        min_uV  = 600000;
        max_uV  = 3500000;
        step_uV = 25000;
        break;
	case NXE2000_LDO1 ... NXE2000_LDO4:
    case NXE2000_LDO7 ... NXE2000_LDO10:
    case NXE2000_LDORTC2:
        min_uV  = 900000;
        max_uV  = 3500000;
        step_uV = 25000;
		break;
    case NXE2000_LDORTC1:
        min_uV  = 1700000;
        max_uV  = 3500000;
        step_uV = 25000;
        break;
	default:
        return val;
	}

	if (set_uV < min_uV)
        set_uV = min_uV;
	if (set_uV > max_uV)
        set_uV = max_uV;

    val = (set_uV - min_uV) / step_uV;

	return val;
}

int nxe2000_set_vol(
    struct pmic *p, int pwr_src, int set_uV, int pwr_on)
{
	u32 reg, val, shift, mask;
    int ret = 0;

    if (pwr_on)
    {
        /* Set Voltage */
    	ret = nxe2000_get_vol_reg(pwr_src, &reg, &shift, &mask);
    	if (ret < 0)
    		return ret;

    	val = nxe2000_get_vol_val(pwr_src, set_uV);
    	ret = pmic_reg_write(p, reg, val);
    	if (ret < 0)
    		return ret;
    }

    ret = nxe2000_get_enable_reg(pwr_src, &reg, &mask);
	if (ret < 0)
		return ret;

    ret = pmic_reg_read(p, reg, &val);
	if (ret < 0)
		return ret;

    if (pwr_on)
    {
        /* Set Enable */
        val |= mask;
    }
    else
    {
        /* Set Disable */
        val &= ~mask;
    }

	ret = pmic_reg_write(p, reg, val);
	if (ret < 0)
		return ret;

	return 0;
}

static int pmic_charger_state(struct pmic *p, int state, int current)
{
	unsigned char fc;
	u32 val = 0;

	if (pmic_probe(p))
		return -1;

	if (state == CHARGER_DISABLE) {
		puts("Disable the charger.\n");
#if 0
		pmic_reg_read(p, NXE2000_REG_CHGCTL1, &val);
        val &= ~( (0x1 << NXE2000_POS_CHGCTL1_VUSBCHGEN)
                | (0x1 << NXE2000_POS_CHGCTL1_VADPCHGEN) );
		pmic_reg_write(p, NXE2000_REG_CHGCTL1, val);
#endif
		return -1;
	}

	if (current < CHARGER_MIN_CURRENT || current > CHARGER_MAX_CURRENT) {
		printf("%s: Wrong charge current: %d [mA]\n",
		       __func__, current);
		return -1;
	}

	fc = (current - CHARGER_MIN_CURRENT) / CHARGER_CURRENT_RESOLUTION;
	fc = fc & 0xf; /* up to 800 mA */

	printf("Enable the charger @ %d [mA]\n", fc * CHARGER_CURRENT_RESOLUTION
	       + CHARGER_MIN_CURRENT);

	val = (CHARGER_CURRENT_COMPLETE << NXE2000_POS_CHGISET_ICCHG) | fc;
	pmic_reg_write(p, NXE2000_REG_CHGISET, val);

    /* Set enable charger */
#if 0
	pmic_reg_read(p, NXE2000_REG_CHGCTL1, &val);
    val |= ( (0x1 << NXE2000_POS_CHGCTL1_VUSBCHGEN)
           | (0x1 << NXE2000_POS_CHGCTL1_VADPCHGEN) );
	pmic_reg_write(p, NXE2000_REG_CHGCTL1, val);
#endif

	return 0;
}

static int pmic_charger_bat_present(struct pmic *p)
{
	u32 val;
    int ret = 1;

	if (pmic_probe(p))
		return -1;

	pmic_reg_read(p, NXE2000_REG_CHGSTATE, &val);

    switch (val & NXE2000_POS_CHGSTATE_RDSTATE_MASK) {
    case NXE2000_VAL_CHGSTATE_NO_BATT2:
    case NXE2000_VAL_CHGSTATE_NO_BATT:
        ret = 0;
        break;
    default:
        ret = 1;
        break;
    }

    return ret;
}

static int pmic_chrg_get_type(struct pmic *p, u32 ctrl_en)
{
	return CHARGER_NO;
}

static struct power_chrg power_chrg_pmic_ops = {
	.chrg_type = pmic_chrg_get_type,
	.chrg_bat_present = pmic_charger_bat_present,
	.chrg_state = pmic_charger_state,
};

int power_pmic_init(unsigned char bus)
{
	static const char name[] = "PMIC_NXE2000";
	struct pmic *p = pmic_alloc();

	if (!p) {
		printf("%s: POWER allocation error!\n", __func__);
		return -ENOMEM;
	}

	debug("Board PMIC init\n");

	p->name = name;
	p->interface = PMIC_I2C;
	p->number_of_regs = NXE2000_NUM_OF_REGS;
	p->hw.i2c.addr = NXE2000_I2C_ADDR;
	p->hw.i2c.tx_num = 1;
	p->bus = bus;

	p->chrg = &power_chrg_pmic_ops;
	return 0;
}
