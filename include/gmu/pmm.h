#ifndef GMU_PMM_H
#define GMU_PMM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../boot/limine-src/limine-protocol/include/limine.h"

#define GMU_PAGE_SIZE 4096ULL

typedef struct {
    uint64_t total_memory;
    uint64_t usable_memory;
    uint64_t used_memory;
    uint64_t free_memory;
    uint64_t total_pages;
    uint64_t usable_pages;
    uint64_t used_pages;
    uint64_t free_pages;
} gmu_mem_stats_t;

void gmu_pmm_init(struct limine_memmap_response *memmap);
void *gmu_pmm_alloc_page(void);
void gmu_pmm_free_page(void *ptr);
void *gmu_pmm_alloc_pages(size_t count);
void gmu_pmm_free_pages(void *ptr, size_t count);
gmu_mem_stats_t gmu_pmm_get_stats(void);

#endif
