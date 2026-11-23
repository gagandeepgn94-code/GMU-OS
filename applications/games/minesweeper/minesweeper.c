#include <stdint.h>
#include <stdbool.h>
#include <gmu/minesweeper.h>
#include <gmu/app.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/draw.h>
#include <gmu/font.h>
#include <gmu/timer.h>
#include <gmu/theme.h>

#define MS_COLS 10
#define MS_ROWS 10
#define TOTAL_MINES 12
#define CELL_PX 32

typedef struct {
    bool has_mine;
    bool is_revealed;
    bool is_flagged;
    int neighbor_mines;
} cell_t;

static cell_t board[MS_ROWS][MS_COLS];
static int cursor_r = 0;
static int cursor_c = 0;
static bool ms_game_over = false;
static bool ms_game_won = false;
static uint64_t ms_start_sec = 0;

static uint32_t ms_rng = 0x98765432;

static uint32_t ms_rand(void) {
    ms_rng ^= ms_rng << 13;
    ms_rng ^= ms_rng >> 17;
    ms_rng ^= ms_rng << 5;
    return ms_rng;
}

static void init_minesweeper(void) {
    for (int r = 0; r < MS_ROWS; r++) {
        for (int c = 0; c < MS_COLS; c++) {
            board[r][c].has_mine = false;
            board[r][c].is_revealed = false;
            board[r][c].is_flagged = false;
            board[r][c].neighbor_mines = 0;
        }
    }

    int placed = 0;
    while (placed < TOTAL_MINES) {
        int r = ms_rand() % MS_ROWS;
        int c = ms_rand() % MS_COLS;
        if (!board[r][c].has_mine) {
            board[r][c].has_mine = true;
            placed++;
        }
    }

    for (int r = 0; r < MS_ROWS; r++) {
        for (int c = 0; c < MS_COLS; c++) {
            if (board[r][c].has_mine) continue;
            int count = 0;
            for (int dr = -1; dr <= 1; dr++) {
                for (int dc = -1; dc <= 1; dc++) {
                    int nr = r + dr;
                    int nc = c + dc;
                    if (nr >= 0 && nr < MS_ROWS && nc >= 0 && nc < MS_COLS) {
                        if (board[nr][nc].has_mine) count++;
                    }
                }
            }
            board[r][c].neighbor_mines = count;
        }
    }

    cursor_r = 4;
    cursor_c = 4;
    ms_game_over = false;
    ms_game_won = false;
    ms_start_sec = gmu_timer_seconds();
}

static void reveal_cell(int r, int c) {
    if (r < 0 || r >= MS_ROWS || c < 0 || c >= MS_COLS) return;
    if (board[r][c].is_revealed || board[r][c].is_flagged) return;

    board[r][c].is_revealed = true;

    if (board[r][c].has_mine) {
        ms_game_over = true;
        return;
    }

    /* Flood fill if neighbor count is 0 */
    if (board[r][c].neighbor_mines == 0) {
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                if (dr != 0 || dc != 0) {
                    reveal_cell(r + dr, c + dc);
                }
            }
        }
    }

    /* Check win condition */
    int unrevealed_safe = 0;
    for (int row = 0; row < MS_ROWS; row++) {
        for (int col = 0; col < MS_COLS; col++) {
            if (!board[row][col].has_mine && !board[row][col].is_revealed) {
                unrevealed_safe++;
            }
        }
    }
    if (unrevealed_safe == 0) {
        ms_game_won = true;
        ms_game_over = true;
    }
}

static void format_num(uint64_t val, char *buf) {
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char temp[24]; int t = 0;
    while (val > 0) { temp[t++] = '0' + (val % 10); val /= 10; }
    int p = 0; while (t > 0) buf[p++] = temp[--t]; buf[p] = '\0';
}

