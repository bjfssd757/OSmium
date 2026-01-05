#include "pmm.h"
#include "../libc/string.h"
#include "stdint.h"
#include "vmm.h"
#include <stddef.h>

uint8_t *bitmap;
uint64_t bitmap_size;
uint64_t last_checked_page = 0;
uint64_t allocated_pages = 0;
spinlock_t pmm_lock = SPINLOCK_INIT;
uint64_t max_pages;

static void bitmap_set(uint64_t page)
{
    bitmap[page / 8] |= (1 << (page % 8));
}

void bitmap_clear(uint64_t page)
{
    bitmap[page / 8] &= ~(1 << (page % 8));
}

int bitmap_test(uint64_t page)
{
    return (bitmap[page / 8] & (1 << (page % 8))) != 0;
}

uint64_t get_total_memory()
{
    uint64_t total = 0;
    for (uint64_t i = 0; i < memmap_res->entry_count; i++)
    {
        total += memmap_res->entries[i]->length;
    }
    return total;
}

uint64_t get_count_free_pages()
{
    return max_pages - allocated_pages;
}

void pmm_init()
{
    struct limine_memmap_response *map = memmap_res;
    uint64_t top_address = 0;

    for (uint64_t i = 0; i < map->entry_count; i++)
    {
        struct limine_memmap_entry *entry = map->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE)
        {
            top_address = entry->base + entry->length;
        }
    }

    max_pages = top_address / PAGE_SIZE;
    bitmap_size = ALIGN_UP(max_pages / 8, PAGE_SIZE);

    for (uint64_t i = 0; i < map->entry_count; i++)
    {
        struct limine_memmap_entry *entry = map->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size)
        {
            uint64_t bitmap_phys_addr = entry->base;
            bitmap = (uint8_t*)(bitmap_phys_addr + hhdm_res->offset);

            memset(bitmap, 0xFF, bitmap_size);

            entry->base += bitmap_size;
            entry->length -= bitmap_size;

            if (entry->length < PAGE_SIZE)
            {
                entry->type = LIMINE_MEMMAP_RESERVED;
            }

            break;
        }
    }

    for (uint64_t i = 0; i < map->entry_count; i++)
    {
        struct limine_memmap_entry *entry = map->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE)
        {
            uint64_t start = entry->base / PAGE_SIZE;
            uint64_t count = entry->length / PAGE_SIZE;

            while (count > 0 && (start % 8 != 0))
            {
                bitmap_clear(start);
                start++;
                count--;
            }

            uint64_t full_bytes = count / 8;
            if (full_bytes > 0) {
                memset(&bitmap[start / 8], 0, full_bytes);
                start += full_bytes * 8;
                count -= full_bytes * 8;
            }

            while (count > 0) {
                bitmap_clear(start);
                start++;
                count--;
            }
        }
    }
}

void *alloc_pages(size_t count)
{
    uint64_t irq_flags = save_irq_disable();
    spin_lock(&pmm_lock);

    for (uint64_t i = last_checked_page; i + count <= max_pages;)
    {
        if (i % 8 == 0 && bitmap[i / 8] == 0xFF)
        {
            i += 8;
            continue;
        }

        if (!bitmap_test(i))
        {
            uint64_t j;
            for (j = 1; j < count; j++)
            {
                if (bitmap_test(i + j))
                {
                    break;
                }
            }

            if (j == count)
            {
                for (uint64_t k = 0; k < count; k++)
                {
                    bitmap_set(i + k);
                }
                last_checked_page = i + count;
                allocated_pages += count;
                spin_unlock(&pmm_lock);
                restore_irq(irq_flags);
                return (void*)((i * PAGE_SIZE) + hhdm_offset);
            }

            i += j + 1;
        }
        else
        {
            i++;
        }
    }
    for (uint64_t i = 0; i + count <= last_checked_page;)
    {
        if (i % 8 == 0 && bitmap[i / 8] == 0xFF)
        {
            i += 8;
            continue;
        }

        if (!bitmap_test(i))
        {
            uint64_t j;
            for (j = 1; j < count; j++)
            {
                if (bitmap_test(i + j))
                {
                    break;
                }
            }

            if (j == count)
            {
                for (uint64_t k = 0; k < count; k++)
                {
                    bitmap_set(i + k);
                }
                last_checked_page = i + count;
                allocated_pages += count;
                spin_unlock(&pmm_lock);
                restore_irq(irq_flags);
                return (void*)((i * PAGE_SIZE) + hhdm_offset);
            }

            i += j + 1;
        }
        else
        {
            i++;
        }
    }

    spin_unlock(&pmm_lock);
    restore_irq(irq_flags);

    return NULL;
}

