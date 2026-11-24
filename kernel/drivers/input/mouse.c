#include <stdint.h>
#include <stdbool.h>
#include <gmu/mouse.h>
#include <gmu/ps2.h>
#include <gmu/framebuffer.h>

#define PS2_COMMAND 0x64
#define PS2_STATUS  0x64
#define STATUS_INPUT_FULL 0x02

/* Global persistent mouse state */
static gmu_mouse_t global_mouse = {
    .x = 0,
    .y = 0,
    .buttons = 0,
    .initialized = false
};

static bool hardware_initialized = false;
static uint8_t packet[3];
static uint8_t packet_index = 0;

static int32_t max_x = 1023;
static int32_t max_y = 767;

/* Universal Cursor Bitmap (16x16 Modern macOS / Windows 11 Arrow) */
static const char *cursor_bitmap[GMU_CURSOR_H] = {
    "B...............",
    "BB..............",
    "BWB.............",
    "BWWWB...........",
    "BWWWWWB.........",
    "BWWWWWWWB.......",
    "BWWWWWWWWWB.....",
    "BWWWWWWWWWWWB...",
    "BWWWWWWWWWWWWWB.",
    "BWWWWWWWWWWBB...",
    "BWWWWWWBBBBB....",
    "BWWWBWWWB.......",
    "BWWB.BWWWB......",
    "BBB...BWWWB.....",
    "B......BWWWB....",
    "........BBB....."
};

static uint32_t cursor_background[GMU_CURSOR_H][GMU_CURSOR_W];
static int32_t cursor_saved_x = 0;
static int32_t cursor_saved_y = 0;
static bool cursor_visible = false;

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

void gmu_mouse_set_bounds(uint64_t screen_width, uint64_t screen_height) {
    if (screen_width > 0)  max_x = (int32_t)screen_width - 1;
    if (screen_height > 0) max_y = (int32_t)screen_height - 1;

    if (global_mouse.x > max_x) global_mouse.x = max_x;
    if (global_mouse.y > max_y) global_mouse.y = max_y;
}

void gmu_mouse_get_coords(int32_t *x, int32_t *y) {
    if (x) *x = global_mouse.x;
    if (y) *y = global_mouse.y;
}

void gmu_mouse_set_coords(int32_t x, int32_t y) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > max_x) x = max_x;
    if (y > max_y) y = max_y;
    global_mouse.x = x;
    global_mouse.y = y;
}

uint8_t gmu_mouse_get_buttons(void) {
    return global_mouse.buttons;
}

bool gmu_mouse_init(gmu_mouse_t *mouse,
                    uint64_t screen_width,
                    uint64_t screen_height) {
    if (!screen_width || !screen_height)
        return false;

    gmu_mouse_set_bounds(screen_width, screen_height);

    /* If hardware is already initialized, do NOT re-send disruptive PS/2 reset commands */
    if (hardware_initialized) {
        if (mouse) {
            mouse->x = global_mouse.x;
            mouse->y = global_mouse.y;
            mouse->buttons = global_mouse.buttons;
            mouse->initialized = true;
        }
        return true;
    }

    global_mouse.x = (int32_t)(screen_width / 2);
    global_mouse.y = (int32_t)(screen_height / 2);
    global_mouse.buttons = 0;
    global_mouse.initialized = false;
    packet_index = 0;

    /* 1. Enable PS/2 aux mouse port */
    if (!wait_input_clear())
        return false;
    outb(PS2_COMMAND, 0xA8);

    /* 2. Configure controller command byte: enable aux clock & IRQ */
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

    /* 3. Reset to default mouse settings */
    (void)gmu_ps2_mouse_command(0xF6);

    /* 4. Set sample rate to 200 Hz for silky smooth tracking */
    if (gmu_ps2_mouse_command(0xF3)) {
        (void)gmu_ps2_mouse_command(200);
    }

    /* 5. Set resolution to 8 counts/mm for high precision */
    if (gmu_ps2_mouse_command(0xE8)) {
        (void)gmu_ps2_mouse_command(3);
    }

    /* 6. Enable data reporting */
    (void)gmu_ps2_mouse_command(0xF4);

    gmu_ps2_flush_mouse();
    packet_index = 0;

    hardware_initialized = true;
    global_mouse.initialized = true;

    if (mouse) {
        mouse->x = global_mouse.x;
        mouse->y = global_mouse.y;
        mouse->buttons = 0;
        mouse->initialized = true;
    }

    return true;
}

static int32_t apply_ballistics(int32_t delta) {
    if (delta == 0) return 0;
    bool neg = (delta < 0);
    int32_t mag = neg ? -delta : delta;

    if (mag <= 2) {
        /* 1:1 pixel precision */
    } else if (mag <= 8) {
        mag = (mag * 3) / 2;
    } else if (mag <= 20) {
        mag = mag * 2;
    } else {
        /* Rapid movement: accelerated sweep across whole screen */
        mag = (mag * 5) / 2;
    }

    return neg ? -mag : mag;
}

