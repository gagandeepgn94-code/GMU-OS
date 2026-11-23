#include <gmu/framebuffer.h>

bool gmu_fb_init(
    gmu_framebuffer_t *fb,
    void *address,
    uint64_t width,
    uint64_t height,
    uint64_t pitch,
    uint16_t bpp
) {
    if (!fb || !address || !width || !height || bpp != 32)
        return false;

    if (pitch < width * 4)
        return false;

    fb->address = (volatile uint32_t *)address;
    fb->backbuffer = 0;
    fb->width = width;
    fb->height = height;
    fb->pitch = pitch;
    fb->bpp = bpp;
    fb->initialized = true;
    fb->double_buffered = false;

    return true;
}

void gmu_fb_set_backbuffer(gmu_framebuffer_t *fb, uint32_t *buffer) {
    if (!fb) return;
    fb->backbuffer = buffer;
    fb->double_buffered = (buffer != 0);
}

void gmu_fb_present(gmu_framebuffer_t *fb) {
    if (!fb || !fb->initialized || !fb->double_buffered || !fb->backbuffer)
        return;

    uint64_t pixels_per_row = fb->pitch / 4;
    for (uint64_t y = 0; y < fb->height; y++) {
        volatile uint32_t *vram_row = (volatile uint32_t *)((uintptr_t)fb->address + y * fb->pitch);
        uint32_t *buf_row = &fb->backbuffer[y * pixels_per_row];
        for (uint64_t x = 0; x < fb->width; x++) {
            vram_row[x] = buf_row[x];
        }
    }
}

void gmu_fb_put_pixel(
    gmu_framebuffer_t *fb,
    uint64_t x,
    uint64_t y,
    uint32_t color
) {
    if (!fb || !fb->initialized || x >= fb->width || y >= fb->height)
        return;

    if (fb->double_buffered && fb->backbuffer) {
        uint64_t pixels_per_row = fb->pitch / 4;
        fb->backbuffer[y * pixels_per_row + x] = color;
    } else {
        volatile uint32_t *row =
            (volatile uint32_t *)(
                (uintptr_t)fb->address + y * fb->pitch
            );
        row[x] = color;
    }
}

uint32_t gmu_fb_get_pixel(
    gmu_framebuffer_t *fb,
    uint64_t x,
    uint64_t y
) {
    if (!fb || !fb->initialized || x >= fb->width || y >= fb->height)
        return 0;

    if (fb->double_buffered && fb->backbuffer) {
        uint64_t pixels_per_row = fb->pitch / 4;
        return fb->backbuffer[y * pixels_per_row + x];
    } else {
        volatile uint32_t *row =
            (volatile uint32_t *)(
                (uintptr_t)fb->address + y * fb->pitch
            );
        return row[x];
    }
}

void gmu_fb_clear(gmu_framebuffer_t *fb, uint32_t color) {
    if (!fb || !fb->initialized)
        return;

    uint64_t pixels_per_row = fb->pitch / 4;

    if (fb->double_buffered && fb->backbuffer) {
        for (uint64_t y = 0; y < fb->height; y++) {
            uint32_t *buf_row = &fb->backbuffer[y * pixels_per_row];
            for (uint64_t x = 0; x < fb->width; x++) {
                buf_row[x] = color;
            }
        }
    } else {
        for (uint64_t y = 0; y < fb->height; y++) {
            volatile uint32_t *row =
                (volatile uint32_t *)(
                    (uintptr_t)fb->address + y * fb->pitch
                );
            for (uint64_t x = 0; x < fb->width; x++) {
                row[x] = color;
            }
        }
    }
}
