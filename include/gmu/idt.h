#ifndef GMU_IDT_H
#define GMU_IDT_H

#include <stdint.h>
#include <stdbool.h>

#define GMU_IDT_ENTRIES 256

typedef struct __attribute__((packed)) {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attributes;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} gmu_idt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} gmu_idt_pointer_t;

void gmu_idt_init(void);

#endif
