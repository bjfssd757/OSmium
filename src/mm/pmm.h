#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include "../limine.h"
#include "../spinlock/spinlock.h"

#define PAGE_SIZE 4096

#define ALIGN_UP(addr, align) (((addr) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define PAGES_COUNT(virt_start, virt_end) \
    ((ALIGN_UP(virt_end, PAGE_SIZE) - ALIGN_DOWN(virt_start, PAGE_SIZE)) / PAGE_SIZE)

extern struct limine_hhdm_response *hhdm_res;
extern struct limine_memmap_response *memmap_res;
extern uint64_t max_pages;
extern spinlock_t pmm_lock;

uint64_t get_count_free_pages();
uint64_t get_total_memory();
void pmm_init();
void *alloc_page();
void *alloc_huge_page();
void *alloc_pages(size_t count);
int free_page(void *ptr);
int free_huge_page(void *ptr);

void bitmap_clear(uint64_t page);
int bitmap_test(uint64_t page);

#endif