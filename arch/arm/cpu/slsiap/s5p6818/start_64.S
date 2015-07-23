/*
 * (C) Copyright 2013
 * David Feng <fenghua@phytium.com.cn>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm-offsets.h>
#include <config.h>
#include <version.h>
#include <linux/linkage.h>
#include <asm/macro.h>
#include <asm/armv8/mmu.h>
#include <asm/system.h>

/*************************************************************************
 *
 * Startup Code (reset vector)
 *
 *************************************************************************/

.globl	_start
_start:
	b	reset

	.align 3

.globl	TEXT_BASE
TEXT_BASE:
	.quad	CONFIG_SYS_TEXT_BASE

/*
 * These are defined in the linker script.
 */
.globl	_end_ofs
_end_ofs:
	.quad	_end - _start

.globl	_bss_start_ofs
_bss_start_ofs:
	.quad	__bss_start - _start

.globl	_bss_end_ofs
_bss_end_ofs:
	.quad	__bss_end - _start

reset:
	/*
	 * Could be EL3/EL2/EL1, Initial State:
	 * Little Endian, MMU Disabled, i/dCache Disabled
	 */
	adr	x0, vectors
	switch_el x1, 3f, 2f, 1f
3:	msr	vbar_el3, x0
	mrs	x0, scr_el3
	orr	x0, x0, #0xf			/* SCR_EL3.NS|IRQ|FIQ|EA */
	msr	scr_el3, x0
	msr	cptr_el3, xzr			/* Enable FP/SIMD */
	ldr	x0, =COUNTER_FREQUENCY
	msr	cntfrq_el0, x0			/* Initialize CNTFRQ */
	b	0f
2:	msr	vbar_el2, x0
	mov	x0, #0x33ff
	msr	cptr_el2, x0			/* Enable FP/SIMD */
	b	0f
1:	msr	vbar_el1, x0
	mov	x0, #3 << 20
	msr	cpacr_el1, x0			/* Enable FP/SIMD */
0:

	/*
	 * Cache/BPB/TLB Invalidate
	 * i-cache is invalidated before enabled in icache_enable()
	 * tlb is invalidated before mmu is enabled in dcache_enable()
	 * d-cache is invalidated before enabled in dcache_enable()
	 */
	switch_el x1, 3f, 2f, 1f
3:	mrs	x0, sctlr_el3
	bic	x0, x0, #CR_I
	msr sctlr_el3, x0
	b	0f
2:	mrs	x0, sctlr_el2
	bic	x0, x0, #CR_I
	msr sctlr_el2, x0
	b	0f
1:	mrs	x0, sctlr_el1
	bic	x0, x0, #CR_I
	msr sctlr_el1, x0
0:
	bl __asm_invalidate_icache_all

	switch_el x1, 3f, 2f, 1f
3:	mrs	x0, sctlr_el3
	orr	x0, x0, #CR_I
	msr sctlr_el3, x0
	b	0f
2:	mrs	x0, sctlr_el2
	orr	x0, x0, #CR_I
	msr sctlr_el2, x0
	b	0f
1:	mrs	x0, sctlr_el1
	orr	x0, x0, #CR_I
	msr sctlr_el1, x0
0:

#ifdef CONFIG_RELOC_TO_TEXT_BASE
	/*
 	 * relocation downloaded code to text base
 	 */
relc_to_text:
	adr	x0, _start				/* r0 <- current position of code   */
	ldr	x1, TEXT_BASE
	cmp x0, x1              	/* don't reloc during debug         */
	b.eq 	clear_bss

	ldr	x2, _bss_start_ofs
	add	x2, x0, x2				/* r2 <- source end address         */

copy_text:
	ldp	x10, x11,[x0], #16		/* copy from source address [x0]    */
	stp	x10, x11,[x1], #16		/* copy to   target address [x1]    */
	cmp	x0, x2					/* until source end addreee [r2]    */
	b.lo   copy_text

	ldr	lr, TEXT_BASE			/* restart at text base */
	ret
#endif

	/* Processor specific initialization */
	bl	lowlevel_init

	branch_if_master x0, x1, master_cpu

	/*
	 * Slave CPUs
	 */
