#ifndef KERNEL_SORT_H
#define KERNEL_SORT_H

#include "../malloc/malloc.h"
#include "string.h"

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));

#endif // KERNEL_SORT_H