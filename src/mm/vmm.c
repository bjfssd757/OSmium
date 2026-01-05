#include "vmm.h"
#include "pmm.h"
#include "../libc/string.h"
#include "../graphics/formatting.h"

static uint64_t get_addr(pt_entry_t entry)
{
    return entry & 0x000FFFFFFFFFF000;
}

static page_table_t* get_next_table(page_table_t *current_table, uint64_t index, bool user) {
    if (!(current_table->entries[index] & PTE_PRESENT)) {
        uint64_t new_table_phys = (uintptr_t)alloc_page();
        if (!new_table_phys) return NULL;

        memset(virt(new_table_phys), 0, PAGE_SIZE);

        uint64_t flags = PTE_PRESENT | PTE_WRITABLE;
        if (user) flags |= PTE_USER;

        current_table->entries[index] = new_table_phys | flags;
    }
    return (page_table_t*)virt(get_addr(current_table->entries[index]));
}

uint64_t vmm_get_phys(page_table_t *pml4, uint64_t virt_addr) {
    uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt_addr >> 12) & 0x1FF;

    if (!(pml4->entries[pml4_idx] & PTE_PRESENT)) return 0;
    page_table_t *pdpt = virt(get_addr(pml4->entries[pml4_idx]));

    if (!(pdpt->entries[pdpt_idx] & PTE_PRESENT)) return 0;
    page_table_t *pd = virt(get_addr(pdpt->entries[pdpt_idx]));

    if (!(pd->entries[pd_idx] & PTE_PRESENT)) return 0;
    page_table_t *pt = virt(get_addr(pd->entries[pd_idx]));

    if (!(pt->entries[pt_idx] & PTE_PRESENT)) return 0;

    return get_addr(pt->entries[pt_idx]) | (virt_addr & 0xFFF);
}

static uint64_t v_heap_cursor = 0xFFFFa00000000000;

void* find_free_area(size_t pages)
{
    void* addr = (void*)v_heap_cursor;
    v_heap_cursor += (pages * PAGE_SIZE);
    return addr;
}

void mmap(page_table_t *pml4, uint64_t virt_addr, uint64_t phys_addr, uint64_t flags)
{
    bool is_user = (flags & PTE_USER);

    uint64_t pml4_i = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_i = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_i   = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_i   = (virt_addr >> 12) & 0x1FF;

    page_table_t *pdpt = get_next_table(pml4, pml4_i, is_user);
    page_table_t *pd   = get_next_table(pdpt, pdpt_i, is_user);
    page_table_t *pt   = get_next_table(pd,   pd_i,   is_user);

    pt->entries[pt_i] = phys_addr | flags | PTE_PRESENT;

    asm volatile("invlpg (%0)" :: "r"(virt_addr) : "memory");
}

page_table_t *create_address_space()
{
    page_table_t *new_pml4 = virt((uintptr_t)alloc_page());
    memset(new_pml4, 0, PAGE_SIZE);

    uint64_t current_pml4_phys;
    asm volatile("mov %%cr3, %0" : "=r"(current_pml4_phys));
    page_table_t *old_pml4 = virt(current_pml4_phys);

    for (int i = 256; i < 512; i++)
    {
        new_pml4->entries[i] = old_pml4->entries[i];
    }

    return new_pml4;
}

static void cleanup_level(page_table_t *table, int level)
{
    for (int i = 0; i < 512; i++)
    {
        uint64_t entry = table->entries[i];

        if (!(entry & PTE_PRESENT)) continue;

        if ((level == 2 || level == 3) && (entry & PTE_HUGE_PAGE))
        {
            free_huge_page((void*)(entry & ~0xFFFULL));
            continue;
        }

        if (level > 1)
        {
            cleanup_level((page_table_t*)virt(entry & ~0xFFFULL), level - 1);
        }
        else
        {
            free_page((void*)(entry & ~0xFFFULL));
        }
    }
    free_page(table);
}

void destroy_address_space(page_table_t *pml4_virt)
{
    for (int i = 0; i < 256; i++)
    {
        uint64_t entry = pml4_virt->entries[i];

        if (entry & PTE_PRESENT)
            cleanup_level((page_table_t*)virt(entry & ~0xFFFULL), LEVEL_PDPT);
    }

    free_page(pml4_virt);

    asm volatile("invlpg (%0)" :: "r"(pml4_virt) : "memory");
}

void unmap(page_table_t *pml4, uint64_t virt_addr) {
    uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt_addr >> 12) & 0x1FF;

    if (!(pml4->entries[pml4_idx] & PTE_PRESENT)) return;
    page_table_t *pdpt = virt(get_addr(pml4->entries[pml4_idx]));

    if (!(pdpt->entries[pdpt_idx] & PTE_PRESENT)) return;
    page_table_t *pd = virt(get_addr(pdpt->entries[pdpt_idx]));

    if (!(pd->entries[pd_idx] & PTE_PRESENT)) return;
    page_table_t *pt = virt(get_addr(pd->entries[pd_idx]));

    if (!(pt->entries[pt_idx] & PTE_PRESENT)) return;

    pt->entries[pt_idx] = 0;

    asm volatile("invlpg (%0)" :: "r"(virt_addr) : "memory");
}

void *map_user_memory(page_table_t *pml4, uint64_t virt_start, size_t pages, uint64_t flags)
{
    for (size_t i = 0; i < pages; i++)
    {
        void *phys_page = alloc_page() - hhdm_offset;
        if (!phys_page) return NULL;

        mmap(
            pml4,
            virt_start + (i * PAGE_SIZE), 
            (uintptr_t)phys_page,
            flags | PTE_USER
        );
    }
    return (void*)virt_start;
}

void page_fault_handler(uint64_t vector, uint64_t error_code)
{
    uint64_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

    if (error_code & 0x4)
    {
        // TODO: Check if the address is in the Virtual Memory Area.
        // If yes, allocate a memory page and update the page table
        // If no, kill the process
        return;
    }
    kprint(KPRINT_ERROR, "PAGE FAULT at %x, error: %d\n", faulting_address, error_code);
    for (;;) asm volatile("hlt");
}