static void draw_minesweeper(gmu_framebuffer_t *fb) {
    const gmu_theme_t *th = gmu_theme_get();

    uint64_t gw = MS_COLS * CELL_PX;
    uint64_t gh = MS_ROWS * CELL_PX;
    uint64_t ww = gw + 36;
    uint64_t wh = gh + 92;
    uint64_t wx = (fb->width - ww) / 2;
    uint64_t wy = (fb->height - wh) / 2 - 10;

    gmu_draw_window_frame(fb, wx, wy, ww, wh,
                          "MINESWEEPER CLASSIC",
                          th->surface_accent, th->surface_dark, th->border);

    /* Status Bar */
    uint64_t bar_y = wy + 32;
    gmu_fill_rect(fb, wx + 1, bar_y, ww - 2, 38, th->surface);
    gmu_fill_rect(fb, wx + 1, bar_y + 37, ww - 2, 1, th->border);

    int flags_used = 0;
    for (int r = 0; r < MS_ROWS; r++) {
        for (int c = 0; c < MS_COLS; c++) {
            if (board[r][c].is_flagged) flags_used++;
        }
    }

    char mine_str[24] = "MINES: ";
    char m_buf[8]; format_num(TOTAL_MINES - flags_used, m_buf);
    int mp = 7; const char *mp_p = m_buf; while (*mp_p) mine_str[mp++] = *mp_p++; mine_str[mp] = '\0';
    gmu_draw_text(fb, wx + 16, bar_y + 14, mine_str, th->chiclet_red, th->surface);

    /* Center Face */
    const char *face = "[:)]";
    uint32_t face_col = th->chiclet_yellow;
    if (ms_game_won) { face = "B)"; face_col = th->chiclet_green; }
    else if (ms_game_over) { face = "X("; face_col = th->chiclet_red; }

    uint64_t fx = wx + ww / 2 - 14;
    gmu_fill_rect(fb, fx - 4, bar_y + 8, 36, 22, th->surface_dark);
    gmu_draw_rect(fb, fx - 4, bar_y + 8, 36, 22, th->border);
    gmu_draw_text(fb, fx + 4, bar_y + 14, face, face_col, th->surface_dark);

    /* Timer */
    uint64_t el_sec = gmu_timer_seconds() - ms_start_sec;
    char t_str[24] = "TIME: ";
    format_num(el_sec, m_buf);
    mp = 6; mp_p = m_buf; while (*mp_p) t_str[mp++] = *mp_p++; t_str[mp] = '\0';
    gmu_draw_text(fb, wx + ww - 80, bar_y + 14, t_str, th->accent, th->surface);

    /* Grid Area */
    uint64_t bx = wx + 18;
    uint64_t by = bar_y + 46;

    for (int r = 0; r < MS_ROWS; r++) {
        for (int c = 0; c < MS_COLS; c++) {
            uint64_t cx = bx + c * CELL_PX;
            uint64_t cy = by + r * CELL_PX;
            bool is_cur = (r == cursor_r && c == cursor_c);

            if (board[r][c].is_revealed) {
                if (board[r][c].has_mine) {
                    gmu_fill_rect(fb, cx, cy, CELL_PX, CELL_PX, th->chiclet_red);
                    gmu_draw_rect(fb, cx, cy, CELL_PX, CELL_PX, th->border);
                    gmu_draw_text(fb, cx + 12, cy + 12, "*", GMU_WHITE, th->chiclet_red);
                } else {
                    gmu_fill_rect(fb, cx, cy, CELL_PX, CELL_PX, th->surface_dark);
                    gmu_draw_rect(fb, cx, cy, CELL_PX, CELL_PX, th->border);

                    if (board[r][c].neighbor_mines > 0) {
                        char n_str[2] = {'0' + board[r][c].neighbor_mines, '\0'};
                        uint32_t num_cols[5] = {0, 0x003B82F6, 0x0010B981, 0x00EF4444, 0x00A855F7};
                        uint32_t nc = (board[r][c].neighbor_mines <= 4) ? num_cols[board[r][c].neighbor_mines] : 0x00EF4444;
                        gmu_draw_text_scaled(fb, cx + 10, cy + 8, n_str, nc, th->surface_dark, 2, false);
                    }
                }
            } else {
                /* Covered cell */
                uint32_t cell_bg = is_cur ? th->surface_accent : th->surface;
                gmu_fill_rect(fb, cx, cy, CELL_PX, CELL_PX, cell_bg);
                gmu_draw_rect(fb, cx, cy, CELL_PX, CELL_PX, is_cur ? th->accent : th->border);

                if (board[r][c].is_flagged) {
                    gmu_draw_text(fb, cx + 12, cy + 12, "P", th->chiclet_red, cell_bg);
                }
            }

            if (is_cur) {
                gmu_draw_rect(fb, cx + 1, cy + 1, CELL_PX - 2, CELL_PX - 2, th->accent);
            }
        }
    }
}