void *alloc_page()
{
    uint64_t irq_flags = save_irq_disable();
    spin_lock(&pmm_lock);

    for (uint64_t i = last_checked_page; i < max_pages; i++)
    {
        if (i % 8 == 0 && i + 8 <= max_pages)
        {
            if (bitmap[i / 8] == 0xFF)
            {
                i += 8;
                continue;
            }
        }

        if (!bitmap_test(i))
        {
            bitmap_set(i);
            last_checked_page = i;
            allocated_pages++;
            spin_unlock(&pmm_lock);
            restore_irq(irq_flags);
            return (void*)((i * PAGE_SIZE) + hhdm_offset);
        }
    }
    for (uint64_t i = 0; i < last_checked_page; i++)
    {
        if (i % 8 == 0 && i + 8 <= last_checked_page)
        {
            if (bitmap[i / 8] == 0xFF)
            {
                i += 8;
                continue;
            }
        }

        if (!bitmap_test(i))
        {
            bitmap_set(i);
            last_checked_page = i;
            allocated_pages++;
            spin_unlock(&pmm_lock);
            restore_irq(irq_flags);
            return (void*)((i * PAGE_SIZE) + hhdm_offset);
        }
    }

    spin_unlock(&pmm_lock);
    restore_irq(irq_flags);

    return NULL;
}

void *alloc_huge_page()
{
    uint64_t irq_flags = save_irq_disable();
    spin_lock(&pmm_lock);

    const uint64_t count = 512;

    for (uint64_t i = ALIGN_UP(last_checked_page, count); i + count <= max_pages; i += count)
    {
        bool found = true;
        for (uint64_t k = 0; k < count; k++)
        {
            if (bitmap_test(i + k))
            {
                found = false;
                break;
            }
        }

        if (found)
        {
            for (uint64_t k = 0; k < count; k++)
            {
                bitmap_set(i + k);
            }

            allocated_pages += count;
            last_checked_page = i + count;

            spin_unlock(&pmm_lock);
            restore_irq(irq_flags);
            return (void*)((i * PAGE_SIZE) + hhdm_offset);
        }
    }

    spin_unlock(&pmm_lock);
    restore_irq(irq_flags);
    return NULL;
}

int free_page(void *ptr)
{
    uint64_t vaddr = (uintptr_t)ptr;
    if (vaddr % PAGE_SIZE != 0 || vaddr < hhdm_offset)
        return -1;

    uint64_t phys_addr = vaddr - hhdm_offset;
    uint64_t page = phys_addr / PAGE_SIZE;

    if (page >= max_pages) return -1;

    uint64_t irq_flags = save_irq_disable();
    spin_lock(&pmm_lock);

    if (bitmap_test(page))
    {
        bitmap_clear(page);
        allocated_pages--;
    }

    spin_unlock(&pmm_lock);
    restore_irq(irq_flags);
    return 0;
}

int free_huge_page(void *ptr)
{
    uintptr_t addr = (uintptr_t)ptr - hhdm_offset;
    if (addr % (512 * PAGE_SIZE) != 0) return -1;

    uint64_t start_page = addr / PAGE_SIZE;
    
    uint64_t irq_flags = save_irq_disable();
    spin_lock(&pmm_lock);

    for (uint64_t i = 0; i < 512; i++)
    {
        bitmap_clear(start_page + i);
    }
    allocated_pages -= 512;

    spin_unlock(&pmm_lock);
    restore_irq(irq_flags);
    return 0;
}