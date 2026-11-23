#include <stdint.h>
#include <gmu/timer.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define PIT_COMMAND  0x43
#define PIT_CHANNEL0 0x40
#define PIT_INPUT_HZ 1193182

extern uint64_t gmu_timer_ticks(void);

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

static void pic_remap(void) {
    outb(PIC1_COMMAND, 0x11);
    io_wait();
    outb(PIC2_COMMAND, 0x11);
    io_wait();

    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();

    outb(PIC1_DATA, 0x04);
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();

    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    /* Unmask IRQ0 (Timer) and IRQ2 (Cascade) on PIC1; PIC2 all masked for now */
    outb(PIC1_DATA, 0xFE);
    outb(PIC2_DATA, 0xFF);
}

static void pit_configure(void) {
    uint16_t divisor = (uint16_t)(PIT_INPUT_HZ / GMU_TIMER_HZ);

    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)(divisor >> 8));
}

void gmu_timer_init(void) {
    __asm__ volatile ("cli" ::: "memory");

    pic_remap();
    pit_configure();

    __asm__ volatile ("sti" ::: "memory");
}

uint64_t gmu_timer_ms(void) {
    return (gmu_timer_ticks() * 1000ULL) / GMU_TIMER_HZ;
}

uint64_t gmu_timer_seconds(void) {
    return gmu_timer_ticks() / GMU_TIMER_HZ;
}

void gmu_sleep_ms(uint64_t milliseconds) {
    uint64_t start = gmu_timer_ms();

    while ((gmu_timer_ms() - start) < milliseconds) {
        __asm__ volatile ("hlt");
    }
}
