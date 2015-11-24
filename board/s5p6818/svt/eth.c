/*
 * (C) Copyright 2009
 * jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <netdev.h>

/*------------------------------------------------------------------------------
 * u-boot eth interface
 */
#include <net.h>
#include <phy.h>

#ifdef CONFIG_CMD_NET

#if defined(CONFIG_PHY_MICREL)
#include <common.h>
#include <miiphy.h>
#include <micrel.h>

int micrel_rgmii_rework(struct phy_device *phydev)
{
#if defined(CONFIG_PHY_MICREL_KSZ9031)
	u16 val;

	/*
	 * Bug: Apparently uDoo does not works with Gigabit switches...
	 * Limiting speed to 10/100Mbps, and setting master mode, seems to
	 * be the only way to have a successfull PHY auto negotiation.
	 * How to fix: Understand why Linux kernel do not have this issue.
	 */
	phy_write(phydev, MDIO_DEVAD_NONE, MII_CTRL1000, 0x1c00);

	/* EEE Advertisement - devaddr = 0x07, register = 0x3C */
	val = (1 << MII_KSZ9031_EXT_EEE_1000BASE_SHIFT)
		| (1 << MII_KSZ9031_EXT_EEE_100BASE_SHIFT);
	ksz9031_phy_extended_write(phydev, 0x07,
					MII_KSZ9031_EXT_EEE_ADVERTISEMENT,
					MII_KSZ9031_MOD_DATA_NO_POST_INC, val);
	/* control data pad skew - devaddr = 0x02, register = 0x04 */
	val = (0x0 << MII_KSZ9031_EXT_RGMII_RX_DV_SHIFT)
		| (0x7 << MII_KSZ9031_EXT_RGMII_TX_EN_SHIFT);
	ksz9031_phy_extended_write(phydev, 0x02,
					MII_KSZ9031_EXT_RGMII_CTRL_SIG_SKEW,
					MII_KSZ9031_MOD_DATA_NO_POST_INC, val);
	/* rx data pad skew - devaddr = 0x02, register = 0x05 */
	val = (0x0 << MII_KSZ9031_EXT_RGMII_RX3_SHIFT)
		| (0x0 << MII_KSZ9031_EXT_RGMII_RX2_SHIFT)
		| (0x0 << MII_KSZ9031_EXT_RGMII_RX1_SHIFT)
		| (0x0 << MII_KSZ9031_EXT_RGMII_RX0_SHIFT);
	ksz9031_phy_extended_write(phydev, 0x02,
					MII_KSZ9031_EXT_RGMII_RX_DATA_SKEW,
					MII_KSZ9031_MOD_DATA_NO_POST_INC, val);
	/* tx data pad skew - devaddr = 0x02, register = 0x06 */
	val = (0x7 << MII_KSZ9031_EXT_RGMII_TX3_SHIFT)
		| (0x7 << MII_KSZ9031_EXT_RGMII_TX2_SHIFT)
		| (0x7 << MII_KSZ9031_EXT_RGMII_TX1_SHIFT)
		| (0x7 << MII_KSZ9031_EXT_RGMII_TX0_SHIFT);
	ksz9031_phy_extended_write(phydev, 0x02,
					MII_KSZ9031_EXT_RGMII_TX_DATA_SKEW,
					MII_KSZ9031_MOD_DATA_NO_POST_INC, val);
	/* gtx and rx clock pad skew - devaddr = 0x02, register = 0x08 */
	val = (0x1F << MII_KSZ9031_EXT_RGMII_GTX_CLK_SHIFT)
		| (0x0E << MII_KSZ9031_EXT_RGMII_RX_CLK_SHIFT);
	ksz9031_phy_extended_write(phydev, 0x02,
					MII_KSZ9031_EXT_RGMII_CLOCK_SKEW,
					MII_KSZ9031_MOD_DATA_NO_POST_INC, val);

#endif

	return 0;
}
#endif

#if defined(CONFIG_DESIGNWARE_ETH)
int board_phy_config(struct phy_device *phydev)
{
	#if defined(CONFIG_PHY_MICREL)
	micrel_rgmii_rework(phydev);
	#endif
	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}
#endif

int board_eth_init(bd_t *bis)
{
#ifdef	CONFIG_DRIVER_DM9000
	dm9000_initialize(bis);
	return eth_init(bis);
#endif

#if defined(CONFIG_DESIGNWARE_ETH)
	u32 interface;
	int num = 0;

	interface = PHY_INTERFACE_MODE_RGMII;

	if (designware_initialize(CONFIG_DWCGMAC_BASE, interface) >= 0)
		num++;

	return num;
#endif  /* CONFIG_DESIGNWARE_ETH */

}
#endif	/* CONFIG_CMD_NET */

