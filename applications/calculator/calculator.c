#include <stdint.h>
#include <stdbool.h>
#include <gmu/calculator.h>
#include <gmu/app.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/draw.h>
#include <gmu/font.h>
#include <gmu/theme.h>

#define CALC_MAX 48

static char expression[CALC_MAX] = "";
static uint32_t expr_len = 0;
static char result_str[32] = "0";
static char history_str[48] = "";

/* ========================================================================= */
/* 1. Number Formatting Helper                                              */
/* ========================================================================= */

static void format_num(int64_t val, char *buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    bool neg = false;
    if (val < 0) {
        neg = true;
        val = -val;
    }
    char temp[24];
    int t = 0;
    while (val > 0) {
        temp[t++] = '0' + (val % 10);
        val /= 10;
    }
    int p = 0;
    if (neg) buf[p++] = '-';
    while (t > 0) buf[p++] = temp[--t];
    buf[p] = '\0';
}

/* ========================================================================= */
/* 2. Full Recursive-Descent Expression Evaluator (PEMDAS + Parentheses)     */
/* ========================================================================= */

static const char *parse_ptr;

static bool parse_expr(int64_t *val);
static bool parse_term(int64_t *val);
static bool parse_factor(int64_t *val);

static void skip_spaces(void) {
    while (*parse_ptr == ' ') parse_ptr++;
}

static bool parse_factor(int64_t *val) {
    skip_spaces();
    bool neg = false;

    if (*parse_ptr == '+') {
        parse_ptr++;
        skip_spaces();
    } else if (*parse_ptr == '-') {
        neg = true;
        parse_ptr++;
        skip_spaces();
    }

    if (*parse_ptr == '(') {
        parse_ptr++; /* consume '(' */
        if (!parse_expr(val)) return false;
        skip_spaces();
        if (*parse_ptr != ')') return false;
        parse_ptr++; /* consume ')' */
    } else if (*parse_ptr >= '0' && *parse_ptr <= '9') {
        int64_t num = 0;
        while (*parse_ptr >= '0' && *parse_ptr <= '9') {
            num = num * 10 + (*parse_ptr - '0');
            parse_ptr++;
        }
        *val = num;
    } else {
        return false;
    }

    if (neg) *val = -(*val);
    return true;
}

static bool parse_term(int64_t *val) {
    if (!parse_factor(val)) return false;

    for (;;) {
        skip_spaces();
        char op = *parse_ptr;
        if (op != '*' && op != '/' && op != '%') break;
        parse_ptr++;

        int64_t rhs = 0;
        if (!parse_factor(&rhs)) return false;

        if (op == '*') {
            *val *= rhs;
        } else if (op == '/') {
            if (rhs == 0) return false; /* Division by zero error */
            *val /= rhs;
        } else if (op == '%') {
            if (rhs == 0) return false;
            *val %= rhs;
        }
    }
    return true;
}

static bool parse_expr(int64_t *val) {
    if (!parse_term(val)) return false;

    for (;;) {
        skip_spaces();
        char op = *parse_ptr;
        if (op != '+' && op != '-') break;
        parse_ptr++;

        int64_t rhs = 0;
        if (!parse_term(&rhs)) return false;

        if (op == '+') {
            *val += rhs;
        } else if (op == '-') {
            *val -= rhs;
        }
    }
    return true;
}

static bool evaluate_expr(const char *s, int64_t *out_val) {
    if (!s || !*s) return false;
    parse_ptr = s;
    if (!parse_expr(out_val)) return false;
    skip_spaces();
    return (*parse_ptr == '\0');
}

/* ========================================================================= */
/* 3. Universal Cursor for Calculator                                        */
/* ========================================================================= */

#define cursor_show(fb, x, y)   gmu_mouse_cursor_show((fb), (x), (y))
#define cursor_hide(fb)         gmu_mouse_cursor_hide((fb))


/* ========================================================================= */
/* 4. Calculator GUI Rendering                                              */
/* ========================================================================= */

static const char *btn_labels[5][4] = {
    {"C",  "(",  ")",  "/"},
    {"7",  "8",  "9",  "*"},
    {"4",  "5",  "6",  "-"},
    {"1",  "2",  "3",  "+"},
    {"0",  "DEL", "%", "="}
};

