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

#ifndef __NXE2000_POWER_H_
#define __NXE2000_POWER_H_

#include <power/pmic.h>
#include <platform.h>
#include "nxe2000-private.h"


/*
 *	Config Value
 */

#define NXE2000_I2C_ADDR					(0x64 >> 1)

#define CHARGER_MIN_CURRENT					100		/* VAL(mV) */
#define CHARGER_MAX_CURRENT					800		/* VAL(mV) */
#define CHARGER_CURRENT_RESOLUTION			100		/* VAL(mV) */
#define CHARGER_CURRENT_COMPLETE			3		/* (n + 1) x 50mA   */

#define GPIO_OTG_USBID_DET					(PAD_GPIO_D + 16)
#define GPIO_OTG_VBUS_DET					(PAD_GPIO_D + 21)
#define GPIO_PMIC_VUSB_DET					(PAD_GPIO_ALV + 2)
#define GPIO_PMIC_LOWBAT_DET				(PAD_GPIO_ALV + 3)
#define GPIO_POWER_KEY_DET					(PAD_GPIO_ALV + 0)

/*
 *	Default Value
 */

#define	NXE2000_DEF_OFF_PRESS_TIME			5		/* VAL = 0:0, 1:1, 2:2, 3:4, 4:6, 5:8, 6:10, 7:12 sec */

#define NXE2000_DEF_LIMIT_ADP_AMP			2500000	/* VAL(mA), MAX = 2500mA */
#define NXE2000_DEF_LIMIT_USB_AMP			1500000	/* VAL(mA), MAX = 1500mA */
#define NXE2000_DEF_LIMIT_USBDATA_AMP		 500000	/* VAL(mA), MAX = 1500mA */

#define NXE2000_DEF_CHG_ADP_AMP				1500000	/* VAL(mA), MAX = 1800mA */
#define NXE2000_DEF_CHG_USB_AMP				1500000	/* VAL(mA), MAX = 1800mA */

#define	NXE2000_DEF_CUTOFF_VOL				3300000	/* VAL(uV), No Charger */		/* Refer form ADC	*/
#define	NXE2000_DEF_LOWBAT_BATTERY_VOL		3500000	/* VAL(uV), No Charger */		/* Refer form ADC	*/
#define	NXE2000_DEF_LOWBAT_USB_PC_VOL		3600000	/* VAL(uV), USB Charger */		/* Refer form ADC	*/
#define	NXE2000_DEF_LOWBAT_USB_ADP_VOL		3660000	/* VAL(uV), USB Charger  */	/* Refer form ADC	*/
#define	NXE2000_DEF_LOWBAT_ADP_VOL			3660000	/* VAL(uV), ADP Charger  */	/* Refer form ADC	*/

#define	NXE2000_DEF_DDC1_ON					1		/* VAL = 0: Off, 1: On, default(OTP) = On */	/* 1.3V ARM		*/
#define	NXE2000_DEF_DDC2_ON					1		/* VAL = 0: Off, 1: On, default(OTP) = On */	/* 1.2V CORE	*/
#define	NXE2000_DEF_DDC3_ON					1		/* VAL = 0: Off, 1: On, default(OTP) = On */	/* 3.3V SYS		*/
#define	NXE2000_DEF_DDC4_ON					1		/* VAL = 0: Off, 1: On, default(OTP) = On */	/* 1.6V DDR		*/
#define	NXE2000_DEF_DDC5_ON					1		/* VAL = 0: Off, 1: On, default(OTP) = On */	/* 1.6V SYS		*/

