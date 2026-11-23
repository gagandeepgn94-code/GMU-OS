#include <stdint.h>
#include <stdbool.h>
#include <gmu/ps2.h>

#define PS2_DATA       0x60
#define PS2_STATUS     0x64
#define PS2_COMMAND    0x64

#define STATUS_OUTPUT_FULL 0x01
#define STATUS_INPUT_FULL  0x02
#define STATUS_AUXILIARY   0x20

#define QUEUE_SIZE 512
#define QUEUE_MASK (QUEUE_SIZE - 1)

static uint8_t keyboard_queue[QUEUE_SIZE];
static uint8_t mouse_queue[QUEUE_SIZE];

static volatile uint16_t keyboard_head, keyboard_tail;
static volatile uint16_t mouse_head, mouse_tail;

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void queue_push(uint8_t *queue,
                       volatile uint16_t *head,
                       volatile uint16_t *tail,
                       uint8_t value) {
    uint16_t next = (*head + 1) & QUEUE_MASK;

    if (next == *tail)
        return;

    queue[*head] = value;
    *head = next;
}

static bool queue_pop(uint8_t *queue,
                      volatile uint16_t *head,
                      volatile uint16_t *tail,
                      uint8_t *value) {
    if (*tail == *head)
        return false;

    *value = queue[*tail];
    *tail = (*tail + 1) & QUEUE_MASK;
    return true;
}

/* Read controller output and route bytes to the correct device queue. */
static void pump(void) {
    for (uint32_t i = 0; i < 64; i++) {
        uint8_t status = inb(PS2_STATUS);

        if (!(status & STATUS_OUTPUT_FULL))
            break;

        uint8_t data = inb(PS2_DATA);

        if (status & STATUS_AUXILIARY)
            queue_push(mouse_queue, &mouse_head, &mouse_tail, data);
        else
            queue_push(keyboard_queue, &keyboard_head,
                       &keyboard_tail, data);
    }
}

static bool wait_input_clear(void) {
    for (uint32_t i = 0; i < 100000; i++) {
        if (!(inb(PS2_STATUS) & STATUS_INPUT_FULL))
            return true;
    }

    return false;
}

void gmu_ps2_init(void) {
    keyboard_head = keyboard_tail = 0;
    mouse_head = mouse_tail = 0;
}

void gmu_ps2_flush_mouse(void) {
    mouse_head = mouse_tail = 0;
}

void gmu_ps2_flush_keyboard(void) {
    keyboard_head = keyboard_tail = 0;
}

bool gmu_ps2_read_keyboard_byte(uint8_t *data) {
    if (!data)
        return false;

    pump();

    return queue_pop(keyboard_queue, &keyboard_head,
                     &keyboard_tail, data);
}

bool gmu_ps2_read_mouse_byte(uint8_t *data) {
    if (!data)
        return false;

    pump();

    return queue_pop(mouse_queue, &mouse_head, &mouse_tail, data);
}

bool gmu_ps2_mouse_command(uint8_t command) {
    uint8_t response;

    if (!wait_input_clear())
        return false;

    outb(PS2_COMMAND, 0xD4);

    if (!wait_input_clear())
        return false;

    outb(PS2_DATA, command);

    for (uint32_t i = 0; i < 100000; i++) {
        pump();

        if (queue_pop(mouse_queue, &mouse_head,
                      &mouse_tail, &response)) {
            if (response == 0xFA)
                return true;

            /* Ignore queued movement data while waiting for ACK. */
        }
    }

    return false;
}
