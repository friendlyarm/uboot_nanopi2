#ifndef __ASM_LINUX_SPINLOCK_H
#define __ASM_LINUX_SPINLOCK_H

#include <asm/io.h>

#define barrier()	 	__asm__ __volatile__("": : :"memory")
#define cpu_relax()     barrier()

static inline void raw_spin_lock(spinlock_t *lock)
{
    unsigned long tmp;

    __asm__ __volatile__(
	"1: ldrex   %0, [%1]\n"
	"   teq %0, #0\n"
		"wfene\n"
	"   strexeq %0, %2, [%1]\n"
	"   teqeq   %0, #0\n"
	"   bne 1b"
    	: "=&r" (tmp)
    	: "r" (&lock->lock), "r" (1)
    	: "cc");

   dmb();
}

static inline void raw_spin_unlock(spinlock_t *lock)
{
	dmb();

    __asm__ __volatile__(
	"   str %1, [%0]\n"
    	:
    	: "r" (&lock->lock), "r" (0)
    	: "cc");

	__asm__ __volatile__ (
		"dsb\n"
		"sev"
	);
}

#endif /* __ASM_LINUX_SPINLOCK_H */