#define	NXE2000_DEF_LDO1_ON					0		/* VAL = 0: Off, 1: On */	/* 3.3V GPS & WIFI	*/
#define	NXE2000_DEF_LDO2_ON					0		/* VAL = 0: Off, 1: On */	/* 1.8V CAM1	*/
#define	NXE2000_DEF_LDO3_ON					1		/* VAL = 0: Off, 1: On */	/* 1.8V SYS1,	Boot On	*/
#define	NXE2000_DEF_LDO4_ON					1		/* VAL = 0: Off, 1: On */	/* 1.9V SYS,		Boot On	*/
#define	NXE2000_DEF_LDO5_ON					0		/* VAL = 0: Off, 1: On */	/* 2.8V CAM		*/
#define	NXE2000_DEF_LDO6_ON					1		/* VAL = 0: Off, 1: On */	/* 3.3V ALIVE,	Boot On	*/
#define	NXE2000_DEF_LDO7_ON					1		/* VAL = 0: Off, 1: On */	/* 2.8V VID,		Boot On	*/
#define	NXE2000_DEF_LDO8_ON					0		/* VAL = 0: Off, 1: On */	/* 3.3V none,	Not Use	*/
#define	NXE2000_DEF_LDO9_ON					0		/* VAL = 0: Off, 1: On */	/* 3.3V none,	Not Use	*/
#define	NXE2000_DEF_LDO10_ON				0		/* VAL = 0: Off, 1: On */	/* 1.2V none,	Not Use	*/
#define	NXE2000_DEF_LDORTC1_ON				1		/* VAL = 0: Off, 1: On */	/* 1.8V ALIVE,	Boot On	*/
#define	NXE2000_DEF_LDORTC2_ON				1		/* VAL = 0: Off, 1: On */	/* 1.0V ALIVE,	Boot On	*/

#define	NXE2000_DEF_DDC1_VOL				1125000	/* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 1.3V */
#define	NXE2000_DEF_DDC2_VOL				1100000	/* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 1.2V */
#define	NXE2000_DEF_DDC3_VOL				3300000	/* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 3.3V */
#define	NXE2000_DEF_DDC4_VOL				1500000	/* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 1.6V */
#define	NXE2000_DEF_DDC5_VOL				1500000	/* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 1.6V */

#define	NXE2000_DEF_DDC1_SLP_VOL			1125000	/* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 1.3V */
#define	NXE2000_DEF_DDC2_SLP_VOL			1100000	/* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 1.2V */
#define	NXE2000_DEF_DDC3_SLP_VOL			3300000	/* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 3.3V */
#define	NXE2000_DEF_DDC4_SLP_VOL			1500000	/* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 1.6V */
#define	NXE2000_DEF_DDC5_SLP_VOL			1500000	/* VAL(uV) = 0: 0.60 ~ 3.5V, Step 12.5 mV, default(OTP) = 1.6V */

#define	NXE2000_DEF_LDO1_VOL				3300000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 3.30V */
#define	NXE2000_DEF_LDO2_VOL				1800000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 1.80V */
#define	NXE2000_DEF_LDO3_VOL				1800000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 1.80V */
#define	NXE2000_DEF_LDO4_VOL				1900000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 1.90V */
#define	NXE2000_DEF_LDO5_VOL				2800000	/* VAL(uV) = 0: 0.60 ~ 3.5V, Step = 25mV, default(OTP) = 2.80V */
#define	NXE2000_DEF_LDO6_VOL				3300000	/* VAL(uV) = 0: 0.60 ~ 3.5V, Step = 25mV, default(OTP) = 3.30V */
#define	NXE2000_DEF_LDO7_VOL				2800000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 2.80V */
#define	NXE2000_DEF_LDO8_VOL				3300000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 3.30V */
#define	NXE2000_DEF_LDO9_VOL				3300000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 3.30V */
#define	NXE2000_DEF_LDO10_VOL				1200000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 1.20V */
#define	NXE2000_DEF_LDORTC1_VOL				1800000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 3.30V */
#define	NXE2000_DEF_LDORTC2_VOL				1000000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 1.20V */

#define	NXE2000_DEF_LDO1_SLP_VOL			3300000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 3.30V */
#define	NXE2000_DEF_LDO2_SLP_VOL			1800000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 1.80V */
#define	NXE2000_DEF_LDO3_SLP_VOL			1800000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 1.80V */
#define	NXE2000_DEF_LDO4_SLP_VOL			1900000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 1.90V */
#define	NXE2000_DEF_LDO5_SLP_VOL			2800000	/* VAL(uV) = 0: 0.60 ~ 3.5V, Step = 25mV, default(OTP) = 2.80V */
#define	NXE2000_DEF_LDO6_SLP_VOL			3300000	/* VAL(uV) = 0: 0.60 ~ 3.5V, Step = 25mV, default(OTP) = 3.30V */
#define	NXE2000_DEF_LDO7_SLP_VOL			2800000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 2.80V */
#define	NXE2000_DEF_LDO8_SLP_VOL			3300000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 3.30V */
#define	NXE2000_DEF_LDO9_SLP_VOL			3300000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 3.30V */
#define	NXE2000_DEF_LDO10_SLP_VOL			1200000	/* VAL(uV) = 0: 0.90 ~ 3.5V, Step = 25mV, default(OTP) = 1.20V */

