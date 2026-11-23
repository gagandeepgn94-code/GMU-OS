#include <gmu/font.h>

static const uint8_t font_letters[26][7] = {
    {14,17,17,31,17,17,17}, // A
    {30,17,17,30,17,17,30}, // B
    {14,17,16,16,16,17,14}, // C
    {30,17,17,17,17,17,30}, // D
    {31,16,16,30,16,16,31}, // E
    {31,16,16,30,16,16,16}, // F
    {14,17,16,23,17,17,15}, // G
    {17,17,17,31,17,17,17}, // H
    {14,4,4,4,4,4,14},      // I
    {7,2,2,2,18,18,12},     // J
    {17,18,20,24,20,18,17}, // K
    {16,16,16,16,16,16,31}, // L
    {17,27,21,21,17,17,17}, // M
    {17,25,21,19,17,17,17}, // N
    {14,17,17,17,17,17,14}, // O
    {30,17,17,30,16,16,16}, // P
    {14,17,17,17,21,18,13}, // Q
    {30,17,17,30,20,18,17}, // R
    {15,16,16,14,1,1,30},   // S
    {31,4,4,4,4,4,4},       // T
    {17,17,17,17,17,17,14}, // U
    {17,17,17,17,17,10,4},  // V
    {17,17,17,21,21,21,10}, // W
    {17,17,10,4,10,17,17},  // X
    {17,17,10,4,4,4,4},     // Y
    {31,1,2,4,8,16,31},     // Z
};

static const uint8_t font_digits[10][7] = {
    {14,17,19,21,25,17,14}, // 0
    {4,12,4,4,4,4,14},      // 1
    {14,17,1,2,4,8,31},     // 2
    {30,1,1,14,1,1,30},     // 3
    {2,6,10,18,31,2,2},     // 4
    {31,16,16,30,1,1,30},   // 5
    {14,16,16,30,17,17,14}, // 6
    {31,1,2,4,8,8,8},       // 7
    {14,17,17,14,17,17,14}, // 8
    {14,17,17,15,1,1,14},   // 9
};

static const uint8_t glyph_space[7] = {0,0,0,0,0,0,0};

static const uint8_t *get_glyph(char c) {
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c >= 'A' && c <= 'Z') return font_letters[c - 'A'];
    if (c >= '0' && c <= '9') return font_digits[c - '0'];

    static const uint8_t s_excl[7]   = {4,4,4,4,4,0,4};
    static const uint8_t s_quote[7]  = {10,10,10,0,0,0,0};
    static const uint8_t s_hash[7]   = {10,31,10,10,31,10,0};
    static const uint8_t s_dollar[7] = {4,15,20,14,5,30,4};
    static const uint8_t s_pct[7]    = {24,25,2,4,8,19,3};
    static const uint8_t s_amp[7]    = {12,18,20,8,21,18,13};
    static const uint8_t s_apos[7]   = {4,4,8,0,0,0,0};
    static const uint8_t s_lparen[7] = {2,4,8,8,8,4,2};
    static const uint8_t s_rparen[7] = {8,4,2,2,2,4,8};
    static const uint8_t s_star[7]   = {0,4,21,14,21,4,0};
    static const uint8_t s_plus[7]   = {0,4,4,31,4,4,0};
    static const uint8_t s_comma[7]  = {0,0,0,0,4,4,8};
    static const uint8_t s_minus[7]  = {0,0,0,31,0,0,0};
    static const uint8_t s_dot[7]    = {0,0,0,0,0,12,12};
    static const uint8_t s_slash[7]  = {1,2,2,4,8,8,16};
    static const uint8_t s_colon[7]  = {0,12,12,0,12,12,0};
    static const uint8_t s_scolon[7] = {0,12,12,0,12,8,16};
    static const uint8_t s_lt[7]     = {2,4,8,16,8,4,2};
    static const uint8_t s_eq[7]     = {0,31,0,31,0,0,0};
    static const uint8_t s_gt[7]     = {8,4,2,1,2,4,8};
    static const uint8_t s_quest[7]  = {14,17,1,2,4,0,4};
    static const uint8_t s_at[7]     = {14,17,21,21,22,16,14};
    static const uint8_t s_lbrack[7] = {14,8,8,8,8,8,14};
    static const uint8_t s_bslash[7] = {16,8,8,4,2,2,1};
    static const uint8_t s_rbrack[7] = {14,2,2,2,2,2,14};
    static const uint8_t s_caret[7]  = {4,10,17,0,0,0,0};
    static const uint8_t s_under[7]  = {0,0,0,0,0,0,31};
    static const uint8_t s_backt[7]  = {8,4,2,0,0,0,0};
    static const uint8_t s_lbrace[7] = {2,4,4,8,4,4,2};
    static const uint8_t s_pipe[7]   = {4,4,4,4,4,4,4};
    static const uint8_t s_rbrace[7] = {8,4,4,2,4,4,8};
    static const uint8_t s_tilde[7]  = {0,13,22,0,0,0,0};

    switch (c) {
        case '!': return s_excl;
        case '"': return s_quote;
        case '#': return s_hash;
        case '$': return s_dollar;
        case '%': return s_pct;
        case '&': return s_amp;
        case '\'': return s_apos;
        case '(': return s_lparen;
        case ')': return s_rparen;
        case '*': return s_star;
        case '+': return s_plus;
        case ',': return s_comma;
        case '-': return s_minus;
        case '.': return s_dot;
        case '/': return s_slash;
        case ':': return s_colon;
        case ';': return s_scolon;
        case '<': return s_lt;
        case '=': return s_eq;
        case '>': return s_gt;
        case '?': return s_quest;
        case '@': return s_at;
        case '[': return s_lbrack;
        case '\\': return s_bslash;
        case ']': return s_rbrack;
        case '^': return s_caret;
        case '_': return s_under;
        case '`': return s_backt;
        case '{': return s_lbrace;
        case '|': return s_pipe;
        case '}': return s_rbrace;
        case '~': return s_tilde;
        default: return glyph_space;
    }
}

