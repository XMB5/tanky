#ifndef __UTIL_H__
#define __UTIL_H__

#include <stddef.h>

extern const double PI;

/**
 * malloc(), but assert that the result is not NULL
 */
void *malloc_safe(size_t size);

/**
 * realloc(), but assert that the result is not NULL
 */
void *realloc_safe(void *ptr, size_t size);

double rand_range(double min, double max);
const char *strdup_safe(const char *str);

#endif