#ifndef GMU_DRAW_H
#define GMU_DRAW_H

#include <stdint.h>
#include <stdbool.h>
#include <gmu/framebuffer.h>

#define GMU_BLACK       0x00000000
#define GMU_WHITE       0x00FFFFFF
#define GMU_BACKGROUND  0x00101827
#define GMU_PANEL       0x001B263B
#define GMU_BLUE        0x003B82F6
#define GMU_CYAN        0x0006B6D4
#define GMU_PURPLE      0x008B5CF6
#define GMU_GREEN       0x0022C55E
#define GMU_RED         0x00EF4444
#define GMU_YELLOW      0x00FBBF24
#define GMU_MUTED       0x0094A3B8

uint32_t gmu_blend_color(uint32_t c1, uint32_t c2, uint8_t alpha);

void gmu_draw_pixel(gmu_framebuffer_t *fb,
                    uint64_t x, uint64_t y, uint32_t color);

void gmu_draw_rect(gmu_framebuffer_t *fb,
                   uint64_t x, uint64_t y,
                   uint64_t width, uint64_t height,
                   uint32_t color);

void gmu_fill_rect(gmu_framebuffer_t *fb,
                   uint64_t x, uint64_t y,
                   uint64_t width, uint64_t height,
                   uint32_t color);

void gmu_draw_gradient_v(gmu_framebuffer_t *fb,
                         uint64_t x, uint64_t y,
                         uint64_t width, uint64_t height,
                         uint32_t col_top, uint32_t col_bottom);

void gmu_draw_gradient_h(gmu_framebuffer_t *fb,
                         uint64_t x, uint64_t y,
                         uint64_t width, uint64_t height,
                         uint32_t col_left, uint32_t col_right);

void gmu_fill_circle(gmu_framebuffer_t *fb,
                     int64_t cx, int64_t cy,
                     int64_t radius, uint32_t color);

void gmu_draw_circle(gmu_framebuffer_t *fb,
                     int64_t cx, int64_t cy,
                     int64_t radius, uint32_t color);

void gmu_fill_rounded_rect(gmu_framebuffer_t *fb,
                           uint64_t x, uint64_t y,
                           uint64_t width, uint64_t height,
                           uint64_t radius, uint32_t color);

void gmu_draw_rounded_rect(gmu_framebuffer_t *fb,
                           uint64_t x, uint64_t y,
                           uint64_t width, uint64_t height,
                           uint64_t radius, uint32_t color);

void gmu_draw_shadow(gmu_framebuffer_t *fb,
                     uint64_t x, uint64_t y,
                     uint64_t width, uint64_t height,
                     uint64_t depth);

void gmu_draw_window_frame(gmu_framebuffer_t *fb,
                           uint64_t x, uint64_t y,
                           uint64_t width, uint64_t height,
                           const char *title,
                           uint32_t header_bg,
                           uint32_t body_bg,
                           uint32_t border_col);

#endif
