#ifndef LIBC_STRING_H
#define LIBC_STRING_H

#include <stddef.h>

static inline int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

static inline int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

static inline char* strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

static inline char* strncpy(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (n && (*src)) {
        *d++ = *src++;
        n--;
    }
    while (n) {
        *d++ = '\0';
        n--;
    }
    return dest;
}

static inline char* strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) {
        d++;
    }
    while ((*d++ = *src++));
    return dest;
}

static inline char* strncat(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (*d) {
        d++;
    }
    while (n && (*src)) {
        *d++ = *src++;
        n--;
    }
    *d = '\0';
    return dest;
}

static inline int strncasecmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && *s2) {
        char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? (*s1 + 32) : *s1;
        char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? (*s2 + 32) : *s2;
        if (c1 != c2) {
            return (unsigned char)c1 - (unsigned char)c2;
        }
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static inline char* strchrnul(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) {
            return (char *)s;
        }
        s++;
    }
    return (char *)s;
}

static inline size_t strlcpy(char *dest, const char *src, size_t size) {
    size_t src_len = 0;
    const char *s = src;
    while (*s++) {
        src_len++;
    }

    if (size == 0) {
        return src_len;
    }

    size_t to_copy = (src_len >= size) ? (size - 1) : src_len;
    for (size_t i = 0; i < to_copy; i++) {
        dest[i] = src[i];
    }
    dest[to_copy] = '\0';

    return src_len;
}

static inline size_t strlcat(char *dest, const char *src, size_t size) {
    size_t dest_len = 0;
    while (dest_len < size && dest[dest_len]) {
        dest_len++;
    }

    size_t src_len = 0;
    const char *s = src;
    while (*s++) {
        src_len++;
    }

    if (dest_len == size) {
        return size + src_len;
    }

    size_t to_copy = (src_len < (size - dest_len - 1)) ? src_len : (size - dest_len - 1);
    for (size_t i = 0; i < to_copy; i++) {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + to_copy] = '\0';

    return dest_len + src_len;
}

static inline long strtol(const char *nptr, char **endptr, int base) {
    const char *p = nptr;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\f' || *p == '\v') {
        p++;
    }

    int sign = 1;
    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    long result = 0;
    while (1) {
        char c = *p;
        int digit;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'z') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'Z') {
            digit = c - 'A' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        result = result * base + digit;
        p++;
    }

    if (endptr) {
        *endptr = (char *)p;
    }

    return sign * result;
}

static inline size_t strlen(const char *s) {
    size_t i = 0;
    while (s[i]) {
        i++;
    }
    return i;
}

static inline void* memcpy(void *dest, const void *src, size_t n) {
    char *d = (char*)dest;
    const char *s = (const char*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

static inline void* memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else {
        d += n;
        s += n;
        while (n--) {
            *(--d) = *(--s);
        }
    }

    return dest;
}

static inline int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

static inline void* memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

static inline const char* strstr(const char* haystack, const char* needle) {
    if (!*needle) {
        return haystack;
    }
    const char* p1 = haystack;
    while (*p1) {
        const char* p1_begin = p1;
        const char* p2 = needle;
        while (*p1 && *p2 && *p1 == *p2) {
            p1++;
            p2++;
        }
        if (!*p2) {
            return p1_begin;
        }
        p1 = p1_begin + 1;
    }
    return NULL;
}

static inline char* strchr(const char *s, int c) {
    while (*s != (char)c) {
        if (!*s++) {
            return NULL;
        }
    }
    return (char *)s;
}

static inline size_t strspn(const char *s, const char *accept) {
    const char *p = s;
    const char *a;
    size_t count = 0;
    
    while (*p) {
        for (a = accept; *a; a++) {
            if (*p == *a) {
                break;
            }
        }
        if (*a == '\0') {
            return count;
        }
        count++;
        p++;
    }
    
    return count;
}

static inline int tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

static inline int strcasecmp(const char *s1, const char *s2) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    int result;

    if (p1 == p2) {
        return 0;
    }

    while ((result = tolower(*p1) - tolower(*p2++)) == 0) {
        if (*p1++ == '\0') {
            break;
        }
    }

    return result;
}

#endif // LIBC_STRING_H