static void draw_calculator(gmu_framebuffer_t *fb, const char *highlight_label) {
    const gmu_theme_t *th = gmu_theme_get();

    uint64_t win_w = 380;
    uint64_t win_h = 490;
    uint64_t win_x = (fb->width - win_w) / 2;
    uint64_t win_y = (fb->height - win_h) / 2 - 10;

    /* Window Frame */
    gmu_draw_shadow(fb, win_x, win_y, win_w, win_h, 8);
    gmu_draw_window_frame(fb, win_x, win_y, win_w, win_h, "GMU CALCULATOR",
                          th->surface_accent, th->surface, th->border);

    /* Display Screen Area */
    uint64_t disp_x = win_x + 16;
    uint64_t disp_y = win_y + 44;
    uint64_t disp_w = win_w - 32;
    uint64_t disp_h = 80;

    gmu_fill_rect(fb, disp_x, disp_y, disp_w, disp_h, th->surface_dark);
    gmu_draw_rect(fb, disp_x, disp_y, disp_w, disp_h, th->border);

    /* Top line: History or pending equation */
    if (history_str[0]) {
        gmu_draw_text(fb, disp_x + 12, disp_y + 10, history_str, th->text_secondary, th->surface_dark);
    }

    /* Main Digital Display */
    const char *main_disp = (expr_len > 0) ? expression : result_str;
    uint64_t dlen = 0;
    const char *dp = main_disp;
    while (*dp++) dlen++;

    uint64_t font_scale = 2;
    uint64_t text_w = dlen * GMU_FONT_WIDTH * font_scale;
    uint64_t text_x = (text_w < disp_w - 24) ? (disp_x + disp_w - 12 - text_w) : (disp_x + 12);

    gmu_draw_text_scaled(fb, text_x, disp_y + 38, main_disp, th->text_primary, th->surface_dark, font_scale, false);

    /* Buttons layout: 4 columns x 5 rows */
    uint64_t grid_x = disp_x;
    uint64_t grid_y = disp_y + disp_h + 16;
    uint64_t btn_w = 76;
    uint64_t btn_h = 54;
    uint64_t gap = 12;

    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 4; c++) {
            uint64_t bx = grid_x + c * (btn_w + gap);
            uint64_t by = grid_y + r * (btn_h + gap);
            const char *label = btn_labels[r][c];

            bool is_op = (c == 3) || (r == 0 && c > 0) || (r == 4 && c == 2);
            bool is_eq = (r == 4 && c == 3);
            bool is_clr = (r == 0 && c == 0);
            bool is_del = (r == 4 && c == 1);

            uint32_t bg_col = th->surface_accent;
            uint32_t text_col = th->text_primary;

            if (is_eq) {
                bg_col = th->accent;
                text_col = GMU_WHITE;
            } else if (is_clr) {
                bg_col = th->chiclet_red;
                text_col = GMU_WHITE;
            } else if (is_del) {
                bg_col = th->chiclet_yellow;
                text_col = 0x000F172A;
            } else if (is_op) {
                bg_col = th->surface_dark;
                text_col = th->accent;
            }

            /* Highlight on click or press */
            if (highlight_label && (highlight_label[0] == label[0])) {
                bg_col = th->accent_hover;
                text_col = GMU_WHITE;
            }

            gmu_fill_rect(fb, bx, by, btn_w, btn_h, bg_col);
            gmu_draw_rect(fb, bx, by, btn_w, btn_h, th->border);

            /* Center label inside button */
            uint64_t llen = 0;
            const char *lp = label;
            while (*lp++) llen++;
            uint64_t lx = bx + (btn_w - llen * GMU_FONT_WIDTH * 2) / 2;
            uint64_t ly = by + (btn_h - GMU_FONT_HEIGHT * 2) / 2;
            gmu_draw_text_scaled(fb, lx, ly, label, text_col, bg_col, 2, false);
        }
    }
}

/* ========================================================================= */
/* 5. Main Calculator Event Loop (Keyboard + Full Mouse Support)             */
/* ========================================================================= */

