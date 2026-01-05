#ifndef LIBC_UNISTD_H
#define LIBC_UNISTD_H

// Заглушка для geteuid(). В нашей ОС пока нет пользователей,
// поэтому возвращаем 0 (root).
static inline int geteuid(void) {
    return 0;
}

// Заглушка для getegid(). Аналогично, возвращаем 0.
static inline int getegid(void) {
    return 0;
}

#endif // LIBC_UNISTD_H