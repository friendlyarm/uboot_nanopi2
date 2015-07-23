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


#ifndef __LINUX_AXP228_H
#define __LINUX_AXP228_H

#include <axp228_cfg.h>

#define AXP228_I2C_ADDR				(0x68 >> 1)

/* AXP228 registers */
#define	AXP228_NUM_OF_REGS			0xEF


/* For AXP228 */ 
#define AXP22						(22)
#define AXP22_STATUS				(0x00)
#define AXP22_MODE_CHGSTATUS		(0x01)
#define AXP22_IC_TYPE				(0x03)
#define AXP22_BUFFER1				(0x04)
#define AXP22_BUFFER2				(0x05)
#define AXP22_BUFFER3				(0x06)
#define AXP22_BUFFER4				(0x07)
#define AXP22_BUFFER5				(0x08)
#define AXP22_BUFFER6				(0x09)
#define AXP22_BUFFER7				(0x0A)
#define AXP22_BUFFER8				(0x0B)
#define AXP22_BUFFER9				(0x0C)
#define AXP22_BUFFERA				(0x0D)
#define AXP22_BUFFERB				(0x0E)
#define AXP22_BUFFERC				(0x0F)
#define AXP22_IPS_SET				(0x30)
#define AXP22_VOFF_SET				(0x31)
#define AXP22_OFF_CTL				(0x32)
#define AXP22_PDBC					(0x32)
#define AXP22_CHARGE1				(0x33)
#define AXP22_CHARGE2				(0x34)
#define AXP22_CHARGE3				(0x35)
#define AXP22_POK_SET				(0x36)
#define AXP22_INTEN1				(0x40)
#define AXP22_INTEN2				(0x41)
#define AXP22_INTEN3				(0x42)
#define AXP22_INTEN4				(0x43)
#define AXP22_INTEN5				(0x44)
#define AXP22_INTSTS1				(0x48)
#define AXP22_INTSTS2				(0x49)
#define AXP22_INTSTS3				(0x4A)
#define AXP22_INTSTS4				(0x4B)
#define AXP22_INTSTS5				(0x4C)

#define AXP22_LDO_DC_EN1			(0X10)
#define AXP22_LDO_DC_EN2			(0X12)
#define AXP22_LDO_DC_EN3			(0X13)
#define AXP22_DLDO1OUT_VOL			(0x15)
#define AXP22_DLDO2OUT_VOL			(0x16)
#define AXP22_DLDO3OUT_VOL			(0x17)
#define AXP22_DLDO4OUT_VOL			(0x18)
#define AXP22_ELDO1OUT_VOL			(0x19)
#define AXP22_ELDO2OUT_VOL			(0x1A)
#define AXP22_ELDO3OUT_VOL 			(0x1B)
#define AXP22_DC5LDOOUT_VOL			(0x1C)
#define AXP22_DC1OUT_VOL			(0x21)
#define AXP22_DC2OUT_VOL			(0x22)
#define AXP22_DC3OUT_VOL			(0x23)
#define AXP22_DC4OUT_VOL			(0x24)
#define AXP22_DC5OUT_VOL			(0x25)
#define AXP22_GPIO0LDOOUT_VOL		(0x91)
#define AXP22_GPIO1LDOOUT_VOL		(0x93)
#define AXP22_ALDO1OUT_VOL			(0x28)
#define AXP22_ALDO2OUT_VOL			(0x29)
#define AXP22_ALDO3OUT_VOL			(0x2A)

#define AXP22_VBATH_RES				(0x78)
#define AXP22_VBATL_RES				(0x79)

#define AXP22_ICHGH_RES				(0x7A)
#define AXP22_ICHGL_RES				(0x7B)

#define AXP22_DISICHGH_RES			(0x7C)
#define AXP22_DISICHGL_RES			(0x7D)

#define AXP22_OCVBATH_RES			(0xBC)
#define AXP22_OCVBATL_RES			(0xBD)

#define AXP22_BATFULLCAPH_RES		(0xE0)
#define AXP22_BATFULLCAPL_RES		(0xE1)

#define AXP22_DCDC_MODESET			(0x80)
#define AXP22_DCDC_FREQSET			(0x37) 
#define AXP22_ADC_EN				(0x82)
#define AXP22_PWREN_CTL1			(0x8C)
#define AXP22_PWREN_CTL2			(0x8D)
#define AXP22_HOTOVER_CTL			(0x8F)

