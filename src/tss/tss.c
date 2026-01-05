#include "tss.h"
#include "../libc/string.h"

extern tss_t tss_buffer;
extern uint64_t gdt[];
extern uint64_t stack64_top;
extern void gdt_reload();

#define IOPB_SIZE 8192

static uint8_t g_tss_buffer[sizeof(tss_t) + IOPB_SIZE];

static tss_t *g_tss = NULL;
static uint8_t *g_iopb = NULL;

static void set_port_permission(uint16_t port, int allow)
{
    if (!g_iopb)
    {
        return;
    }

    int byte_i = port / 8;
    int bit_i = port % 8;

    if (allow)
    {
        g_iopb[byte_i] &= ~(1 << bit_i);
    }
    else
    {
        g_iopb[byte_i] |= (1 << bit_i);
    }
}

int sys_io_port_grant(uint16_t port)
{
    set_port_permission(port, 1);
    return 0;
}

int sys_port_revoke(uint16_t port)
{
    set_port_permission(port, 0);
    return 0;
}

void tss_init(void)
{
    g_tss = (tss_t*)g_tss_buffer;
    g_iopb = g_tss_buffer + sizeof(tss_t);

    memset(g_tss_buffer, 0, sizeof(g_tss_buffer));

    extern uint64_t stack64_top;
    g_tss->rsp0 = (uint64_t)&stack64_top;

    g_tss->iopb_offset = sizeof(tss_t);

    memset(g_iopb, 0xFF, IOPB_SIZE);

    uint64_t tss_addr = (uint64_t)g_tss;
    uint16_t tss_limit = sizeof(tss_t) + IOPB_SIZE - 1;

    gdt_reload();

    gdt[5] = (tss_limit & 0xFFFF) | // Limit 15:0
             ((tss_addr & 0xFFFFFF) << 16) | // Base 23:0
             (0x89ULL << 40) | // Type=9 (64-bit TSS), P=1
             ((uint64_t)(tss_limit & 0xF0000) << 32) | // Limit 19:16
             (((tss_addr >> 24) & 0xFF) << 56); // Base 31:24

    gdt[6] = (tss_addr >> 32);

    asm volatile("mov $0x28, %%ax; ltr %%ax" ::: "ax");
}

void tss_update_rsp0(uint64_t rsp0)
{
    if (g_tss)
        g_tss->rsp0 = rsp0;
}