#define	NXE2000_DEF_DDCx_VOL_STEP			12500	/* UINT = 1uV, 12.5mV */
#define	NXE2000_DEF_LDOx_VOL_STEP			25000	/* UINT = 1uV, 25mV */

#define	NXE2000_DEF_DDCx_VOL_MIN			600000	/* UINT = 1uV, 0.6V */
#define	NXE2000_DEF_DDCx_VOL_MAX			3500000	/* UINT = 1uV, 3.5V */

#define	NXE2000_DEF_LDOx_VOL_MIN			900000	/* UINT = 1uV, 0.9V */
#define	NXE2000_DEF_LDOx_VOL_MAX			3500000	/* UINT = 1uV, 3.5V */
#define	NXE2000_DEF_LDO56_VOL_MIN			600000	/* UINT = 1uV, 0.6V */
#define	NXE2000_DEF_LDO56_VOL_MAX			3500000	/* UINT = 1uV, 3.5V */
#define	NXE2000_DEF_LDORTC1_VOL_MIN			1700000	/* UINT = 1uV, 1.7V */
#define	NXE2000_DEF_LDORTC1_VOL_MAX			3500000	/* UINT = 1uV, 3.5V */

#define	NXE2000_DEF_DDC1_SLP_MODE			0		/* VAL = 0:AUTO(default), 1:PWM, 2:PSM, 3:AUTO */
#define	NXE2000_DEF_DDC2_SLP_MODE			0		/* VAL = 0:AUTO(default), 1:PWM, 2:PSM, 3:AUTO */
#define	NXE2000_DEF_DDC3_SLP_MODE			0		/* VAL = 0:AUTO(default), 1:PWM, 2:PSM, 3:AUTO */
#define	NXE2000_DEF_DDC4_SLP_MODE			0		/* VAL = 0:AUTO(default), 1:PWM, 2:PSM, 3:AUTO */
#define	NXE2000_DEF_DDC5_SLP_MODE			0		/* VAL = 0:AUTO(default), 1:PWM, 2:PSM, 3:AUTO */

#define	NXE2000_DEF_DDC1_MODE				0		/* VAL = 0:AUTO(default), 1:PWM, 2:PSM, 3:AUTO */
#define	NXE2000_DEF_DDC2_MODE				0		/* VAL = 0:AUTO(default), 1:PWM, 2:PSM, 3:AUTO */
#define	NXE2000_DEF_DDC3_MODE				0		/* VAL = 0:AUTO(default), 1:PWM, 2:PSM, 3:AUTO */
#define	NXE2000_DEF_DDC4_MODE				0		/* VAL = 0:AUTO(default), 1:PWM, 2:PSM, 3:AUTO */
#define	NXE2000_DEF_DDC5_MODE				0		/* VAL = 0:AUTO(default), 1:PWM, 2:PSM, 3:AUTO */

#define	NXE2000_DEF_DDC1_DSC_CTRL			1		/* VAL = 0: Off, 1: On(default) */
#define	NXE2000_DEF_DDC2_DSC_CTRL			1		/* VAL = 0: Off, 1: On(default) */
#define	NXE2000_DEF_DDC3_DSC_CTRL			1		/* VAL = 0: Off, 1: On(default) */
#define	NXE2000_DEF_DDC4_DSC_CTRL			1		/* VAL = 0: Off, 1: On(default) */
#define	NXE2000_DEF_DDC5_DSC_CTRL			1		/* VAL = 0: Off, 1: On(default) */

