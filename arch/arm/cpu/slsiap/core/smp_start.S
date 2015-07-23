#include <asm-offsets.h>
#include <config.h>
#include <version.h>
#include <asm/system.h>
#include <linux/linkage.h>


	.globl secondary_startup
secondary_startup:
	mrc	p15, 0, r0, c0, c0, 5
	tst r0, #0x4400
	orrne r0, r0, #4
	and	r0, r0, #15
	adr	r4, 1f
	ldmia	r4, {r5, r6}
	sub	r4, r4, r5
	add	r6, r6, r4
pen:	ldr	r7, [r6]
	cmp	r7, r0
	bne	pen

	/*
	 * set secondary stack
	 */
	ldr	r1, =(CONFIG_SYS_SMP_SP_ADDR)	@ secondary stack
	mov	r2, #CONFIG_SYS_SMP_SP_SIZE
	mul	r3, r0, r2
	sub	sp, r1, r3

	/*
	 * set the cpu to SVC32 mode
	 */
	mrs	r0, cpsr
	bic	r0, r0, #0x1f
	orr	r0, r0, #0xd3
	msr	cpsr,r0

#ifndef CONFIG_SKIP_LOWLEVEL_INIT
	bl	cpu_init_cp15
	bl	cpu_init_crit
#endif

	/*
	 * set gd to r9 (refer to arch/arm/include/asm/global_data.h)
	 */
	ldr	r9, =(CONFIG_SYS_INIT_SP_ADDR)
	bic	r9, r9, #7					/* 8-byte alignment for ABI compliance */
	sub	r9, #GD_SIZE				/* allocate one GD above SP */
	bic	r9, r9, #7					/* 8-byte alignment for ABI compliance */

	bl smp_cpu_prepare
#ifdef CONFIG_MMU_ENABLE
	bl mmu_secondary_on
#endif
	b  smp_cpu_bootup

	.align
1:	.long	.
	.long	pen_status
