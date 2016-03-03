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

#ifdef CONFIG_CMD_NET

#if defined(CONFIG_DESIGNWARE_ETH)
#include <phy.h>

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}
#endif

int board_eth_init(bd_t *bis)
{
#if defined(CONFIG_DESIGNWARE_ETH)
	u32 interface = PHY_INTERFACE_MODE_RGMII;
	int num = 0;

	if (designware_initialize(CONFIG_DWCGMAC_BASE, interface) >= 0)
		num++;

	return num;
#elif defined(CONFIG_DRIVER_DM9000)
	dm9000_initialize(bis);
	return eth_init(bis);
#else
	/* Unknown ethernet PHY/driver */
	return -1;
#endif
}

#endif	/* CONFIG_CMD_NET */

