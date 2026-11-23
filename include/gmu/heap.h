#ifndef GMU_HEAP_H
#define GMU_HEAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void gmu_heap_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void *kcalloc(size_t num, size_t size);
void *krealloc(void *ptr, size_t new_size);

size_t gmu_heap_get_total(void);
size_t gmu_heap_get_used(void);
size_t gmu_heap_get_free(void);

#endif