#define AXP22_GPIO0_CTL				(0x90)
#define AXP22_GPIO1_CTL				(0x92)
#define AXP22_GPIO01_SIGNAL			(0x94)
#define AXP22_BAT_CHGCOULOMB3		(0xB0)
#define AXP22_BAT_CHGCOULOMB2		(0xB1)
#define AXP22_BAT_CHGCOULOMB1		(0xB2)
#define AXP22_BAT_CHGCOULOMB0		(0xB3)
#define AXP22_BAT_DISCHGCOULOMB3	(0xB4)
#define AXP22_BAT_DISCHGCOULOMB2	(0xB5)
#define AXP22_BAT_DISCHGCOULOMB1	(0xB6)
#define AXP22_BAT_DISCHGCOULOMB0	(0xB7)
#define AXP22_COULOMB_CTL			(0xB8)

#define AXP22_ADC_CONTROL3			(0x84)

#define AXP22_CAP					(0xB9)
#define AXP22_RDC0					(0xBA)
#define AXP22_RDC1					(0xBB)

#define AXP22_OCV_TABLE				(0xC0)

#define AXP22_WARNING_LEVEL			(0xE6)
#define AXP22_ADJUST_PARA			(0xE8)

#define AXP22_CHARGE_VBUS			AXP22_IPS_SET


/* register bit position */


/* bit definitions for AXP events ,irq event */
/*  AXP22  */
#define	AXP22_IRQ_USBLO				(1 <<  1)
#define	AXP22_IRQ_USBRE				(1 <<  2)
#define	AXP22_IRQ_USBIN				(1 <<  3)
#define	AXP22_IRQ_USBOV				(1 <<  4)
#define	AXP22_IRQ_ACRE				(1 <<  5)
#define	AXP22_IRQ_ACIN				(1 <<  6)
#define	AXP22_IRQ_ACOV				(1 <<  7)
#define	AXP22_IRQ_TEMLO				(1 <<  8)
#define	AXP22_IRQ_TEMOV				(1 <<  9)
#define	AXP22_IRQ_CHAOV				(1 << 10)
#define	AXP22_IRQ_CHAST				(1 << 11)
#define	AXP22_IRQ_BATATOU			(1 << 12)
#define	AXP22_IRQ_BATATIN 			(1 << 13)
#define AXP22_IRQ_BATRE				(1 << 14)
#define AXP22_IRQ_BATIN				(1 << 15)
#define	AXP22_IRQ_POKLO				(1 << 16)
#define	AXP22_IRQ_POKSH				(1 << 17)
#define AXP22_IRQ_CHACURLO			(1 << 22)
#define AXP22_IRQ_ICTEMOV			(1 << 23)
#define AXP22_IRQ_EXTLOWARN2		(1 << 24)
#define AXP22_IRQ_EXTLOWARN1		(1 << 25)
#define AXP22_IRQ_GPIO0TG			((uint64_t)1 << 32)
#define AXP22_IRQ_GPIO1TG			((uint64_t)1 << 33)
#define AXP22_IRQ_GPIO2TG			((uint64_t)1 << 34)
#define AXP22_IRQ_GPIO3TG			((uint64_t)1 << 35)

#define AXP22_IRQ_PEKFE     		((uint64_t)1 << 37)
#define AXP22_IRQ_PEKRE     		((uint64_t)1 << 38)
#define AXP22_IRQ_TIMER     		((uint64_t)1 << 39)


/* Status Query Interface */
/*  AXP22  */
#define AXP22_STATUS_SOURCE			(1 << 0)
#define AXP22_STATUS_ACUSBSH		(1 << 1)
#define AXP22_STATUS_BATCURDIR		(1 << 2)
#define AXP22_STATUS_USBLAVHO		(1 << 3)
#define AXP22_STATUS_USBVA			(1 << 4)
#define AXP22_STATUS_USBEN			(1 << 5)
#define AXP22_STATUS_ACVA			(1 << 6)
#define AXP22_STATUS_ACEN			(1 << 7)

