/*
 * armboot - Startup Code for NXPxxxx/ARM Cortex CPU-core
 */

#include <asm-offsets.h>
#include <config.h>
#include <version.h>
#include <asm/system.h>
#include <linux/linkage.h>

/*
 *************************************************************************
 *
 * Exception vectors as described in ARM reference manuals
 *
 * replace arm/lib/vectors.S
 *
 *************************************************************************
 */
	.globl	_stext
_stext:
	b 	reset
	ldr	pc, _undefined_instruction
	ldr	pc, _software_interrupt
	ldr	pc, _prefetch_abort
	ldr	pc, _data_abort
	ldr	pc, _not_used
	ldr	pc, _irq
	ldr	pc, _fiq

_undefined_instruction:	.word undefined_instruction
_software_interrupt:	.word software_interrupt
_prefetch_abort:	.word prefetch_abort
_data_abort:		.word data_abort
_not_used:		.word not_used
_irq:			.word irq
_fiq:			.word fiq

	.balignl 16,0xdeadbeef

/*
 *************************************************************************
 *
 * Text and section base
 *
 *************************************************************************
 */

.globl TEXT_BASE
TEXT_BASE:
	.word	CONFIG_SYS_TEXT_BASE

/*
 * These are defined in the board-specific linker script.
 */
.globl _bss_start_ofs
_bss_start_ofs:
	.word __bss_start - _stext

.globl _bss_end_ofs
_bss_end_ofs:
	.word __bss_end - _stext

.globl _end_ofs
_end_ofs:
	.word _end - _stext

/*
 *************************************************************************
 *
 * Reset handling
 *
 *************************************************************************
 */

	.globl reset

reset:
	bl	save_boot_params
	/*
	 * set the cpu to SVC32 mode
	 */
	mrs	r0, cpsr
	bic	r0, r0, #0x1f
	orr	r0, r0, #0xd3
	msr	cpsr,r0

	/* the mask ROM code should have PLL and others stable */
#ifndef CONFIG_SKIP_LOWLEVEL_INIT
	bl	cpu_init_cp15
	bl	cpu_init_crit
#endif

#ifdef CONFIG_RELOC_TO_TEXT_BASE
relocate_to_text:
	/*
	 * relocate u-boot code on memory to text base
	 * for nexell arm core (add by jhkim)
	 */
	adr	r0, _stext				/* r0 <- current position of code   */
	ldr	r1, TEXT_BASE			/* test if we run from flash or RAM */
	cmp r0, r1              	/* don't reloc during debug         */
	beq clear_bss

	ldr	r2, _bss_start_ofs
	add	r2, r0, r2				/* r2 <- source end address         */

copy_loop_text:
	ldmia	r0!, {r3-r10}		/* copy from source address [r0]    */
	stmia	r1!, {r3-r10}		/* copy to   target address [r1]    */
	cmp	r0, r2					/* until source end addreee [r2]    */
	ble	copy_loop_text

	ldr	r1, TEXT_BASE			/* restart at text base */
	mov pc, r1

clear_bss:
	ldr	r0, _bss_start_ofs
	ldr	r1, _bss_end_ofs
	ldr	r4, TEXT_BASE			/* text addr */
	add	r0, r0, r4
	add	r1, r1, r4
	mov	r2, #0x00000000			/* clear */

clbss_l:str	r2, [r0]			/* clear loop... */
	add	r0, r0, #4
	cmp	r0, r1
	bne	clbss_l

#ifdef CONFIG_MMU_ENABLE
	bl	mmu_turn_on
