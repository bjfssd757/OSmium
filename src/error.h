#ifndef ERROR_H
#define ERROR_H

[[noreturn]] void inline error()
{
    while (1)
    {
       asm volatile("hlt");
    }
}

#endif /* ERROR_H */