void gmu_minesweeper_run(gmu_framebuffer_t *fb) {
    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);

    init_minesweeper();
    draw_minesweeper(fb);

    uint64_t ww = MS_COLS * CELL_PX + 36;
    uint64_t wh = MS_ROWS * CELL_PX + 92;
    uint64_t wx = (fb->width - ww) / 2;
    uint64_t wy = (fb->height - wh) / 2 - 10;
    uint64_t bx = wx + 18;
    uint64_t by = wy + 78;

    for (;;) {
        uint16_t key = gmu_keyboard_read_key();

        if (key == GMU_KEY_ESCAPE) {
            break;
        } else if (key == 'r' || key == 'R') {
            init_minesweeper();
            draw_minesweeper(fb);
        } else if (key == GMU_KEY_LEFT || key == 'a' || key == 'A') {
            if (cursor_c > 0) cursor_c--;
            draw_minesweeper(fb);
        } else if (key == GMU_KEY_RIGHT || key == 'd' || key == 'D') {
            if (cursor_c < MS_COLS - 1) cursor_c++;
            draw_minesweeper(fb);
        } else if (key == GMU_KEY_UP || key == 'w' || key == 'W') {
            if (cursor_r > 0) cursor_r--;
            draw_minesweeper(fb);
        } else if (key == GMU_KEY_DOWN || key == 's' || key == 'S') {
            if (cursor_r < MS_ROWS - 1) cursor_r++;
            draw_minesweeper(fb);
        } else if (key == ' ' || key == '\n' || key == GMU_KEY_ENTER) {
            if (!ms_game_over) {
                reveal_cell(cursor_r, cursor_c);
                draw_minesweeper(fb);
            }
        } else if (key == 'f' || key == 'F') {
            if (!ms_game_over && !board[cursor_r][cursor_c].is_revealed) {
                board[cursor_r][cursor_c].is_flagged = !board[cursor_r][cursor_c].is_flagged;
                draw_minesweeper(fb);
            }
        }

        /* Mouse interaction */
        if (mouse_ready && gmu_mouse_poll(&mouse)) {
            if (mouse.x >= (int32_t)bx && mouse.x < (int32_t)(bx + MS_COLS * CELL_PX) &&
                mouse.y >= (int32_t)by && mouse.y < (int32_t)(by + MS_ROWS * CELL_PX)) {
                cursor_c = (mouse.x - bx) / CELL_PX;
                cursor_r = (mouse.y - by) / CELL_PX;

                if (mouse.buttons & 1) { /* Left click */
                    if (!ms_game_over) {
                        reveal_cell(cursor_r, cursor_c);
                    }
                    draw_minesweeper(fb);
                } else if (mouse.buttons & 2) { /* Right click */
                    if (!ms_game_over && !board[cursor_r][cursor_c].is_revealed) {
                        board[cursor_r][cursor_c].is_flagged = !board[cursor_r][cursor_c].is_flagged;
                    }
                    draw_minesweeper(fb);
                }
            }
        }

        __asm__ volatile ("pause");
    }
}

void gmu_minesweeper_register(void) {
    const gmu_application_t app = {
        .id = GMU_APP_MINESWEEPER,
        .name = "Minesweeper",
        .run = gmu_minesweeper_run
    };
    (void)gmu_app_register(&app);
}