#endif

	ldr	sp, =(CONFIG_SYS_INIT_SP_ADDR)
	bic	sp, sp, #7					/* 8-byte alignment for ABI compliance */
	sub	sp, #GD_SIZE				/* allocate one GD above SP */
	bic	sp, sp, #7					/* 8-byte alignment for ABI compliance */
	mov	r9, sp						/* GD is above SP */
	mov	r0, #0
	bl	board_init_f

	mov	sp, r9						/* SP is GD's base address */
	bic	sp, sp, #7					/* 8-byte alignment for ABI compliance */
	sub	sp, #GENERATED_BD_INFO_SIZE	/* allocate one BD above SP */
	bic	sp, sp, #7					/* 8-byte alignment for ABI compliance */

	mov	r0, r9						/* gd_t *gd */
	ldr r1, TEXT_BASE				/* ulong text */
	mov r2, sp						/* ulong sp */
	bl	gdt_reset

	/* call board_init_r(gd_t *id, ulong dest_addr) */
	mov	r0, r9							/* gd_t */
	ldr	r1,  =(CONFIG_SYS_MALLOC_END)	/* dest_addr for malloc heap end */
	/* call board_init_r */
	ldr	pc, =board_init_r				/* this is auto-relocated! */

#else	/* CONFIG_RELOC_TO_TEXT_BASE */

	bl	_main
#endif

/*------------------------------------------------------------------------------*/

ENTRY(c_runtime_cpu_setup)
/*
 * If I-cache is enabled invalidate it
 */
#ifndef CONFIG_SYS_ICACHE_OFF
	mcr	p15, 0, r0, c7, c5, 0	@ invalidate icache
	mcr     p15, 0, r0, c7, c10, 4	@ DSB
	mcr     p15, 0, r0, c7, c5, 4	@ ISB
#endif
/*
 * Move vector table
 */
	/* Set vector address in CP15 VBAR register */
	ldr     r0, =_stext
	mcr     p15, 0, r0, c12, c0, 0  @Set VBAR

	bx	lr

ENDPROC(c_runtime_cpu_setup)

/*************************************************************************
 *
 * void save_boot_params(u32 r0, u32 r1, u32 r2, u32 r3)
 *	__attribute__((weak));
 *
 * Stack pointer is not yet initialized at this moment
 * Don't save anything to stack even if compiled with -O0
 *
 *************************************************************************/
ENTRY(save_boot_params)
	bx	lr			@ back to my caller
ENDPROC(save_boot_params)
	.weak	save_boot_params

/*************************************************************************
 *
 * cpu_init_cp15
 *
 * Setup CP15 registers (cache, MMU, TLBs). The I-cache is turned on unless
 * CONFIG_SYS_ICACHE_OFF is defined.
 *
 *************************************************************************/
ENTRY(cpu_init_cp15)
	/*
	 * Invalidate L1 I/D
	 */
	mov	r0, #0			@ set up for MCR
	mcr	p15, 0, r0, c8, c7, 0	@ invalidate TLBs
	mcr	p15, 0, r0, c7, c5, 0	@ invalidate icache
	mcr	p15, 0, r0, c7, c5, 6	@ invalidate BP array

	dsb
	isb

	/*
	 * disable MMU stuff and caches
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002000	@ clear bits 13 (--V-)
	bic	r0, r0, #0x00000007	@ clear bits 2:0 (-CAM)
	orr	r0, r0, #0x00000002	@ set bit 1 (--A-) Align
	orr	r0, r0, #0x00000800	@ set bit 11 (Z---) BTB
#ifdef CONFIG_SYS_ICACHE_OFF
	bic	r0, r0, #0x00001000	@ clear bit 12 (I) I-cache
#else
	orr	r0, r0, #0x00001000	@ set bit 12 (I) I-cache
#endif
	mcr	p15, 0, r0, c1, c0, 0
	mov	pc, lr			@ back to my caller
ENDPROC(cpu_init_cp15)

#ifndef CONFIG_SKIP_LOWLEVEL_INIT
/*************************************************************************
 *
 * CPU_init_critical registers
 *
 * setup important registers
 * setup memory timing
 *
 *************************************************************************/
ENTRY(cpu_init_crit)
	/*
	 * Jump to board specific initialization...
	 * The Mask ROM will have already initialized
	 * basic memory. Go here to bump up clock rate and handle
	 * wake up conditions.
	 */
	b	lowlevel_init		@ go setup pll,mux,memory
ENDPROC(cpu_init_crit)
#endif

