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

#include <config.h>
#include <common.h>
#include <mmc.h>
#include <pwm.h>
#include <asm/io.h>
#include <asm/gpio.h>

#include <platform.h>
#include <mach-api.h>
#include <rtc_nxp.h>
#include <pm.h>

#include <draw_lcd.h>

#include <power/pmic.h>
#include <power/battery.h>
#include <power/regulator.h>
#include <power/axp228.h>

#include <i2c.h>
#include <errno.h>


static u8 axp228_ocv_table[] = {
	OCVREG0,
	OCVREG1,
	OCVREG2,
	OCVREG3,
	OCVREG4,
	OCVREG5,
	OCVREG6,
	OCVREG7,
	OCVREG8,
	OCVREG9,
	OCVREGA,
	OCVREGB,
	OCVREGC,
	OCVREGD,
	OCVREGE,
	OCVREGF,

	OCVREG10,
	OCVREG11,
	OCVREG12,
	OCVREG13,
	OCVREG14,
	OCVREG15,
	OCVREG16,
	OCVREG17,
	OCVREG18,
	OCVREG19,
	OCVREG1A,
	OCVREG1B,
	OCVREG1C,
	OCVREG1D,
	OCVREG1E,
	OCVREG1F,
};


static int axp228_i2c_read(struct axp228_power *power, u8 reg, u8 *value)
{
	uchar chip = power->i2c_addr;
	u32   addr = (u32)reg & 0xFF;
	int   alen = 1;
	return i2c_read(chip, addr, alen, value, 1);
}

static int axp228_i2c_write(struct axp228_power *power, u8 reg, u8 value)
{
	uchar chip = power->i2c_addr;
	u32   addr = (u32)reg & 0xFF;
	int   alen = 1;
	return i2c_write(chip, addr, alen, &value, 1);
}

static int axp228_i2c_set_bits(struct axp228_power *power, int reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	int ret = 0;

	ret = axp228_i2c_read(power, reg, &reg_val);
	if (ret)
		return ret;

	if ((reg_val & bit_mask) != bit_mask) {
		reg_val |= bit_mask;
		ret = axp228_i2c_write(power, reg, reg_val);
	}
	return ret;
}

static int axp228_i2c_clr_bits(struct axp228_power *power, int reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	int ret = 0;

	ret = axp228_i2c_read(power, reg, &reg_val);
	if (ret)
		return ret;

	if (reg_val & bit_mask) {
		reg_val &= ~bit_mask;
		ret = axp228_i2c_write(power, reg, reg_val);
	}
	return ret;
}

static int axp228_i2c_update(struct axp228_power *power, int reg, uint8_t val, uint8_t mask)
{
	u8 reg_val;
	int ret = 0;

	ret = axp228_i2c_read(power, reg, &reg_val);
	if (ret)
		return ret;

	if ((reg_val & mask) != val) {
		reg_val = (reg_val & ~mask) | val;
		ret = axp228_i2c_write(power, reg, reg_val);
	}
	return ret;
}


#if defined(CONFIG_PMIC_REG_DUMP)
void axp228_register_dump(struct axp228_power *power)
{
	int ret = 0;
	int i = 0;
	u8 value = 0;

	printf("##########################################################\n");
	printf("##\e[31m %s()\e[0m \n", __func__);
	printf("----------------------------------------------------------\n");
	printf("##      0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F\n");

	for(i=0; i<=AXP228_NUM_OF_REGS; i++)
	{
		if(i%16 == 0)
			printf("## %02X:", i);

		if(i%4 == 0)
			printf(" ");

		ret = axp228_i2c_read(power, i, &value);
		if(!ret)
		{
			printf("%02x ", value);
		}
		else
		{
			printf("\e[31mxx\e[0m: %d \n", ret);
			break;
		}

		if((i+1)%16 == 0)
			printf("\n");
	}
	printf("##########################################################\n");

}
#endif


static void axp228_set_charging_current(u32 current)
{
	struct axp228_power power_config = {
		.i2c_addr = AXP228_I2C_ADDR,
		.i2c_bus = CONFIG_PMIC_I2C_BUS,
	};
	u8 val =0;

	val = (current -200001)/150000;
	if(current >= 300000 && current <= 2550000)
		axp228_i2c_update(&power_config, AXP22_CHARGE1, val,0x0F);
	else if(LIMIT_CHARGE_CURRENT < 300000)
		axp228_i2c_clr_bits(&power_config, AXP22_CHARGE1,0x0F);
	else
		axp228_i2c_set_bits(&power_config, AXP22_CHARGE1,0x0F);
}

static u8 axp228_get_vol_step(int want_vol, int step, int min, int max)
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


	return	(u8)(vol_step & 0xFF);
}

