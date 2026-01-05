#include "spinlock.h"

static atomic_flag g_fb_lock = ATOMIC_FLAG_INIT;

void fb_lock_acquire(void)
{
    while (atomic_flag_test_and_set_explicit(&g_fb_lock, memory_order_acquire))
    {
        asm volatile("pause" ::: "memory");
    }
}

void fb_lock_release(void)
{
    atomic_flag_clear_explicit(&g_fb_lock, memory_order_release);
}

void spin_lock(spinlock_t *l)
{
    while (atomic_exchange_explicit(&l->flag, true, memory_order_acquire))
    {
        while (atomic_load_explicit(&l->flag, memory_order_relaxed))
        {
            asm volatile("pause" ::: "memory");
        }
    }
}

void spin_unlock(spinlock_t *l)
{
    atomic_store_explicit(&l->flag, false, memory_order_release);
}

int spin_trylock(spinlock_t *l)
{
    bool expected = false;
    return atomic_compare_exchange_strong_explicit(
               &l->flag, &expected, true,
               memory_order_acquire,
               memory_order_relaxed
               )
               ? 1
               : 0;
}

int spin_is_locked(spinlock_t *l)
{
    return atomic_load_explicit(&l->flag, memory_order_relaxed) ? 1 : 0;
}