#include <assert.h>
#include <stdlib.h>
#include <util.h>
#include <string.h>

const double PI = 3.1415926535897932384626433832795;

void *malloc_safe(size_t size) {
  void *pointer = malloc(size);
  assert(pointer != NULL);
  return pointer;
}

void *realloc_safe(void *ptr, size_t size) {
  void *pointer = realloc(ptr, size);
  assert(pointer != NULL);
  return pointer;
}

double rand_range(double min, double max) {
  return ((double)rand()) / RAND_MAX * (max - min) + min;
}

const char *strdup_safe(const char *str) {
  size_t str_len = strlen(str);
  const char *new = malloc_safe(str_len + 1);
  memcpy(new, str, str_len + 1);
  return new;
}