static int axp228_param_setup(struct axp228_power *power)
{
	int ret=0, i=0;
	int var,tmp;
	int Cur_CoulombCounter,rdc;
	u8 val2,val, ocv_cmp=0;

	PMIC_DBGOUT("%s\n", __func__);

#if (CFG_POLY_PHASE_FUNCTION == 1)
	/* Enable DC-DC 2&3 Poly-phase Function*/
	axp228_i2c_set_bits(power, AXP22_DCDC_FREQSET, 0x10);
#endif

	/* REG 80H:DC-DC Work mode */
	axp228_i2c_read(power, AXP22_DCDC_MODESET, &val);
	val &= 0xE0;
	val |= 	( AXP_DCDC1_MODE_ENABLE << AXP_DCDC1_MODE_BIT)
			|(AXP_DCDC2_MODE_ENABLE << AXP_DCDC2_MODE_BIT)
			|(AXP_DCDC3_MODE_ENABLE << AXP_DCDC3_MODE_BIT)
			|(AXP_DCDC4_MODE_ENABLE << AXP_DCDC4_MODE_BIT)
			|(AXP_DCDC5_MODE_ENABLE << AXP_DCDC5_MODE_BIT);
	axp228_i2c_write(power, AXP22_DCDC_MODESET, val);

	/* REG 10H: DCDC1/2/3/4/5&ALDO1/2&DC5LDO Enable Set */
	val = 	( AXP_ALDO2_ENABLE << AXP_ALDO2_EN_BIT)
			|(AXP_ALDO1_ENABLE << AXP_ALDO1_EN_BIT)
			|(AXP_DCDC5_ENABLE << AXP_DCDC5_EN_BIT)
			|(AXP_DCDC4_ENABLE << AXP_DCDC4_EN_BIT)
			|(AXP_DCDC3_ENABLE << AXP_DCDC3_EN_BIT)
			|(AXP_DCDC2_ENABLE << AXP_DCDC2_EN_BIT)
			|(AXP_DCDC1_ENABLE << AXP_DCDC1_EN_BIT)
			|(AXP_DC5LDO_ENABLE << AXP_DC5LDO_EN_BIT);
	axp228_i2c_write(power, AXP22_LDO_DC_EN1, val);

	/* REG 12H: Power Output Control */
	val = 	( AXP_DC1SW_ENABLE << AXP_DC1SW_EN_BIT)
			|(AXP_DLDO4_ENABLE << AXP_DLDO4_EN_BIT)
			|(AXP_DLDO3_ENABLE << AXP_DLDO3_EN_BIT)
			|(AXP_DLDO2_ENABLE << AXP_DLDO2_EN_BIT)
			|(AXP_DLDO1_ENABLE << AXP_DLDO1_EN_BIT)
			|(AXP_ELDO3_ENABLE << AXP_ELDO3_EN_BIT)
			|(AXP_ELDO2_ENABLE << AXP_ELDO2_EN_BIT)
			|(AXP_ELDO1_ENABLE << AXP_ELDO1_EN_BIT);
	axp228_i2c_write(power, AXP22_LDO_DC_EN2, val);

	/* REG 13H:Power Output Control */
	val = 	( AXP_ALDO3_ENABLE << AXP_ALDO3_EN_BIT) | 0x00 ;
	axp228_i2c_write(power, AXP22_LDO_DC_EN3, val);

	/* REG 21H:DCDC1 Output Voltage Set */
	val = axp228_get_vol_step(AXP_DCDC1_VALUE, AXP22_DCDC1_STEP, AXP22_DCDC1_MIN, AXP22_DCDC1_MAX);
	axp228_i2c_write(power, AXP22_DC1OUT_VOL, val);

#if defined(CONFIG_ENABLE_INIT_VOLTAGE)
	/* REG 22H:DCDC2 Output Voltage Set */
	val = axp228_get_vol_step(AXP_DCDC2_VALUE, AXP22_DCDC2_STEP, AXP22_DCDC2_MIN, AXP22_DCDC2_MAX);
	axp228_i2c_write(power, AXP22_DC2OUT_VOL, val);

	/* REG 23H:DCDC3 Output Voltage Set */
	val = axp228_get_vol_step(AXP_DCDC3_VALUE, AXP22_DCDC3_STEP, AXP22_DCDC3_MIN, AXP22_DCDC3_MAX);
	axp228_i2c_write(power, AXP22_DC3OUT_VOL, val);
#endif

	/* REG 24H:DCDC4 Output Voltage Set */
	val = axp228_get_vol_step(AXP_DCDC4_VALUE, AXP22_DCDC4_STEP, AXP22_DCDC4_MIN, AXP22_DCDC4_MAX);
	axp228_i2c_write(power, AXP22_DC4OUT_VOL, val);

	/* REG 25H:DCDC5 Output Voltage Set */
	val = axp228_get_vol_step(AXP_DCDC5_VALUE, AXP22_DCDC5_STEP, AXP22_DCDC5_MIN, AXP22_DCDC5_MAX);
	axp228_i2c_write(power, AXP22_DC5OUT_VOL, val);

	/* REG 15H:DLDO1 Output Voltage Set */
	val = axp228_get_vol_step(AXP_DLDO1_VALUE, AXP22_DLDO1_STEP, AXP22_DLDO1_MIN, AXP22_DLDO1_MAX);
	axp228_i2c_write(power, AXP22_DLDO1OUT_VOL, val);

	/* REG 16H:DLDO2 Output Voltage Set */
	val = axp228_get_vol_step(AXP_DLDO2_VALUE, AXP22_DLDO2_STEP, AXP22_DLDO2_MIN, AXP22_DLDO2_MAX);
	axp228_i2c_write(power, AXP22_DLDO2OUT_VOL, val);

	/* REG 17H:DLDO3 Output Voltage Set */
	val = axp228_get_vol_step(AXP_DLDO3_VALUE, AXP22_DLDO3_STEP, AXP22_DLDO3_MIN, AXP22_DLDO3_MAX);
	axp228_i2c_write(power, AXP22_DLDO3OUT_VOL, val);

	/* REG 18H:DLDO4 Output Voltage Set */
	val = axp228_get_vol_step(AXP_DLDO4_VALUE, AXP22_DLDO4_STEP, AXP22_DLDO4_MIN, AXP22_DLDO4_MAX);
	axp228_i2c_write(power, AXP22_DLDO4OUT_VOL, val);

	/* REG 19H:ELDO1 Output Voltage Set */
	val = axp228_get_vol_step(AXP_ELDO1_VALUE, AXP22_ELDO1_STEP, AXP22_ELDO1_MIN, AXP22_ELDO1_MAX);
	axp228_i2c_write(power, AXP22_ELDO1OUT_VOL, val);

	/* REG 1AH:ELDO2 Output Voltage Set */
	val = axp228_get_vol_step(AXP_ELDO2_VALUE, AXP22_ELDO2_STEP, AXP22_ELDO2_MIN, AXP22_ELDO2_MAX);
	axp228_i2c_write(power, AXP22_ELDO2OUT_VOL, val);

	/* REG 1BH:ELDO3 Output Voltage Set */
	val = axp228_get_vol_step(AXP_ELDO3_VALUE, AXP22_ELDO3_STEP, AXP22_ELDO3_MIN, AXP22_ELDO3_MAX);
	axp228_i2c_write(power, AXP22_ELDO3OUT_VOL, val);

	/* REG 1CH:DC5LDO Output Voltage Set */
	val = axp228_get_vol_step(AXP_DC5LDO_VALUE, AXP22_DC5LDO_STEP, AXP22_DC5LDO_MIN, AXP22_DC5LDO_MAX);
	axp228_i2c_write(power, AXP22_DC5LDOOUT_VOL, val);

	/* REG 28H:ALDO1 Output Voltage Set */
	val = axp228_get_vol_step(AXP_ALDO1_VALUE, AXP22_ALDO1_STEP, AXP22_ALDO1_MIN, AXP22_ALDO1_MAX);
	axp228_i2c_write(power, AXP22_ALDO1OUT_VOL, val);

	/* REG 29H:ALDO2 Output Voltage Set */
	val = axp228_get_vol_step(AXP_ALDO2_VALUE, AXP22_ALDO2_STEP, AXP22_ALDO2_MIN, AXP22_ALDO2_MAX);
	axp228_i2c_write(power, AXP22_ALDO2OUT_VOL, val);

	/* REG 2AH:ALDO3 Output Voltage Set */
	val = axp228_get_vol_step(AXP_ALDO3_VALUE, AXP22_ALDO3_STEP, AXP22_ALDO3_MIN, AXP22_ALDO3_MAX);
	axp228_i2c_write(power, AXP22_ALDO3OUT_VOL, val);


	/* PWREN Control1 */
	val = (AXP_DCDC1_SLEEP_OFF << AXP_DCDC1_BIT)
			|(AXP_DCDC2_SLEEP_OFF << AXP_DCDC2_BIT)
			|(AXP_DCDC3_SLEEP_OFF << AXP_DCDC3_BIT)
			|(AXP_DCDC4_SLEEP_OFF << AXP_DCDC4_BIT)
			|(AXP_DCDC5_SLEEP_OFF << AXP_DCDC5_BIT)
			|(AXP_ALDO1_SLEEP_OFF << AXP_ALDO1_BIT)
			|(AXP_ALDO2_SLEEP_OFF << AXP_ALDO2_BIT)
			|(AXP_ALDO3_SLEEP_OFF << AXP_ALDO3_BIT);
	axp228_i2c_write(power, AXP22_PWREN_CTL1, val);

	/* PWREN Control2 */
	val = (AXP_DLDO1_SLEEP_OFF << AXP_DLDO1_BIT)
			|(AXP_DLDO1_SLEEP_OFF << AXP_DLDO2_BIT)
			|(AXP_DLDO1_SLEEP_OFF << AXP_DLDO3_BIT)
			|(AXP_DLDO1_SLEEP_OFF << AXP_DLDO4_BIT)
			|(AXP_ELDO1_SLEEP_OFF << AXP_ELDO1_BIT)
			|(AXP_ELDO1_SLEEP_OFF << AXP_ELDO2_BIT)
			|(AXP_ELDO1_SLEEP_OFF << AXP_ELDO3_BIT)
			|(AXP_DC5LDO_SLEEP_OFF << AXP_DC5LDO_BIT);
	axp228_i2c_write(power, AXP22_PWREN_CTL2, val);


	/* REG 31H: Wakeup Control and Voff Voltage Set */
	axp228_i2c_update(power, AXP22_VOFF_SET, 0x00, 0x7);


	/* REG 84H:ADC Sample rate Set ,TS Pin Control */
	axp228_i2c_set_bits(power, AXP22_ADC_CONTROL3,0x04);


	/* USB voltage limit */
	if((USBVOLLIM) && (USBVOLLIMEN))
	{
		axp228_i2c_set_bits(power, AXP22_CHARGE_VBUS, 0x40);
		var = USBVOLLIM * 1000;
		if(var >= 4000000 && var <=4700000)
		{
			tmp = (var - 4000000)/100000;
			axp228_i2c_read(power, AXP22_CHARGE_VBUS,&val);
			val &= 0xC7;
			val |= tmp << 3;
			axp228_i2c_write(power, AXP22_CHARGE_VBUS,val);
		}
	}
	else
	{
		axp228_i2c_clr_bits(power, AXP22_CHARGE_VBUS, 0x40);
	}


	/* USB current limit */
	if((USBCURLIM) && (USBCURLIMEN))
	{
		axp228_i2c_clr_bits(power, AXP22_CHARGE_VBUS, 0x02);
		var = USBCURLIM * 1000;
		if(var == 900000)
		{
			axp228_i2c_clr_bits(power, AXP22_CHARGE_VBUS, 0x03);
		}
		else if (var == 500000)
		{
			axp228_i2c_set_bits(power, AXP22_CHARGE_VBUS, 0x01);
		}
	}
	else
	{
		axp228_i2c_set_bits(power, AXP22_CHARGE_VBUS, 0x03);
	}


	/* limit charge current */
	val = (LIMIT_CHARGE_CURRENT -200001)/150000;
	if(LIMIT_CHARGE_CURRENT >= 300000 && LIMIT_CHARGE_CURRENT <= 2550000)
		axp228_i2c_update(power, AXP22_CHARGE3, val,0x0F);
	else if(LIMIT_CHARGE_CURRENT < 300000)
		axp228_i2c_clr_bits(power, AXP22_CHARGE3,0x0F);
	else
		axp228_i2c_set_bits(power, AXP22_CHARGE3,0x0F);

	/* charge current */
    if(CHARGE_CURRENT == 0)
        axp228_i2c_clr_bits(power,AXP22_CHARGE1,0x80);
    else
        axp228_i2c_set_bits(power,AXP22_CHARGE1,0x80);

#if 1
	axp228_i2c_read(power, AXP22_STATUS,&val);
	axp228_i2c_read(power, AXP22_MODE_CHGSTATUS,&val2);

	tmp = (val2 << 8 )+ val;
	val = (tmp & AXP22_STATUS_ACVA)?1:0;

	if(val)
		axp228_set_charging_current(CHARGE_CURRENT);
	else
		axp228_set_charging_current(CHARGE_CURRENT_500);
#else
	axp228_set_charging_current(CHARGE_CURRENT);
#endif

	// set lowe power warning/shutdown level
	axp228_i2c_write(power, AXP22_WARNING_LEVEL,((BATLOWLV1-5) << 4)+BATLOWLV2);


	/* OCV Table */
	for(i=0; i<ARRAY_SIZE(axp228_ocv_table); i++)
	{
		ret = axp228_i2c_read(power, AXP22_OCV_TABLE+i, &val);
		if(val!= axp228_ocv_table[i])
		{
			ocv_cmp = 1;
			break;
		}
	}
	if(ocv_cmp)
	{
		for(i=0; i<ARRAY_SIZE(axp228_ocv_table); i++)
		{
			ret = axp228_i2c_write(power, AXP22_OCV_TABLE+i, axp228_ocv_table[i]);
		}
	}


	/* pok open time set */
	axp228_i2c_read(power,AXP22_POK_SET,&val);
	if (PEKOPEN < 1000)
		val &= 0x3f;
	else if(PEKOPEN < 2000){
		val &= 0x3f;
		val |= 0x40;
	}
	else if(PEKOPEN < 3000){
		val &= 0x3f;
		val |= 0x80;
	}
	else {
		val &= 0x3f;
		val |= 0xc0;
	}
	axp228_i2c_write(power,AXP22_POK_SET,val);


	/* pok long time set*/
	if(PEKLONG < 1000)
		tmp = 1000;
	else if(PEKLONG > 2500)
		tmp = 2500;
	else
		tmp = PEKLONG;
	axp228_i2c_read(power,AXP22_POK_SET,&val);
	val &= 0xcf;
	val |= (((tmp - 1000) / 500) << 4);
	axp228_i2c_write(power,AXP22_POK_SET,val);


	/* pek offlevel poweroff en set*/
	if(PEKOFFEN)
		tmp = 1;
	else
		tmp = 0;			
	axp228_i2c_read(power,AXP22_POK_SET,&val);
	val &= 0xf7;
	val |= (tmp << 3);
	axp228_i2c_write(power,AXP22_POK_SET,val);


	/*Init offlevel restart or not */
	if(PEKOFFRESTART)
		axp228_i2c_set_bits(power,AXP22_POK_SET,0x04); //restart
	else
		axp228_i2c_clr_bits(power,AXP22_POK_SET,0x04); //not restart


	/* pek delay set */
	axp228_i2c_read(power,AXP22_OFF_CTL,&val);
	val &= 0xfc;
	val |= ((PEKDELAY / 8) - 1);
	axp228_i2c_write(power,AXP22_OFF_CTL,val);


	/* pek offlevel time set */
	if(PEKOFF < 4000)
		tmp = 4000;
	else if(PEKOFF > 10000)
		tmp =10000;
	else
		tmp = PEKOFF;
	tmp = (tmp - 4000) / 2000 ;
	axp228_i2c_read(power,AXP22_POK_SET,&val);
	val &= 0xfc;
	val |= tmp ;
	axp228_i2c_write(power,AXP22_POK_SET,val);


	/*Init 16's Reset PMU en */
	if(PMURESET)
		axp228_i2c_set_bits(power,0x8F,0x08); //enable
	else
		axp228_i2c_clr_bits(power,0x8F,0x08); //disable


	/*Init IRQ wakeup en*/
	if(IRQWAKEUP)
		axp228_i2c_set_bits(power,0x8F,0x80); //enable
	else
		axp228_i2c_clr_bits(power,0x8F,0x80); //disable


	/*Init N_VBUSEN status*/
	if(VBUSEN)
		axp228_i2c_set_bits(power,0x8F,0x10); //output
	else
		axp228_i2c_clr_bits(power,0x8F,0x10); //input


	/*Init InShort status*/
	if(VBUSACINSHORT)
		axp228_i2c_set_bits(power,0x8F,0x60); //InShort
	else
		axp228_i2c_clr_bits(power,0x8F,0x60); //auto detect


	/*Init CHGLED function*/
	if(CHGLEDFUN)
		axp228_i2c_set_bits(power,0x32,0x08); //control by charger
	else
		axp228_i2c_clr_bits(power,0x32,0x08); //drive MOTO


	/*set CHGLED Indication Type*/
	if(CHGLEDTYPE)
		axp228_i2c_set_bits(power,0x45,0x10); //Type A
	else
		axp228_i2c_clr_bits(power,0x45,0x10); //Type B


	/*Init PMU Over Temperature protection*/
	if(OTPOFFEN)
		axp228_i2c_set_bits(power,0x8f,0x04); //enable
	else
		axp228_i2c_clr_bits(power,0x8f,0x04); //disable


	/*Init battery capacity correct function*/
	if(BATCAPCORRENT)
		axp228_i2c_set_bits(power,0xb8,0x20); //enable
	else
		axp228_i2c_clr_bits(power,0xb8,0x20); //disable


	/* Init battery regulator enable or not when charge finish*/
	if(BATREGUEN)
		axp228_i2c_set_bits(power,0x34,0x20); //enable
	else
		axp228_i2c_clr_bits(power,0x34,0x20); //disable
 
	if(BATDET)
		axp228_i2c_set_bits(power,AXP22_PDBC,0x40);
	else
		axp228_i2c_clr_bits(power,AXP22_PDBC,0x40);
 	

	/* RDC initial */
	axp228_i2c_read(power, AXP22_RDC0,&val2);
	if(ocv_cmp || ((BATRDC) && (!(val2 & 0x40))))
	{
		rdc = (BATRDC * 10000 + 5371) / 10742;
		axp228_i2c_write(power, AXP22_RDC0, ((rdc >> 8) & 0x1F)|0x80);
		axp228_i2c_write(power,AXP22_RDC1,rdc & 0x00FF);
	}


	//probe RDC, OCV
	axp228_i2c_read(power,AXP22_BATFULLCAPH_RES,&val2);
	if(ocv_cmp || ((BATCAP) && (!(val2 & 0x80))))
	{
		Cur_CoulombCounter = BATCAP * 1000 / 1456;
		axp228_i2c_write(power, AXP22_BATFULLCAPH_RES, ((Cur_CoulombCounter >> 8) | 0x80));
		axp228_i2c_write(power, AXP22_BATFULLCAPL_RES, Cur_CoulombCounter & 0x00FF);		
	}
	else if(!BATCAP)
	{
		axp228_i2c_write(power, AXP22_BATFULLCAPH_RES, 0x00);
		axp228_i2c_write(power, AXP22_BATFULLCAPL_RES, 0x00);
	}

	return ret;
}

