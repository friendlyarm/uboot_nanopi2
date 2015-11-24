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
#include <malloc.h>
#include <linux/list.h>
#include <asm/io.h>
#include <asm/armv7.h>
#include <asm/errno.h>
#include <asm/arch/cpu.h>

#include <pm.h>
#include <platform.h>
#include <mach-types.h>

#if (0)
#define	pr_debug(m...)	printf(m)
#else
#define	pr_debug(m...)	do { } while (0)
#endif

DECLARE_GLOBAL_DATA_PTR;

extern gd_t *global_descriptor;
extern void secondary_startup(void);

struct cpu_fnc_t {
	int  cpu;
	void (*fn)(int cpu);
	struct list_head link;
};
#define	FN_POOL_SIZE	(5)

static struct list_head cpu_fn_head[NR_CPUS];
static struct cpu_fnc_t cpu_fn_pool[NR_CPUS][FN_POOL_SIZE];
static int cpu_fn_pos[NR_CPUS] =  { 0, };
static int smp_online = 0;
volatile int pen_status = -1;

static inline void core_low_power(void)
{
    unsigned int v;

	// flush_dcache_all();
    asm volatile(
    "   mcr p15, 0, %1, c7, c5, 0\n"
    "   mcr p15, 0, %1, c7, c10, 4\n"
    /*
     * Turn off coherency
     */
    "   mrc p15, 0, %0, c1, c0, 1\n"
    "   bic %0, %0, #0x20\n"
    "   mcr p15, 0, %0, c1, c0, 1\n"
    "   mrc p15, 0, %0, c1, c0, 0\n"
    "   bic %0, %0, %2\n"
    "   mcr p15, 0, %0, c1, c0, 0\n"
      : "=&r" (v)
      : "r" (0), "Ir" (CR_C)
      : "cc");
}

static inline void go_low_power(int cpu)
{
	/*
	 * there is no power-control hardware on this platform, so all
	 * we can do is put the core into WFI; this is safe as the calling
	 * code will have already disabled interrupts
	 */
	/*
	 * here's the WFI
	 */
	asm(".word	0xe320f003\n" : : : "memory", "cc");
}

static void smp_pen_status(int val)
{
	dmb();
	pen_status = val;

	#if 1
	flush_dcache_all();
	#else
	u32 start = (u32)&pen_status;
	/* wakeup all other cores */
	flush_dcache_range(start, start+sizeof(pen_status));
	#endif
}

static inline int smp_boot_core(int cpu, int up)
{
	u32 val = up ? IO_ADDRESS((unsigned int)secondary_startup) : (-1UL);

	__raw_writel(val, SCR_ARM_SECOND_BOOT);
#ifdef SCR_SMP_WAKE_CPU_ID
	__raw_writel(up ? cpu : (-1UL), SCR_SMP_WAKE_CPU_ID);
#endif
	/*
	 udelay(100);
	 */
	return 0;
}

#ifdef CONFIG_MMU_ENABLE
extern void disable_mmu(void);
#endif

static int smp_cpu_down(int cpu)
{
	void (*entry)(ulong, ulong) = NULL;
	ulong mach = machine_arch_type;
	ulong addr = 0x40008000;
	u32 stack = CONFIG_SYS_SMP_SP_ADDR;

	pr_debug("%s cpu.%d \n", __func__, cpu);

low_power:
	core_low_power();
	go_low_power(cpu);

	/* raise */
	entry = (void (*)(ulong, ulong))__raw_readl(SCR_ARM_SECOND_BOOT);
	if ((-1UL) == (u32)entry)
		goto low_power;

#ifdef SCR_SMP_WAKE_CPU_ID
	if (cpu != __raw_readl(SCR_SMP_WAKE_CPU_ID))
		goto low_power;
#endif

	stack -= (cpu * CONFIG_SYS_SMP_SP_SIZE);

	/* jump second bout */
	asm("mov sp, %0":"=r" (stack));

	disable_mmu();
	entry(addr, mach);

	return 0;
}

