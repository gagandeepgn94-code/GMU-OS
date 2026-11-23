#ifndef GMU_FONT_H
#define GMU_FONT_H

#include <stdint.h>
#include <stdbool.h>
#include <gmu/framebuffer.h>

#define GMU_FONT_WIDTH  6
#define GMU_FONT_HEIGHT 8

void gmu_draw_char(
    gmu_framebuffer_t *fb,
    uint64_t x,
    uint64_t y,
    char c,
    uint32_t fg,
    uint32_t bg
);

void gmu_draw_text(
    gmu_framebuffer_t *fb,
    uint64_t x,
    uint64_t y,
    const char *text,
    uint32_t fg,
    uint32_t bg
);

void gmu_draw_text_transparent(
    gmu_framebuffer_t *fb,
    uint64_t x,
    uint64_t y,
    const char *text,
    uint32_t fg
);

void gmu_draw_text_scaled(
    gmu_framebuffer_t *fb,
    uint64_t x,
    uint64_t y,
    const char *text,
    uint32_t fg,
    uint32_t bg,
    uint32_t scale,
    bool transparent
);

#endif