#define AXP22_STATUS_BATINACT		(1 << 3)
#define AXP22_STATUS_BATEN			(1 << 5)
#define AXP22_STATUS_INCHAR			(1 << 6)
#define AXP22_STATUS_ICTEMOV		(1 << 7)


#define AXP_DCDC1_MODE_BIT			(0)
#define AXP_DCDC2_MODE_BIT			(1)
#define AXP_DCDC3_MODE_BIT			(2)
#define AXP_DCDC4_MODE_BIT			(3)
#define AXP_DCDC5_MODE_BIT			(4)

#define AXP_DC5LDO_EN_BIT			(0)
#define AXP_DCDC1_EN_BIT			(1)
#define AXP_DCDC2_EN_BIT			(2)
#define AXP_DCDC3_EN_BIT			(3)
#define AXP_DCDC4_EN_BIT			(4)
#define AXP_DCDC5_EN_BIT			(5)
#define AXP_ALDO1_EN_BIT			(6)
#define AXP_ALDO2_EN_BIT			(7)

#define AXP_ELDO1_EN_BIT			(0)
#define AXP_ELDO2_EN_BIT			(1)
#define AXP_ELDO3_EN_BIT			(2)
#define AXP_DLDO1_EN_BIT			(3)
#define AXP_DLDO2_EN_BIT			(4)
#define AXP_DLDO3_EN_BIT			(5)
#define AXP_DLDO4_EN_BIT			(6)
#define AXP_DC1SW_EN_BIT			(7)

#define AXP_ALDO3_EN_BIT			(7)

#define AXP_DCDC1_BIT				(7)
#define AXP_DCDC2_BIT				(6)
#define AXP_DCDC3_BIT				(5)
#define AXP_DCDC4_BIT				(4)
#define AXP_DCDC5_BIT				(3)
#define AXP_ALDO1_BIT				(2)
#define AXP_ALDO2_BIT				(1)
#define AXP_ALDO3_BIT				(0)

#define AXP_DLDO1_BIT				(7)
#define AXP_DLDO2_BIT				(6)
#define AXP_DLDO3_BIT				(5)
#define AXP_DLDO4_BIT				(4)
#define AXP_ELDO1_BIT				(3)
#define AXP_ELDO2_BIT				(2)
#define AXP_ELDO3_BIT				(1)
#define AXP_DC5LDO_BIT				(0)