int smp_cpu_id(void)
{
	u32 cpu;
	__asm__ __volatile__("mrc p15, 0, %0, c0, c0, 5":"=r" (cpu));
	if ((cpu & 0x4400))
		cpu += 4;
	return (int)(cpu & 0xFF);
}

int smp_cpu_register_fn(int cpu, void (*fn)(int cpu))
{
	int pos = cpu_fn_pos[cpu];
	struct cpu_fnc_t *new = &cpu_fn_pool[cpu][pos];
	struct list_head *head = &cpu_fn_head[cpu];

	pr_debug("%s: cpu.%d (new 0x%08x) (list %d)\n", __func__, cpu, (u32)new, pos);
	if (cpu_fn_pos[cpu] >= FN_POOL_SIZE) {
		printf("%s full function lists (%d) is fulled (%d)\n",
			__func__, cpu_fn_pos[cpu], FN_POOL_SIZE);
		return -ENOMEM;
	}

	new->cpu = cpu;
	new->fn = fn;
	cpu_fn_pos[cpu]++;
	list_add_tail(&new->link, head);

	return 0;
}

int smp_cpu_raise(int cpu)
{
	int count = 100000 * 1;	/* 1sec */
	int ret = -1;

	pr_debug("%s cpu.%d \n", __func__, cpu);

	smp_boot_core(cpu, 1);

	smp_pen_status(cpu);

	gic_raise_softirq(cpu);

	while (count-- > 0) {
		dmb();
		if (pen_status == -1)
			break;
		udelay(10);
	}

	ret = pen_status != -1 ? -ENOSYS : 0;
	if (0 == ret)
		smp_online |= 1<<cpu;

	printf("cpu.%d raise %s (%d)\n", cpu, ret?"failed":"done", count);
	return ret;
}

void smp_cpu_init_f(void)
{
	struct list_head *head = cpu_fn_head;
	struct cpu_fnc_t *fn_p = NULL;
	int size = FN_POOL_SIZE;
	int i, n;

	pr_debug("SMP cpu NR %d func lists %d\n", NR_CPUS, size);

	for (i=0; NR_CPUS > i; i++) {
		head = &cpu_fn_head[i] ;
		fn_p = &cpu_fn_pool[i][0];

		cpu_fn_pos[i] = 0;
		INIT_LIST_HEAD(head);

		for (n = 0; size > n; n++, fn_p++)
			INIT_LIST_HEAD(&fn_p->link);
	}
}

/*
 * called smp_start.S on SMP core
 */
void smp_cpu_prepare(void)
{
	smp_boot_core(smp_cpu_id(), 0);

#ifdef CONFIG_SYS_GENERIC_GLOBAL_DATA
	/* refer to board_init_f */
	gd = global_descriptor;
#endif
}

void smp_cpu_bootup(void)
{
	struct list_head *head, *pos;
	int cpu = smp_cpu_id();

	smp_pen_status(-1);
	pr_debug("%s cpu.%d pen %d\n", __func__, cpu, pen_status);

	head = &cpu_fn_head[cpu];
	list_for_each(pos, head) {
		struct cpu_fnc_t *new = container_of(pos, struct cpu_fnc_t, link);
		pr_debug("%s: cpu.%d (new 0x%08x) (list %d)\n", __func__, cpu, (u32)new, cpu_fn_pos[cpu]);
		if (new->fn)
			new->fn(cpu);

		pos = pos->prev;
		list_del(&new->link);
	}
	cpu_fn_pos[cpu] = 0;
	smp_cpu_down(cpu);

	pr_debug("%s cpu.%d wakeup (pen=%d)\n", __func__, cpu, pen_status);
}


int smp_cpu_check_stop(void)	__attribute__((weak, alias("__smp_cpu_check_stop")));
void smp_cpu_set_end(void)		__attribute__((weak, alias("__smp_cpu_set_end")));

static int __smp_cpu_check_stop(void)
{
	return __raw_readl(SCR_SMP_SIG_READ) == SMP_SIGNATURE_STOP ? 1 : 0;
}

static void __smp_cpu_set_end(void)
{
	__raw_writel((-1UL), SCR_SMP_SIG_RESET);
	__raw_writel(SMP_SIGNATURE_EXIT, SCR_SMP_SIG_SET);
}