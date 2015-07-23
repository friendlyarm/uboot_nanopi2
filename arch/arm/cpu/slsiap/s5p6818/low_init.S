
#ifndef CONFIG_SKIP_LOWLEVEL_INIT
/*
 *************************************************************************
 *
 * CPU_init_critical
 *
 *************************************************************************
 */
	.globl lowlevel_init
lowlevel_init:

	/* get cpu id */
  	mrc     p15, 0, r0, c0, c0, 5     	@ Read CPU ID register
  	ands    r0, r0, #0x03             	@ Mask off, leaving the CPU ID field
  	mov     r1, #0xF                  	@ Move 0xF (represents all four ways) into r1

	/* join SMP */
  	mrc     p15, 0, r0, c1, c0, 1   	@ Read ACTLR
  	mov     r1, r0
  	orr     r0, r0, #0x040          	@ Set bit 6
  	cmp     r0, r1
  	mcrne   p15, 0, r0, c1, c0, 1   	@ Write ACTLR

	/* enable maintenance broadcast */
	mrc     p15, 0, r0, c1, c0, 1      	@ Read Aux Ctrl register
	mov     r1, r0
    orr     r0, r0, #0x01              	@ Set the FW bit (bit 0)
	cmp     r0, r1
    mcrne   p15, 0, r0, c1, c0, 1      	@ Write Aux Ctrl register

	mov	 pc, lr							@ back to caller

#endif /* CONFIG_SKIP_LOWLEVEL_INIT */