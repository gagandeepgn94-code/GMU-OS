#include <stdint.h>
#include <stdbool.h>
#include <gmu/breakout.h>
#include <gmu/app.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/draw.h>
#include <gmu/font.h>
#include <gmu/timer.h>
#include <gmu/theme.h>

#define BRICK_ROWS 4
#define BRICK_COLS 8
#define TOTAL_BRICKS (BRICK_ROWS * BRICK_COLS)

static bool bricks[BRICK_ROWS][BRICK_COLS];
static int paddle_x = 180;
static const int paddle_w = 70;
static const int paddle_h = 10;

static int ball_x = 200;
static int ball_y = 260;
static int ball_dx = 3;
static int ball_dy = -3;
static const int ball_r = 4;

static int bo_score = 0;
static int bo_lives = 3;
static bool bo_game_over = false;
static bool bo_game_won = false;

static void reset_breakout(void) {
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            bricks[r][c] = true;
        }
    }
    paddle_x = 180;
    ball_x = 210;
    ball_y = 260;
    ball_dx = 3;
    ball_dy = -3;
    bo_score = 0;
    bo_lives = 3;
    bo_game_over = false;
    bo_game_won = false;
}

static void format_num(uint64_t val, char *buf) {
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char temp[24]; int t = 0;
    while (val > 0) { temp[t++] = '0' + (val % 10); val /= 10; }
    int p = 0; while (t > 0) buf[p++] = temp[--t]; buf[p] = '\0';
}

static void draw_breakout(gmu_framebuffer_t *fb, uint64_t ax, uint64_t ay,
                          uint64_t aw, uint64_t ah, uint64_t wx, uint64_t wy,
                          uint64_t ww, uint64_t wh) {
    const gmu_theme_t *th = gmu_theme_get();

    /* Window Frame */
    gmu_draw_window_frame(fb, wx, wy, ww, wh,
                          "BREAKOUT - ARCADE BRICK BREAKER",
                          th->surface_accent, th->surface_dark, th->border);

    /* Header Bar: Score, Lives */
    uint64_t bar_y = wy + 32;
    gmu_fill_rect(fb, wx + 1, bar_y, ww - 2, 28, th->surface);
    gmu_fill_rect(fb, wx + 1, bar_y + 27, ww - 2, 1, th->border);

    char sc_str[24] = "SCORE: ";
    char n_buf[16]; format_num(bo_score, n_buf);
    int p = 7; const char *np = n_buf; while (*np) sc_str[p++] = *np++; sc_str[p] = '\0';
    gmu_draw_text(fb, wx + 16, bar_y + 8, sc_str, th->chiclet_green, th->surface);

    char lv_str[16] = "LIVES: ";
    format_num(bo_lives, n_buf);
    p = 7; np = n_buf; while (*np) lv_str[p++] = *np++; lv_str[p] = '\0';
    gmu_draw_text(fb, wx + 140, bar_y + 8, lv_str, th->chiclet_red, th->surface);

    gmu_draw_text(fb, wx + ww - 220, bar_y + 8, "ARROWS / MOUSE: MOVE  [ESC] EXIT", th->text_secondary, th->surface);

    /* Arena */
    gmu_fill_rect(fb, ax, ay, aw, ah, 0x0008101E);
    gmu_draw_rect(fb, ax - 1, ay - 1, aw + 2, ah + 2, th->accent);

    /* Bricks */
    uint64_t brick_w = (aw - 36) / BRICK_COLS;
    uint64_t brick_h = 16;
    uint32_t row_cols[4] = {0x00EF4444, 0x00FBBF24, 0x0010B981, 0x003B82F6};

    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (bricks[r][c]) {
                uint64_t bx = ax + 18 + c * brick_w;
                uint64_t by = ay + 20 + r * (brick_h + 4);
                gmu_fill_rect(fb, bx, by, brick_w - 4, brick_h, row_cols[r]);
                gmu_draw_rect(fb, bx, by, brick_w - 4, brick_h, GMU_WHITE);
            }
        }
    }

    /* Paddle */
    uint64_t px = ax + paddle_x;
    uint64_t py = ay + ah - 24;
    gmu_fill_rounded_rect(fb, px, py, paddle_w, paddle_h, 4, th->accent);
    gmu_draw_rect(fb, px, py, paddle_w, paddle_h, GMU_WHITE);

    /* Ball */
    gmu_fill_circle(fb, ax + ball_x, ay + ball_y, ball_r, GMU_WHITE);

    /* Overlays */
    if (bo_game_over || bo_game_won) {
        uint64_t ow = 260;
        uint64_t oh = 110;
        uint64_t ox = ax + (aw - ow) / 2;
        uint64_t oy = ay + (ah - oh) / 2;

        gmu_draw_shadow(fb, ox, oy, ow, oh, 6);
        gmu_fill_rect(fb, ox, oy, ow, oh, th->surface);
        gmu_draw_rect(fb, ox, oy, ow, oh, bo_game_won ? th->chiclet_green : th->chiclet_red);

        gmu_draw_text(fb, ox + 60, oy + 20, bo_game_won ? "VICTORY! YOU WON!" : "GAME OVER!",
                      bo_game_won ? th->chiclet_green : th->chiclet_red, th->surface);
        gmu_draw_text(fb, ox + 70, oy + 48, sc_str, th->text_primary, th->surface);
        gmu_draw_text(fb, ox + 40, oy + 76, "Press [R] to Play Again", th->chiclet_yellow, th->surface);
    }
}

