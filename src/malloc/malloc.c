#include "malloc.h"
#include "../spinlock/spinlock.h"
#include "../libc/string.h"

struct node {
    struct node *next;
};

struct {
    spinlock_t lock;
    struct node *free_lists[MAX_ORDER];
    uint64_t total_free;
} allocator;

struct page
{
    uint8_t order : 4;
    bool is_free : 1;
    bool is_slab : 1;
    uint8_t cache_idx : 4;
} __attribute__((packed));

struct slab {
    void *free_list;
    size_t active_count;
    struct slab *next;
    struct slab *prev;
    void *page_addr;
    struct mem_cache *cache;
};

struct mem_cache
{
    size_t obj_size;
    size_t objs_per_slab;
    struct slab *slabs_free;
    struct slab *slabs_partial;
    struct slab *slabs_full;
    spinlock_t lock;
    size_t free_slabs_count;
    int id;
};

struct page *pages_meta;

static void slab_list_remove(struct slab **head, struct slab *s)
{
    if (!s || !head || !*head) return;

    if (s->prev)
    {
        s->prev->next = s->next;
    }
    else
    {
        *head = s->next;
    }

    if (s->next)
    {
        s->next->prev = s->prev;
    }

    s->next = s->prev = NULL;
}

static void slab_list_add(struct slab **head, struct slab *s)
{
    s->next = *head;
    s->prev = NULL;
    if (*head)
    {
        (*head)->prev = s;
    }
    *head = s;
}

static void move_slab(struct slab *s, struct slab **from, struct slab **to)
{
    slab_list_remove(from, s);
    slab_list_add(to, s);
}

static void remove_from_list(struct node *block, uint8_t order)
{
    struct node **curr = &allocator.free_lists[order];
    while (*curr)
    {
        if (*curr == block)
        {
            *curr = block->next;
            return;
        }
        curr = &((*curr)->next);
    }
}

void *b_malloc(size_t size)
{
    uint8_t order = size_to_order(size);

    if (order >= MAX_ORDER)
        return NULL;

    uint8_t irq_flags = save_irq_disable();
    spin_lock(&allocator.lock);

    for (int i = order; i < MAX_ORDER; i++)
    {
        if (allocator.free_lists[i])
        {
            struct node *block = allocator.free_lists[i];
            allocator.free_lists[i] = block->next;

            while (i > order)
            {
                i--;
                uintptr_t buddy = (uintptr_t)block + (PAGE_SIZE << i);
                struct node *node = (struct node*)buddy;

                node->next = allocator.free_lists[i];
                allocator.free_lists[i] = node;
            }

            size_t i = get_page_index(block);
            pages_meta[i].is_free = false;
            pages_meta[i].order = order;

            spin_unlock(&allocator.lock);
            restore_irq(irq_flags);
            return (void*)block;
        }
    }

    spin_unlock(&allocator.lock);
    restore_irq(irq_flags);
    return NULL;
}

void *b_calloc(size_t count, size_t size)
{
    size_t total_size = count * size;
    void *mem = b_malloc(total_size);
    
    if (mem)
        memset(mem, 0, total_size);

    return mem;
}

void b_free(void *ptr) {
    if (!ptr) return;

    size_t start_node_idx = get_page_index(ptr);
    uint8_t original_order = pages_meta[start_node_idx].order;
    uint8_t current_order = original_order;
    uintptr_t addr = (uintptr_t)ptr;

    uint64_t irq_flags = save_irq_disable();
    spin_lock(&allocator.lock);

    while (current_order < MAX_ORDER - 1) {
        uintptr_t buddy_addr = get_buddy_addr(addr, current_order);
        size_t buddy_idx = get_page_index((void*)buddy_addr);

        if (buddy_addr < max_pages && pages_meta[buddy_idx].is_free && pages_meta[buddy_idx].order == current_order) {
            remove_from_list((struct node*)buddy_addr, current_order);
            addr &= ~(PAGE_SIZE << current_order);
            current_order++;
        } else {
            break;
        }
    }

    size_t final_idx = get_page_index((void*)addr);
    pages_meta[final_idx].is_free = true;
    pages_meta[final_idx].order = current_order;

    struct node *node = (struct node*)addr;
    node->next = allocator.free_lists[current_order];
    allocator.free_lists[current_order] = node;

    allocator.total_free += order_to_size(original_order);

    spin_unlock(&allocator.lock);
    restore_irq(irq_flags);
}

void *b_realloc(void *ptr, size_t size)
{
    size_t i = get_page_index(ptr);
    size_t old_size = order_to_size(pages_meta[i].order);
    void *mem = b_malloc(size);
    if (mem && ptr)
        memcpy(mem, ptr, (old_size < size) ? old_size : size);

    b_free(ptr);

    return mem;
}

static struct slab *alloc_slab(struct mem_cache *cache)
{
    void *page = b_malloc(PAGE_SIZE);
    if (!page) return NULL;

    struct slab *s = (struct slab*)page;
    s->page_addr = page;
    s->active_count = 0;
    s->cache = cache;

    uint8_t *data = (uint8_t*)page + sizeof(struct slab);
    s->free_list = data;
    cache->free_slabs_count++;

    for (size_t i = 0; i < cache->objs_per_slab - 1; i++)
    {
        void **next_ptr = (void**)(data + i * cache->obj_size);
        *next_ptr = data + (i + 1) * cache->obj_size;
    }

    *(void**)(data + (cache->objs_per_slab- 1) * cache->obj_size) = NULL;
    return s;
}

