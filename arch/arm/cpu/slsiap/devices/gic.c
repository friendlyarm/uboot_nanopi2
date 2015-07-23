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

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch/cpu.h>

#define	writel_relaxed 	__raw_writel
#define	readl_relaxed 	__raw_readl

#define GIC_CPU_CTRL			0x00
#define GIC_CPU_PRIMASK			0x04
#define GIC_CPU_BINPOINT		0x08
#define GIC_CPU_INTACK			0x0c
#define GIC_CPU_EOI				0x10
#define GIC_CPU_RUNNINGPRI		0x14
#define GIC_CPU_HIGHPRI			0x18

#define GIC_DIST_CTRL			0x000
#define GIC_DIST_CTR			0x004
#define GIC_DIST_ENABLE_SET		0x100
#define GIC_DIST_ENABLE_CLEAR	0x180
#define GIC_DIST_PENDING_SET	0x200
#define GIC_DIST_PENDING_CLEAR	0x280
#define GIC_DIST_ACTIVE_BIT		0x300
#define GIC_DIST_PRI			0x400
#define GIC_DIST_TARGET			0x800
#define GIC_DIST_CONFIG			0xc00
#define GIC_DIST_SOFTINT		0xf00

struct gic_chip_data {
	void __iomem *dist_base;
	void __iomem *cpu_base;
	unsigned int gic_irqs;
};

#define	SKIP_GIC_INIT		(1)
static struct gic_chip_data gic_data[MAX_GIC_NR];

static inline void __iomem *gic_data_dist_base(struct gic_chip_data *data)
{
	return data->dist_base;
}

static inline void __iomem *gic_data_cpu_base(struct gic_chip_data *data)
{
	return data->cpu_base;
}

#ifndef SKIP_GIC_INIT
static void gic_cpu_init(struct gic_chip_data *gic)
{
	void __iomem *dist_base = gic_data_dist_base(gic);
	void __iomem *base = gic_data_cpu_base(gic);
	int i;

	/*
	 * Deal with the banked PPI and SGI interrupts - disable all
	 * PPI interrupts, ensure all SGI interrupts are enabled.
	 */
	writel_relaxed(0xffff0000, dist_base + GIC_DIST_ENABLE_CLEAR);
	writel_relaxed(0x0000ffff, dist_base + GIC_DIST_ENABLE_SET);

	/*
	 * Set priority on PPI and SGI interrupts
	 */
	for (i = 0; i < 32; i += 4)
		writel_relaxed(0xa0a0a0a0, dist_base + GIC_DIST_PRI + i * 4 / 4);

	writel_relaxed(0xf0, base + GIC_CPU_PRIMASK);
	writel_relaxed(1, base + GIC_CPU_CTRL);
}

static void gic_dist_init(struct gic_chip_data *gic)
{
	unsigned int i;
	u32 cpumask;
	unsigned int gic_irqs = gic->gic_irqs;
	void __iomem *base = gic_data_dist_base(gic);
	u32 cpu = 0; // cpu_logical_map(smp_processor_id());

	cpumask = 1 << cpu;
	cpumask |= cpumask << 8;
	cpumask |= cpumask << 16;

	writel_relaxed(0, base + GIC_DIST_CTRL);

	/*
	 * Set all global interrupts to be level triggered, active low.
	 */
	for (i = 32; i < gic_irqs; i += 16)
		writel_relaxed(0, base + GIC_DIST_CONFIG + i * 4 / 16);

	/*
	 * Set all global interrupts to this CPU only.
	 */
	for (i = 32; i < gic_irqs; i += 4)
		writel_relaxed(cpumask, base + GIC_DIST_TARGET + i * 4 / 4);

	/*
	 * Set priority on all global interrupts.
	 */
	for (i = 32; i < gic_irqs; i += 4)
		writel_relaxed(0xa0a0a0a0, base + GIC_DIST_PRI + i * 4 / 4);

	/*
	 * Disable all interrupts.  Leave the PPI and SGIs alone
	 * as these enables are banked registers.
	 */
	for (i = 32; i < gic_irqs; i += 32)
		writel_relaxed(0xffffffff, base + GIC_DIST_ENABLE_CLEAR + i * 4 / 32);

	writel_relaxed(1, base + GIC_DIST_CTRL);
}
#endif

#if defined(CONFIG_MACH_S5P6818) && !defined (CONFIG_ARCH_S5P6818_REV)
#include <asm/arch/pm.h>

#define	R_WFE() 	IO_ADDRESS(0xC00111BC)
#define	R_GIC() 	IO_ADDRESS(0xC0011078)
#define	CPU_RAISE(core)	{ __raw_writel(core, SCR_SMP_WAKE_CPU_ID); }
#define	GIC_RAISE(bits)	{ dmb(); __raw_writel(__raw_readl(R_GIC()) & ~(bits<<8), R_GIC()); }
#define	GIC_CLEAR(bits)	{ __raw_writel(__raw_readl(R_GIC()) | (bits<<8), R_GIC());  dmb(); }
#endif

void gic_raise_softirq(int cpu)
{
#if defined(CONFIG_MACH_S5P6818) && !defined (CONFIG_ARCH_S5P6818_REV)
	CPU_RAISE(cpu);
	GIC_RAISE(1<<cpu);
	GIC_CLEAR(1<<cpu);
#else
	unsigned long map = 0;
	int irq = 1;

	map |= 1 << cpu;

	/*
	 * Ensure that stores to Normal memory are visible to the
	 * other CPUs before issuing the IPI.
	 */
	dmb();

	/* this always happens on GIC0 */
	writel_relaxed(map << 16 | irq, gic_data_dist_base(&gic_data[0]) + GIC_DIST_SOFTINT);
#endif
}

void gic_dev_init(unsigned int nr, int start,
			   void __iomem *dist_base, void __iomem *cpu_base)
{
	struct gic_chip_data *gic;
	gic = &gic_data[nr];

	gic->dist_base = dist_base;
	gic->cpu_base = cpu_base;

#ifndef SKIP_GIC_INIT
	gic_dist_init(gic);
	gic_cpu_init(gic);
#endif
}