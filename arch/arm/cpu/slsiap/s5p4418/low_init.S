
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

	/* secure SCU invalidate */
	and     r0, r0, #0x03             	@ Mask off unused bits of CPU ID
    mov     r0, r0, lsl #2             	@ Convert into bit offset (four bits per core)

    and     r1, r1, #0x0F              	@ Mask off unused bits of ways
    mov     r1, r1, lsl r0             	@ Shift ways into the correct CPU field

    mrc     p15, 4, r2, c15, c0, 0     	@ Read periph base address
    str     r1, [r2, #0x0C]            	@ Write to SCU Invalidate All in Secure State

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