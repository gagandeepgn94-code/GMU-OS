#include <stdint.h>
#include <stdbool.h>
#include <gmu/paint.h>
#include <gmu/app.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/draw.h>
#include <gmu/font.h>
#include <gmu/theme.h>

#define PALETTE_SIZE 10

static const uint32_t palette_colors[PALETTE_SIZE] = {
    0x00000000, /* 0: Black */
    0x00FFFFFF, /* 1: White */
    0x00EF4444, /* 2: Red */
    0x0010B981, /* 3: Emerald */
    0x003B82F6, /* 4: Blue */
    0x00FBBF24, /* 5: Amber */
    0x0006B6D4, /* 6: Cyan */
    0x00A855F7, /* 7: Purple */
    0x00F97316, /* 8: Orange */
    0x0064748B  /* 9: Slate Gray */
};

static uint32_t current_color_idx = 4; /* Default: Blue */
static uint32_t brush_size = 2; /* 1, 2, 4 */

static void draw_paint_ui(gmu_framebuffer_t *fb, uint64_t cx, uint64_t cy,
                          uint64_t cw, uint64_t ch, uint64_t wx, uint64_t wy,
                          uint64_t ww, uint64_t wh) {
    const gmu_theme_t *th = gmu_theme_get();

    /* Window Frame */
    gmu_draw_window_frame(fb, wx, wy, ww, wh,
                          "GMU PAINT - PIXEL ART & DRAWING STUDIO",
                          th->surface_accent, th->surface_dark, th->border);

    /* Left Toolbar (Palette & Controls) */
    uint64_t tool_x = wx + 12;
    uint64_t tool_y = wy + 42;
    uint64_t tool_w = 90;
    uint64_t tool_h = wh - 54;

    gmu_fill_rect(fb, tool_x, tool_y, tool_w, tool_h, th->surface);
    gmu_draw_rect(fb, tool_x, tool_y, tool_w, tool_h, th->border);

    gmu_draw_text(fb, tool_x + 8, tool_y + 10, "COLORS [0-9]", th->accent, th->surface);

    for (int i = 0; i < PALETTE_SIZE; i++) {
        uint64_t sy = tool_y + 28 + i * 22;
        bool is_sel = ((uint32_t)i == current_color_idx);

        gmu_fill_rect(fb, tool_x + 10, sy, 24, 16, palette_colors[i]);
        gmu_draw_rect(fb, tool_x + 10, sy, 24, 16, is_sel ? GMU_WHITE : th->border);

        char num_label[4] = {'0' + i, '\0'};
        gmu_draw_text(fb, tool_x + 42, sy + 4, num_label, is_sel ? GMU_WHITE : th->text_secondary, th->surface);

        if (is_sel) {
            gmu_draw_text(fb, tool_x + 60, sy + 4, "<", th->chiclet_green, th->surface);
        }
    }

    /* Brush Size & Tool Info */
    uint64_t brush_y = tool_y + 28 + PALETTE_SIZE * 22 + 10;
    gmu_fill_rect(fb, tool_x + 8, brush_y, tool_w - 16, 1, th->border);

    gmu_draw_text(fb, tool_x + 8, brush_y + 8, "BRUSH: +/-", th->text_secondary, th->surface);
    char bstr[12] = "Size: ";
    bstr[6] = '0' + brush_size;
    bstr[7] = 'p';
    bstr[8] = 'x';
    bstr[9] = '\0';
    gmu_draw_text(fb, tool_x + 8, brush_y + 24, bstr, th->chiclet_yellow, th->surface);

    gmu_draw_text(fb, tool_x + 8, brush_y + 46, "[C] CLEAR", th->chiclet_red, th->surface);
    gmu_draw_text(fb, tool_x + 8, brush_y + 64, "[ESC] EXIT", th->text_secondary, th->surface);

    /* Draw Canvas Border */
    gmu_draw_rect(fb, cx - 1, cy - 1, cw + 2, ch + 2, th->accent);
}

