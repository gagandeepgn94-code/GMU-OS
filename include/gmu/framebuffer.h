#ifndef GMU_FRAMEBUFFER_H
#define GMU_FRAMEBUFFER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    volatile uint32_t *address;
    uint32_t *backbuffer;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint16_t bpp;
    bool initialized;
    bool double_buffered;
} gmu_framebuffer_t;

bool gmu_fb_init(
    gmu_framebuffer_t *fb,
    void *address,
    uint64_t width,
    uint64_t height,
    uint64_t pitch,
    uint16_t bpp
);

void gmu_fb_set_backbuffer(gmu_framebuffer_t *fb, uint32_t *buffer);
void gmu_fb_present(gmu_framebuffer_t *fb);

void gmu_fb_put_pixel(
    gmu_framebuffer_t *fb,
    uint64_t x,
    uint64_t y,
    uint32_t color
);

uint32_t gmu_fb_get_pixel(
    gmu_framebuffer_t *fb,
    uint64_t x,
    uint64_t y
);

void gmu_fb_clear(gmu_framebuffer_t *fb, uint32_t color);

#endif
