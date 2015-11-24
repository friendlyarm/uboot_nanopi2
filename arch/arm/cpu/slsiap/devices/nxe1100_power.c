/*
 * (C) Copyright 2013
 * bong kwan kook, Nexell Co, <kook@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <command.h>
#include <i2c.h>

#include "nxe1100_power.h"


/*
 * Debug
 */
#if (0)
#define DBGOUT(msg...)		do { printf("pmic: " msg); } while (0)
#define	NXE1100_REG_DUMP
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

static u_char nxe1100_cache_reg[256];

static int nxe1100_i2c_read(u8 reg, u8 *value, struct nxe1100_power *power)
{
	uchar chip = power->i2c_addr;
	u32   addr = (u32)reg & 0xFF;
	int   alen = 1;

	return i2c_read(chip, addr, alen, value, 1);
}

static int nxe1100_i2c_write(u8 reg, u8 value, struct nxe1100_power *power)
{
	uchar chip = power->i2c_addr;
	u32   addr = (u32)reg & 0xFF;
	int   alen = 1;

	return i2c_write(chip, addr, alen, &value, 1);
}

u8 nxe1100_get_ldo_step(u8 ldo_num, int want_vol)
{
	u32	vol_step = 0;
	u32	temp = 0;

	if (ldo_num == 3)
	{
		if (want_vol < NXE1100_DEF_LDO3_VOL_MIN)
		{
			want_vol = NXE1100_DEF_LDO3_VOL_MIN;
		}
		else if (want_vol > NXE1100_DEF_LDO3_VOL_MAX)
		{
			want_vol = NXE1100_DEF_LDO3_VOL_MAX;
		}

		temp = (want_vol - NXE1100_DEF_LDO3_VOL_MIN);
	}
	else
	{
		if (want_vol < NXE1100_DEF_LDOx_VOL_MIN)
		{
			want_vol = NXE1100_DEF_LDOx_VOL_MIN;
		}
		else if (want_vol > NXE1100_DEF_LDOx_VOL_MAX)
		{
			want_vol = NXE1100_DEF_LDOx_VOL_MAX;
		}

		temp = (want_vol - NXE1100_DEF_LDOx_VOL_MIN);
	}

	vol_step	= (temp / NXE1100_DEF_LDOx_VOL_STEP);

	if (ldo_num == 3)
	{
		temp	= (vol_step * NXE1100_DEF_LDOx_VOL_STEP) + NXE1100_DEF_LDO3_VOL_MIN;
	}
	else
	{
		temp	= (vol_step * NXE1100_DEF_LDOx_VOL_STEP) + NXE1100_DEF_LDOx_VOL_MIN;
	}

	return	(u8)(vol_step & 0x7F);
}

u8 nxe1100_get_dcdc_step(u8 dcdc_num, int want_vol)
{
	u32	vol_step = 0;
	u32	temp = 0;

	if (want_vol < NXE1100_DEF_DDCx_VOL_MIN)
	{
		want_vol = NXE1100_DEF_DDCx_VOL_MIN;
	}
	else if (want_vol > NXE1100_DEF_DDCx_VOL_MAX)
	{
		want_vol = NXE1100_DEF_DDCx_VOL_MAX;
	}

	temp		= (want_vol - NXE1100_DEF_DDCx_VOL_MIN);
	vol_step	= (temp / NXE1100_DEF_DDCx_VOL_STEP);
	temp		= (vol_step * NXE1100_DEF_DDCx_VOL_STEP) + NXE1100_DEF_DDCx_VOL_MIN;

	return	(u8)(vol_step & 0xFF);
}