void gmu_paint_run(gmu_framebuffer_t *fb) {
    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);
    uint8_t prev_buttons = 0;

    uint64_t ww = fb->width > 860 ? 820 : fb->width - 40;
    uint64_t wh = fb->height > 600 ? 540 : fb->height - 50;
    uint64_t wx = (fb->width - ww) / 2;
    uint64_t wy = (fb->height - wh) / 2 - 10;

    uint64_t cw = ww - 140;
    uint64_t ch = wh - 64;
    uint64_t cx = wx + 120;
    uint64_t cy = wy + 42;

    /* Initialize blank white canvas */
    gmu_fill_rect(fb, cx, cy, cw, ch, GMU_WHITE);
    draw_paint_ui(fb, cx, cy, cw, ch, wx, wy, ww, wh);

    int32_t pen_x = cx + cw / 2;
    int32_t pen_y = cy + ch / 2;

    for (;;) {
        uint16_t key = gmu_keyboard_read_key();
        bool drawing = false;

        if (key == GMU_KEY_ESCAPE) {
            break;
        } else if (key >= '0' && key <= '9') {
            current_color_idx = key - '0';
            draw_paint_ui(fb, cx, cy, cw, ch, wx, wy, ww, wh);
        } else if (key == '+' || key == '=') {
            if (brush_size < 8) brush_size++;
            draw_paint_ui(fb, cx, cy, cw, ch, wx, wy, ww, wh);
        } else if (key == '-' || key == '_') {
            if (brush_size > 1) brush_size--;
            draw_paint_ui(fb, cx, cy, cw, ch, wx, wy, ww, wh);
        } else if (key == 'c' || key == 'C') {
            gmu_fill_rect(fb, cx, cy, cw, ch, GMU_WHITE);
        } else if (key == GMU_KEY_LEFT) {
            if (pen_x > (int32_t)cx + 4) pen_x -= 4;
        } else if (key == GMU_KEY_RIGHT) {
            if (pen_x < (int32_t)(cx + cw) - 4) pen_x += 4;
        } else if (key == GMU_KEY_UP) {
            if (pen_y > (int32_t)cy + 4) pen_y -= 4;
        } else if (key == GMU_KEY_DOWN) {
            if (pen_y < (int32_t)(cy + ch) - 4) pen_y += 4;
        } else if (key == ' ' || key == '\n' || key == GMU_KEY_ENTER) {
            drawing = true;
        }

        /* Mouse drawing & controls */
        if (mouse_ready && gmu_mouse_poll(&mouse)) {
            pen_x = mouse.x;
            pen_y = mouse.y;
            bool left_click = (mouse.buttons & 1) && !(prev_buttons & 1);
            prev_buttons = mouse.buttons;

            if (left_click) {
                /* Red Close Chiclet */
                if (mouse.x >= (int32_t)(wx + 10) && mouse.x <= (int32_t)(wx + 24) &&
                    mouse.y >= (int32_t)(wy + 10) && mouse.y <= (int32_t)(wy + 24)) {
                    break;
                }
                /* Toolbar Color clicks */
                uint64_t tool_x = wx + 12;
                uint64_t tool_y = wy + 42;
                if (mouse.x >= (int32_t)tool_x && mouse.x <= (int32_t)(tool_x + 90)) {
                    for (int i = 0; i < PALETTE_SIZE; i++) {
                        uint64_t sy = tool_y + 28 + i * 22;
                        if (mouse.y >= (int32_t)sy && mouse.y <= (int32_t)(sy + 20)) {
                            current_color_idx = (uint32_t)i;
                            draw_paint_ui(fb, cx, cy, cw, ch, wx, wy, ww, wh);
                            break;
                        }
                    }
                    /* Clear canvas button on toolbar */
                    uint64_t brush_y = tool_y + 28 + PALETTE_SIZE * 22 + 10;
                    if (mouse.y >= (int32_t)(brush_y + 44) && mouse.y <= (int32_t)(brush_y + 58)) {
                        gmu_fill_rect(fb, cx, cy, cw, ch, GMU_WHITE);
                    }
                }
            }

            if (mouse.buttons & 1) {
                drawing = true;
            }
        }

        /* Perform brush stroke */
        if (drawing) {
            if (pen_x >= (int32_t)cx && pen_x < (int32_t)(cx + cw) &&
                pen_y >= (int32_t)cy && pen_y < (int32_t)(cy + ch)) {
                uint32_t col = palette_colors[current_color_idx];
                gmu_fill_circle(fb, pen_x, pen_y, brush_size, col);
            }
        } else {
            /* Draw subtle pen crosshair indicator */
            if (pen_x >= (int32_t)cx && pen_x < (int32_t)(cx + cw) &&
                pen_y >= (int32_t)cy && pen_y < (int32_t)(cy + ch)) {
                gmu_draw_pixel(fb, pen_x, pen_y, 0x003B82F6);
            }
        }

        __asm__ volatile ("pause");
    }
}

void gmu_paint_register(void) {
    const gmu_application_t app = {
        .id = GMU_APP_PAINT,
        .name = "Paint Studio",
        .run = gmu_paint_run
    };
    (void)gmu_app_register(&app);
}
