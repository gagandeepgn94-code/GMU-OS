#include <gmu/draw.h>
#include <gmu/font.h>

uint32_t gmu_blend_color(uint32_t c1, uint32_t c2, uint8_t alpha) {
    uint32_t inv = 255 - alpha;
    uint32_t r = (((c1 >> 16) & 0xFF) * inv + ((c2 >> 16) & 0xFF) * alpha) / 255;
    uint32_t g = (((c1 >> 8) & 0xFF) * inv + ((c2 >> 8) & 0xFF) * alpha) / 255;
    uint32_t b = ((c1 & 0xFF) * inv + (c2 & 0xFF) * alpha) / 255;
    return (r << 16) | (g << 8) | b;
}

void gmu_draw_pixel(gmu_framebuffer_t *fb,
                    uint64_t x, uint64_t y, uint32_t color) {
    gmu_fb_put_pixel(fb, x, y, color);
}

void gmu_fill_rect(gmu_framebuffer_t *fb,
                   uint64_t x, uint64_t y,
                   uint64_t width, uint64_t height,
                   uint32_t color) {
    if (!fb || !fb->initialized || x >= fb->width || y >= fb->height)
        return;

    uint64_t max_x = width > fb->width - x ? fb->width : x + width;
    uint64_t max_y = height > fb->height - y ? fb->height : y + height;

    for (uint64_t py = y; py < max_y; py++) {
        for (uint64_t px = x; px < max_x; px++) {
            gmu_fb_put_pixel(fb, px, py, color);
        }
    }
}

void gmu_draw_rect(gmu_framebuffer_t *fb,
                   uint64_t x, uint64_t y,
                   uint64_t width, uint64_t height,
                   uint32_t color) {
    if (!width || !height)
        return;

    gmu_fill_rect(fb, x, y, width, 1, color);
    if (height > 1)
        gmu_fill_rect(fb, x, y + height - 1, width, 1, color);

    gmu_fill_rect(fb, x, y, 1, height, color);
    if (width > 1)
        gmu_fill_rect(fb, x + width - 1, y, 1, height, color);
}

void gmu_draw_gradient_v(gmu_framebuffer_t *fb,
                         uint64_t x, uint64_t y,
                         uint64_t width, uint64_t height,
                         uint32_t col_top, uint32_t col_bottom) {
    if (!fb || !fb->initialized || !width || !height) return;

    for (uint64_t row = 0; row < height; row++) {
        uint8_t factor = (uint8_t)((row * 255) / height);
        uint32_t c = gmu_blend_color(col_top, col_bottom, factor);
        gmu_fill_rect(fb, x, y + row, width, 1, c);
    }
}

void gmu_draw_gradient_h(gmu_framebuffer_t *fb,
                         uint64_t x, uint64_t y,
                         uint64_t width, uint64_t height,
                         uint32_t col_left, uint32_t col_right) {
    if (!fb || !fb->initialized || !width || !height) return;

    for (uint64_t col = 0; col < width; col++) {
        uint8_t factor = (uint8_t)((col * 255) / width);
        uint32_t c = gmu_blend_color(col_left, col_right, factor);
        gmu_fill_rect(fb, x + col, y, 1, height, c);
    }
}

void gmu_fill_circle(gmu_framebuffer_t *fb,
                     int64_t cx, int64_t cy,
                     int64_t radius, uint32_t color) {
    if (!fb || radius <= 0) return;

    for (int64_t dy = -radius; dy <= radius; dy++) {
        for (int64_t dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                int64_t px = cx + dx;
                int64_t py = cy + dy;
                if (px >= 0 && py >= 0 &&
                    (uint64_t)px < fb->width && (uint64_t)py < fb->height) {
                    gmu_fb_put_pixel(fb, (uint64_t)px, (uint64_t)py, color);
                }
            }
        }
    }
}

