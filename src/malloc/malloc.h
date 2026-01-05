#ifndef MALLOC_H
#define MALLOC_H

#include "../mm/vmm.h"
#include "../mm/pmm.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_ORDER 11
#define MAX_FREE_SLAB 3

extern struct limine_memmap_response *memmap_res;
extern struct limine_hhdm_response *hhdm_res;

static struct mem_cache *k_caches[10];

void malloc_init();
void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
void get_kmalloc_stats(uint64_t *free_mem);

static inline uintptr_t get_buddy_addr(uintptr_t addr, uint8_t order)
{
    return addr ^ (PAGE_SIZE << order);
}

static inline uint8_t calculate_max_fit_order(uintptr_t addr, uint64_t len)
{
    uint8_t align_order = 63;
    if (addr != 0)
    {
        align_order = __builtin_ctzll(addr / PAGE_SIZE);
    }

    uint8_t len_order = 0;
    while ((1ULL << (len_order + 1)) * PAGE_SIZE <= len)
    {
        len_order++;
    }

    uint8_t order = align_order;
    if (len_order < order)  order = len_order;
    if (order >= MAX_ORDER) order = MAX_ORDER - 1;
    return order;
}

static inline uint8_t size_to_order(size_t size)
{
    uint8_t order = 0;
    size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    while ((1ULL << order) < pages) {
        order++;
    }
    return order;
}

static inline size_t order_to_size(uint8_t order)
{
    if (order > MAX_ORDER)
        return 0;
    return (1ULL << order) * PAGE_SIZE;
}

static inline size_t get_page_index(void *ptr)
{
    uintptr_t addr = (uintptr_t)ptr;
    if (addr >= hhdm_res->offset) {
        addr -= hhdm_res->offset;
    }
    return addr / PAGE_SIZE;
}

static inline int get_cache_index(size_t size) {
    if (size <= 16) return 0;
    return 31 - __builtin_clz((uint32_t)(size - 1)) - 3; 
}

#endif // MALLOC_H