#define	NXE2000_DEF_DDC1_OSC_FREQ			0		/* VAL(MHz) = 0: 1.5, 1: prohibit, 2: 3.0, 3: prohibit, default(OTP) = 1.5MHz */
#define	NXE2000_DEF_DDC2_OSC_FREQ			0		/* VAL(MHz) = 0: 1.5, 1: prohibit, 2: 3.0, 3: prohibit, default(OTP) = 1.5MHz */
#define	NXE2000_DEF_DDC3_OSC_FREQ			0		/* VAL(MHz) = 0: 1.5, 1: prohibit, 2: 3.0, 3: prohibit, default(OTP) = 1.5MHz */
#define	NXE2000_DEF_DDC4_OSC_FREQ			0		/* VAL(MHz) = 0: 1.5, 1: prohibit, 2: 3.0, 3: prohibit, default(OTP) = 1.5MHz */
#define	NXE2000_DEF_DDC5_OSC_FREQ			0		/* VAL(MHz) = 0: 1.5, 1: prohibit, 2: 3.0, 3: prohibit, default(OTP) = 1.5MHz */

#define	NXE2000_DEF_DDC1_RAMP_SLOP			0		/* VAL(mV/us) = 0: 14, 1: 7, 2: 3.5, 3: prohibit, default = 14mV/us */
#define	NXE2000_DEF_DDC2_RAMP_SLOP			0		/* VAL(mV/us) = 0: 14, 1: 7, 2: 3.5, 3: prohibit, default = 14mV/us */
#define	NXE2000_DEF_DDC3_RAMP_SLOP			0		/* VAL(mV/us) = 0: 14, 1: 7, 2: 3.5, 3: prohibit, default = 14mV/us */
#define	NXE2000_DEF_DDC4_RAMP_SLOP			0		/* VAL(mV/us) = 0: 14, 1: 7, 2: 3.5, 3: prohibit, default = 14mV/us */
#define	NXE2000_DEF_DDC5_RAMP_SLOP			0		/* VAL(mV/us) = 0: 14, 1: 7, 2: 3.5, 3: prohibit, default = 14mV/us */

#define	NXE2000_DEF_DDC1_CUR_LIMIT			3		/* VAL(A) = 0: No limit, 1: 3.2A, 2: 3.7A, 3: 4.0A, default(OTP) = 3.2A */
#define	NXE2000_DEF_DDC2_CUR_LIMIT			3		/* VAL(A) = 0: No limit, 1: 3.2A, 2: 3.7A, 3: 4.0A, default(OTP) = 3.2A */
#define	NXE2000_DEF_DDC3_CUR_LIMIT			1		/* VAL(A) = 0: No limit, 1: 3.2A, 2: 3.7A, 3: 4.0A, default(OTP) = 3.2A */
#define	NXE2000_DEF_DDC4_CUR_LIMIT			1		/* VAL(A) = 0: No limit, 1: 3.2A, 2: 3.7A, 3: 4.0A, default(OTP) = 3.2A */
#define	NXE2000_DEF_DDC5_CUR_LIMIT			1		/* VAL(A) = 0: No limit, 1: 3.2A, 2: 3.7A, 3: 4.0A, default(OTP) = 3.2A */

#define	NXE2000_DEF_DDC1_LIMSHUT_EN			1		/* VAL = 0: Disable, 1: Enable */
#define	NXE2000_DEF_DDC2_LIMSHUT_EN			1		/* VAL = 0: Disable, 1: Enable */
#define	NXE2000_DEF_DDC3_LIMSHUT_EN			1		/* VAL = 0: Disable, 1: Enable */
#define	NXE2000_DEF_DDC4_LIMSHUT_EN			1		/* VAL = 0: Disable, 1: Enable */
#define	NXE2000_DEF_DDC5_LIMSHUT_EN			1		/* VAL = 0: Disable, 1: Enable */

#define	NXE2000_DEF_ADC_AIN0				0		/* VAL = 0: Disable, 1: Enable, default = Disable*/
#define	NXE2000_DEF_ADC_AIN1				0		/* VAL = 0: Disable, 1: Enable, default = Disable*/
#define	NXE2000_DEF_ADC_VTHM				0		/* VAL = 0: Disable, 1: Enable, default = Disable*/
#define	NXE2000_DEF_ADC_VSYS				1		/* VAL = 0: Disable, 1: Enable, default = Disable*/
#define	NXE2000_DEF_ADC_VUSB				0		/* VAL = 0: Disable, 1: Enable, default = Disable*/
#define	NXE2000_DEF_ADC_VADP				0		/* VAL = 0: Disable, 1: Enable, default = Disable*/
#define	NXE2000_DEF_ADC_VBAT				1		/* VAL = 0: Disable, 1: Enable, default = Disable*/
#define	NXE2000_DEF_ADC_ILIM				0		/* VAL = 0: Disable, 1: Enable, default = Disable*/

