/*
 *  Copyright (C) 2013 NEXELL SOC Lab.
 *  YoungBok Park <ybpark@nexell.co.kr>
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

#include <common.h>
#include <command.h>
#include <platform.h>
#include <rtc.h>
#include <asm/io.h>

#if (0)
#define DBGOUT(msg...)      { printf("RTC: " msg); }
#else
#define DBGOUT(msg...)      do {} while (0)
#endif


void nxp_rtc_init(void)
{
	DBGOUT("%s\n", __func__);
	NX_RTC_Initialize();
	NX_RTC_SetBaseAddress((void*)IO_ADDRESS(NX_RTC_GetPhysicalAddress()));
	NX_RTC_OpenModule();
	NX_RTC_ClearInterruptPendingAll();
	NX_RTC_SetInterruptEnableAll(CFALSE);
 }


u32 nxp_rtc_get(void)
{
	u32 rtc;

	rtc = NX_RTC_GetRTCCounter();

	DBGOUT("Read rtc : %d\n", rtc);

	return rtc;
}

void nxp_rtc_reset(void)
{
	NX_RTC_SetRTCCounterWriteEnable(CTRUE);
	NX_RTC_SetRTCCounter(0);
	while(NX_RTC_IsBusyRTCCounter()) { ; }

	NX_RTC_SetRTCCounterWriteEnable(CFALSE);

	while(NX_RTC_IsBusyRTCCounter()) { ; }
}

