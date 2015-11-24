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

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <s5p4418.h>

#ifndef __ASSEMBLY__
#include <cfg_type.h>
#include <cfg_gpio.h>
#include <cfg_main.h>
#include <cfg_mem.h>

#include <hardware.h>
#include <linux-types.h>

#ifndef BOOL
typedef unsigned int BOOL;
#define	TRUE		1
#define	FALSE		0
#endif

#ifndef bool
typedef unsigned int bool;
#define	true		1
#define	false		0
#define BOOL_WAS_DEFINED
#endif

#ifdef IO_ADDRESS
#undef IO_ADDRESS
#endif
#define IO_ADDRESS(addr)	(addr)

#define NS_IN_HZ (1000000000UL)
#define TO_PERIOD_NS(freq)				(NS_IN_HZ/(freq))
#define TO_DUTY_NS(duty, freq)       	(duty ? TO_PERIOD_NS(freq)/(100/duty) : 0)

#endif	/* __ASSEMBLY__ */
#endif	/* __PLATFORM_H__ */



