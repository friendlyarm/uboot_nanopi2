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

#include <common.h>
#include <command.h>
#include <asm/system.h>
#include <asm/cache.h>
#include <asm/armv7.h>
#include <asm/sections.h>
#include <platform.h>
#include <mach-types.h>
#include <asm/arch/mach-api.h>
#include <asm/arch/cpu.h>

extern ulong _bss_start_ofs;	/* BSS start relative to _start */
extern ulong _bss_end_ofs;		/* BSS end relative to _start */
extern ulong _end_ofs;			/* end of image relative to _start */

DECLARE_GLOBAL_DATA_PTR;

void __weak cpu_cache_initialization(void){}

int cleanup_before_linux(void)
{
	/*
	 * this function is called just before we call linux
	 * it prepares the processor for linux
	 *
	 * we turn off caches etc ...
	 */
#ifndef CONFIG_SPL_BUILD
	disable_interrupts();
#endif

	/*
	 * Turn off I-cache and invalidate it
	 */
	icache_disable();
	invalidate_icache_all();

	/*
	 * turn off D-cache
	 * dcache_disable() in turn flushes the d-cache and disables MMU
	 */
	dcache_disable();
	v7_outer_cache_disable();

	/*
	 * After D-cache is flushed and before it is disabled there may
	 * be some new valid entries brought into the cache. We are sure
	 * that these lines are not dirty and will not affect our execution.
	 * (because unwinding the call-stack and setting a bit in CP15 SCTRL
	 * is all we did during this. We have not pushed anything on to the
	 * stack. Neither have we affected any static data)
	 * So just invalidate the entire d-cache again to avoid coherency
	 * problems for kernel
	 */
	invalidate_dcache_all();

	/*
	 * Some CPU need more cache attention before starting the kernel.
	 */
	cpu_cache_initialization();

	nxp_before_linux();

	return 0;
}

void enable_caches(void)
{
}

/*------------------------------------------------------------------------------
 * u-boot cpu interface
 */
#ifndef	CONFIG_ARCH_CPU_INIT
#error unable to access prototype, must be define the macro "CONFIG_ARCH_CPU_INIT"
#endif

struct stack {
	u32 irq[3];
	u32 abt[3];
} ____cacheline_aligned;

static struct stack stacks;
extern ulong _IRQ_STACK_START_IN_;
#ifdef CONFIG_USE_IRQ
extern ulong _IRQ_STACK_START_;
extern ulong _FIQ_STACK_START_;
#endif

#if defined(CONFIG_ARCH_CPU_INIT)
int arch_cpu_init (void)
{
	struct stack *stack = &stacks;

	/*
	 * global data (_start is invalid so reconfig mon_len value)
	 * reset stack point for Exception
	 */
	gd->mon_len = (ulong)&__bss_end - CONFIG_SYS_TEXT_BASE;
	gd->irq_sp = (unsigned long)stack->abt;	/* Abort stack */

#if defined(CONFIG_SILENT_CONSOLE)
	gd->flags |= GD_FLG_SILENT;
#endif

	_IRQ_STACK_START_IN_ = gd->irq_sp + 8;
#ifdef CONFIG_USE_IRQ
	_IRQ_STACK_START_ = gd->irq_sp - 4;
	_FIQ_STACK_START_ = _IRQ_STACK_START_ - CONFIG_STACKSIZE_IRQ;
#endif
	flush_dcache_all();

	/*
	 * cpu initialize
	 */
	nxp_cpu_init();
	nxp_clk_init();
	nxp_periph_init();

#if defined (CONFIG_SMP)
	gic_dev_init(0, 16, (void __iomem *)0xF0001000, (void __iomem *)0xF0000100);
	smp_cpu_init_f();
	flush_dcache_all();
#endif

	return 0;
}
#endif

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	nxp_print_cpuinfo();
	return 0;
}
#endif

/* u-boot dram initialize  */
int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
	return 0;
}

/* u-boot dram board specific */
void dram_init_banksize(void)
{
	/* set global data memory */
	gd->bd->bi_arch_number 	 = machine_arch_type;
	gd->bd->bi_boot_params 	 = CONFIG_SYS_SDRAM_BASE + 0x00000100;

	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size  = CONFIG_SYS_SDRAM_SIZE;
}

#ifdef CONFIG_RELOC_TO_TEXT_BASE
extern uchar default_environment[];
gd_t *global_descriptor = NULL;

void gdt_reset(gd_t *gd, ulong text, ulong sp)
{
	ulong text_start, text_end, heap_end;
	ulong bd;

	/* for smp cores */
	global_descriptor = gd;

	/* reconfig stack info */
	gd->relocaddr = text;
	gd->start_addr_sp = sp;
	gd->reloc_off = 0;

	/* copy bd info  */
	bd = (unsigned int)gd - sizeof(bd_t);
	memcpy((void *)bd, (void *)gd->bd, sizeof(bd_t));

	/* reset gd->bd */
	gd->bd = (bd_t *)bd;

	/* prevent dataabort, when access enva_addr + data (0x04) */
	gd->env_addr = (ulong)default_environment;

	/* get cpu info */
	text_start = (unsigned int)(gd->relocaddr);
	text_end = (unsigned int)(gd->relocaddr + _bss_end_ofs);
	heap_end = CONFIG_SYS_MALLOC_END;

#if defined(CONFIG_SYS_GENERIC_BOARD)
	/* refer initr_malloc (common/board_r.c) */
	gd->relocaddr = heap_end;
#endif
	flush_dcache_all();

#if defined(CONFIG_DISPLAY_CPUINFO)
	ulong pc;

	asm("mov %0, pc":"=r" (pc));
	asm("mov %0, sp":"=r" (sp));

	printf("Heap = 0x%08lx~0x%08lx\n", heap_end-TOTAL_MALLOC_LEN, heap_end);
	printf("Code = 0x%08lx~0x%08lx\n", text_start, text_end);
	printf("GLD  = 0x%08lx\n", (ulong)gd);
	printf("GLBD = 0x%08lx\n", (ulong)gd->bd);
	printf("SP   = 0x%08lx,0x%08lx(CURR)\n", gd->start_addr_sp, sp);
	printf("PC   = 0x%08lx\n", pc);

	printf("TAGS = 0x%08lx \n", gd->bd->bi_boot_params);
	#ifdef CONFIG_MMU_ENABLE
	ulong page_tlb =  (text_end & 0xffff0000) + 0x10000;
	printf("PAGE = 0x%08lx~0x%08lx\n", page_tlb, page_tlb + 0xc000 );
	#endif
	#ifdef CONFIG_SMP
	printf("SMPSP= 0x%08x (-0x%08x)\n", CONFIG_SYS_SMP_SP_ADDR, CONFIG_SYS_SMP_SP_SIZE*(NR_CPUS-1));
	#endif
	printf("MACH = [%ld]   \n", gd->bd->bi_arch_number);
	printf("VER  = %u      \n", nxp_cpu_version());
	printf("BOARD= [%s]    \n", CONFIG_SYS_BOARD);
#endif
}
#endif

/*------------------------------------------------------------------------------
 * u-boot cpu arch initialize
 */

/* u-boot miscellaneous arch dependent initialisations */
#if defined(CONFIG_ARCH_MISC_INIT)
int arch_misc_init(void)
{
	return 0;
}
#endif	/* CONFIG_ARCH_MISC_INIT */

/*------------------------------------------------------------------------------
 * u-boot boot os
 */
void arch_preboot_os(void)
{
#ifdef CONFOG_BOARD_PREBOOT_OS
	nxp_preboot_os();
#endif
}