#if defined(NXE1100_REG_DUMP)
static void nxe1100_register_dump(struct nxe1100_power *power)
{
	u_char *cache = nxe1100_cache_reg;


	printf("+++++++++++++++++++++++++\n");

#if 1
#if 0
	nxe1100_i2c_read(NXE2000_REG_PSWR		, &cache[NXE2000_REG_PSWR]		, power);
	nxe1100_i2c_read(NXE2000_REG_DC1DAC 	, &cache[NXE2000_REG_DC1DAC]	, power);
	nxe1100_i2c_read(NXE2000_REG_DC2DAC 	, &cache[NXE2000_REG_DC2DAC]	, power);
	nxe1100_i2c_read(NXE2000_REG_DC3DAC 	, &cache[NXE2000_REG_DC3DAC]	, power);
	nxe1100_i2c_read(NXE2000_REG_DC1DAC_SLP , &cache[NXE2000_REG_DC1DAC_SLP], power);
	nxe1100_i2c_read(NXE2000_REG_DC2DAC_SLP , &cache[NXE2000_REG_DC2DAC_SLP], power);
	nxe1100_i2c_read(NXE2000_REG_DC3DAC_SLP , &cache[NXE2000_REG_DC3DAC_SLP], power);
	nxe1100_i2c_read(NXE2000_REG_LDO1DAC	, &cache[NXE2000_REG_LDO1DAC]	, power);
	nxe1100_i2c_read(NXE2000_REG_LDO2DAC	, &cache[NXE2000_REG_LDO2DAC]	, power);
	nxe1100_i2c_read(NXE2000_REG_LDO3DAC	, &cache[NXE2000_REG_LDO3DAC]	, power);
	nxe1100_i2c_read(NXE2000_REG_LDO4DAC	, &cache[NXE2000_REG_LDO4DAC]	, power);
	nxe1100_i2c_read(NXE2000_REG_LDO5DAC	, &cache[NXE2000_REG_LDO5DAC]	, power);
	nxe1100_i2c_read(NXE2000_REG_LDO1DAC_SLP, &cache[NXE2000_REG_LDO1DAC_SLP], power);
	nxe1100_i2c_read(NXE2000_REG_LDO2DAC_SLP, &cache[NXE2000_REG_LDO2DAC_SLP], power);
	nxe1100_i2c_read(NXE2000_REG_LDO3DAC_SLP, &cache[NXE2000_REG_LDO3DAC_SLP], power);
	nxe1100_i2c_read(NXE2000_REG_LDO4DAC_SLP, &cache[NXE2000_REG_LDO4DAC_SLP], power);
	nxe1100_i2c_read(NXE2000_REG_LDO5DAC_SLP, &cache[NXE2000_REG_LDO5DAC_SLP], power);
	nxe1100_i2c_read(NXE2000_REG_DC1CTL2	, &cache[NXE2000_REG_DC1CTL2]	, power);
	nxe1100_i2c_read(NXE2000_REG_DC2CTL2	, &cache[NXE2000_REG_DC2CTL2]	, power);
	nxe1100_i2c_read(NXE2000_REG_DC3CTL2	, &cache[NXE2000_REG_DC3CTL2]	, power);
	nxe1100_i2c_read(NXE2000_REG_DC1CTL 	, &cache[NXE2000_REG_DC1CTL]	, power);
	nxe1100_i2c_read(NXE2000_REG_DC2CTL 	, &cache[NXE2000_REG_DC2CTL]	, power);
	nxe1100_i2c_read(NXE2000_REG_DC3CTL 	, &cache[NXE2000_REG_DC3CTL]	, power);
	nxe1100_i2c_read(NXE2000_REG_LDOEN1 	, &cache[NXE2000_REG_LDOEN1]	, power);
	nxe1100_i2c_read(NXE2000_REG_CHGCTL1	, &cache[NXE2000_REG_CHGCTL1]	, power);
	nxe1100_i2c_read(NXE2000_REG_CHGCTL2	, &cache[NXE2000_REG_CHGCTL2]	, power);
	nxe1100_i2c_read(NXE2000_REG_VSYSSET	, &cache[NXE2000_REG_VSYSSET]	, power);
	nxe1100_i2c_read(NXE2000_REG_REGISET1	, &cache[NXE2000_REG_REGISET1]	, power);
	nxe1100_i2c_read(NXE2000_REG_REGISET2	, &cache[NXE2000_REG_REGISET2]	, power);
	nxe1100_i2c_read(NXE2000_REG_CHGISET	, &cache[NXE2000_REG_CHGISET]	, power);
	nxe1100_i2c_read(NXE2000_REG_TIMSET 	, &cache[NXE2000_REG_TIMSET]	, power);
	nxe1100_i2c_read(NXE2000_REG_BATSET1	, &cache[NXE2000_REG_BATSET1]	, power);
	nxe1100_i2c_read(NXE2000_REG_BATSET2	, &cache[NXE2000_REG_BATSET2]	, power);
	nxe1100_i2c_read(NXE2000_REG_FG_CTRL	, &cache[NXE2000_REG_FG_CTRL]	, power);
#endif

	printf("[0x%02x]PSWR        = 0x%02x\n", NXE2000_REG_PSWR           , cache[NXE2000_REG_PSWR]       );
	printf("[0x%02x]DC1DAC      = 0x%02x\n", NXE2000_REG_DC1DAC         , cache[NXE2000_REG_DC1DAC]     );
	printf("[0x%02x]DC2DAC      = 0x%02x\n", NXE2000_REG_DC2DAC         , cache[NXE2000_REG_DC2DAC]     );
	printf("[0x%02x]DC3DAC      = 0x%02x\n", NXE2000_REG_DC3DAC         , cache[NXE2000_REG_DC3DAC]     );
	printf("[0x%02x]DC1DAC_SLP  = 0x%02x\n", NXE2000_REG_DC1DAC_SLP     , cache[NXE2000_REG_DC1DAC_SLP] );
	printf("[0x%02x]DC2DAC_SLP  = 0x%02x\n", NXE2000_REG_DC2DAC_SLP     , cache[NXE2000_REG_DC2DAC_SLP] );
	printf("[0x%02x]DC3DAC_SLP  = 0x%02x\n", NXE2000_REG_DC3DAC_SLP     , cache[NXE2000_REG_DC3DAC_SLP] );
	printf("[0x%02x]LDO1DAC     = 0x%02x\n", NXE2000_REG_LDO1DAC        , cache[NXE2000_REG_LDO1DAC]    );
	printf("[0x%02x]LDO2DAC     = 0x%02x\n", NXE2000_REG_LDO2DAC        , cache[NXE2000_REG_LDO2DAC]    );
	printf("[0x%02x]LDO3DAC     = 0x%02x\n", NXE2000_REG_LDO3DAC        , cache[NXE2000_REG_LDO3DAC]    );
	printf("[0x%02x]LDO4DAC     = 0x%02x\n", NXE2000_REG_LDO4DAC        , cache[NXE2000_REG_LDO4DAC]    );
	printf("[0x%02x]LDO5DAC     = 0x%02x\n", NXE2000_REG_LDO5DAC        , cache[NXE2000_REG_LDO5DAC]    );
	printf("[0x%02x]LDORTC1DAC  = 0x%02x\n", NXE2000_REG_LDORTC1DAC     , cache[NXE2000_REG_LDORTC1DAC] );
	printf("[0x%02x]LDORTC2DAC  = 0x%02x\n", NXE2000_REG_LDORTC2DAC     , cache[NXE2000_REG_LDORTC2DAC] );
	printf("[0x%02x]LDO1DAC_SLP = 0x%02x\n", NXE2000_REG_LDO1DAC_SLP    , cache[NXE2000_REG_LDO1DAC_SLP]);
	printf("[0x%02x]LDO2DAC_SLP = 0x%02x\n", NXE2000_REG_LDO2DAC_SLP    , cache[NXE2000_REG_LDO2DAC_SLP]);
	printf("[0x%02x]LDO3DAC_SLP = 0x%02x\n", NXE2000_REG_LDO3DAC_SLP    , cache[NXE2000_REG_LDO3DAC_SLP]);
	printf("[0x%02x]LDO4DAC_SLP = 0x%02x\n", NXE2000_REG_LDO4DAC_SLP    , cache[NXE2000_REG_LDO4DAC_SLP]);
	printf("[0x%02x]LDO5DAC_SLP = 0x%02x\n", NXE2000_REG_LDO5DAC_SLP    , cache[NXE2000_REG_LDO5DAC_SLP]);
	printf("[0x%02x]DC1CTL2     = 0x%02x\n", NXE2000_REG_DC1CTL2        , cache[NXE2000_REG_DC1CTL2]    );
	printf("[0x%02x]DC2CTL2     = 0x%02x\n", NXE2000_REG_DC2CTL2        , cache[NXE2000_REG_DC2CTL2]    );
	printf("[0x%02x]DC3CTL2     = 0x%02x\n", NXE2000_REG_DC3CTL2        , cache[NXE2000_REG_DC3CTL2]    );
	printf("[0x%02x]DC1CTL      = 0x%02x\n", NXE2000_REG_DC1CTL         , cache[NXE2000_REG_DC1CTL]     );
	printf("[0x%02x]DC2CTL      = 0x%02x\n", NXE2000_REG_DC2CTL         , cache[NXE2000_REG_DC2CTL]     );
	printf("[0x%02x]DC3CTL      = 0x%02x\n", NXE2000_REG_DC3CTL         , cache[NXE2000_REG_DC3CTL]     );
	printf("[0x%02x]LDOEN1      = 0x%02x\n", NXE2000_REG_LDOEN1         , cache[NXE2000_REG_LDOEN1]     );
	printf("[0x%02x]LDOEN2      = 0x%02x\n", NXE2000_REG_LDOEN2         , cache[NXE2000_REG_LDOEN2]     );
	printf("[0x%02x]CHGCTL1     = 0x%02x\n", NXE2000_REG_CHGCTL1        , cache[NXE2000_REG_CHGCTL1]    );
	printf("[0x%02x]CHGCTL2     = 0x%02x\n", NXE2000_REG_CHGCTL2        , cache[NXE2000_REG_CHGCTL2]    );
	printf("[0x%02x]VSYSSET     = 0x%02x\n", NXE2000_REG_VSYSSET        , cache[NXE2000_REG_VSYSSET]    );
	printf("[0x%02x]REGISET1    = 0x%02x\n", NXE2000_REG_REGISET1       , cache[NXE2000_REG_REGISET1]   );
	printf("[0x%02x]REGISET2    = 0x%02x\n", NXE2000_REG_REGISET2       , cache[NXE2000_REG_REGISET2]   );
	printf("[0x%02x]CHGISET     = 0x%02x\n", NXE2000_REG_CHGISET        , cache[NXE2000_REG_CHGISET]    );
	printf("[0x%02x]TIMSET      = 0x%02x\n", NXE2000_REG_TIMSET         , cache[NXE2000_REG_TIMSET]     );
	printf("[0x%02x]BATSET1     = 0x%02x\n", NXE2000_REG_BATSET1        , cache[NXE2000_REG_BATSET1]    );
	printf("[0x%02x]BATSET2     = 0x%02x\n", NXE2000_REG_BATSET2        , cache[NXE2000_REG_BATSET2]    );
	printf("[0x%02x]FG_CTRL     = 0x%02x\n", NXE2000_REG_FG_CTRL        , cache[NXE2000_REG_FG_CTRL]    );
#endif

	printf("-------------------------\n");
}
#else
#define	nxe1100_register_dump(d)	do { } while(0);
#endif

