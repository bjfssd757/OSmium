#include "sort.h"

static void swap(void *a, void *b, size_t size) {
    uint8_t *p = a, *q = b, tmp;
    while (size--) {
        tmp = *p;
        *p++ = *q;
        *q++ = tmp;
    }
}

static void qsort_alt(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *))
{
    if (nmemb < 2 || size == 0) return;
    int32_t stack[64]; 
    int32_t top = -1;

    stack[++top] = 0;
    stack[++top] = (int32_t)nmemb - 1;

    while (top >= 0) {
        int32_t high = stack[top--];
        int32_t low = stack[top--];

        int32_t i = low;
        void *pivot = (uint8_t *)base + high * size;

        for (int32_t j = low; j < high; j++) {
            if (compar((uint8_t *)base + j * size, pivot) < 0) {
                swap((uint8_t *)base + i * size, (uint8_t *)base + j * size, size);
                i++;
            }
        }
        swap((uint8_t *)base + i * size, (uint8_t *)base + high * size, size);
        
        int32_t p = i;

        if (p - 1 > low) {
            stack[++top] = low;
            stack[++top] = p - 1;
        }
        if (p + 1 < high) {
            stack[++top] = p + 1;
            stack[++top] = high;
        }
    }
}

static void merge(uint8_t *base, size_t nmemb, size_t size, 
                  int (*compar)(const void *, const void *), uint8_t *tmp)
{
    size_t mid = nmemb / 2;
    size_t i = 0;
    size_t j = mid;
    size_t k = 0;

    while (i < mid && j < nmemb) {
        if (compar(base + i * size, base + j * size) <= 0) {
            memcpy(tmp + k * size, base + i * size, size);
            i++;
        } else {
            memcpy(tmp + k * size, base + j * size, size);
            j++;
        }
        k++;
    }

    if (i < mid) {
        memcpy(tmp + k * size, base + i * size, (mid - i) * size);
    } else if (j < nmemb) {
        memcpy(tmp + k * size, base + j * size, (nmemb - j) * size);
    }

    memcpy(base, tmp, nmemb * size);
}

static void msort_recursive(void *base, size_t nmemb, size_t size,
                            int (*compar)(const void *, const void *), void *tmp)
{
    if (nmemb < 2) return;

    size_t mid = nmemb / 2;

    msort_recursive(base, mid, size, compar, tmp);
    msort_recursive((uint8_t *)base + mid * size, nmemb - mid, size, compar, tmp);

    merge(base, nmemb, size, compar, tmp);
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *))
{
    if (nmemb < 2 || size == 0) return;

    void *tmp = malloc(nmemb * size);
    
    if (tmp) {
        msort_recursive(base, nmemb, size, compar, tmp);
        free(tmp);
    } else {
        qsort_alt(base, nmemb, size, compar);
    }
}