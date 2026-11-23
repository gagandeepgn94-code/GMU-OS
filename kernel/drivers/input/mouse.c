#include <stdint.h>
#include <stdbool.h>
#include <gmu/mouse.h>
#include <gmu/ps2.h>

#define PS2_COMMAND 0x64
#define PS2_STATUS  0x64
#define STATUS_INPUT_FULL 0x02

static uint8_t packet[3];
static uint8_t packet_index = 0;

static int32_t max_x;
static int32_t max_y;

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static bool wait_input_clear(void) {
    for (uint32_t i = 0; i < 100000; i++) {
        if (!(inb(PS2_STATUS) & STATUS_INPUT_FULL))
            return true;
    }

    return false;
}

bool gmu_mouse_init(gmu_mouse_t *mouse,
                    uint64_t screen_width,
                    uint64_t screen_height) {
    if (!mouse || !screen_width || !screen_height)
        return false;

    mouse->x = (int32_t)(screen_width / 2);
    mouse->y = (int32_t)(screen_height / 2);
    mouse->buttons = 0;
    mouse->initialized = false;

    max_x = (int32_t)screen_width - 1;
    max_y = (int32_t)screen_height - 1;
    packet_index = 0;

    /* Enable the PS/2 auxiliary mouse port. */
    if (!wait_input_clear())
        return false;

    outb(PS2_COMMAND, 0xA8);

    /* Enable aux device clock in PS/2 controller config byte */
    if (wait_input_clear()) {
        outb(PS2_COMMAND, 0x20);
        for (uint32_t i = 0; i < 10000; i++) {
            if (inb(PS2_STATUS) & 0x01) {
                uint8_t cfg = inb(0x60);
                cfg &= ~0x20; /* Clear bit 5: enable aux clock */
                cfg |= 0x02;  /* Enable aux interrupt */
                if (wait_input_clear()) {
                    outb(PS2_COMMAND, 0x60);
                    if (wait_input_clear()) {
                        outb(0x60, cfg);
                    }
                }
                break;
            }
        }
    }

    /* Restore default mouse settings. */
    if (!gmu_ps2_mouse_command(0xF6))
        return false;

    /* Set sample rate to 200 Hz for maximum smoothness */
    if (gmu_ps2_mouse_command(0xF3)) {
        (void)gmu_ps2_mouse_command(200);
    }

    /* Set resolution to 8 counts/mm for high precision */
    if (gmu_ps2_mouse_command(0xE8)) {
        (void)gmu_ps2_mouse_command(3);
    }

    /* Enable mouse movement reporting. */
    if (!gmu_ps2_mouse_command(0xF4))
        return false;

    gmu_ps2_flush_mouse();
    packet_index = 0;

    mouse->initialized = true;
    return true;
}

static int32_t apply_ballistics(int32_t delta) {
    if (delta == 0) return 0;
    bool neg = (delta < 0);
    int32_t mag = neg ? -delta : delta;

    if (mag <= 2) {
        /* Pixel-perfect micro targeting */
    } else if (mag <= 6) {
        mag = (mag * 3) / 2;
    } else {
        mag = mag * 2;
        if (mag > 42) mag = 42;
    }
    return neg ? -mag : mag;
}

bool gmu_mouse_poll(gmu_mouse_t *mouse) {
    if (!mouse || !mouse->initialized)
        return false;

    bool updated = false;
    uint8_t data;

    while (gmu_ps2_read_mouse_byte(&data)) {
        if (packet_index == 0) {
            /* Bit 3 of byte 0 in standard PS/2 mouse packet must always be 1.
             * If not, we have lost byte synchronization. Discard and retry. */
            if (!(data & 0x08)) {
                continue;
            }
        }

        packet[packet_index++] = data;

        if (packet_index == 3) {
            packet_index = 0;

            /* Ignore packets with X or Y hardware overflow. */
            if (packet[0] & 0xC0) {
                continue;
            }

            /* Decode 9-bit signed X delta using sign bit (packet[0] bit 4) */
            int32_t dx = (int32_t)packet[1];
            if (packet[0] & 0x10) {
                dx |= ~((int32_t)0xFF);
            }

            /* Decode 9-bit signed Y delta using sign bit (packet[0] bit 5) */
            int32_t dy = (int32_t)packet[2];
            if (packet[0] & 0x20) {
                dy |= ~((int32_t)0xFF);
            }

            /* Sanity clamping to prevent spurious glitches */
            if (dx > 80) dx = 80;
            if (dx < -80) dx = -80;
            if (dy > 80) dy = 80;
            if (dy < -80) dy = -80;

            dx = apply_ballistics(dx);
            dy = apply_ballistics(dy);

            mouse->x += dx;
            mouse->y -= dy; /* PS/2 Y delta is positive upward; screen Y is downward */

            if (mouse->x < 0) mouse->x = 0;
            if (mouse->y < 0) mouse->y = 0;
            if (mouse->x > max_x) mouse->x = max_x;
            if (mouse->y > max_y) mouse->y = max_y;

            mouse->buttons = packet[0] & 0x07;
            updated = true;
        }
    }

    return updated;
}
