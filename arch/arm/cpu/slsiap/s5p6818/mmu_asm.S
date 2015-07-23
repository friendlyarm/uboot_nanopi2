#include <asm-offsets.h>
#include <config.h>
#include <version.h>
#include <asm/system.h>
#include <linux/linkage.h>

#ifdef CONFIG_MMU_ENABLE
/*
 *************************************************************************
 * mmu_turn_on
 **************************************************************************
 */

	.globl mmu_turn_on
mmu_turn_on:
	ldr	sp, =(CONFIG_SYS_INIT_SP_ADDR)
	mov r13, lr
	bl mmu_on

	mov	 pc, r13						@ back to caller

/*
 *************************************************************************
 * enable_mmu
 **************************************************************************
 */
.equ	PAGE_DOMAIN,	0x55555555

	.globl enable_mmu
enable_mmu:
		mov		r10, r0							@ r0: PTs
		mov		r1, #0
		mcr		p15, 0, r1, c2, c0, 2			@ performs a page table walk on TLB miss and TTB0 has 16KB boundary

		ldr		r1, =PAGE_DOMAIN

		mcr		p15, 0, r1, c3, c0, 0			@ setup access to domain 0
		orr		r10, r10, #0x4a					@ Enable the Data Cache
		mcr		p15, 0, r10, c2, c0, 0

		mov  	r0, #0x0
		mcr		p15, 0, r0, c8, c7, 0			@ flush I+D TLBs

		mrc		p15, 0, r1, c1, c0, 0
		orr		r1, r1, #(0x1<< 0)				@ Enable MMU
		orr		r1, r1, #(0x1<< 1)				@ Enable Alignment Fault Checking
		orr		r1, r1, #(0x1<< 2)				@ Enable the Data Cache
		orr		r1, r1, #(0x1<<12)				@ Enable I Cache
		orr		r1, r1, #(0x1<<13)				@ Enable V USE 0xffff0000
		orr		r1, r1, #(0x1<<28)				@ Enable TRE Cache//bok
		mcr		p15, 0, r1, c1, c0, 0

		mov		pc, lr							@ return
#endif

	.globl disable_mmu
disable_mmu:
		mrc		p15, 0, r0, c1, c0, 0
		bic		r0, r0, #(1<<0) | (1<<2)		@ 0: MMU, 2: Dcache
		mcr		p15, 0, r0, c1, c0, 0
		mov		pc, lr