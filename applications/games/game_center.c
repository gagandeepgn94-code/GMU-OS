#include <stdint.h>
#include <stdbool.h>
#include <gmu/game_center.h>
#include <gmu/app.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/draw.h>
#include <gmu/font.h>
#include <gmu/theme.h>

static int selected_game = 0;

static void draw_game_center_window(gmu_framebuffer_t *fb) {
    const gmu_theme_t *th = gmu_theme_get();

    uint64_t ww = fb->width > 860 ? 760 : fb->width - 40;
    uint64_t wh = fb->height > 600 ? 480 : fb->height - 50;
    uint64_t wx = (fb->width - ww) / 2;
    uint64_t wy = (fb->height - wh) / 2 - 10;

    gmu_draw_window_frame(fb, wx, wy, ww, wh,
                          "GMU ARCADE - GAME CENTER",
                          th->surface_accent, th->surface_dark, th->border);

    /* Subheader banner */
    uint64_t bar_y = wy + 32;
    gmu_fill_rect(fb, wx + 1, bar_y, ww - 2, 34, th->surface);
    gmu_fill_rect(fb, wx + 1, bar_y + 33, ww - 2, 1, th->border);
    gmu_draw_text(fb, wx + 20, bar_y + 12, "CHOOSE A GAME TO PLAY:", th->accent, th->surface);
    gmu_draw_text(fb, wx + ww - 220, bar_y + 12, "[1-4] DIRECT LAUNCH  [ESC] EXIT", th->text_secondary, th->surface);

    /* 4 Game Cards Grid: 2 rows x 2 cols */
    static const struct {
        const char *name;
        const char *genre;
        const char *desc;
        const char *badge;
        uint32_t col;
    } game_list[4] = {
        {"1. GMU Strike", "Space Shooter", "Laser cannons, alien waves & score tracking.", "SPACE", 0x0006B6D4},
        {"2. GMU Snake",  "Retro Classic", "Eat food, grow length, avoid walls & tail.",  "RETRO", 0x0010B981},
        {"3. Minesweeper","Logic Puzzle",  "Uncover hidden mines using numerical clues.", "LOGIC", 0x00EF4444},
        {"4. Breakout",   "Brick Breaker", "Reflect the bouncing ball to break bricks.",  "PADDLE", 0x00FBBF24}
    };

    uint64_t card_w = (ww - 60) / 2;
    uint64_t card_h = 160;

    for (int i = 0; i < 4; i++) {
        int r = i / 2;
        int c = i % 2;

        uint64_t cx = wx + 20 + c * (card_w + 20);
        uint64_t cy = bar_y + 48 + r * (card_h + 16);
        bool is_sel = (i == selected_game);

        uint32_t card_bg = is_sel ? th->surface_accent : th->surface;
        gmu_fill_rect(fb, cx, cy, card_w, card_h, card_bg);
        gmu_draw_rect(fb, cx, cy, card_w, card_h, is_sel ? th->accent : th->border);

        if (is_sel) {
            /* Accent top line */
            gmu_fill_rect(fb, cx, cy, card_w, 4, th->accent);
        }

        /* Badge */
        gmu_fill_rect(fb, cx + 16, cy + 16, 54, 20, game_list[i].col);
        gmu_draw_text(fb, cx + 22, cy + 22, game_list[i].badge, GMU_WHITE, game_list[i].col);

        /* Title & Genre */
        gmu_draw_text(fb, cx + 80, cy + 22, game_list[i].name, is_sel ? GMU_WHITE : th->text_primary, card_bg);
        gmu_draw_text(fb, cx + 16, cy + 54, game_list[i].genre, th->accent, card_bg);
        gmu_draw_text(fb, cx + 16, cy + 80, game_list[i].desc, th->text_secondary, card_bg);

        /* Action button */
        uint64_t btn_y = cy + 116;
        gmu_fill_rect(fb, cx + 16, btn_y, card_w - 32, 28, is_sel ? th->accent : th->surface_dark);
        gmu_draw_rect(fb, cx + 16, btn_y, card_w - 32, 28, th->border);
        gmu_draw_text(fb, cx + card_w / 2 - 36, btn_y + 10, is_sel ? "[ENTER: PLAY]" : "CLICK TO PLAY",
                      is_sel ? GMU_WHITE : th->text_primary, is_sel ? th->accent : th->surface_dark);
    }
}