static int axp228_device_setup(struct axp228_power *power)
{
	int		bus = power->i2c_bus;
	int ret = 0;
#if defined(CONFIG_PMIC_REG_DUMP)
	int i;
	u8 value = 0;
#endif

	PMIC_DBGOUT("%s\n", __func__);

	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	i2c_set_bus_num(bus);

#if defined(CONFIG_PMIC_REG_DUMP)
	printf("##########################################################\n");
	printf("##\e[31m PMIC OTP Value \e[0m \n");
	printf("##      0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F\n");
	for(i=0; i<=AXP228_NUM_OF_REGS; i++)
	{
		if(i%16 == 0)
			printf("## %02X:", i);
		if(i%4 == 0)
			printf(" ");

		ret = axp228_i2c_read(power, i, &value);
		if(!ret)
		{
			printf("%02x ", value);
		}
		else
		{
			printf("\e[31mxx\e[0m: %d \n", ret);
			break;
		}
		if((i+1)%16 == 0)
			printf("\n");
	}
	printf("##########################################################\n");
#endif

	axp228_param_setup(power);

#if defined(CONFIG_PMIC_REG_DUMP)
	axp228_register_dump(power);
#endif

	return ret;
}

int power_pmic_function_init(void)
{
	int ret = 0;
	int i2c_bus = CONFIG_PMIC_I2C_BUS;
	PMIC_DBGOUT("%s\n", __func__);

	ret = power_pmic_init(i2c_bus);
	ret = power_bat_init(i2c_bus);
	ret = power_muic_init(i2c_bus);
	ret = power_fg_init(i2c_bus);
#if defined(CONFIG_POWER_REGU_AXP228)
	ret = power_regu_init(i2c_bus);
#endif

	return ret;
}

