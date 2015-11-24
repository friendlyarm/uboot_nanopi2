/*
 * (C) Copyright 2009 Nexell Co.,
 * jung hyun kim<jhkim@nexell.co.kr>
 *
 * Configuation settings for the Nexell board.
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
#include <asm/byteorder.h>
#include <common.h>
#include <command.h>
#include <asm/errno.h>
#include <fastboot.h>

/*
#define	debug	printf
*/

#define	TCLK_TICK_KHZ			(1000)


typedef struct cmd_fastboot_interface f_cmd_inf;
static int fboot_rx_handler(const unsigned char *buffer, unsigned int length);
static void fboot_reset_handler(void);

static f_cmd_inf f_interface = {
	.rx_handler = fboot_rx_handler,
	.reset_handler = fboot_reset_handler,
	.product_name = NULL,
	.serial_no = NULL,
	.transfer_buffer = (unsigned char *)CFG_FASTBOOT_TRANSFER_BUFFER,
	.transfer_buffer_size = CFG_FASTBOOT_TRANSFER_BUFFER_SIZE,
};

static int fboot_rx_handler(const unsigned char *buffer, unsigned int length)
{
	return 0;
}

static void fboot_reset_handler(void)
{
	return;
}


extern int dwc_otg_get_ep0_state(void);
extern void dwc_otg_clear_ep0_state(void);

int otg_bind_check(int miliSec_Timeout)
{
	f_cmd_inf *inf = &f_interface;
	unsigned int tclk = TCLK_TICK_KHZ;
	int status = FASTBOOT_ERROR;

	dwc_otg_clear_ep0_state();

	if (fastboot_init(inf) == 0) {
		int miliSec_CurrTime = (get_ticks()/tclk);
		int miliSec_EndTime = miliSec_CurrTime + miliSec_Timeout;

		while (1) {
			status = fastboot_poll();

			miliSec_CurrTime = (get_ticks()/tclk);

			if ( (status == FASTBOOT_ERROR) || (status == FASTBOOT_DISCONNECT) )
				break;

			if (miliSec_CurrTime >= miliSec_EndTime) {
				status = -ETIMEDOUT;
				break;
			}
		} /* while (1) */
	}

	fastboot_shutdown();

	return dwc_otg_get_ep0_state();
}