#define	NXE2000_DEF_ADCCNT3_ADRQ			2		/* VAL = 0: Stop, 1: Single-mode, 2: Auto-mode, 3: Prohibit, default = Stop*/
#define	NXE2000_DEF_ADCCNT3_AVE				1		/* VAL = 0: one-time, 1: average of four-time, default = one-time*/
#define	NXE2000_DEF_ADCCNT3_ADSEL			0		/* Select input of ADC in single-mode A/D conbersion */

#define NXE2000_DEF_CHG_PRIORITY			0		/* VAL = 0: VADP,	1:VUSB, 	default = VUSB */
#define NXE2000_DEF_CHG_COMPLETE_DIS		0		/* VAL = 0: Enable,	1: Disable,	default = Enable */
#define NXE2000_DEF_CHG_NOBAT_OVLIM_EN		0		/* VAL = 0: Off, 		1: On, 	default = On */
#define NXE2000_DEF_CHG_OTG_BOOST			0		/* VAL = 0: Off, 		1: On, 	default = On */
#define NXE2000_DEF_CHG_SUSPEND				0		/* VAL = 0: Off, 		1: On, 	default = Off */
#define NXE2000_DEF_CHG_JEITAEN				0		/* VAL = 0: Off, 		1: On, 	default = Off */
#define NXE2000_DEF_CHG_USB_EN				1		/* VAL = 0: Off, 		1: On, 	default = On */
#define NXE2000_DEF_CHG_ADP_EN				1		/* VAL = 0: Off, 		1: On, 	default = On */

#define NXE2000_DEF_CHG_USB_VCONTMASK		0		/* VAL = 0: Off, 1: On, default(OTP) = Off */
#define NXE2000_DEF_CHG_ADP_VCONTMASK		0		/* VAL = 0: Off, 1: On, default(OTP) = Off */
#define NXE2000_DEF_CHG_VUSB_BUCK_THS		0		/* VAL(V) = 0: 4.1 ~ 4.2, 1: 4.2 ~ 4.3, 2: 4.3 ~ 4.4, 3: 4.4 ~ 4.5, default(OTP) = 4.1 ~ 4.2V */
#define NXE2000_DEF_CHG_VADP_BUCK_THS		0		/* VAL(V) = 0: 4.1 ~ 4.2, 1: 4.2 ~ 4.3, 2: 4.3 ~ 4.4, 3: 4.4 ~ 4.5, default(OTP) = 4.1 ~ 4.2V */

#define NXE2000_DEF_CHG_VSYS_VOL			3		/* VAL(V) = 0: 3.60, 1: 4.00, 2: 4.40, 3: 4.55, default = 4.00V */
#define NXE2000_DEF_CHG_VSYS_OVER_VOL		1		/* VAL(V) = 0: 4.8 ~ 5.3, 1: 5.0 ~ 5.5, 2: 5.2 ~ 5.7, 3: 5.2 ~ 5.7, default = 5.0 ~ 5.5V */

#define NXE2000_DEF_CHG_RAPID_TTIM_40		0
#define NXE2000_DEF_CHG_RAPID_TTIM_80		1
#define NXE2000_DEF_CHG_RAPID_TTIMDIS		2
#define NXE2000_DEF_CHG_RAPID_CTIME			2		/* VAL(sec) = 0: 0.1, 1: 0.5, 2: 1 3: 5, default = 1sec */
#define NXE2000_DEF_CHG_RAPID_RTIME			3		/* VAL(min) = 0: 120, 1: 180, 2: 240 3: 300, default = 300min */

#define NXE2000_DEF_CHG_POWER_ON_VOL		4		/* VAL(V) = 0: 2.7 ~ 2.8, 1: 2.8 ~ 2.9, 2: 2.9 ~ 3.0, 3: 3.0 ~ 3.1, 4: 3.1 ~ 3.2, 5: 3.2 ~ 3.3, 6: 3.3 ~ 3.4, 7: 3.4 ~ 3.5 */
													/* default(OTP) = 3.4 ~ 3.5V */
