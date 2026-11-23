#include <gmu/pmm.h>

#define MAX_PHYS_PAGES 262144ULL  /* Supports up to 1 GB physical memory */
#define BITMAP_SIZE (MAX_PHYS_PAGES / 64ULL)

static uint64_t pmm_bitmap[BITMAP_SIZE];
static gmu_mem_stats_t mem_stats;

static inline void bitmap_set(uint64_t page) {
    if (page < MAX_PHYS_PAGES) {
        pmm_bitmap[page / 64] |= (1ULL << (page % 64));
    }
}

static inline void bitmap_clear(uint64_t page) {
    if (page < MAX_PHYS_PAGES) {
        pmm_bitmap[page / 64] &= ~(1ULL << (page % 64));
    }
}

static inline bool bitmap_test(uint64_t page) {
    if (page >= MAX_PHYS_PAGES) return true;
    return (pmm_bitmap[page / 64] & (1ULL << (page % 64))) != 0;
}

void gmu_pmm_init(struct limine_memmap_response *memmap) {
    /* Initialize bitmap as completely used / reserved */
    for (uint64_t i = 0; i < BITMAP_SIZE; i++) {
        pmm_bitmap[i] = 0xFFFFFFFFFFFFFFFFULL;
    }

    mem_stats.total_memory = 0;
    mem_stats.usable_memory = 0;
    mem_stats.used_memory = 0;
    mem_stats.free_memory = 0;
    mem_stats.total_pages = 0;
    mem_stats.usable_pages = 0;
    mem_stats.used_pages = 0;
    mem_stats.free_pages = 0;

    if (!memmap) return;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        mem_stats.total_memory += entry->length;

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            mem_stats.usable_memory += entry->length;
            uint64_t start_page = entry->base / GMU_PAGE_SIZE;
            uint64_t page_count = entry->length / GMU_PAGE_SIZE;

            for (uint64_t p = 0; p < page_count; p++) {
                uint64_t page_idx = start_page + p;
                if (page_idx < MAX_PHYS_PAGES) {
                    /* Reserve first 1MB of memory for BIOS/VRAM/legacy */
                    if (page_idx * GMU_PAGE_SIZE >= 0x100000ULL) {
                        bitmap_clear(page_idx);
                        mem_stats.free_pages++;
                    }
                }
            }
        }
    }

    mem_stats.total_pages = mem_stats.total_memory / GMU_PAGE_SIZE;
    mem_stats.usable_pages = mem_stats.usable_memory / GMU_PAGE_SIZE;
    mem_stats.free_memory = mem_stats.free_pages * GMU_PAGE_SIZE;
    mem_stats.used_pages = mem_stats.usable_pages > mem_stats.free_pages ?
                           mem_stats.usable_pages - mem_stats.free_pages : 0;
    mem_stats.used_memory = mem_stats.used_pages * GMU_PAGE_SIZE;
}

void *gmu_pmm_alloc_page(void) {
    for (uint64_t i = 0; i < MAX_PHYS_PAGES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            if (mem_stats.free_pages > 0) mem_stats.free_pages--;
            mem_stats.used_pages++;
            mem_stats.free_memory = mem_stats.free_pages * GMU_PAGE_SIZE;
            mem_stats.used_memory = mem_stats.used_pages * GMU_PAGE_SIZE;
            return (void *)(uintptr_t)(i * GMU_PAGE_SIZE);
        }
    }
    return 0;
}

void gmu_pmm_free_page(void *ptr) {
    uint64_t addr = (uint64_t)(uintptr_t)ptr;
    uint64_t page = addr / GMU_PAGE_SIZE;
    if (page < MAX_PHYS_PAGES && bitmap_test(page)) {
        bitmap_clear(page);
        mem_stats.free_pages++;
        if (mem_stats.used_pages > 0) mem_stats.used_pages--;
        mem_stats.free_memory = mem_stats.free_pages * GMU_PAGE_SIZE;
        mem_stats.used_memory = mem_stats.used_pages * GMU_PAGE_SIZE;
    }
}

void *gmu_pmm_alloc_pages(size_t count) {
    if (count == 0) return 0;
    if (count == 1) return gmu_pmm_alloc_page();

    uint64_t found_count = 0;
    uint64_t start_page = 0;

    for (uint64_t i = 0; i < MAX_PHYS_PAGES; i++) {
        if (!bitmap_test(i)) {
            if (found_count == 0) start_page = i;
            found_count++;
            if (found_count == count) {
                for (uint64_t p = 0; p < count; p++) {
                    bitmap_set(start_page + p);
                }
                if (mem_stats.free_pages >= count) mem_stats.free_pages -= count;
                mem_stats.used_pages += count;
                mem_stats.free_memory = mem_stats.free_pages * GMU_PAGE_SIZE;
                mem_stats.used_memory = mem_stats.used_pages * GMU_PAGE_SIZE;
                return (void *)(uintptr_t)(start_page * GMU_PAGE_SIZE);
            }
        } else {
            found_count = 0;
        }
    }
    return 0;
}

void gmu_pmm_free_pages(void *ptr, size_t count) {
    uint64_t addr = (uint64_t)(uintptr_t)ptr;
    uint64_t start_page = addr / GMU_PAGE_SIZE;
    for (size_t i = 0; i < count; i++) {
        gmu_pmm_free_page((void *)(uintptr_t)((start_page + i) * GMU_PAGE_SIZE));
    }
}

gmu_mem_stats_t gmu_pmm_get_stats(void) {
    return mem_stats;
}
