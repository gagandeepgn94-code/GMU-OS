
#include <stdint.h>
#include <stdbool.h>
#include <gmu/shooter.h>
#include <gmu/app.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/draw.h>
#include <gmu/font.h>

#define GAME_BG       0x00071220
#define GAME_PANEL    0x00132238
#define GAME_CYAN     0x0006B6D4
#define GAME_BLUE     0x003B82F6
#define GAME_RED      0x00EF4444
#define GAME_WHITE    0x00FFFFFF
#define GAME_GREEN    0x0022C55E
#define MAX_BULLETS   8
#define ENEMY_COUNT   5

typedef struct {
    int64_t x;
    int64_t y;
    bool active;
} bullet_t;

static void draw_game(gmu_framebuffer_t *fb,
                      int64_t player_x,
                      int64_t player_y,
                      bullet_t *bullets,
                      int64_t *enemy_x,
                      int64_t *enemy_y,
                      int score,
                      int lives) {
    gmu_fb_clear(fb, GAME_BG);

    gmu_fill_rect(fb, 0, 0, fb->width, 42, GAME_PANEL);
    gmu_draw_text(fb, 16, 14, "GMU STRIKE", GAME_CYAN, GAME_PANEL);

    gmu_draw_text(fb, 190, 14, "SCORE:", GAME_WHITE, GAME_PANEL);

    char number[12];
    int n = score;
    int pos = 0;

    if (n == 0) number[pos++] = '0';
    else {
        char temp[12];
        int t = 0;
        while (n > 0 && t < 10) {
            temp[t++] = '0' + n % 10;
            n /= 10;
        }
        while (t > 0) number[pos++] = temp[--t];
    }
    number[pos] = '\0';

    gmu_draw_text(fb, 240, 14, number, GAME_GREEN, GAME_PANEL);

    gmu_draw_text(fb, fb->width - 100, 14,
                  "LIVES:", GAME_WHITE, GAME_PANEL);

    if (lives >= 0) {
        char life_text[2] = {'0' + lives, '\0'};
        gmu_draw_text(fb, fb->width - 48, 14,
                      life_text, GAME_RED, GAME_PANEL);
    }

    /* Player ship */
    gmu_fill_rect(fb, player_x - 14, player_y, 28, 22, GAME_BLUE);
    gmu_fill_rect(fb, player_x - 6, player_y - 10, 12, 12, GAME_CYAN);
    gmu_fill_rect(fb, player_x - 20, player_y + 17, 40, 5, GAME_WHITE);

    /* Bullets */
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active)
            gmu_fill_rect(fb, bullets[i].x - 2,
                          bullets[i].y, 4, 12, GAME_CYAN);
    }

    /* Enemies */
    for (int i = 0; i < ENEMY_COUNT; i++) {
        gmu_fill_rect(fb, enemy_x[i] - 13,
                      enemy_y[i], 26, 20, GAME_RED);
        gmu_fill_rect(fb, enemy_x[i] - 8,
                      enemy_y[i] + 20, 16, 5, GAME_WHITE);
    }

    gmu_draw_text(fb, 16, fb->height - 20,
                  "ARROWS: MOVE   SPACE: FIRE   ESC: EXIT",
                  GAME_WHITE, GAME_BG);
}

