
#include <stdint.h>

static volatile uint64_t timer_ticks = 0;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
}

void gmu_exception_dispatch(void *frame) {
    (void)frame;

    __asm__ volatile ("cli");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}

void gmu_timer_interrupt(void *frame) {
    (void)frame;

    timer_ticks++;

    /* Acknowledge IRQ0 to the master PIC. */
    outb(0x20, 0x20);
}

uint64_t gmu_timer_ticks(void) {
    return timer_ticks;
}