#define NXE2000_DEF_CHG_VBATOV_SET			0		/* VAL(V) = 0: 3.95 ~ 4.38, 1: 4.10 ~ 4.53, default = 3.95 ~ 4.38V */
#define NXE2000_DEF_CHG_VWEAK				1		/* VAL(V) = 0: 3.0 ~ 3.1, 1: 3.1 ~ 3.2, 2: 3.2 ~ 3.3, 3: 3.3 ~ 3.4, default(OTP) = 3.3 ~ 3.4V */
#define NXE2000_DEF_CHG_VDEAD				0		/* VAL(V) = 0: 2.9 ~ 3.0, 1: 2.8 ~ 2.9, default(OTP) = 2.9 ~ 3.0V */
#define NXE2000_DEF_CHG_VSHORT				1		/* VAL(V) = 0: 2.7 ~ 2.8, 1: 2.5 ~ 2.6, default(OTP) = 2.7 ~ 2.8V */

#define NXE2000_DEF_CHG_VFCHG				3		/* VAL(V) = 0: 4.05, 1: 4.10, 2: 4.15, 3: 4.20,  4 ~ 7: 4.35, default = 4.10V */
#define NXE2000_DEF_CHG_VRCHG				3		/* VAL(V) = 0: 3.85, 1: 3.90, 2: 3.95, 3: 4.00,  4 ~ 7: 4.10  default = 3.90V */

#define NXE2000_DEF_CHG_CURR_ADP 			24		/* VAL(mA) = 100mA x (n + 1), MAX = 2500mA, default(OTP) = 2500mA */
#define NXE2000_DEF_CHG_CURR_USB 			4		/* VAL(mA) = 100mA x (n + 1), MAX = 1500mA, default(OTP) = 500mA */
#define NXE2000_DEF_CHG_CURR_RAPID_ADP		17		/* VAL(mA) = 100mA x (n + 1), MAX = 1800mA, default(OTP) = 100mA */
#define NXE2000_DEF_CHG_CURR_RAPID_USB		5		/* VAL(mA) = 100mA x (n + 1), MAX = 1800mA, default(OTP) = 100mA */
#define NXE2000_DEF_CHG_CURR_RAPID_COMP 	0		/* VAL(mA) = 50mA x (n + 1), MAX = 200mA, default = 50mA */

#define NXE2000_DEF_DIE_RETURN_TEMP			0		/* VAL(C)= 0: 80, 1: 100, default(OTP) = 80' */
#define NXE2000_DEF_DIE_ERROR_TEMP			0		/* VAL(C)= 0: 100, 1: 120 */
#define NXE2000_DEF_DIE_SHUTDOWN_TEMP		1		/* VAL(C)= 0: 100, 1: 120, 2: 135, 3: 135, default(OTP) = 120'  */

#define NXE2000_DEF_CHG_READY_CUR			0		/* VAL(mA)= 0:0(deefault), 1:10 */
#define NXE2000_DEF_CHG_SUSPENDB			1

/* REPCNT */
#define	NXE2000_DEF_REPCNT_REPWRON			0
#define	NXE2000_DEF_REPCNT_REPWRTIM			0
#define	NXE2000_DEF_REPCNT_OFF_RESETO		0

/* WATHDOG */
#define	NXE2000_DEF_WATHDOG_WDOGTIM			3
#define	NXE2000_DEF_WATHDOG_WDOG_EN			0
#define	NXE2000_DEF_WATHDOG_WDOGSLPEN		0

/* PWRIREN */
#define	NXE2000_DEF_PWRIREN_WDOG			0
#define	NXE2000_DEF_PWRIREN_NOE_OFF			0
#define	NXE2000_DEF_PWRIREN_PWRON_OFF 		0
#define	NXE2000_DEF_PWRIREN_PREOT			0
#define	NXE2000_DEF_PWRIREN_PRVINDT			0
#define	NXE2000_DEF_PWRIREN_EXTIN			0
#define	NXE2000_DEF_PWRIREN_PWRON			0

#define NXE2000_NUM_BUCK					5
#define NXE2000_NUM_LDO						10
#define NXE2000_NUM_PSO						5

#define NXE2000_DEF_FET1CNT					0x05
#define NXE2000_DEF_FET2CNT					0x00
#define NXE2000_DEF_TSET					0x20
#define NXE2000_DEF_CMPSET					0x00
#define NXE2000_DEF_SUSPEND					0x01

#endif  //#ifndef __NXE2000_POWER_H_