#define AXP22_DCDC1_MIN				1600000		/* VCC3P3_SYS			DCDC1	 : 	AXP22:1600~3400, 100/setp*/
#define AXP22_DCDC1_MAX				3400000
#define AXP22_DCDC1_STEP			100000
#define AXP22_DCDC2_MIN				600000		/* VCC1P1_ARM			DCDC2	£º 	AXP22:  600~1540,   20/step*/
#define AXP22_DCDC2_MAX				1540000
#define AXP22_DCDC2_STEP			20000
#define AXP22_DCDC3_MIN				600000		/* VCC1P0_CORE		DCDC3	£º 	AXP22:  600~1860,   20/step*/
#define AXP22_DCDC3_MAX				1860000
#define AXP22_DCDC3_STEP			20000
#define AXP22_DCDC4_MIN				600000		/* VCC1P5_SYS			DCDC4	£º 	AXP22:  600~1540,   20/step*/
#define AXP22_DCDC4_MAX				1540000
#define AXP22_DCDC4_STEP			20000
#define AXP22_DCDC5_MIN				1000000		/* VCC1P5_DDR			DCDC5	£º 	AXP22:1000~2550,   50/step*/
#define AXP22_DCDC5_MAX				2550000
#define AXP22_DCDC5_STEP			50000
#define AXP22_LDO1_MIN				3000000
#define AXP22_LDO1_MAX				3000000
#define AXP22_ALDO1_MIN				700000		/* VCC3P3_ALIVE		ALDO1	£º 	AXP22:  700~3300, 100/step*/	
#define AXP22_ALDO1_MAX				3300000
#define AXP22_ALDO1_STEP			100000
#define AXP22_ALDO2_MIN				700000		/* VCC1P8_ALIVE		ALDO2	£º 	AXP22:  700~3300, 100/step*/
#define AXP22_ALDO2_MAX				3300000
#define AXP22_ALDO2_STEP			100000
#define AXP22_ALDO3_MIN				700000		/* VCC1P0_ALIVE		ALDO3	£º 	AXP22:  700~3300, 100/step*/
#define AXP22_ALDO3_MAX				3300000
#define AXP22_ALDO3_STEP			100000
#define AXP22_DLDO1_MIN				700000		/* VCC_WIDE			DLDO1	£º 	AXP22:  700~3300, 100/step*/
#define AXP22_DLDO1_MAX				3300000
#define AXP22_DLDO1_STEP			100000
#define AXP22_DLDO2_MIN				700000		/* VCC1P8_CAM			DLDO2	£º 	AXP22 : 700~3300, 100/step*/
#define AXP22_DLDO2_MAX				3300000
#define AXP22_DLDO2_STEP			100000
#define AXP22_DLDO3_MIN				700000		/* NC					DLDO3	£º 	AXP22:  700~3300, 100/step*/
#define AXP22_DLDO3_MAX				3300000
#define AXP22_DLDO3_STEP			100000
#define AXP22_DLDO4_MIN				700000		/* NC					DLDO4	£º 	AXP22:  700~3300, 100/step*/
#define AXP22_DLDO4_MAX				3300000
#define AXP22_DLDO4_STEP			100000
#define AXP22_ELDO1_MIN				700000		/* VCC1P8_SYS			ELDO1	£º 	AXP22:  700~3300, 100/step*/
#define AXP22_ELDO1_MAX				3300000
#define AXP22_ELDO1_STEP			100000
#define AXP22_ELDO2_MIN				700000		/* VCC3P3_WIFI			ELDO2	£º 	AXP22:  700~3300, 100/step*/
#define AXP22_ELDO2_MAX				3300000
#define AXP22_ELDO2_STEP			100000
#define AXP22_ELDO3_MIN				700000		/* NC					ELDO3	£º 	AXP22:  700~3300, 100/step*/
#define AXP22_ELDO3_MAX				3300000
#define AXP22_ELDO3_STEP			100000
#define AXP22_DC5LDO_MIN			700000		/* VCC1P2_CVBS		DC5LDO	£º 	AXP22:  700~1400, 100/step*/
#define AXP22_DC5LDO_MAX			1400000
#define AXP22_DC5LDO_STEP			100000
#define AXP22_LDOIO0_MIN			700000
#define AXP22_LDOIO0_MAX			3300000
#define AXP22_LDOIO1_MIN			700000
#define AXP22_LDOIO1_MAX			3300000


/* ETC */
#define	AXP228_REG_BANKSEL			0xFF


#define ABS(x)						((x) >0 ? (x) : -(x) )


#define USBVOLLIM					4700		/* AXP22:4000~4700£¬100/step */
#define USBVOLLIMEN					1

#define USBCURLIM					500		/* AXP22:500/900 */
#define USBCURLIMEN					1


/* charge current */
#define CHARGE_CURRENT				1050*1000
#define POWEROFF_CHARGE_CURRENT		1500*1000

/* limit charge current */
#define LIMIT_CHARGE_CURRENT		1500*1000


/* set lowe power warning level */
#define BATLOWLV1					15		/* AXP22:5%~20% */

/* set lowe power shutdown level */
#define BATLOWLV2					0		/* AXP22:0%~15% */

/* set lowe power animation level */
#define BATLOW_ANIMATION_CAP		1		/* AXP22:0%~100% */

/* pek open time set */
#define PEKOPEN						1000		/* AXP22:128/1000/2000/3000 */

/* pek long time set*/
#define PEKLONG						1500		/* AXP22:1000/1500/2000/2500 */

/* pek offlevel poweroff en set*/
#define PEKOFFEN					1

/* Init offlevel restart or not */
#define PEKOFFRESTART				0

/* pek delay set */
#define PEKDELAY					32		/* AXP20:8/16/32/64 */

/*  pek offlevel time set */
#define PEKOFF						6000		/* AXP22:4000/6000/8000/10000 */

/* Init 16's Reset PMU en */
#define PMURESET					0

/* Init IRQ wakeup en*/
#define IRQWAKEUP					0

/* Init N_VBUSEN status*/
#define VBUSEN						1

/* Init InShort status*/
#define VBUSACINSHORT				0

/* Init CHGLED function*/
#define CHGLEDFUN					1