int nxe1100_param_setup(struct nxe1100_power *power)
{
	u_char *cache = nxe1100_cache_reg;

	DBGOUT("%s\n", __func__);

	nxe1100_i2c_read(NXE2000_REG_PSWR		, &cache[NXE2000_REG_PSWR]		, power);
	nxe1100_i2c_read(NXE2000_REG_CHGSTATE	, &cache[NXE2000_REG_CHGSTATE]	, power);

	power->warm_reset = cache[NXE2000_REG_PSWR] & (1 << NXE2000_POS_PSWR_PRESET) ? 1 : 0;

	if (power->warm_reset)
	{
		/* Get DCDC voltage register */
		nxe1100_i2c_read(NXE2000_REG_DC1DAC		, &cache[NXE2000_REG_DC1DAC]	, power);
		nxe1100_i2c_read(NXE2000_REG_DC2DAC		, &cache[NXE2000_REG_DC2DAC]	, power);
		nxe1100_i2c_read(NXE2000_REG_DC3DAC		, &cache[NXE2000_REG_DC3DAC]	, power);

		nxe1100_i2c_read(NXE2000_REG_DC1DAC_SLP	, &cache[NXE2000_REG_DC1DAC_SLP], power);
		nxe1100_i2c_read(NXE2000_REG_DC2DAC_SLP	, &cache[NXE2000_REG_DC2DAC_SLP], power);
		nxe1100_i2c_read(NXE2000_REG_DC3DAC_SLP	, &cache[NXE2000_REG_DC3DAC_SLP], power);

		/* Get LDO voltage register */
		nxe1100_i2c_read(NXE2000_REG_LDO1DAC	, &cache[NXE2000_REG_LDO1DAC]	, power);
		nxe1100_i2c_read(NXE2000_REG_LDO2DAC	, &cache[NXE2000_REG_LDO2DAC]	, power);
		nxe1100_i2c_read(NXE2000_REG_LDO3DAC	, &cache[NXE2000_REG_LDO3DAC]	, power);
		nxe1100_i2c_read(NXE2000_REG_LDO4DAC	, &cache[NXE2000_REG_LDO4DAC]	, power);
		nxe1100_i2c_read(NXE2000_REG_LDO5DAC	, &cache[NXE2000_REG_LDO5DAC]	, power);

		nxe1100_i2c_read(NXE2000_REG_LDORTC1DAC	, &cache[NXE2000_REG_LDORTC1DAC], power);
		nxe1100_i2c_read(NXE2000_REG_LDORTC2DAC	, &cache[NXE2000_REG_LDORTC2DAC], power);

		nxe1100_i2c_read(NXE2000_REG_LDO1DAC_SLP, &cache[NXE2000_REG_LDO1DAC_SLP], power);
		nxe1100_i2c_read(NXE2000_REG_LDO2DAC_SLP, &cache[NXE2000_REG_LDO2DAC_SLP], power);
		nxe1100_i2c_read(NXE2000_REG_LDO3DAC_SLP, &cache[NXE2000_REG_LDO3DAC_SLP], power);
		nxe1100_i2c_read(NXE2000_REG_LDO4DAC_SLP, &cache[NXE2000_REG_LDO4DAC_SLP], power);
		nxe1100_i2c_read(NXE2000_REG_LDO5DAC_SLP, &cache[NXE2000_REG_LDO5DAC_SLP], power);

		/* Get DCDC control register */
		nxe1100_i2c_read(NXE2000_REG_DC1CTL2	, &cache[NXE2000_REG_DC1CTL2]	, power);
		nxe1100_i2c_read(NXE2000_REG_DC2CTL2	, &cache[NXE2000_REG_DC2CTL2]	, power);
		nxe1100_i2c_read(NXE2000_REG_DC3CTL2	, &cache[NXE2000_REG_DC3CTL2]	, power);

		/* Get DCDC enable register */
		nxe1100_i2c_read(NXE2000_REG_DC1CTL		, &cache[NXE2000_REG_DC1CTL]	, power);
		nxe1100_i2c_read(NXE2000_REG_DC2CTL		, &cache[NXE2000_REG_DC2CTL]	, power);
		nxe1100_i2c_read(NXE2000_REG_DC3CTL		, &cache[NXE2000_REG_DC3CTL]	, power);

		/* Get LDO enable register */
		nxe1100_i2c_read(NXE2000_REG_LDOEN1		, &cache[NXE2000_REG_LDOEN1]	, power);
		nxe1100_i2c_read(NXE2000_REG_LDOEN2		, &cache[NXE2000_REG_LDOEN2]	, power);

		/* Set charge control register. */
		nxe1100_i2c_read(NXE2000_REG_CHGCTL1	, &cache[NXE2000_REG_CHGCTL1]	, power);
		nxe1100_i2c_read(NXE2000_REG_CHGCTL2	, &cache[NXE2000_REG_CHGCTL2]	, power);
		nxe1100_i2c_read(NXE2000_REG_VSYSSET	, &cache[NXE2000_REG_VSYSSET]	, power);

		/* Charge current setting register. */
		nxe1100_i2c_read(NXE2000_REG_REGISET1	, &cache[NXE2000_REG_REGISET1]	, power);
		nxe1100_i2c_read(NXE2000_REG_REGISET2	, &cache[NXE2000_REG_REGISET2]	, power);
		nxe1100_i2c_read(NXE2000_REG_CHGISET	, &cache[NXE2000_REG_CHGISET]	, power);

		nxe1100_i2c_read(NXE2000_REG_TIMSET		, &cache[NXE2000_REG_TIMSET]	, power);
		nxe1100_i2c_read(NXE2000_REG_BATSET1	, &cache[NXE2000_REG_BATSET1]	, power);
		nxe1100_i2c_read(NXE2000_REG_BATSET2	, &cache[NXE2000_REG_BATSET2]	, power);
		nxe1100_i2c_read(NXE2000_REG_FG_CTRL	, &cache[NXE2000_REG_FG_CTRL]	, power);
	}
	else
	{
		/* Set DCDC voltage register */
		cache[NXE2000_REG_DC1DAC]		= nxe1100_get_dcdc_step(1, NXE1100_DEF_DDC1_VOL);
		cache[NXE2000_REG_DC2DAC]		= nxe1100_get_dcdc_step(2, NXE1100_DEF_DDC2_VOL);
		cache[NXE2000_REG_DC3DAC]		= nxe1100_get_dcdc_step(3, NXE1100_DEF_DDC3_VOL);

		cache[NXE2000_REG_DC1DAC_SLP]	= nxe1100_get_dcdc_step(1, NXE1100_DEF_DDC1_SLP_VOL);
		cache[NXE2000_REG_DC2DAC_SLP]	= nxe1100_get_dcdc_step(2, NXE1100_DEF_DDC2_SLP_VOL);
		cache[NXE2000_REG_DC3DAC_SLP]	= nxe1100_get_dcdc_step(3, NXE1100_DEF_DDC3_SLP_VOL);

		/* Set LDO voltage register */
		cache[NXE2000_REG_LDO1DAC]		= nxe1100_get_ldo_step(1, NXE1100_DEF_LDO1_VOL);
		cache[NXE2000_REG_LDO2DAC]		= nxe1100_get_ldo_step(2, NXE1100_DEF_LDO2_VOL);
		cache[NXE2000_REG_LDO3DAC]		= nxe1100_get_ldo_step(3, NXE1100_DEF_LDO3_VOL);
		cache[NXE2000_REG_LDO4DAC]		= nxe1100_get_ldo_step(4, NXE1100_DEF_LDO4_VOL);
		cache[NXE2000_REG_LDO5DAC]		= nxe1100_get_ldo_step(5, NXE1100_DEF_LDO5_VOL);

		cache[NXE2000_REG_LDORTC1DAC]	= nxe1100_get_ldo_step(11, NXE1100_DEF_LDORTC1_VOL);
		cache[NXE2000_REG_LDORTC2DAC]	= nxe1100_get_ldo_step(12, NXE1100_DEF_LDORTC2_VOL);

		cache[NXE2000_REG_LDO1DAC_SLP]	= nxe1100_get_ldo_step(1, NXE1100_DEF_LDO1_SLP_VOL);
		cache[NXE2000_REG_LDO2DAC_SLP]	= nxe1100_get_ldo_step(2, NXE1100_DEF_LDO2_SLP_VOL);
		cache[NXE2000_REG_LDO3DAC_SLP]	= nxe1100_get_ldo_step(3, NXE1100_DEF_LDO3_SLP_VOL);
		cache[NXE2000_REG_LDO4DAC_SLP]	= nxe1100_get_ldo_step(4, NXE1100_DEF_LDO4_SLP_VOL);
		cache[NXE2000_REG_LDO5DAC_SLP]	= nxe1100_get_ldo_step(5, NXE1100_DEF_LDO5_SLP_VOL);

		/* Set DCDC control register */
		cache[NXE2000_REG_DC1CTL2]	= ( (NXE1100_DEF_DDC1_OSC_FREQ	 << NXE2000_POS_DCxCTL2_DCxOSC)	|
										(NXE1100_DEF_DDC1_RAMP_SLOP  << NXE2000_POS_DCxCTL2_DCxSR)	|
										(NXE1100_DEF_DDC1_CUR_LIMIT  << NXE2000_POS_DCxCTL2_DCxLIM)	|
										(NXE1100_DEF_DDC1_LIMSHUT_EN << NXE2000_POS_DCxCTL2_DCxLIMSDEN) );

		cache[NXE2000_REG_DC2CTL2]	= ( (NXE1100_DEF_DDC2_OSC_FREQ	 << NXE2000_POS_DCxCTL2_DCxOSC)	|
										(NXE1100_DEF_DDC2_RAMP_SLOP  << NXE2000_POS_DCxCTL2_DCxSR)	|
										(NXE1100_DEF_DDC2_CUR_LIMIT  << NXE2000_POS_DCxCTL2_DCxLIM)	|
										(NXE1100_DEF_DDC2_LIMSHUT_EN << NXE2000_POS_DCxCTL2_DCxLIMSDEN) );

		cache[NXE2000_REG_DC3CTL2]	= ( (NXE1100_DEF_DDC3_OSC_FREQ	 << NXE2000_POS_DCxCTL2_DCxOSC)	|
										(NXE1100_DEF_DDC3_RAMP_SLOP  << NXE2000_POS_DCxCTL2_DCxSR)	|
										(NXE1100_DEF_DDC3_CUR_LIMIT  << NXE2000_POS_DCxCTL2_DCxLIM)	|
										(NXE1100_DEF_DDC3_LIMSHUT_EN << NXE2000_POS_DCxCTL2_DCxLIMSDEN) );

		/* DCDC - Enable */
		cache[NXE2000_REG_DC1CTL]	= ( (NXE1100_DEF_DDC1_SLP_MODE	<< NXE2000_POS_DCxCTL_DCxMODE_SLP)	|
										(NXE1100_DEF_DDC1_OSC_FREQ	<< NXE2000_POS_DCxCTL_DCxMODE)	|
										(NXE1100_DEF_DDC1_DSC_CTRL	<< NXE2000_POS_DCxCTL_DCxDIS)	|
										(NXE1100_DEF_DDC1_ON		<< NXE2000_POS_DCxCTL_DCxEN) );

		cache[NXE2000_REG_DC2CTL]	= ( (NXE1100_DEF_DDC2_SLP_MODE	<< NXE2000_POS_DCxCTL_DCxMODE_SLP)	|
										(NXE1100_DEF_DDC2_OSC_FREQ	<< NXE2000_POS_DCxCTL_DCxMODE)	|
										(NXE1100_DEF_DDC2_DSC_CTRL	<< NXE2000_POS_DCxCTL_DCxDIS)	|
										(NXE1100_DEF_DDC2_ON		<< NXE2000_POS_DCxCTL_DCxEN) );

		cache[NXE2000_REG_DC3CTL]	= ( (NXE1100_DEF_DDC3_SLP_MODE	<< NXE2000_POS_DCxCTL_DCxMODE_SLP)	|
										(NXE1100_DEF_DDC3_OSC_FREQ	<< NXE2000_POS_DCxCTL_DCxMODE)	|
										(NXE1100_DEF_DDC3_DSC_CTRL	<< NXE2000_POS_DCxCTL_DCxDIS)	|
										(NXE1100_DEF_DDC3_ON		<< NXE2000_POS_DCxCTL_DCxEN) );

		/* LDO - Enable */
		cache[NXE2000_REG_LDOEN1]	= ( (NXE1100_DEF_LDO1_ON << NXE2000_POS_LDOEN1_LDO1EN) |
										(NXE1100_DEF_LDO2_ON << NXE2000_POS_LDOEN1_LDO2EN) |
										(NXE1100_DEF_LDO3_ON << NXE2000_POS_LDOEN1_LDO3EN) |
										(NXE1100_DEF_LDO4_ON << NXE2000_POS_LDOEN1_LDO4EN) |
										(NXE1100_DEF_LDO5_ON << NXE2000_POS_LDOEN1_LDO5EN) );

		cache[NXE2000_REG_LDOEN2]	= ( (NXE1100_DEF_LDORTC1_ON << NXE2000_POS_LDOEN2_LDORTC1EN) |
                                        (NXE1100_DEF_LDORTC2_ON << NXE2000_POS_LDOEN2_LDORTC2EN) );

		/* Set charge control register. */
		cache[NXE2000_REG_CHGCTL1]	= ( (NXE1100_DEF_CHG_NOBAT_OVLIM_EN << NXE2000_POS_CHGCTL1_NOBATOVLIM) |
										(NXE1100_DEF_CHG_USB_EN << NXE2000_POS_CHGCTL1_VUSBCHGEN) |
										(NXE1100_DEF_CHG_ADP_EN << NXE2000_POS_CHGCTL1_VADPCHGEN) );

		cache[NXE2000_REG_CHGCTL2]	= ( (NXE1100_DEF_CHG_USB_VCONTMASK	<< NXE2000_POS_CHGCTL2_USB_VCONTMASK) |
										(NXE1100_DEF_CHG_ADP_VCONTMASK	<< NXE2000_POS_CHGCTL2_ADP_VCONTMASK) |
										(NXE1100_DEF_CHG_VUSB_BUCK_THS	<< NXE2000_POS_CHGCTL2_VUSBBUCK_VTH) |
										(NXE1100_DEF_CHG_VADP_BUCK_THS	<< NXE2000_POS_CHGCTL2_VADPBUCK_VTH) );

		cache[NXE2000_REG_VSYSSET]	= ( (NXE1100_DEF_CHG_VSYS_VOL		<< NXE2000_POS_VSYSSET_VSYSSET) |
										(NXE1100_DEF_CHG_VSYS_OVER_VOL	<< NXE2000_POS_VSYSSET_VSYSOVSET) );

		/* Charge current setting register. */
		cache[NXE2000_REG_REGISET1]	= NXE1100_DEF_CHG_CURR_ADP;
		cache[NXE2000_REG_REGISET2]	= NXE1100_DEF_CHG_CURR_USB;

		if ( cache[NXE2000_REG_CHGSTATE] & (1 << NXE2000_POS_CHGSTATE_USEADP) )
			cache[NXE2000_REG_CHGISET]	= ( ((NXE1100_DEF_CHG_CURR_RAPID_COMP & 3) << NXE2000_POS_CHGISET_ICCHG) | (NXE1100_DEF_CHG_CURR_RAPID_ADP & 0x1F) );
		else
			cache[NXE2000_REG_CHGISET]	= ( ((NXE1100_DEF_CHG_CURR_RAPID_COMP & 3) << NXE2000_POS_CHGISET_ICCHG) | (NXE1100_DEF_CHG_CURR_RAPID_USB & 0x1F) );

		cache[NXE2000_REG_TIMSET]	= ( ((NXE1100_DEF_CHG_RAPID_CTIME & 3) << NXE2000_POS_TIMSET_CTIMSET) | (NXE1100_DEF_CHG_RAPID_RTIME & 3) );

		cache[NXE2000_REG_BATSET1]	= ( ((NXE1100_DEF_CHG_POWER_ON_VOL & 7)	<< NXE2000_POS_BATSET1_CHGPON) |
										((NXE1100_DEF_CHG_VBATOV_SET & 1)	<< NXE2000_POS_BATSET1_VBATOVSET) |
										((NXE1100_DEF_CHG_VWEAK	& 3)		<< NXE2000_POS_BATSET1_VWEAK) |
										((NXE1100_DEF_CHG_VDEAD	& 1)		<< NXE2000_POS_BATSET1_VDEAD) |
										((NXE1100_DEF_CHG_VSHORT & 1)		<< NXE2000_POS_BATSET1_VSHORT) );
		cache[NXE2000_REG_BATSET2]	= ( ((NXE1100_DEF_CHG_VFCHG & 7) << NXE2000_POS_BATSET2_VFCHG) | (NXE1100_DEF_CHG_VRCHG & 7) );

		cache[NXE2000_REG_FG_CTRL]	= ( (1 << NXE2000_POS_FG_CTRL_SRST0) | (1 << NXE2000_POS_FG_CTRL_FG_EN) );
	}

//	cache[NXE1100_REG_PSWR] |= (1 << NXE1100_POS_PSWR_PRESET);

//	nxe1100_register_dump(power);

	return 0;
}

