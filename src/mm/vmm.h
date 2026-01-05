#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>
#include "../limine.h"

#define PAGE_SIZE 4096

#define ALIGN_UP(addr, align) (((addr) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define PAGES_COUNT(virt_start, virt_end) \
    ((ALIGN_UP(virt_end, PAGE_SIZE) - ALIGN_DOWN(virt_start, PAGE_SIZE)) / PAGE_SIZE)

#define PTE_PRESENT     (1ULL << 0)
#define PTE_WRITABLE    (1ULL << 1)
#define PTE_USER        (1ULL << 2)
#define PTE_NX          (1ULL << 63)
#define PTE_HUGE_PAGE   (1ULL << 7)

#define LEVEL_PML4      4
#define LEVEL_PDPT      3
#define LEVEL_PD        2
#define LEVEL_PT        1

#define PCD     // Page-level Cache Disable

typedef uint64_t pt_entry_t;

typedef struct
{
    pt_entry_t entries[512];
} page_table_t;

extern uint64_t hhdm_offset;

void unmap(page_table_t *pml4, uint64_t virt_addr);
page_table_t *create_address_space();
void mmap(page_table_t *pml4, uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);
void* find_free_area(size_t pages);
uint64_t vmm_get_phys(page_table_t *pml4, uint64_t virt_addr);
void *map_user_memory(page_table_t *pml4, uint64_t virt_start, size_t pages, uint64_t flags);

void page_fault_handler(uint64_t vector, uint64_t error_code);

static inline page_table_t* read_cr3_virt(void) {
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return (page_table_t*)((cr3 & ~0xFFFULL) + hhdm_offset);
}

static inline void vmm_switch_pml4(page_table_t *pml4) {
    uint64_t phys = (uint64_t)pml4 - hhdm_offset;
    asm volatile("mov %0, %%cr3" :: "r"(phys) : "memory");
}

static inline void *virt(uint64_t phys)
{
    return (void*)(phys + hhdm_offset);
}

#endif