void gmu_shooter_run(gmu_framebuffer_t *fb) {
    if (!fb || !fb->initialized)
        return;

    int64_t player_x = fb->width / 2;
    int64_t player_y = fb->height - 65;

    int64_t enemy_x[ENEMY_COUNT];
    int64_t enemy_y[ENEMY_COUNT];

    bullet_t bullets[MAX_BULLETS] = {0};

    int score = 0;
    int lives = 3;
    int frame = 0;
    bool game_over = false;

    for (int i = 0; i < ENEMY_COUNT; i++) {
        enemy_x[i] = 70 + i * ((fb->width - 140) / ENEMY_COUNT);
        enemy_y[i] = 90 + (i % 2) * 45;
    }

    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);
    uint8_t prev_buttons = 0;

    gmu_fb_clear(fb, GAME_BG);
    gmu_draw_text(fb, fb->width / 2 - 65,
                  fb->height / 2 - 55,
                  "GMU STRIKE", GAME_CYAN, GAME_BG);
    gmu_draw_text(fb, fb->width / 2 - 100,
                  fb->height / 2 - 15,
                  "PRESS ENTER TO START", GAME_WHITE, GAME_BG);
    gmu_draw_text(fb, fb->width / 2 - 85,
                  fb->height / 2 + 15,
                  "ESC TO RETURN", GAME_WHITE, GAME_BG);

    for (;;) {
        uint16_t k = gmu_keyboard_read_key();
        if (k == GMU_KEY_ENTER || k == ' ') break;
        if (k == GMU_KEY_ESCAPE) return;
        if (mouse_ready && gmu_mouse_poll(&mouse)) {
            if (mouse.buttons & 1) break;
            if (mouse.buttons & 2) return;
        }
        __asm__ volatile ("pause");
    }

    while (!game_over) {
        uint16_t key = gmu_keyboard_read_key();

        if (key == GMU_KEY_ESCAPE)
            break;

        if (key == GMU_KEY_LEFT && player_x > 25)
            player_x -= 18;

        if (key == GMU_KEY_RIGHT && player_x < (int64_t)fb->width - 25)
            player_x += 18;

        bool mouse_fire = false;
        if (mouse_ready && gmu_mouse_poll(&mouse)) {
            if (mouse.x > 25 && mouse.x < (int64_t)fb->width - 25) {
                player_x = mouse.x;
            }
            if ((mouse.buttons & 1) && !(prev_buttons & 1)) {
                mouse_fire = true;
            }
            prev_buttons = mouse.buttons;
        }

        if (key == ' ' || mouse_fire) {
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) {
                    bullets[i].active = true;
                    bullets[i].x = player_x;
                    bullets[i].y = player_y - 12;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                bullets[i].y -= 9;
                if (bullets[i].y < 45)
                    bullets[i].active = false;
            }
        }

        if (++frame % 12 == 0) {
            for (int i = 0; i < ENEMY_COUNT; i++) {
                enemy_y[i] += 9;

                if (enemy_y[i] > player_y - 20) {
                    lives--;
                    enemy_y[i] = 55;
                    enemy_x[i] = 40 + (i * 83) % (fb->width - 80);

                    if (lives <= 0)
                        game_over = true;
                }
            }
        }

        for (int b = 0; b < MAX_BULLETS; b++) {
            if (!bullets[b].active)
                continue;

            for (int e = 0; e < ENEMY_COUNT; e++) {
                if (bullets[b].x >= enemy_x[e] - 15 &&
                    bullets[b].x <= enemy_x[e] + 15 &&
                    bullets[b].y >= enemy_y[e] &&
                    bullets[b].y <= enemy_y[e] + 25) {

                    bullets[b].active = false;
                    score += 10;
                    enemy_y[e] = 55;
                    enemy_x[e] = 40 + (e * 97 + score * 3)
                                     % (fb->width - 80);
                    break;
                }
            }
        }

        draw_game(fb, player_x, player_y, bullets,
                  enemy_x, enemy_y, score, lives);

        /* Temporary frame pacing until the timer driver is ready. */
        for (volatile uint32_t delay = 0; delay < 180000; delay++)
            __asm__ volatile ("pause");
    }

    gmu_fb_clear(fb, GAME_BG);
    gmu_draw_text(fb, fb->width / 2 - 45,
                  fb->height / 2 - 30,
                  "GAME OVER", GAME_RED, GAME_BG);

    gmu_draw_text(fb, fb->width / 2 - 55,
                  fb->height / 2,
                  "FINAL SCORE:", GAME_WHITE, GAME_BG);

    /* Show final score using the existing score renderer. */
    bullet_t empty_bullets[MAX_BULLETS] = {0};
    draw_game(fb, player_x, player_y, empty_bullets,
              enemy_x, enemy_y, score, lives);
}

void gmu_shooter_register(void) {
    extern bool gmu_app_register(const gmu_application_t *app);

    const gmu_application_t app = {
        .id = GMU_APP_SHOOTER,
        .name = "GMU STRIKE",
        .run = gmu_shooter_run
    };

    (void)gmu_app_register(&app);
}
