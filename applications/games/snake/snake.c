#include <stdint.h>
#include <stdbool.h>
#include <gmu/snake.h>
#include <gmu/app.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/draw.h>
#include <gmu/font.h>
#include <gmu/timer.h>
#include <gmu/theme.h>

#define GRID_W 32
#define GRID_H 22
#define CELL_SIZE 18
#define MAX_SNAKE_LEN (GRID_W * GRID_H)

typedef struct {
    int x;
    int y;
} point_t;

static point_t snake[MAX_SNAKE_LEN];
static int snake_len = 4;
static int dir_x = 1;
static int dir_y = 0;
static point_t food = {10, 10};
static int score = 0;
static int high_score = 0;
static bool game_over = false;

static uint32_t rng_state = 0x12345678;

static uint32_t pseudo_random(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void spawn_food(void) {
    food.x = 1 + (pseudo_random() % (GRID_W - 2));
    food.y = 1 + (pseudo_random() % (GRID_H - 2));
}

static void reset_snake(void) {
    snake_len = 4;
    dir_x = 1;
    dir_y = 0;
    snake[0].x = 8; snake[0].y = 10;
    snake[1].x = 7; snake[1].y = 10;
    snake[2].x = 6; snake[2].y = 10;
    snake[3].x = 5; snake[3].y = 10;
    score = 0;
    game_over = false;
    spawn_food();
}

static void format_num(uint64_t val, char *buf) {
    if (val == 0) {
        buf[0] = '0'; buf[1] = '\0'; return;
    }
    char temp[24]; int t = 0;
    while (val > 0) { temp[t++] = '0' + (val % 10); val /= 10; }
    int p = 0; while (t > 0) buf[p++] = temp[--t]; buf[p] = '\0';
}

static void draw_snake_game(gmu_framebuffer_t *fb, uint64_t gx, uint64_t gy,
                            uint64_t gw, uint64_t gh, uint64_t wx, uint64_t wy,
                            uint64_t ww, uint64_t wh) {
    const gmu_theme_t *th = gmu_theme_get();

    /* Window Frame */
    gmu_draw_window_frame(fb, wx, wy, ww, wh,
                          "GMU SNAKE - RETRO ARCADE",
                          th->surface_accent, th->surface_dark, th->border);

    /* Header Bar: Score & Controls */
    uint64_t bar_y = wy + 32;
    gmu_fill_rect(fb, wx + 1, bar_y, ww - 2, 28, th->surface);
    gmu_fill_rect(fb, wx + 1, bar_y + 27, ww - 2, 1, th->border);

    char score_str[32] = "SCORE: ";
    char sc_buf[16];
    format_num(score, sc_buf);
    int sp = 7; const char *sp_str = sc_buf; while (*sp_str) score_str[sp++] = *sp_str++;
    score_str[sp] = '\0';
    gmu_draw_text(fb, wx + 16, bar_y + 8, score_str, th->chiclet_green, th->surface);

    char hs_str[32] = "HIGH: ";
    format_num(high_score, sc_buf);
    sp = 6; sp_str = sc_buf; while (*sp_str) hs_str[sp++] = *sp_str++;
    hs_str[sp] = '\0';
    gmu_draw_text(fb, wx + 140, bar_y + 8, hs_str, th->chiclet_yellow, th->surface);

    gmu_draw_text(fb, wx + ww - 240, bar_y + 8, "WASD / ARROWS: MOVE  [ESC] EXIT", th->text_secondary, th->surface);

    /* Game Play Area */
    gmu_fill_rect(fb, gx, gy, gw, gh, 0x00060C14);
    gmu_draw_rect(fb, gx - 1, gy - 1, gw + 2, gh + 2, th->accent);

    /* Food */
    uint64_t fx = gx + food.x * CELL_SIZE;
    uint64_t fy = gy + food.y * CELL_SIZE;
    gmu_fill_circle(fb, fx + CELL_SIZE / 2, fy + CELL_SIZE / 2, CELL_SIZE / 2 - 2, th->chiclet_red);

    /* Snake segments */
    for (int i = 0; i < snake_len; i++) {
        uint64_t sx = gx + snake[i].x * CELL_SIZE;
        uint64_t sy = gy + snake[i].y * CELL_SIZE;
        uint32_t col = (i == 0) ? 0x0034D399 : 0x0010B981;
        gmu_fill_rect(fb, sx + 1, sy + 1, CELL_SIZE - 2, CELL_SIZE - 2, col);
    }

    /* Game Over Modal Overlay */
    if (game_over) {
        uint64_t ow = 280;
        uint64_t oh = 120;
        uint64_t ox = gx + (gw - ow) / 2;
        uint64_t oy = gy + (gh - oh) / 2;

        gmu_draw_shadow(fb, ox, oy, ow, oh, 6);
        gmu_fill_rect(fb, ox, oy, ow, oh, th->surface);
        gmu_draw_rect(fb, ox, oy, ow, oh, th->chiclet_red);

        gmu_draw_text(fb, ox + 80, oy + 24, "GAME OVER!", th->chiclet_red, th->surface);
        gmu_draw_text(fb, ox + 60, oy + 54, score_str, th->text_primary, th->surface);
        gmu_draw_text(fb, ox + 40, oy + 84, "Press [R] to Play Again", th->chiclet_yellow, th->surface);
    }
}

void gmu_snake_run(gmu_framebuffer_t *fb) {
    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);
    uint8_t prev_buttons = 0;

    reset_snake();

    uint64_t gw = GRID_W * CELL_SIZE;
    uint64_t gh = GRID_H * CELL_SIZE;
    uint64_t ww = gw + 32;
    uint64_t wh = gh + 72;
    uint64_t wx = (fb->width - ww) / 2;
    uint64_t wy = (fb->height - wh) / 2 - 10;
    uint64_t gx = wx + 16;
    uint64_t gy = wy + 62;

    draw_snake_game(fb, gx, gy, gw, gh, wx, wy, ww, wh);

    uint64_t last_move_time = gmu_timer_ms();

    for (;;) {
        /* Mouse Input */
        if (mouse_ready && gmu_mouse_poll(&mouse)) {
            bool left_click = (mouse.buttons & 1) && !(prev_buttons & 1);
            prev_buttons = mouse.buttons;

            if (left_click) {
                /* Click Red Close Chiclet */
                if (mouse.x >= (int32_t)(wx + 10) && mouse.x <= (int32_t)(wx + 24) &&
                    mouse.y >= (int32_t)(wy + 10) && mouse.y <= (int32_t)(wy + 24)) {
                    break;
                }
            }
        }

        uint16_t key = gmu_keyboard_read_key();
        if (key == GMU_KEY_ESCAPE) {
            break;
        } else if (key == 'r' || key == 'R') {
            reset_snake();
            draw_snake_game(fb, gx, gy, gw, gh, wx, wy, ww, wh);
        } else if ((key == GMU_KEY_UP || key == 'w' || key == 'W') && dir_y == 0) {
            dir_x = 0; dir_y = -1;
        } else if ((key == GMU_KEY_DOWN || key == 's' || key == 'S') && dir_y == 0) {
            dir_x = 0; dir_y = 1;
        } else if ((key == GMU_KEY_LEFT || key == 'a' || key == 'A') && dir_x == 0) {
            dir_x = -1; dir_y = 0;
        } else if ((key == GMU_KEY_RIGHT || key == 'd' || key == 'D') && dir_x == 0) {
            dir_x = 1; dir_y = 0;
        }

        /* Move snake every 110ms */
        uint64_t now = gmu_timer_ms();
        if (!game_over && (now - last_move_time >= 110)) {
            last_move_time = now;

            point_t new_head = {snake[0].x + dir_x, snake[0].y + dir_y};

            /* Wall collision check */
            if (new_head.x < 0 || new_head.x >= GRID_W ||
                new_head.y < 0 || new_head.y >= GRID_H) {
                game_over = true;
                if (score > high_score) high_score = score;
            } else {
                /* Self-collision check */
                for (int i = 0; i < snake_len; i++) {
                    if (snake[i].x == new_head.x && snake[i].y == new_head.y) {
                        game_over = true;
                        if (score > high_score) high_score = score;
                        break;
                    }
                }
            }

            if (!game_over) {
                /* Check food eating */
                bool ate_food = (new_head.x == food.x && new_head.y == food.y);

                if (ate_food) {
                    score += 10;
                    if (score > high_score) high_score = score;
                    if (snake_len < MAX_SNAKE_LEN - 1) snake_len++;
                    spawn_food();
                }

                /* Shift body */
                for (int i = snake_len - 1; i > 0; i--) {
                    snake[i] = snake[i - 1];
                }
                snake[0] = new_head;
            }

            draw_snake_game(fb, gx, gy, gw, gh, wx, wy, ww, wh);
        }

        __asm__ volatile ("pause");
    }
}

void gmu_snake_register(void) {
    const gmu_application_t app = {
        .id = GMU_APP_SNAKE,
        .name = "GMU Snake",
        .run = gmu_snake_run
    };
    (void)gmu_app_register(&app);
}
