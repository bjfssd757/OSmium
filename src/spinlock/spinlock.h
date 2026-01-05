#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdatomic.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    volatile atomic_bool flag;
} spinlock_t;

#define SPINLOCK_INIT { false }

void fb_lock_acquire(void);
void fb_lock_release(void);

void spin_lock(spinlock_t *l);
void spin_unlock(spinlock_t *l);
int spin_trylock(spinlock_t *l);
int spin_is_locked(spinlock_t *l);

static inline uint64_t save_irq_disable() {
    uint64_t flags;
    asm volatile(
        "pushfq\n\t"
        "pop %0\n\t"
        "cli" : "=r"(flags) : : "memory"
    );
    return flags;
}

static inline void restore_irq(uint64_t flags) {
    asm volatile(
        "push %0\n\t"
        "popfq"
        : 
        : "g"(flags)
        : "memory", "cc"
    );
}

#endif /* SPINLOCK_H */