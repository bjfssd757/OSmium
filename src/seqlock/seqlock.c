#include "seqlock.h"

void seqlock_write_begin(seqlock_t *s)
{
    atomic_fetch_add_explicit(&s->seq, 1u, memory_order_relaxed);
}

void seqlock_write_end(seqlock_t *s)
{
    atomic_fetch_add_explicit(&s->seq, 1u, memory_order_release);
}

unsigned seqlock_read_begin(const seqlock_t *s)
{
    unsigned seq;

    for (;;)
    {
        seq = atomic_load_explicit(&((seqlock_t *)s)->seq, memory_order_acquire);

        if ((seq & 1u) == 0u)
            return seq;

        asm volatile("pause" ::: "memory");
    }
}

int seqlock_read_retry(const seqlock_t *s, unsigned start_seq)
{
    atomic_thread_fence(memory_order_acquire);

    unsigned end_seq =
        atomic_load_explicit(&((seqlock_t *)s)->seq, memory_order_relaxed);

    return end_seq != start_seq;
}