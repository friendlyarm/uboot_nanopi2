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

#ifndef __CPU_H_
#define __CPU_H_

#define	NR_CPUS		8
#define	MAX_GIC_NR	1

extern void gic_dev_init(unsigned int nr, int start, void __iomem *dist , void __iomem *cpu);
extern void gic_raise_softirq(int cpu);

extern void smp_cpu_init_f(void);
extern int  smp_cpu_register_fn(int cpu, void (*fn)(int cpu));
extern int  smp_cpu_raise(int cpu);
extern int  smp_cpu_check_stop(void);
extern void smp_cpu_set_end(void);

extern void timer_start(int ch);
extern void timer_stop(int ch);

#endif
