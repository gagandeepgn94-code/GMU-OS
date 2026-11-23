
#include <stdint.h>
#include <gmu/idt.h>

extern void gmu_isr_unhandled(void);
extern void gmu_isr_0(void);
extern void gmu_isr_1(void);
extern void gmu_isr_2(void);
extern void gmu_isr_3(void);
extern void gmu_isr_4(void);
extern void gmu_isr_5(void);
extern void gmu_isr_6(void);
extern void gmu_isr_7(void);
extern void gmu_isr_8(void);
extern void gmu_isr_9(void);
extern void gmu_isr_10(void);
extern void gmu_isr_11(void);
extern void gmu_isr_12(void);
extern void gmu_isr_13(void);
extern void gmu_isr_14(void);
extern void gmu_isr_15(void);
extern void gmu_isr_16(void);
extern void gmu_isr_17(void);
extern void gmu_isr_18(void);
extern void gmu_isr_19(void);
extern void gmu_isr_20(void);
extern void gmu_isr_21(void);
extern void gmu_isr_22(void);
extern void gmu_isr_23(void);
extern void gmu_isr_24(void);
extern void gmu_isr_25(void);
extern void gmu_isr_26(void);
extern void gmu_isr_27(void);
extern void gmu_isr_28(void);
extern void gmu_isr_29(void);
extern void gmu_isr_30(void);
extern void gmu_isr_31(void);
extern void gmu_isr_32(void);

static gmu_idt_entry_t idt[GMU_IDT_ENTRIES]
    __attribute__((aligned(16)));

static gmu_idt_pointer_t idt_ptr;

static void (*const interrupt_handlers[33])(void) = {
    gmu_isr_0,
    gmu_isr_1,
    gmu_isr_2,
    gmu_isr_3,
    gmu_isr_4,
    gmu_isr_5,
    gmu_isr_6,
    gmu_isr_7,
    gmu_isr_8,
    gmu_isr_9,
    gmu_isr_10,
    gmu_isr_11,
    gmu_isr_12,
    gmu_isr_13,
    gmu_isr_14,
    gmu_isr_15,
    gmu_isr_16,
    gmu_isr_17,
    gmu_isr_18,
    gmu_isr_19,
    gmu_isr_20,
    gmu_isr_21,
    gmu_isr_22,
    gmu_isr_23,
    gmu_isr_24,
    gmu_isr_25,
    gmu_isr_26,
    gmu_isr_27,
    gmu_isr_28,
    gmu_isr_29,
    gmu_isr_30,
    gmu_isr_31,
    gmu_isr_32
};

static uint16_t read_cs(void) {
    uint16_t selector;
    __asm__ volatile ("mov %%cs, %0" : "=r"(selector));
    return selector;
}

static void idt_set_gate(uint8_t vector, uintptr_t handler,
                         uint16_t selector) {
    gmu_idt_entry_t *entry = &idt[vector];

    entry->offset_low = handler & 0xFFFF;
    entry->selector = selector;
    entry->ist = 0;
    entry->type_attributes = 0x8E;
    entry->offset_middle = (handler >> 16) & 0xFFFF;
    entry->offset_high = (handler >> 32) & 0xFFFFFFFF;
    entry->reserved = 0;
}

void gmu_idt_init(void) {
    uint16_t selector = read_cs();

    for (uint16_t i = 0; i < GMU_IDT_ENTRIES; i++) {
        idt_set_gate((uint8_t)i,
                     (uintptr_t)gmu_isr_unhandled,
                     selector);
    }

    for (uint16_t i = 0; i < 33; i++) {
        idt_set_gate((uint8_t)i,
                     (uintptr_t)interrupt_handlers[i],
                     selector);
    }

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uintptr_t)idt;

    __asm__ volatile ("lidt %0" :: "m"(idt_ptr) : "memory");
}
