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

#ifndef __NXP_RTC_H_
#define __NXP_RTC_H_

#include <common.h>
#include <command.h>
#include <platform.h>
#include <rtc.h>
#include <asm/io.h>

extern void nxp_rtc_init(void);
extern u32  nxp_rtc_get(void);
extern void nxp_rtc_reset(void);

#endif  // __NXP_RTC_H_

