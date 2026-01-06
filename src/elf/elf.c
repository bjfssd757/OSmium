#include "elf.h"
#include "../libc/string.h"
#include "../mm/pmm.h"

bool load_elf(void *elf_buf, size_t size, page_table_t **out_pml4, Elf64_Addr *entry)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)elf_buf;

    if (memcmp(ehdr->e_ident, "\x7F""ELF", 4) != 0) return false;
    if (ehdr->e_ident[4] != ELFCLASS64) return false;

    page_table_t *pml4 = create_address_space();
    if (!pml4) return false;

    Elf64_Phdr *phdr = (Elf64_Phdr*)((char*)elf_buf + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            uint64_t virt_start = phdr[i].p_vaddr;
            uint64_t virt_end = virt_start + phdr[i].p_memsz;
            uint64_t pages = PAGES_COUNT(virt_start, virt_end);

            uint64_t map_flags = PTE_USER;
            if (phdr[i].p_flags & 2) map_flags |= PTE_WRITABLE;

            for (uint64_t j = 0; j < pages; j++)
            {
                uint64_t phys_addr = (uint64_t)alloc_page();
                memset(virt(phys_addr), 0, PAGE_SIZE);

                uint64_t page_vaddr = ALIGN_UP(virt_start, PAGE_SIZE) + (j * PAGE_SIZE);

                mmap(pml4, page_vaddr, phys_addr, map_flags);

                uint64_t copy_off = 0;
                if (page_vaddr < virt_start) copy_off = virt_start - page_vaddr;

                if (copy_off < phdr[i].p_filesz)
                {
                    uint64_t to_copy = phdr[i].p_filesz - copy_off;
                    if (to_copy > (PAGE_SIZE - copy_off)) to_copy = PAGE_SIZE - copy_off;

                    memcpy((char*)virt(phys_addr) + copy_off,
                            (char*)elf_buf + phdr[i].p_offset + (page_vaddr + copy_off - virt_start),
                        to_copy
                    );
                }
            }
        }
    }

    *out_pml4 = pml4;
    *entry = ehdr->e_entry;

    return true;
}