int bd_pmic_init(void)
{
	struct axp228_power nxe_power_config = {
		.i2c_addr = AXP228_I2C_ADDR,
		.i2c_bus = CONFIG_PMIC_I2C_BUS,
	};
	PMIC_DBGOUT("%s\n", __func__);

	axp228_device_setup(&nxe_power_config);
	return 0;
}

#if defined(CONFIG_BAT_CHECK)
#if defined(CONFIG_DISPLAY_OUT)
#define HEADER_PIXEL(data,pixel) {\
pixel[0] = (((data[0] - 33) << 2) | ((data[1] - 33) >> 4)); \
pixel[1] = ((((data[1] - 33) & 0xF) << 4) | ((data[2] - 33) >> 2)); \
pixel[2] = ((((data[2] - 33) & 0x3) << 6) | ((data[3] - 33))); \
data += 4; \
}

#define IMAGE_WIDTH 		240
#define IMAGE_HEIGHT		1
#define IMAGE_PIXELBYTE		4

static char *color1 =
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!(#M=.6)V-6]R+'AI"
	")(-A&H=7#(1)!()!!(9!!(A!!(E!!(I!!(I!!8M\"&9]6-;QR.\\%X.<!V.+]U.+YU"
	"-[UT-KQS-;MR-+IQ,[IP,KEO,;=N,+=M,+9M+K5K+K1K++-I*[)H*[%H*K!G*;!F"
	"**]E)ZUD)JUC)JQC)*MA)*IA(ZI@(ZE@(:A>(:=>(*=='J1;':-:'J1;'*)9%YU4"
	"\"I!'!XU$!XU$!XU$!XU$!XU$!XU$!XU$!XY$!XY$!XY$!XY$\"8]&\"9!&\"(Y%\"(Y%"
	"\"I!'\"9!&\"9!&#)))$9=.$IA/$IA/$YI0%)I1%)M1%9M2%IQ3%YU4&)]5&)]5&:!6"
	"&J!7&Z%8'*)9'*-9':-:'J1;'Z5<(*==(:A>(JA?(ZE@)*IA)*MA)JUC)ZUD*:]F"
	"+K1K-;MR.+]U.\\%X/\\9\\0\\E`1LV#2]&(3=2*2]&(2,V%2LZ'2<J&-K)S*9MF*(UE"
	".(QU4Y60;9VJ:HJG0UE`#1U*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"";

#if 0
static char *color2 =
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!,35;4U1Q7E9L95-C"
	";$Y;;T13;#E(:C!!;\"]!;2]!;B]!;R]!;B]!<#%\"@4)3FEMLGV!QG5YPG5UOG%UN"
	"FUQMFUMLF5IKF%EKF%EJEUAIEE=HEE9GE59GE%5FDU1EDE-DD5)CD5%CD%%BCU!A"
	"CD]ACDY@C4U?C$U>C$Q=BDM<BDM<B4I<B4I;B$E:ATA9A497A$57A497A$56?T!1"
	"=#5&<3)#<3)#<3)#<3)#<3)#<3)#<3)#<C-$<C-$<C-$<C-$<S-%<S1%<S-$<S-$"
	"=#1&<S1%<S1%=39(>CM,>SQ->SQ-?#U.?3U.?3Y/?CY0?C]0?T!2@$%2@$%2@D)3"
	"@D-4@T-5A$56A$56A$57A497AT=8ATA9B4I;B4I;B4I<BDM<C$Q=C4U?CDY@CU!A"
	"DU1FF5IKG5UOGV!QHF-UIF9XJ6I[K&U_KV^!K&U_JFI[JFQ]IVM\\E5IK@T]?>DQ>"
	">UAJA&R!C'^:?7F;45)[%AU*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"";

static char *color3 =
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!-\"M<6T1U:3]P<S9G"
	"?\"]@?R56?!A)>!!!>Q!!?1!!?A!!?A!!?A!!?Q%\"DR56L$%RM4=XM$5VLT1ULD1U"
	"L4-TL$)SKT%RKD!QKC]PK3YOJSUNJSQMJCQMJ3IKJ#IKISAIIC=HI3=HI#9GI#5F"
	"HS1EH3-DH3)CH#)CGS!AGC!AGB]@G2]@G\"U>FRU>FRQ=F\"I;ERE:F\"I;EBA9D2-4"
	"A!9'@1-$@1-$@1-$@1-$@1-$@1-$@1-$@A-$@A-$@A-$@A-$@Q5&A!5&@A1%@A1%"
	"A!9'A!5&A!5&AAA)BQU.C!Y/C!Y/CA]0CB!1CR!1CR%2D\")3D2-4DR15DR15E\"56"
	"E\"97E2=8EBA9ERA9ERE:F\"I;F2M<FRQ=G\"U>G\"Y?G2]@GC!AGS!AH3)CH3-DHS5F"
	"J#IKKT%RLT1UM4=XNDM\\O4]`P5*#Q5>(R%F*Q5>(PE.$PE6&OU2%J4%RES-DBS%B"
	"C#]PE%B)FG\"AAF^@54Q]&!E*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	"";
#endif

#define	AXP228_RGB888TO565(c)		((((c>>16)&0xFF)&0xF8)<<8) | ((((c>>8)&0xFF)&0xFC)<<3) | ((((c>>0 )&0xFF)&0xF8)>>3)

static void AXP228_PutPixel888To565(unsigned int base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	*(U16*)(base + (ypos * width + xpos) * 2) = (U16)AXP228_RGB888TO565(color);
}

static void AXP228_PutPixel888To888(unsigned int base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	base = base + (ypos * width + xpos) * 3;
	*(U8*)(base++) = ((color>> 0)&0xFF);	// B
	*(U8*)(base++) = ((color>> 8)&0xFF);	// G
	*(U8*)(base)   = ((color>>16)&0xFF);	// R
}

static void AXP228_PutPixel888To8888(unsigned int base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	*(U32*)(base + (ypos * width + xpos) * 4) = (0xFF000000) | (color & 0xFFFFFF);
}


static void (*AXP228_PUTPIXELTABLE[])(U32, int, int, int, int, U32) =
{
	AXP228_PutPixel888To565,
	AXP228_PutPixel888To888,
	AXP228_PutPixel888To8888,
};

void axp228_raw_image_draw(lcd_info *lcd, int sx, int sy, unsigned int *img_data, int img_width, int img_height, int pixelbyte)
{
	lcd_info *plcd = lcd;
	int x = 0, y = 0;

	void (*draw_pixel)(unsigned int, int, int, int, int, unsigned int)
						= AXP228_PUTPIXELTABLE[pixelbyte-2];

	for (y = 0; y < img_height; y++)
	{
		for (x = 0; x < img_width; x++)
		{
			draw_pixel(plcd->fb_base, sx+x, sy+y, plcd->lcd_width, plcd->lcd_height, img_data[img_width*y+x]);
		}
	}

	flush_dcache_all();
}

#define GAUGE_MAX					235

static int skip_check(struct power_battery *pb, int bat_state)
{
	static int skip_bat_ani = 0;
	int ret = 0;

	if(bat_state != 3)
	{
		if (!gpio_get_value(GPIO_POWER_KEY_DET))
			skip_bat_ani++;
		else
			skip_bat_ani = 0;

		if ((skip_bat_ani > 20) && (pb->bat->voltage_uV > BAT_CUTOFF_VOL))
		{
			printf("\n");
			ret = 1;
		}
	}

	if(ctrlc())
	{
		printf("\n");
		ret = 1;
	}

	return ret;	
}
#endif

int power_battery_check(int skip, void (*bd_display_run)(char *, int, int))
{
#if defined(CONFIG_DISPLAY_OUT)
	lcd_info lcd = {
		.fb_base		= CONFIG_FB_ADDR,
		.bit_per_pixel	= CFG_DISP_PRI_SCREEN_PIXEL_BYTE * 8,
		.lcd_width		= CFG_DISP_PRI_RESOL_WIDTH,
		.lcd_height		= CFG_DISP_PRI_RESOL_HEIGHT,
		.back_color		= 0x000000,
		.text_color		= (0<<16) + (255 << 8) + (255),
		.dbg_win_left	= 10,
		.dbg_win_width	= 1260,
		.dbg_win_top	= 10,
		.dbg_win_height	= 780,
		.alphablend		= 0,
	};
#endif

#if defined(CONFIG_PMIC_REG_DUMP)
	struct axp228_power nxe_power_config = {
		.i2c_addr = AXP228_I2C_ADDR,
		.i2c_bus = CONFIG_PMIC_I2C_BUS,
	};
#endif

	struct power_battery *pb;
	struct pmic *p_fg, *p_chrg, *p_muic, *p_bat;

	int ret=0, i=0;
	int chrg;
	int shutdown_ilim_uV = BAT_LOWBAT_BATTERY_VOL;
	int bl_duty = CFG_LCD_PRI_PWM_DUTYCYCLE;
	int show_bat_state = 0;
	int skip_bat_ani = 0;
	u8  power_depth = 6;
	u32 chg_state=0, val=0, poweron=0;
	u16 tmp=0;

	PMIC_DBGOUT("%s\n", __func__);

	p_chrg = pmic_get("PMIC_AXP228");
	if (!p_chrg) {
		printf("PMIC_AXP228: Not found\n");
		return -ENODEV;
	}

	p_bat = pmic_get("BAT_AXP228");
	if (!p_bat) {
		printf("BAT_AXP228: Not found\n");
		return -ENODEV;
	}

	p_muic = pmic_get("MUIC_AXP228");
	if (!p_muic) {
		printf("MUIC_AXP228: Not found\n");
		return -ENODEV;
	}

	p_fg = pmic_get("FG_AXP228");
	if (!p_fg) {
		printf("FG_AXP228: Not found\n");
		return -ENODEV;
	}

	p_bat->low_power_mode = NULL;
	p_bat->pbat->battery_init(p_bat, p_fg, p_chrg, p_muic);
	pb = p_bat->pbat;

	chrg = p_muic->chrg->chrg_type(p_muic, 1);
	p_fg->fg->fg_battery_check(p_fg, p_bat);

	pmic_reg_read(p_chrg, AXP22_DCDC_MODESET, &val);
	printf("## DCDC_MODE(0x%02x): DCDC1[%s], DCDC2[%s], DCDC3[%s], DCDC4[%s], DCDC5[%s] \n",
				AXP22_DCDC_MODESET,
				(val&(1 << AXP_DCDC1_MODE_BIT)) ? "PWM":"PFM",
				(val&(1 << AXP_DCDC2_MODE_BIT)) ? "PWM":"PFM",
				(val&(1 << AXP_DCDC3_MODE_BIT)) ? "PWM":"PFM",
				(val&(1 << AXP_DCDC4_MODE_BIT)) ? "PWM":"PFM",
				(val&(1 << AXP_DCDC5_MODE_BIT)) ? "PWM":"PFM");
	printf("## STATUS(0x%02x)   : ", AXP22_STATUS);
	for(i=AXP22_STATUS; i<=AXP22_MODE_CHGSTATUS; i++)
	{
		pmic_reg_read(p_chrg, (u32)i, &val);	printf("0x%02x ", val);
	}
	printf("\n");
	printf("## IRQ(0x%02x)      : ", AXP22_INTSTS1);
	for(i=AXP22_INTSTS1; i<=AXP22_INTSTS5; i++)
	{
		if(i == AXP22_INTSTS1)
		{
			pmic_reg_read(p_chrg, (u32)i, &poweron);	printf("0x%02x ", poweron); 	pmic_reg_write(p_chrg, (u32)i, 0xff);
		}
		else
		{
			pmic_reg_read(p_chrg, (u32)i, &val);	printf("0x%02x ", val); 	pmic_reg_write(p_chrg, (u32)i, 0xff);
		}
	}
	printf("\n");
	printf("## CHG_TYPE       : %s\n", chrg == CHARGER_USB ? "USB" : (chrg == CHARGER_TA ? (((chg_state>>6) & 0x2) ? "ADP(USB)" : "ADP"): "NONE"));
	printf("## BAT_VOL        : %dmV \n", pb->bat->voltage_uV/1000);
	printf("## BAT_CAP        : %d%%\n", pb->bat->capacity);

#if 1
	if(chrg == CHARGER_USB)
	{
		shutdown_ilim_uV = BAT_LOWBAT_USB_PC_VOL;
	}
	else if(chrg == CHARGER_TA)
	{
		shutdown_ilim_uV = BAT_LOWBAT_ADP_VOL;
	}
	else
	{
		shutdown_ilim_uV = BAT_LOWBAT_BATTERY_VOL;
	}


	if(pb->bat->voltage_uV < BAT_CUTOFF_VOL)
	{
		goto enter_shutdown;
	}
	else if(pb->bat->voltage_uV < shutdown_ilim_uV)
	{
		show_bat_state = 2;
		skip_bat_ani = 0;
		if(chrg == CHARGER_NO || chrg == CHARGER_UNKNOWN)
		{
			bl_duty = (CFG_LCD_PRI_PWM_DUTYCYCLE / 25);
			power_depth = 3;
		}
	}
	else if((poweron & AXP22_IRQ_ACIN) || (poweron & AXP22_IRQ_USBIN))
	{
		show_bat_state = 1;
		skip_bat_ani = 0;
	}
	else
	{
		show_bat_state = 0;
		skip_bat_ani = 2;
	}

#else

	if(pb->bat->capacity <= BATLOW_ANIMATION_CAP)
	{
		show_bat_state = 2;
		skip_bat_ani = 0;
		if(chrg == CHARGER_NO || chrg == CHARGER_UNKNOWN)
		{
			bl_duty = (CFG_LCD_PRI_PWM_DUTYCYCLE / 25);
			power_depth = 3;
		}
	}
	else if((poweron & AXP22_IRQ_ACIN) || (poweron & AXP22_IRQ_USBIN))
	{
		show_bat_state = 1;
		skip_bat_ani = 0;
	}
	else
	{
		show_bat_state = 0;
		skip_bat_ani = 2;
	}
#endif

	/*===========================================================*/
	if(skip)
		skip_bat_ani = 2;

#if defined(CONFIG_DISPLAY_OUT)
	/*===========================================================*/
    if (skip_bat_ani > 1)
    {
        //bd_display_run(CONFIG_CMD_LOGO_WALLPAPERS, bl_duty, 1);
        goto skip_bat_animation;
    }
    else if (show_bat_state)
    {
        memset((void*)lcd.fb_base, 0, lcd.lcd_width * lcd.lcd_height * (lcd.bit_per_pixel/8));
        bd_display_run(CONFIG_CMD_LOGO_BATTERY, bl_duty, 1);
    }

	/*===========================================================*/
	// draw charing image
	if (show_bat_state)
	{
		int lcdw = lcd.lcd_width, lcdh = lcd.lcd_height;
		int bmpw = 240, bmph = 320;
		int bh = 22;
		int bx = 80, by = 61;
		int sx, sy, dy, str_dy;
		int cnt=0, frame_time=5000, percent=0, max_percent=0;

		ulong start_time_us=0, time_us=0, time_s=0, time_old=0;
		int ani_time = power_depth+6;

		char *org_image=NULL;

		char *str_ac_charging	= " AC Charging...   ";
		char *str_usb_charging	= " USB Charging...  ";
		char *str_discharging	= " Discharging...   ";
		char *str_lowbatt  		= " Low Battery...   ";
		char *str_nobatt  		= " No Battery...    ";

		sx = (lcdw - bmpw)/2 + bx;
		sy = (lcdh - bmph)/2 + by;
		dy = sy + (bh+2)*(9);
		str_dy = dy;

		if(!p_chrg->chrg->chrg_bat_present(p_chrg))
		{
			printf("## No Battery \n");
			// show_bat_state = 3;
		}

		lcd_debug_init(&lcd);

		if(show_bat_state == 3)
			lcd_draw_text(str_nobatt, (lcdw - strlen(str_lowbatt)*8*3)/2 + 30, str_dy+100, 3, 3, 0);
		else if(show_bat_state == 2)
		{
			lcd_draw_text(str_lowbatt, (lcdw - strlen(str_lowbatt)*8*3)/2 + 30, str_dy+100, 3, 3, 0);
		}
		else
		{
			if(chrg == CHARGER_TA)
				lcd_draw_text(str_ac_charging, (lcdw - strlen(str_ac_charging)*8*3)/2 + 30, str_dy+100, 3, 3, 0);
			else if(chrg == CHARGER_USB)
				lcd_draw_text(str_usb_charging, (lcdw - strlen(str_usb_charging)*8*3)/2 + 30, str_dy+100, 3, 3, 0);
			else
				lcd_draw_text(str_discharging, (lcdw - strlen(str_discharging)*8*3)/2 + 30, str_dy+100, 3, 3, 0);
		}

		start_time_us = timer_get_us();

		max_percent = (pb->bat->capacity*GAUGE_MAX)/100;
		if(max_percent < 5)
			max_percent = 5;

		while(1)
		{
			time_us = timer_get_us();
			if(time_us-start_time_us > 1000000)
			{
				start_time_us = timer_get_us();
				time_s++;
				printf(".");
				p_fg->fg->fg_battery_check(p_fg, p_bat);
			}

			if(cnt <= max_percent)
			{
				unsigned int img_data[IMAGE_WIDTH*IMAGE_HEIGHT] = {0};
				int j=0;

				percent = (++cnt*100)/max_percent;
				if(percent >= 90)
					frame_time = 30000;
				else
					frame_time = 5000;

#if 0 
				if(pb->bat->capacity < 20)
					org_image = color3;
				else if(pb->bat->capacity < 50)
					org_image = color2;
				else
#endif 
					org_image = color1;

				for(j=0; j<IMAGE_WIDTH; j++)
				{
					char temp[3];
					HEADER_PIXEL(org_image,temp);
					img_data[j] = (temp[0]<<16)|(temp[1] << 8)|temp[2];
					//lcd_fill_rectangle(sx+j-bx, 438-cnt, 240, 1, img_data[j], 0);
				}
				axp228_raw_image_draw(&lcd, sx-bx, 438-cnt, img_data, IMAGE_WIDTH, IMAGE_HEIGHT, IMAGE_PIXELBYTE);

				udelay(frame_time);

				time_old = time_s;
			}
			else
			{
				if(time_s-time_old >= 2)
				{
					time_old = time_s;
					cnt = 0;
					printf("\n");
					lcd_draw_boot_logo(CONFIG_FB_ADDR, CFG_DISP_PRI_RESOL_WIDTH, CFG_DISP_PRI_RESOL_HEIGHT, CFG_DISP_PRI_SCREEN_PIXEL_BYTE);
				}
				udelay(100000);
			}

			if(skip_check(pb, show_bat_state))
			{
				printf("\n");
		        goto skip_bat_animation;
			}

			if(time_s >= ani_time)
			{
				printf("\n");
				goto enter_shutdown;
			}

		}
	}

skip_bat_animation:
	bd_display_run(CONFIG_CMD_LOGO_WALLPAPERS, bl_duty, 1);
#endif  /* CONFIG_DISPLAY_OUT */

	printf("## Skip BAT Animation. \n");
	//mdelay(200);

#if defined(CONFIG_PMIC_REG_DUMP)
	axp228_register_dump(&nxe_power_config);
#endif

	p_fg->fg->fg_battery_check(p_fg, p_bat);

	printf("## IRQ(0x%02x)   : ", AXP22_INTSTS1);
	for(i=AXP22_INTSTS1; i<=AXP22_INTSTS5; i++)
	{
		pmic_reg_read(p_chrg, (u32)i, &val);	printf("0x%02x ", val);	pmic_reg_write(p_chrg, (u32)i, 0xff);
	}
	printf("\n");
	printf("## chg_type    : %s \n", (chrg == CHARGER_USB ? "USB" : (chrg == CHARGER_TA ? (((chg_state>>6) & 0x2) ? "ADP(USB)" : "ADP"): "NONE")));
	pmic_reg_read(p_chrg, AXP22_VBATH_RES, &val);
	tmp = (val<< 8);
	pmic_reg_read(p_chrg, AXP22_VBATL_RES, &val);
	tmp |= val;
	printf("## battery_vol : %dmV \n", pb->bat->voltage_uV/1000);
	printf("## battery_cap : %d%%\n", pb->bat->capacity);
	printf("## Booting \n");

#if 1
	if(chrg == CHARGER_USB)
		axp228_set_charging_current(CHARGE_CURRENT_500);
	else
#endif
		axp228_set_charging_current(POWEROFF_CHARGE_CURRENT);

	//chrg = p_muic->chrg->chrg_type(p_muic, 1);
	if (chrg == CHARGER_USB)
	{
		ret = 1;
	}

	return ret;

#if defined(CONFIG_DISPLAY_OUT)
enter_shutdown:

#if defined(CONFIG_PMIC_REG_DUMP)
	axp228_register_dump(&nxe_power_config);
#endif
	p_fg->fg->fg_battery_check(p_fg, p_bat);

	printf("## IRQ(0x%02x)   : ", AXP22_INTSTS1);
	for(i=AXP22_INTSTS1; i<=AXP22_INTSTS5; i++)
	{
		pmic_reg_read(p_chrg, (u32)i, &val);	printf("0x%02x ", val);	pmic_reg_write(p_chrg, (u32)i, 0xff);
	}
	printf("\n");
	printf("## chg_type    : %s \n", (chrg == CHARGER_USB ? "USB" : (chrg == CHARGER_TA ? (((chg_state>>6) & 0x2) ? "ADP(USB)" : "ADP"): "NONE")));
	pmic_reg_read(p_chrg, AXP22_VBATH_RES, &val);
	tmp = (val<< 8);
	pmic_reg_read(p_chrg, AXP22_VBATL_RES, &val);
	tmp |= val;
	printf("## battery_vol : %dmV \n", pb->bat->voltage_uV/1000);
	printf("## battery_cap : %d%%\n", pb->bat->capacity);
	printf("## Power Off\n");

	mdelay(500);

#if 1
	if(chrg == CHARGER_USB)
		axp228_set_charging_current(CHARGE_CURRENT_500);
	else
#endif
		axp228_set_charging_current(POWEROFF_CHARGE_CURRENT);

	pmic_reg_write(p_chrg, AXP22_BUFFERC, 0x00);
	mdelay(20);

	pmic_reg_read(p_chrg, AXP22_OFF_CTL, &val);
	val |= 0x80;
	pmic_reg_write(p_chrg, AXP22_OFF_CTL, val);
	
	while(1);

	return 0;
#endif
}
#endif /* CONFIG_BAT_CHECK */