void gmu_draw_char(gmu_framebuffer_t *fb,
                    uint64_t x, uint64_t y, char c,
                    uint32_t fg, uint32_t bg) {
    const uint8_t *glyph = get_glyph(c);

    for (uint64_t row = 0; row < 7; row++) {
        for (uint64_t col = 0; col < 5; col++) {
            uint32_t color =
                (glyph[row] & (1 << (4 - col))) ? fg : bg;

            gmu_fb_put_pixel(fb, x + col, y + row, color);
        }
    }
}

void gmu_draw_text(gmu_framebuffer_t *fb,
                    uint64_t x, uint64_t y,
                    const char *text,
                    uint32_t fg, uint32_t bg) {
    if (!fb || !text)
        return;

    uint64_t cursor_x = x;

    while (*text) {
        gmu_draw_char(fb, cursor_x, y, *text, fg, bg);
        cursor_x += GMU_FONT_WIDTH;
        text++;
    }
}

void gmu_draw_text_transparent(gmu_framebuffer_t *fb,
                               uint64_t x, uint64_t y,
                               const char *text,
                               uint32_t fg) {
    if (!fb || !text) return;

    uint64_t cursor_x = x;
    while (*text) {
        const uint8_t *glyph = get_glyph(*text);
        for (uint64_t row = 0; row < 7; row++) {
            for (uint64_t col = 0; col < 5; col++) {
                if (glyph[row] & (1 << (4 - col))) {
                    gmu_fb_put_pixel(fb, cursor_x + col, y + row, fg);
                }
            }
        }
        cursor_x += GMU_FONT_WIDTH;
        text++;
    }
}

void gmu_draw_text_scaled(gmu_framebuffer_t *fb,
                          uint64_t x, uint64_t y,
                          const char *text,
                          uint32_t fg, uint32_t bg,
                          uint32_t scale,
                          bool transparent) {
    if (!fb || !text || scale == 0) return;

    uint64_t cursor_x = x;
    while (*text) {
        const uint8_t *glyph = get_glyph(*text);
        for (uint64_t row = 0; row < 7; row++) {
            for (uint64_t col = 0; col < 5; col++) {
                bool on = (glyph[row] & (1 << (4 - col))) != 0;
                if (on) {
                    for (uint64_t sy = 0; sy < scale; sy++) {
                        for (uint64_t sx = 0; sx < scale; sx++) {
                            gmu_fb_put_pixel(fb, cursor_x + col * scale + sx,
                                             y + row * scale + sy, fg);
                        }
                    }
                } else if (!transparent) {
                    for (uint64_t sy = 0; sy < scale; sy++) {
                        for (uint64_t sx = 0; sx < scale; sx++) {
                            gmu_fb_put_pixel(fb, cursor_x + col * scale + sx,
                                             y + row * scale + sy, bg);
                        }
                    }
                }
            }
        }
        cursor_x += (GMU_FONT_WIDTH * scale);
        text++;
    }
}
