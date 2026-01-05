#ifndef KPRINT_H
#define KPRINT_H

#include <stddef.h>
#include <stdint.h>
#include "../limine.h"
#define KPRINT_BUFFER_SIZE 512

extern struct limine_framebuffer *fb;

enum kprint_type
{
    KPRINT_LOG,
    KPRINT_ERROR,
    KPRINT_SUCCESS,
    KPRINT_NORMAL,
};

void utoa_unsigned(unsigned long long value, char *out, int base);
void itoa_signed(long long value, char *out);
int kprint(const uint8_t type, const char *format, ...);
int kformat(char *buffer, size_t size, const char *format, ...);

#endif