int nxe1100_device_setup(struct nxe1100_power *power)
{
	u_char	*cache = nxe1100_cache_reg;
	int		bus = power->i2c_bus;

	DBGOUT("%s\n", __func__);

	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	i2c_set_bus_num(bus);

	nxe1100_param_setup(power);

	/* Set DCDC voltage register */
	nxe1100_i2c_write(NXE2000_REG_DC1DAC	, cache[NXE2000_REG_DC1DAC]		, power);
	nxe1100_i2c_write(NXE2000_REG_DC2DAC	, cache[NXE2000_REG_DC2DAC]		, power);
	nxe1100_i2c_write(NXE2000_REG_DC3DAC	, cache[NXE2000_REG_DC3DAC]		, power);

	nxe1100_i2c_write(NXE2000_REG_DC1DAC_SLP, cache[NXE2000_REG_DC1DAC_SLP]	, power);
	nxe1100_i2c_write(NXE2000_REG_DC2DAC_SLP, cache[NXE2000_REG_DC2DAC_SLP]	, power);
	nxe1100_i2c_write(NXE2000_REG_DC3DAC_SLP, cache[NXE2000_REG_DC3DAC_SLP]	, power);

	/* Set LDO voltage register */
	nxe1100_i2c_write(NXE2000_REG_LDO1DAC	, cache[NXE2000_REG_LDO1DAC]	, power);
	nxe1100_i2c_write(NXE2000_REG_LDO2DAC	, cache[NXE2000_REG_LDO2DAC]	, power);
	nxe1100_i2c_write(NXE2000_REG_LDO3DAC	, cache[NXE2000_REG_LDO3DAC]	, power);
	nxe1100_i2c_write(NXE2000_REG_LDO4DAC	, cache[NXE2000_REG_LDO4DAC]	, power);
	nxe1100_i2c_write(NXE2000_REG_LDO5DAC	, cache[NXE2000_REG_LDO5DAC]	, power);

	nxe1100_i2c_write(NXE2000_REG_LDORTC1DAC, cache[NXE2000_REG_LDORTC1DAC]	, power);
	nxe1100_i2c_write(NXE2000_REG_LDORTC2DAC, cache[NXE2000_REG_LDORTC2DAC]	, power);

	nxe1100_i2c_write(NXE2000_REG_LDO1DAC_SLP, cache[NXE2000_REG_LDO1DAC_SLP], power);
	nxe1100_i2c_write(NXE2000_REG_LDO2DAC_SLP, cache[NXE2000_REG_LDO2DAC_SLP], power);
	nxe1100_i2c_write(NXE2000_REG_LDO3DAC_SLP, cache[NXE2000_REG_LDO3DAC_SLP], power);
	nxe1100_i2c_write(NXE2000_REG_LDO4DAC_SLP, cache[NXE2000_REG_LDO4DAC_SLP], power);
	nxe1100_i2c_write(NXE2000_REG_LDO5DAC_SLP, cache[NXE2000_REG_LDO5DAC_SLP], power);

	/* Set DCDC control register */
	nxe1100_i2c_write(NXE2000_REG_DC1CTL2	, cache[NXE2000_REG_DC1CTL2]	, power);
	nxe1100_i2c_write(NXE2000_REG_DC2CTL2	, cache[NXE2000_REG_DC2CTL2]	, power);
	nxe1100_i2c_write(NXE2000_REG_DC3CTL2	, cache[NXE2000_REG_DC3CTL2]	, power);

	/* Set DCDC enable register */
	nxe1100_i2c_write(NXE2000_REG_DC1CTL	, cache[NXE2000_REG_DC1CTL]		, power);
	nxe1100_i2c_write(NXE2000_REG_DC2CTL	, cache[NXE2000_REG_DC2CTL]		, power);
	nxe1100_i2c_write(NXE2000_REG_DC3CTL	, cache[NXE2000_REG_DC3CTL]		, power);

	/* Set LDO enable register */
	nxe1100_i2c_write(NXE2000_REG_LDOEN1	, cache[NXE2000_REG_LDOEN1]		, power);

	/* Set charge control register. */
	nxe1100_i2c_write(NXE2000_REG_CHGCTL1	, cache[NXE2000_REG_CHGCTL1]	, power);
	nxe1100_i2c_write(NXE2000_REG_CHGCTL2	, cache[NXE2000_REG_CHGCTL2]	, power);
	nxe1100_i2c_write(NXE2000_REG_VSYSSET	, cache[NXE2000_REG_VSYSSET]	, power);

	/* Charge current setting register. */
	nxe1100_i2c_write(NXE2000_REG_REGISET1	, cache[NXE2000_REG_REGISET1]	, power);
	nxe1100_i2c_write(NXE2000_REG_REGISET2	, cache[NXE2000_REG_REGISET2]	, power);
	nxe1100_i2c_write(NXE2000_REG_CHGISET	, cache[NXE2000_REG_CHGISET]	, power);

	nxe1100_i2c_write(NXE2000_REG_TIMSET	, cache[NXE2000_REG_TIMSET]		, power);
	nxe1100_i2c_write(NXE2000_REG_BATSET1	, cache[NXE2000_REG_BATSET1]	, power);
	nxe1100_i2c_write(NXE2000_REG_BATSET2	, cache[NXE2000_REG_BATSET2]	, power);
	nxe1100_i2c_write(NXE2000_REG_FG_CTRL	, cache[NXE2000_REG_FG_CTRL]	, power);

	nxe1100_i2c_write(NXE2000_REG_PSWR		, cache[NXE2000_REG_PSWR]   	, power);

	nxe1100_register_dump(power);

	return 0;
}


