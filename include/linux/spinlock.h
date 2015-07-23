#ifndef __LINUX_SPINLOCK_H
#define __LINUX_SPINLOCK_H

typedef struct spinlock {
	volatile unsigned int lock;
} spinlock_t;

#define __SPIN_LOCK_INITIALIZER(lockname) { .lock = 0, }

#define __SPIN_LOCK_UNLOCKED(lockname) \
    (spinlock_t ) __SPIN_LOCK_INITIALIZER(lockname)

#define DEFINE_SPINLOCK(x)  spinlock_t x = __SPIN_LOCK_UNLOCKED(x)

#include <asm/arch/spinlock.h>

#define	 spin_is_locked(x)		((x)->lock !=0)
#define arch_spin_unlock_wait(x) \
        do { while (spin_is_locked(lock)) cpu_relax(); } while (0)

static inline void spin_lock_init(spinlock_t *lock)
{
	lock->lock = 0;
}

static inline void spin_lock(spinlock_t *lock)
{
    raw_spin_lock(lock);
}

static inline void spin_unlock(spinlock_t *lock)
{
    raw_spin_unlock(lock);
}

static inline void spin_lock_irqsave(spinlock_t *lock, ulong flags)
{
    raw_spin_lock(lock);
}

static inline void spin_unlock_irqrestore(spinlock_t *lock, ulong flags)
{
    raw_spin_unlock(lock);
}

#endif /* __LINUX_SPINLOCK_H */