slave_cpu:
	wfe
	ldr	x1, =CPU_RELEASE_ADDR
	ldr	x0, [x1]
	cbz	x0, slave_cpu
	br	x0			/* branch to the given address */

	/*
	 * Master CPU
	 */
master_cpu:

#ifdef CONFIG_RELOC_TO_TEXT_BASE
/*
 * Clear BSS section
 */
clear_bss:
	ldr	x0, =__bss_start		/* this is auto-relocated! */
	ldr	x1, =__bss_end			/* this is auto-relocated! */
	mov	x2, #0
clear_loop:
	str	x2, [x0]
	add	x0, x0, #8
	cmp	x0, x1
	b.lo	clear_loop

	/*
 	 * Set up initial C runtime environment and call board_init_f(0).
 	 * replace _main entry
 	 */
	ldr	x0, =(CONFIG_SYS_INIT_SP_ADDR)
	sub	x0, x0, #GD_SIZE	/* allocate one GD above SP */
	bic	sp, x0, #0xf	/* 16-byte alignment for ABI compliance */
	mov	x18, sp			/* GD is above SP */
	mov	x0, #0
	bl	board_init_f

	mov	x3, x18						/* SP is GD's base address */
	bic	x3, x3, #0xf				/* 16-byte alignment for ABI compliance */
	sub	x3, x3, #GENERATED_BD_INFO_SIZE	/* allocate one BD above SP */
	bic	sp, x3, #0xf				/* 16-byte alignment for ABI compliance */

	mov	x0, x18						/* gd_t *gd */
	ldr x1, TEXT_BASE				/* ulong text */
	mov x2, sp						/* ulong sp */
	bl	gdt_reset

	/*
 	 * Set up final (full) environment
 	 */
	bl	c_runtime_cpu_setup		/* still call old routine */

	mov	x0, x18						/* gd_t */
	ldr	x1, =CONFIG_SYS_MALLOC_END	/* dest_addr */
	b	board_init_r				/* PC relative jump */

	b	relocate_code
#else	/* CONFIG_RELOC_TO_TEXT_BASE */

	bl	_main
#endif

/*-----------------------------------------------------------------------*/

WEAK(lowlevel_init)
	mov	x29, lr			/* Save LR */

#if defined(CONFIG_GICV2) || defined(CONFIG_GICV3)
	branch_if_slave x0, 1f
	ldr	x0, =GICD_BASE
	bl	gic_init_secure
1:
#if defined(CONFIG_GICV3)
	ldr	x0, =GICR_BASE
	bl	gic_init_secure_percpu
#elif defined(CONFIG_GICV2)
	ldr	x0, =GICD_BASE
	ldr	x1, =GICC_BASE
	bl	gic_init_secure_percpu
#endif
#endif

	branch_if_master x0, x1, 2f

	/*
	 * Slave should wait for master clearing spin table.
	 * This sync prevent salves observing incorrect
	 * value of spin table and jumping to wrong place.
	 */
#if defined(CONFIG_GICV2) || defined(CONFIG_GICV3)
#ifdef CONFIG_GICV2
	ldr	x0, =GICC_BASE
#endif
	bl	gic_wait_for_interrupt
#endif

	/*
	 * All slaves will enter EL2 and optionally EL1.
	 */
	bl	armv8_switch_to_el2
#ifdef CONFIG_ARMV8_SWITCH_TO_EL1
	bl	armv8_switch_to_el1
#endif

2:
	mov	lr, x29			/* Restore LR */
	ret
ENDPROC(lowlevel_init)

WEAK(smp_kick_all_cpus)
	/* Kick secondary cpus up by SGI 0 interrupt */
	mov	x29, lr			/* Save LR */
#if defined(CONFIG_GICV2) || defined(CONFIG_GICV3)
	ldr	x0, =GICD_BASE
	bl	gic_kick_secondary_cpus
#endif
	mov	lr, x29			/* Restore LR */
	ret
ENDPROC(smp_kick_all_cpus)

/*-----------------------------------------------------------------------*/

ENTRY(c_runtime_cpu_setup)
	/* Relocate vBAR */
	adr	x0, vectors
	switch_el x1, 3f, 2f, 1f
3:	msr	vbar_el3, x0
	b	0f
2:	msr	vbar_el2, x0
	b	0f
1:	msr	vbar_el1, x0
0:

	ret
ENDPROC(c_runtime_cpu_setup)