void *cache_alloc(struct mem_cache* cache) {
    uint8_t irq_flags = save_irq_disable();
    spin_lock(&cache->lock);

    struct slab *s = cache->slabs_partial;
    if (!s) s = cache->slabs_free;

    if (!s) {
        s = alloc_slab(cache);
        if (!s)
        {
            spin_unlock(&cache->lock);
            restore_irq(irq_flags);
            return NULL;
        }
        size_t page_i = get_page_index(s);
        pages_meta[page_i].is_slab = true;
        pages_meta[page_i].cache_idx = s->cache->id;

        slab_list_add(&cache->slabs_free, s);
    }

    void *obj = s->free_list;
    s->free_list = *(void**)obj;
    s->active_count++;

    if (s->active_count == cache->objs_per_slab) {
        struct slab **from = (s->active_count == 1) ? &cache->slabs_free : &cache->slabs_partial;
        move_slab(s, from, &cache->slabs_full);
    } else if (s->active_count == 1) {
        move_slab(s, &cache->slabs_free, &cache->slabs_partial);
    }

    spin_unlock(&cache->lock);
    restore_irq(irq_flags);
    return obj;
}

void cache_free(struct mem_cache *cache, void *obj) {
    if (!obj) return;
    
    uint8_t irq_flags = save_irq_disable();
    spin_lock(&cache->lock);

    struct slab *s = (struct slab *)((uintptr_t)obj & ~(PAGE_SIZE - 1));

    *(void**)obj = s->free_list;
    s->free_list = obj;
    s->active_count--;

    if (s->active_count == 0) {
        cache->free_slabs_count++;

        if (cache->free_slabs_count > MAX_FREE_SLAB)
        {
            slab_list_remove(&cache->slabs_free, s);
            b_free(s);
            spin_unlock(&cache->lock);
            restore_irq(irq_flags);
            return;
        }
        move_slab(s, &cache->slabs_partial, &cache->slabs_free);
    } else if (s->active_count == cache->objs_per_slab - 1) {
        move_slab(s, &cache->slabs_full, &cache->slabs_partial);
    }

    spin_unlock(&cache->lock);
    restore_irq(irq_flags);
}

struct mem_cache *cache_create(size_t size)
{
    struct mem_cache *cache = b_malloc(sizeof(struct mem_cache));
    if (!cache) return NULL;

    cache->obj_size = ALIGN_UP(size, 8);
    cache->objs_per_slab = (PAGE_SIZE - sizeof(struct slab)) / cache->obj_size;
    cache->slabs_free = NULL;
    cache->slabs_partial = NULL;
    cache->slabs_full = NULL;
    cache->free_slabs_count = 0;
    cache->id = get_cache_index(size);

    return cache;
}

void malloc_init_caches() {
    size_t sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048};
    for (int i = 0; i < 8; i++) {
        k_caches[i] = cache_create(sizes[i]);
    }
}

void malloc_init()
{
    size_t page_count = get_total_memory() / PAGE_SIZE;
    size_t meta_size = sizeof(struct page) * page_count;
    size_t aligned_size = ALIGN_UP(meta_size, PAGE_SIZE);
    pages_meta = (struct page*)(alloc_pages(aligned_size / PAGE_SIZE));

    for (uint64_t i = 0; i < memmap_res->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap_res->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE)
        {
            uintptr_t addr = entry->base;
            uintptr_t len = entry->length;

            while (len >= PAGE_SIZE)
            {
                if (bitmap_test(addr / PAGE_SIZE))
                {
                    addr += PAGE_SIZE;
                    len -= PAGE_SIZE;
                    continue;
                }
                uint8_t order = calculate_max_fit_order(addr, len);

                size_t i = get_page_index((void*)(addr + hhdm_offset));
                pages_meta[i].order = order;
                pages_meta[i].is_free = true;

                struct node *n = (struct node*)(addr + hhdm_offset);
                n->next = allocator.free_lists[order];
                allocator.free_lists[order] = n;

                uint64_t block_size = (1ULL << order) * PAGE_SIZE;
                addr += block_size;
                len -= block_size;
            }
        }
    }

    malloc_init_caches();
}

void *malloc(size_t size)
{
    if (size <= PAGE_SIZE / 2)
    {
        int cache_i = get_cache_index(size);
        return cache_alloc(k_caches[cache_i]);
    }
    return b_malloc(size);
}

void free(void *ptr)
{
    size_t i = get_page_index(ptr);
    if (pages_meta[i].is_slab)
    {
        cache_free(k_caches[pages_meta[i].cache_idx], ptr);
    }
    else
    {
        b_free(ptr);
    }
}

void *calloc(size_t count, size_t size)
{
    size_t total_size = count * size;
    void *mem = malloc(total_size);
    if (mem)
        memset(mem, 0, total_size);

    return mem;
}

void *realloc(void *ptr, size_t size)
{
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return NULL; }

    size_t i = get_page_index(ptr);
    size_t old_size;

    if (pages_meta[i].is_slab) {
        old_size = k_caches[pages_meta[i].cache_idx]->obj_size;
    } else {
        old_size = order_to_size(pages_meta[i].order);
    }

    void *new_ptr = malloc(size);
    if (!new_ptr) return NULL;

    memcpy(new_ptr, ptr, (old_size < size) ? old_size : size);
    free(ptr);

    return new_ptr;
}

void get_kmalloc_stats(uint64_t *free_mem) {
    if (free_mem) *free_mem = allocator.total_free;
}