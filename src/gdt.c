#include <stdint.h>

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

uint64_t gdt[7] __attribute__((aligned(8))) = {
    0x0000000000000000, // 0x00: Null
    0x00AF9A000000FFFF, // 0x08: Kernel Code (64-bit)
    0x00AF92000000FFFF, // 0x10: Kernel Data
    0x00AFFA000000FFFF, // 0x18: User Code
    0x00AFF2000000FFFF, // 0x20: User Data
    0x0000000000000000, // 0x28: TSS Low
    0x0000000000000000  // 0x30: TSS High
};

struct gdt_ptr g_gdt_ptr;

void gdt_reload() {
    g_gdt_ptr.limit = sizeof(gdt) - 1;
    g_gdt_ptr.base = (uint64_t)&gdt;
    
    asm volatile (
        "lgdt %0\n\t"
        "push $0x08\n\t"
        "lea 1f(%%rip), %%rax\n\t"
        "push %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        : : "m"(g_gdt_ptr) : "rax", "memory"
    );
}