void gmu_draw_circle(gmu_framebuffer_t *fb,
                     int64_t cx, int64_t cy,
                     int64_t radius, uint32_t color) {
    if (!fb || radius <= 0) return;

    int64_t x = 0;
    int64_t y = radius;
    int64_t d = 3 - 2 * radius;

    while (y >= x) {
        int64_t pts[8][2] = {
            {cx + x, cy + y}, {cx - x, cy + y},
            {cx + x, cy - y}, {cx - x, cy - y},
            {cx + y, cy + x}, {cx - y, cy + x},
            {cx + y, cy - x}, {cx - y, cy - x}
        };

        for (int i = 0; i < 8; i++) {
            if (pts[i][0] >= 0 && pts[i][1] >= 0 &&
                (uint64_t)pts[i][0] < fb->width &&
                (uint64_t)pts[i][1] < fb->height) {
                gmu_fb_put_pixel(fb, (uint64_t)pts[i][0], (uint64_t)pts[i][1], color);
            }
        }

        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void gmu_fill_rounded_rect(gmu_framebuffer_t *fb,
                           uint64_t x, uint64_t y,
                           uint64_t width, uint64_t height,
                           uint64_t radius, uint32_t color) {
    if (!width || !height) return;
    if (radius > width / 2) radius = width / 2;
    if (radius > height / 2) radius = height / 2;

    /* Center rect */
    gmu_fill_rect(fb, x + radius, y, width - 2 * radius, height, color);
    /* Left and right side bands */
    gmu_fill_rect(fb, x, y + radius, radius, height - 2 * radius, color);
    gmu_fill_rect(fb, x + width - radius, y + radius, radius, height - 2 * radius, color);

    /* 4 corner circles */
    gmu_fill_circle(fb, x + radius, y + radius, radius, color);
    gmu_fill_circle(fb, x + width - radius - 1, y + radius, radius, color);
    gmu_fill_circle(fb, x + radius, y + height - radius - 1, radius, color);
    gmu_fill_circle(fb, x + width - radius - 1, y + height - radius - 1, radius, color);
}

void gmu_draw_rounded_rect(gmu_framebuffer_t *fb,
                           uint64_t x, uint64_t y,
                           uint64_t width, uint64_t height,
                           uint64_t radius, uint32_t color) {
    if (!width || !height) return;
    if (radius > width / 2) radius = width / 2;
    if (radius > height / 2) radius = height / 2;

    /* Top and bottom edges */
    gmu_fill_rect(fb, x + radius, y, width - 2 * radius, 1, color);
    gmu_fill_rect(fb, x + radius, y + height - 1, width - 2 * radius, 1, color);

    /* Left and right edges */
    gmu_fill_rect(fb, x, y + radius, 1, height - 2 * radius, color);
    gmu_fill_rect(fb, x + width - 1, y + radius, 1, height - 2 * radius, color);

    /* Corner arcs */
    gmu_draw_circle(fb, x + radius, y + radius, radius, color);
    gmu_draw_circle(fb, x + width - radius - 1, y + radius, radius, color);
    gmu_draw_circle(fb, x + radius, y + height - radius - 1, radius, color);
    gmu_draw_circle(fb, x + width - radius - 1, y + height - radius - 1, radius, color);
}

void gmu_draw_shadow(gmu_framebuffer_t *fb,
                     uint64_t x, uint64_t y,
                     uint64_t width, uint64_t height,
                     uint64_t depth) {
    if (!fb || depth == 0) return;

    for (uint64_t d = 1; d <= depth; d++) {
        uint8_t alpha = (uint8_t)(80 - (d * 70) / depth);
        /* Right shadow */
        for (uint64_t sy = y + d + 4; sy < y + height + d; sy++) {
            uint64_t sx = x + width + d - 1;
            if (sx < fb->width && sy < fb->height) {
                uint32_t bg = gmu_fb_get_pixel(fb, sx, sy);
                gmu_fb_put_pixel(fb, sx, sy, gmu_blend_color(bg, 0x00000000, alpha));
            }
        }
        /* Bottom shadow */
        for (uint64_t sx = x + d + 4; sx < x + width + d; sx++) {
            uint64_t sy = y + height + d - 1;
            if (sx < fb->width && sy < fb->height) {
                uint32_t bg = gmu_fb_get_pixel(fb, sx, sy);
                gmu_fb_put_pixel(fb, sx, sy, gmu_blend_color(bg, 0x00000000, alpha));
            }
        }
    }
}

void gmu_draw_window_frame(gmu_framebuffer_t *fb,
                           uint64_t x, uint64_t y,
                           uint64_t width, uint64_t height,
                           const char *title,
                           uint32_t header_bg,
                           uint32_t body_bg,
                           uint32_t border_col) {
    if (!fb || width < 120 || height < 60) return;

    /* Drop shadow */
    gmu_draw_shadow(fb, x, y, width, height, 6);

    /* Window body */
    gmu_fill_rect(fb, x, y + 32, width, height - 32, body_bg);

    /* Window titlebar */
    gmu_fill_rect(fb, x, y, width, 32, header_bg);
    gmu_fill_rect(fb, x, y + 31, width, 1, border_col);

    /* Outer border */
    gmu_draw_rect(fb, x, y, width, height, border_col);

    /* Chiclets: Red, Yellow, Green */
    gmu_fill_circle(fb, x + 16, y + 16, 5, 0x00EF4444); /* Close */
    gmu_fill_circle(fb, x + 32, y + 16, 5, 0x00F59E0B); /* Minimize */
    gmu_fill_circle(fb, x + 48, y + 16, 5, 0x0010B981); /* Maximize */

    /* Window title centered */
    if (title) {
        uint64_t len = 0;
        const char *t = title;
        while (*t++) len++;
        uint64_t title_w = len * GMU_FONT_WIDTH;
        uint64_t title_x = (title_w < width) ? x + (width - title_w) / 2 : x + 64;
        gmu_draw_text(fb, title_x, y + 12, title, GMU_WHITE, header_bg);
    }
}
