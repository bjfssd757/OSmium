#include "reboot.h"
#include "../portio/portio.h"

static inline void wait_kbc(void)
{
    while (inb(0x64) & 0x02)
        ;
}

void reboot_system(void)
{
    wait_kbc();
    outb(0x64, 0xFE);

    asm volatile(
        "lidt (0) \n\t"
        "int $3"
    );

    while (1)
        asm volatile("hlt");
}