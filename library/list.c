#include <assert.h>
#include <list.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

static const size_t GROWTH_FACTOR = 2;

struct list {
  size_t size;
  size_t capacity;
  free_func_t freer;
  void **data;
};

list_t *list_init(size_t initial_size, free_func_t freer) {
  list_t *list = malloc_safe(sizeof(list_t));
  list->capacity = initial_size;
  list->size = 0;
  list->freer = freer;
  list->data = malloc_safe(initial_size * sizeof(void *));
  return list;
}

void list_free(list_t *list) {
  if (list->freer) {
    for (size_t i = 0; i < list->size; i++) {
      list->freer(list->data[i]);
    }
  }
  free(list->data);
  free(list);
}

size_t list_size(list_t *list) { return list->size; }

void *list_get(list_t *list, size_t index) {
  assert(index < list->size);
  return list->data[index];
}

void list_add(list_t *list, void *value) {
  assert(value != NULL);
  if (list->size == list->capacity) {
    // expand list
    if (list->capacity == 0) {
      list->capacity = 1;
    } else {
      list->capacity *= GROWTH_FACTOR;
    }
    list->data = realloc_safe(list->data, sizeof(void *) * list->capacity);
  }

  list->data[list->size] = value;
  list->size++;
}

void *list_remove(list_t *list, size_t index) {
  assert(list->size > index);
  void *return_value = list->data[index];
  memmove(list->data + index, list->data + index + 1,
          sizeof(void *) * (list->size - index - 1));
  list->size--;
  return return_value;
}