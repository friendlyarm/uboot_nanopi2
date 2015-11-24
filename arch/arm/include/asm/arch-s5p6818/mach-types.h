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

#define MACH_TYPE_S5P6818		       4330

#if defined(CONFIG_MACH_S5P6818)
	#ifdef machine_arch_type
		#undef  machine_arch_type
		#define machine_arch_type		MACH_TYPE_S5P6818
	#else
		#define machine_arch_type		MACH_TYPE_S5P6818
	#endif
	#define machine_is_s5p6818()		(machine_arch_type == MACH_TYPE_S5P6818)
#else
	#define machine_is_s5p6818()		(0)
#endif