void gmu_breakout_run(gmu_framebuffer_t *fb) {
    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);

    reset_breakout();

    uint64_t aw = 460;
    uint64_t ah = 360;
    uint64_t ww = aw + 36;
    uint64_t wh = ah + 78;
    uint64_t wx = (fb->width - ww) / 2;
    uint64_t wy = (fb->height - wh) / 2 - 10;
    uint64_t ax = wx + 18;
    uint64_t ay = wy + 62;

    draw_breakout(fb, ax, ay, aw, ah, wx, wy, ww, wh);

    uint64_t last_tick = gmu_timer_ms();

    for (;;) {
        uint16_t key = gmu_keyboard_read_key();

        if (key == GMU_KEY_ESCAPE) {
            break;
        } else if (key == 'r' || key == 'R') {
            reset_breakout();
            draw_breakout(fb, ax, ay, aw, ah, wx, wy, ww, wh);
        } else if (key == GMU_KEY_LEFT || key == 'a' || key == 'A') {
            if (paddle_x > 10) paddle_x -= 18;
        } else if (key == GMU_KEY_RIGHT || key == 'd' || key == 'D') {
            if (paddle_x < (int)aw - paddle_w - 10) paddle_x += 18;
        }

        /* Mouse tracking for paddle */
        if (mouse_ready && gmu_mouse_poll(&mouse)) {
            if (mouse.x >= (int32_t)ax && mouse.x <= (int32_t)(ax + aw)) {
                paddle_x = (mouse.x - ax) - (paddle_w / 2);
                if (paddle_x < 5) paddle_x = 5;
                if (paddle_x > (int)aw - paddle_w - 5) paddle_x = (int)aw - paddle_w - 5;
            }
        }

        /* Physics update (every 25ms = ~40 FPS) */
        uint64_t now = gmu_timer_ms();
        if (!bo_game_over && !bo_game_won && (now - last_tick >= 25)) {
            last_tick = now;

            ball_x += ball_dx;
            ball_y += ball_dy;

            /* Left & Right wall bounce */
            if (ball_x <= ball_r) { ball_x = ball_r; ball_dx = -ball_dx; }
            if (ball_x >= (int)aw - ball_r) { ball_x = (int)aw - ball_r; ball_dx = -ball_dx; }

            /* Top wall bounce */
            if (ball_y <= ball_r) { ball_y = ball_r; ball_dy = -ball_dy; }

            /* Paddle collision */
            int paddle_top = (int)ah - 24;
            if (ball_y + ball_r >= paddle_top && ball_y - ball_r <= paddle_top + paddle_h) {
                if (ball_x >= paddle_x - 4 && ball_x <= paddle_x + paddle_w + 4) {
                    ball_dy = -ball_dy;
                    /* Angle reflection based on hit position */
                    int center = paddle_x + paddle_w / 2;
                    int diff = ball_x - center;
                    ball_dx = diff / 8;
                    if (ball_dx == 0) ball_dx = (diff < 0) ? -2 : 2;
                    ball_y = paddle_top - ball_r - 1;
                }
            }

            /* Bottom wall (lose life) */
            if (ball_y >= (int)ah) {
                bo_lives--;
                if (bo_lives <= 0) {
                    bo_game_over = true;
                } else {
                    ball_x = paddle_x + paddle_w / 2;
                    ball_y = 260;
                    ball_dy = -3;
                }
            }

            /* Brick collisions */
            uint64_t brick_w = (aw - 36) / BRICK_COLS;
            uint64_t brick_h = 16;
            bool any_left = false;

            for (int r = 0; r < BRICK_ROWS; r++) {
                for (int c = 0; c < BRICK_COLS; c++) {
                    if (bricks[r][c]) {
                        any_left = true;
                        int bx = 18 + c * brick_w;
                        int by = 20 + r * (brick_h + 4);

                        if (ball_x >= bx && ball_x <= bx + (int)brick_w - 4 &&
                            ball_y >= by && ball_y <= by + (int)brick_h) {
                            bricks[r][c] = false;
                            ball_dy = -ball_dy;
                            bo_score += (4 - r) * 10;
                            break;
                        }
                    }
                }
            }

            if (!any_left) {
                bo_game_won = true;
            }

            draw_breakout(fb, ax, ay, aw, ah, wx, wy, ww, wh);
        }

        __asm__ volatile ("pause");
    }
}

void gmu_breakout_register(void) {
    const gmu_application_t app = {
        .id = GMU_APP_BREAKOUT,
        .name = "Breakout",
        .run = gmu_breakout_run
    };
    (void)gmu_app_register(&app);
}
