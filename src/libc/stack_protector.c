// libc/stack_protector.c
#include <stdint.h>
#include "../graphics/vga.h"

uintptr_t __stack_chk_guard = 0xBAAAD00Du;

static void __attribute__((noreturn)) kstack_panic(const char *msg)
{
    print_string_position("STACK SMASHING DETECTED", 20, 2, WHITE, RED);
    if (msg)
    {
        print_string_position(msg, 20, 3, WHITE, RED);
    }
    /* Глушим всё */
    asm volatile("cli");
    asm volatile("hlt" : : : "memory");
    __builtin_unreachable();
}

void __attribute__((noreturn)) __stack_chk_fail(void)
{
    kstack_panic("in __stack_chk_fail()");
}

void __attribute__((noreturn)) __stack_chk_fail_local(void)
{
    kstack_panic("in __stack_chk_fail_local()");
}