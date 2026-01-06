#ifndef LIBC_STDLIB_H
#define LIBC_STDLIB_H

#include "syscall.h"

static inline void* malloc(size_t size) {
    return syscall_malloc(size);
}

static inline void* calloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    void* ptr = syscall_malloc(total_size);
    if (ptr) {
        unsigned char* p = (unsigned char*)ptr;
        for (size_t i = 0; i < total_size; i++) {
            p[i] = 0;
        }
    }
    return ptr;
}

static inline void free(void* ptr) {
    syscall_free(ptr);
}

[[noreturn]] static inline void abort(void)
{
    syscall_process_exit(-1);
    for (;;);
}

[[noreturn]] static inline void exit(int exit_code)
{
    syscall_process_exit(exit_code);
    for (;;);
}

#endif // LIBC_STDLIB_H