/* set CHGLED Indication Type*/
#define CHGLEDTYPE					0

/* Init PMU Over Temperature protection*/
#define OTPOFFEN					0

/* Init battery capacity correct function*/
#define BATCAPCORRENT				1

/* Init battery regulator enable or not when charge finish*/
#define BATREGUEN					0

#define BATDET						1



/* Charger */
enum {
	CHARGER_ENABLE,
	CHARGER_DISABLE
};

enum {
	POWER_OFF,
	POWER_ON
};

enum {
	STATE_CHARGING = 0,
	STATE_POWER_OFF,
	STATE_BOOT,
};

enum {
    BOOT_SRC_VUSB = 1,
    BOOT_SRC_KEY,
    BOOT_SRC_BOTH,
};

enum {
    BOOT_SRC_POS_VUSB = 0,
    BOOT_SRC_POS_KEY,
};

/* Unified sub device IDs for AXP */
/* LDO0 For RTCLDO ,LDO1-3 for ALDO,LDO*/
enum {
	AXP22_ID_LDO1,		//RTCLDO
	AXP22_ID_ALDO1,		//ALDO1
	AXP22_ID_ALDO2,		//ALDO2
	AXP22_ID_ALDO3,		//ALDO3
	AXP22_ID_DLDO1,		//DLDO1
	AXP22_ID_DLDO2,		//DLDO2
	AXP22_ID_DLDO3,		//DLDO3
	AXP22_ID_DLDO4,		//DLDO4
	AXP22_ID_ELDO1,		//ELDO1
	AXP22_ID_ELDO2,		//ELDO2
	AXP22_ID_ELDO3,		//ELDO3
	AXP22_ID_DC5LDO,	//DC5LDO
	AXP22_ID_DCDC1,
	AXP22_ID_DCDC2,
	AXP22_ID_DCDC3,
	AXP22_ID_DCDC4,
	AXP22_ID_DCDC5,
	AXP22_ID_LDOIO0,
	AXP22_ID_LDOIO1,
	AXP22_ID_SUPPLY,
	AXP22_ID_GPIO,	
};

/* platform device data */
struct axp228_power {
	int		i2c_bus;
	int		i2c_addr;
	int		support_policy;
	int		warm_reset;
};

struct axp228_ocv_val {
    u32 reg;
    u32 val;
};

#if defined(CONFIG_FASTBOOT) && defined(CONFIG_SW_UBC_DETECT)
extern int otg_bind_check(int miliSec_Timeout);
#endif

extern void axp228_usb_limit_set(struct pmic *p, u8 data);

extern u8 axp228_get_ldo_step(u8 ldo_num, int want_vol);
extern u8 axp228_get_dcdc_step(u8 ldo_num, int want_vol);

extern int power_bat_init(unsigned char bus);
extern int power_muic_init(unsigned int bus);
extern int power_fg_init(unsigned char bus);
extern int power_pmic_init(unsigned char bus);

extern int power_pmic_function_init(void);
extern int power_battery_check(int skip, void (*bd_display_run)(char *, int, int));

extern int bd_pmic_init(void);


static inline int axp22_vbat_to_mV(u16 reg)
{
	return ((int)((( reg >> 8) << 4 ) | (reg & 0x000F))) * 1100 / 1000;
}

static inline int axp22_ocvbat_to_mV(u16 reg)
{
	return ((int)((( reg >> 8) << 4 ) | (reg & 0x000F))) * 1100 / 1000;
}


static inline int axp22_vdc_to_mV(u16 reg)
{
	return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F))) * 1700 / 1000;
}


static inline int axp22_ibat_to_mA(u16 reg)
{
	return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F))) ;
}

static inline int axp22_icharge_to_mA(u16 reg)
{
	return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F)));
}

static inline int axp22_iac_to_mA(u16 reg)
{
	return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F))) * 625 / 1000;
}

static inline int axp22_iusb_to_mA(u16 reg)
{
	return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F))) * 375 / 1000;
}



#if defined(CONFIG_PMIC_REG_DUMP)
#define PMIC_DBGOUT(msg...)		do { printf("pmic: " msg); } while (0)
#else
#define PMIC_DBGOUT(msg...)		do {} while (0)
#endif

#endif /*  __LINUX_AXP228_PRIV_H */