void gmu_game_center_run(gmu_framebuffer_t *fb) {
    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);
    uint8_t prev_buttons = 0;

    draw_game_center_window(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }

    uint64_t ww = fb->width > 860 ? 760 : fb->width - 40;
    uint64_t wh = fb->height > 600 ? 480 : fb->height - 50;
    uint64_t wx = (fb->width - ww) / 2;
    uint64_t wy = (fb->height - wh) / 2 - 10;
    uint64_t bar_y = wy + 32;
    uint64_t card_w = (ww - 60) / 2;
    uint64_t card_h = 160;

    for (;;) {
        /* Mouse Input */
        if (mouse_ready && gmu_mouse_poll(&mouse)) {
            gmu_mouse_cursor_update(fb, mouse.x, mouse.y);
            bool left_click = (mouse.buttons & 1) && !(prev_buttons & 1);
            prev_buttons = mouse.buttons;

            if (left_click) {
                /* Click Red Close Chiclet */
                if (mouse.x >= (int32_t)(wx + 10) && mouse.x <= (int32_t)(wx + 24) &&
                    mouse.y >= (int32_t)(wy + 10) && mouse.y <= (int32_t)(wy + 24)) {
                    gmu_mouse_cursor_hide(fb);
                    break;
                }

                /* Click on 4 Game Cards */
                for (int i = 0; i < 4; i++) {
                    int r = i / 2;
                    int c = i % 2;
                    int32_t cx = (int32_t)(wx + 20 + c * (card_w + 20));
                    int32_t cy = (int32_t)(bar_y + 48 + r * (card_h + 16));

                    if (mouse.x >= cx && mouse.x <= cx + (int32_t)card_w &&
                        mouse.y >= cy && mouse.y <= cy + (int32_t)card_h) {
                        selected_game = i;
                        if (i == 0) gmu_app_launch(GMU_APP_SHOOTER, fb);
                        else if (i == 1) gmu_app_launch(GMU_APP_SNAKE, fb);
                        else if (i == 2) gmu_app_launch(GMU_APP_MINESWEEPER, fb);
                        else if (i == 3) gmu_app_launch(GMU_APP_BREAKOUT, fb);
                        draw_game_center_window(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
                        break;
                    }
                }
            }
        }

        uint16_t key = gmu_keyboard_read_key();
        if (key == GMU_KEY_ESCAPE) {
            if (mouse_ready) gmu_mouse_cursor_hide(fb);
            break;
        } else if (key == '1') {
            gmu_app_launch(GMU_APP_SHOOTER, fb);
            draw_game_center_window(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
        } else if (key == '2') {
            gmu_app_launch(GMU_APP_SNAKE, fb);
            draw_game_center_window(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
        } else if (key == '3') {
            gmu_app_launch(GMU_APP_MINESWEEPER, fb);
            draw_game_center_window(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
        } else if (key == '4') {
            gmu_app_launch(GMU_APP_BREAKOUT, fb);
            draw_game_center_window(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
        } else if (key == GMU_KEY_UP || key == 'w' || key == 'W') {
            selected_game = (selected_game + 2) % 4;
            draw_game_center_window(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
        } else if (key == GMU_KEY_DOWN || key == 's' || key == 'S') {
            selected_game = (selected_game + 2) % 4;
            draw_game_center_window(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
        } else if (key == GMU_KEY_LEFT || key == 'a' || key == 'A') {
            selected_game = (selected_game + 3) % 4;
            draw_game_center_window(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
        } else if (key == GMU_KEY_RIGHT || key == 'd' || key == 'D') {
            selected_game = (selected_game + 1) % 4;
            draw_game_center_window(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
        } else if (key == '\n' || key == GMU_KEY_ENTER || key == ' ') {
            if (selected_game == 0) gmu_app_launch(GMU_APP_SHOOTER, fb);
            else if (selected_game == 1) gmu_app_launch(GMU_APP_SNAKE, fb);
            else if (selected_game == 2) gmu_app_launch(GMU_APP_MINESWEEPER, fb);
            else if (selected_game == 3) gmu_app_launch(GMU_APP_BREAKOUT, fb);
            draw_game_center_window(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
        }

        __asm__ volatile ("pause");
    }
}

void gmu_game_center_register(void) {
    const gmu_application_t app = {
        .id = GMU_APP_GAME_CENTER,
        .name = "Game Center",
        .run = gmu_game_center_run
    };
    (void)gmu_app_register(&app);
}