bool gmu_mouse_poll(gmu_mouse_t *mouse) {
    if (!hardware_initialized)
        return false;

    bool updated = false;
    uint8_t data;

    while (gmu_ps2_read_mouse_byte(&data)) {
        if (packet_index == 0) {
            /* Bit 3 of byte 0 in standard PS/2 mouse packet must always be 1.
             * If not, byte synchronization has slipped. Discard until sync found. */
            if (!(data & 0x08)) {
                continue;
            }
        }

        packet[packet_index++] = data;

        if (packet_index == 3) {
            packet_index = 0;

            /* Verify byte 0 sync bit */
            if (!(packet[0] & 0x08)) {
                continue;
            }

            /* Decode 9-bit signed X delta using sign bit (packet[0] bit 4) */
            int32_t dx = (int32_t)packet[1];
            if (packet[0] & 0x10) {
                dx -= 256;
            }

            /* Decode 9-bit signed Y delta using sign bit (packet[0] bit 5) */
            int32_t dy = (int32_t)packet[2];
            if (packet[0] & 0x20) {
                dy -= 256;
            }

            /* Handle hardware overflow without discarding packet */
            if (packet[0] & 0x40) {
                dx = (packet[0] & 0x10) ? -127 : 127;
            }
            if (packet[0] & 0x80) {
                dy = (packet[0] & 0x20) ? -127 : 127;
            }

            /* Bounds clamp on individual packet delta */
            if (dx > 120) dx = 120;
            if (dx < -120) dx = -120;
            if (dy > 120) dy = 120;
            if (dy < -120) dy = -120;

            dx = apply_ballistics(dx);
            dy = apply_ballistics(dy);

            global_mouse.x += dx;
            global_mouse.y -= dy; /* PS/2 Y delta is positive upward; screen Y is downward */

            /* Clamp to full screen boundaries */
            if (global_mouse.x < 0) global_mouse.x = 0;
            if (global_mouse.y < 0) global_mouse.y = 0;
            if (global_mouse.x > max_x) global_mouse.x = max_x;
            if (global_mouse.y > max_y) global_mouse.y = max_y;

            global_mouse.buttons = packet[0] & 0x07;
            updated = true;
        }
    }

    if (mouse) {
        mouse->x = global_mouse.x;
        mouse->y = global_mouse.y;
        mouse->buttons = global_mouse.buttons;
        mouse->initialized = true;
    }

    return updated;
}

/* ========================================================================= */
/* Universal Cursor Rendering & Double-buffering Implementation              */
/* ========================================================================= */

void gmu_mouse_cursor_show(gmu_framebuffer_t *fb, int32_t x, int32_t y) {
    if (!fb || !fb->initialized) return;

    if (cursor_visible) {
        gmu_mouse_cursor_hide(fb);
    }

    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= (int32_t)fb->width) x = (int32_t)fb->width - 1;
    if (y >= (int32_t)fb->height) y = (int32_t)fb->height - 1;

    cursor_saved_x = x;
    cursor_saved_y = y;

    /* 1. Save original background pixels under cursor */
    for (int32_t cy = 0; cy < GMU_CURSOR_H; cy++) {
        for (int32_t cx = 0; cx < GMU_CURSOR_W; cx++) {
            int32_t sx = x + cx;
            int32_t sy = y + cy;

            cursor_background[cy][cx] = 0;
            if (sx < (int32_t)fb->width && sy < (int32_t)fb->height) {
                cursor_background[cy][cx] = gmu_fb_get_pixel(fb, (uint64_t)sx, (uint64_t)sy);
            }
        }
    }

    /* 2. Render crisp 16x16 pointer bitmap */
    for (int32_t cy = 0; cy < GMU_CURSOR_H; cy++) {
        for (int32_t cx = 0; cx < GMU_CURSOR_W; cx++) {
            char p = cursor_bitmap[cy][cx];
            if (p == '.') continue;

            int32_t px = x + cx;
            int32_t py = y + cy;

            if (px >= 0 && py >= 0 &&
                px < (int32_t)fb->width &&
                py < (int32_t)fb->height) {
                uint32_t col = (p == 'W') ? 0x00FFFFFF : 0x000F172A;
                gmu_fb_put_pixel(fb, (uint64_t)px, (uint64_t)py, col);
            }
        }
    }

    cursor_visible = true;
}

void gmu_mouse_cursor_hide(gmu_framebuffer_t *fb) {
    if (!cursor_visible || !fb || !fb->initialized) return;

    for (int32_t cy = 0; cy < GMU_CURSOR_H; cy++) {
        for (int32_t cx = 0; cx < GMU_CURSOR_W; cx++) {
            int32_t px = cursor_saved_x + cx;
            int32_t py = cursor_saved_y + cy;

            if (px >= 0 && py >= 0 &&
                px < (int32_t)fb->width &&
                py < (int32_t)fb->height) {
                gmu_fb_put_pixel(fb, (uint64_t)px, (uint64_t)py, cursor_background[cy][cx]);
            }
        }
    }

    cursor_visible = false;
}

void gmu_mouse_cursor_update(gmu_framebuffer_t *fb, int32_t x, int32_t y) {
    if (!cursor_visible) {
        gmu_mouse_cursor_show(fb, x, y);
        return;
    }

    if (x != cursor_saved_x || y != cursor_saved_y) {
        gmu_mouse_cursor_hide(fb);
        gmu_mouse_cursor_show(fb, x, y);
    }
}

bool gmu_mouse_cursor_is_visible(void) {
    return cursor_visible;
}
