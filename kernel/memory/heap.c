#include <gmu/heap.h>

#define HEAP_CAPACITY (1024 * 1024 * 2) /* 2 MB heap pool */

typedef struct heap_header {
    uint32_t size;
    bool is_free;
    struct heap_header *next;
} heap_header_t;

#define HEADER_SIZE sizeof(heap_header_t)

static uint8_t heap_pool[HEAP_CAPACITY] __attribute__((aligned(16)));
static heap_header_t *free_list = 0;
static size_t used_bytes = 0;

void gmu_heap_init(void) {
    free_list = (heap_header_t *)heap_pool;
    free_list->size = HEAP_CAPACITY - HEADER_SIZE;
    free_list->is_free = true;
    free_list->next = 0;
    used_bytes = 0;
}

void *kmalloc(size_t size) {
    if (size == 0) return 0;

    /* Align size to 16 bytes */
    size = (size + 15) & ~15ULL;

    heap_header_t *curr = free_list;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            /* Can we split this block? */
            if (curr->size >= size + HEADER_SIZE + 16) {
                heap_header_t *next_block = (heap_header_t *)((uint8_t *)curr + HEADER_SIZE + size);
                next_block->size = curr->size - size - HEADER_SIZE;
                next_block->is_free = true;
                next_block->next = curr->next;

                curr->size = (uint32_t)size;
                curr->next = next_block;
            }

            curr->is_free = false;
            used_bytes += curr->size + HEADER_SIZE;
            return (void *)((uint8_t *)curr + HEADER_SIZE);
        }
        curr = curr->next;
    }

    return 0; /* Out of memory */
}

void kfree(void *ptr) {
    if (!ptr) return;

    heap_header_t *block = (heap_header_t *)((uint8_t *)ptr - HEADER_SIZE);
    if ((uint8_t *)block < heap_pool || (uint8_t *)block >= heap_pool + HEAP_CAPACITY) {
        return; /* Outside heap range */
    }

    if (!block->is_free) {
        block->is_free = true;
        if (used_bytes >= block->size + HEADER_SIZE) {
            used_bytes -= (block->size + HEADER_SIZE);
        }
    }

    /* Coalesce adjacent free blocks */
    heap_header_t *curr = free_list;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += curr->next->size + HEADER_SIZE;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

void *kcalloc(size_t num, size_t size) {
    size_t total = num * size;
    void *ptr = kmalloc(total);
    if (ptr) {
        uint8_t *b = (uint8_t *)ptr;
        for (size_t i = 0; i < total; i++) b[i] = 0;
    }
    return ptr;
}

void *krealloc(void *ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return 0;
    }

    heap_header_t *block = (heap_header_t *)((uint8_t *)ptr - HEADER_SIZE);
    if (block->size >= new_size) return ptr;

    void *new_ptr = kmalloc(new_size);
    if (new_ptr) {
        uint8_t *src = (uint8_t *)ptr;
        uint8_t *dst = (uint8_t *)new_ptr;
        size_t copy_len = block->size < new_size ? block->size : new_size;
        for (size_t i = 0; i < copy_len; i++) dst[i] = src[i];
        kfree(ptr);
    }
    return new_ptr;
}

size_t gmu_heap_get_total(void) {
    return HEAP_CAPACITY;
}

size_t gmu_heap_get_used(void) {
    return used_bytes;
}

size_t gmu_heap_get_free(void) {
    return HEAP_CAPACITY > used_bytes ? HEAP_CAPACITY - used_bytes : 0;
}