void gmu_calculator_run(gmu_framebuffer_t *fb) {
    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);
    uint8_t prev_buttons = 0;

    expression[0] = '\0';
    expr_len = 0;
    result_str[0] = '0';
    result_str[1] = '\0';
    history_str[0] = '\0';

    draw_calculator(fb, 0);

    if (mouse_ready) {
        cursor_show(fb, mouse.x, mouse.y);
    }

    uint64_t win_w = 380;
    uint64_t win_h = 490;
    uint64_t win_x = (fb->width - win_w) / 2;
    uint64_t win_y = (fb->height - win_h) / 2 - 10;
    uint64_t disp_h = 80;
    uint64_t grid_x = win_x + 16;
    uint64_t grid_y = win_y + 44 + disp_h + 16;
    uint64_t btn_w = 76;
    uint64_t btn_h = 54;
    uint64_t gap = 12;

    int32_t last_mx = -100;
    int32_t last_my = -100;

    for (;;) {
        const char *trigger_label = 0;
        bool do_action = false;
        char action_char = 0;

        /* 1. Mouse Input Handling */
        if (mouse_ready && gmu_mouse_poll(&mouse)) {
            bool left_click = (mouse.buttons & 1) && !(prev_buttons & 1);
            prev_buttons = mouse.buttons;

            if (mouse.x != last_mx || mouse.y != last_my) {
                cursor_hide(fb);
                cursor_show(fb, mouse.x, mouse.y);
                last_mx = mouse.x;
                last_my = mouse.y;
            }

            if (left_click) {
                /* Click Red Close Button chiclet */
                if (mouse.x >= (int32_t)(win_x + 10) && mouse.x <= (int32_t)(win_x + 24) &&
                    mouse.y >= (int32_t)(win_y + 10) && mouse.y <= (int32_t)(win_y + 24)) {
                    cursor_hide(fb);
                    break;
                }

                /* Click within 4x5 Button Grid */
                if (mouse.x >= (int32_t)grid_x && mouse.y >= (int32_t)grid_y) {
                    int32_t rx = mouse.x - (int32_t)grid_x;
                    int32_t ry = mouse.y - (int32_t)grid_y;
                    int c = rx / (int32_t)(btn_w + gap);
                    int r = ry / (int32_t)(btn_h + gap);
                    int ox = rx % (int32_t)(btn_w + gap);
                    int oy = ry % (int32_t)(btn_h + gap);

                    if (c >= 0 && c < 4 && r >= 0 && r < 5 &&
                        ox < (int32_t)btn_w && oy < (int32_t)btn_h) {
                        trigger_label = btn_labels[r][c];
                        do_action = true;
                    }
                }
            }
        }

        /* 2. Keyboard Input Handling */
        uint16_t key = gmu_keyboard_read_key();
        if (key != 0) {
            if (key == GMU_KEY_ESCAPE) {
                if (expr_len > 0) {
                    trigger_label = "C";
                    do_action = true;
                } else {
                    cursor_hide(fb);
                    break; /* Exit to desktop */
                }
            } else if (key == 'c' || key == 'C') {
                trigger_label = "C";
                do_action = true;
            } else if (key == GMU_KEY_BACKSPACE || key == '\b' || key == GMU_KEY_DELETE) {
                trigger_label = "DEL";
                do_action = true;
            } else if (key == '\n' || key == GMU_KEY_ENTER || key == '=') {
                trigger_label = "=";
                do_action = true;
            } else if ((key >= '0' && key <= '9') ||
                       key == '+' || key == '-' || key == '*' ||
                       key == '/' || key == '%' || key == '(' || key == ')') {
                action_char = (char)key;
                do_action = true;
            }
        }

        /* 3. Execute Calculator Action */
        if (do_action) {
            cursor_hide(fb);

            if (trigger_label) {
                if (trigger_label[0] == 'C' && trigger_label[1] == '\0') {
                    expression[0] = '\0';
                    expr_len = 0;
                    result_str[0] = '0';
                    result_str[1] = '\0';
                    history_str[0] = '\0';
                } else if (trigger_label[0] == 'D' && trigger_label[1] == 'E') {
                    /* DEL / Backspace */
                    if (expr_len > 0) {
                        expr_len--;
                        expression[expr_len] = '\0';
                    }
                } else if (trigger_label[0] == '=') {
                    /* Evaluate */
                    if (expr_len > 0) {
                        int64_t val = 0;
                        if (evaluate_expr(expression, &val)) {
                            /* Copy to history */
                            int hp = 0;
                            while (expression[hp] && hp < 40) {
                                history_str[hp] = expression[hp];
                                hp++;
                            }
                            history_str[hp++] = ' ';
                            history_str[hp++] = '=';
                            history_str[hp] = '\0';

                            format_num(val, result_str);
                            expr_len = 0;
                            expression[0] = '\0';
                        } else {
                            result_str[0] = 'E';
                            result_str[1] = 'R';
                            result_str[2] = 'R';
                            result_str[3] = '\0';
                            expr_len = 0;
                            expression[0] = '\0';
                        }
                    }
                } else {
                    /* Clicked operator or number button */
                    char ch = trigger_label[0];

                    /* Automatic chaining: if operator pressed right after result, use result as LHS */
                    if (expr_len == 0 && (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%')) {
                        if (result_str[0] != 'E' && result_str[0] != '\0') {
                            const char *rp = result_str;
                            while (*rp && expr_len < CALC_MAX - 2) {
                                expression[expr_len++] = *rp++;
                            }
                        }
                    }

                    if (expr_len < CALC_MAX - 1) {
                        expression[expr_len++] = ch;
                        expression[expr_len] = '\0';
                    }
                }
            } else if (action_char != 0) {
                /* Typed character from keyboard */
                if (expr_len == 0 && (action_char == '+' || action_char == '-' ||
                                      action_char == '*' || action_char == '/' || action_char == '%')) {
                    if (result_str[0] != 'E' && result_str[0] != '\0') {
                        const char *rp = result_str;
                        while (*rp && expr_len < CALC_MAX - 2) {
                            expression[expr_len++] = *rp++;
                        }
                    }
                }

                if (expr_len < CALC_MAX - 1) {
                    expression[expr_len++] = action_char;
                    expression[expr_len] = '\0';
                }
            }

            draw_calculator(fb, trigger_label);
            if (mouse_ready) {
                cursor_show(fb, mouse.x, mouse.y);
            }
        }

        __asm__ volatile ("pause");
    }
}

void gmu_calculator_register(void) {
    const gmu_application_t app = {
        .id = GMU_APP_CALCULATOR,
        .name = "Calculator",
        .run = gmu_calculator_run
    };
    (void)gmu_app_register(&app);
}
