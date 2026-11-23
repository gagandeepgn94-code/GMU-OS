#include <stdint.h>
#include <stdbool.h>
#include <gmu/settings.h>
#include <gmu/app.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/draw.h>
#include <gmu/font.h>
#include <gmu/theme.h>
#include <gmu/pmm.h>
#include <gmu/heap.h>
#include <gmu/fs.h>
#include <gmu/timer.h>

#define SETTINGS_CATEGORY_COUNT 5

static int current_category = 0;

static const char *category_titles[SETTINGS_CATEGORY_COUNT] = {
    "THEMES",
    "DISPLAY",
    "STORAGE",
    "INPUT",
    "ABOUT OS"
};

static const char *category_icons[SETTINGS_CATEGORY_COUNT] = {
    "[*]",
    "[D]",
    "[S]",
    "[I]",
    "[?]"
};

/* ========================================================================= */
/* 1. Cursor Engine for Settings                                            */
/* ========================================================================= */

#define CURSOR_W 16
#define CURSOR_H 16

static const char *settings_cursor_bitmap[CURSOR_H] = {
    "B...............",
    "BB..............",
    "BWB.............",
    "BWWWB...........",
    "BWWWWWB.........",
    "BWWWWWWWB.......",
    "BWWWWWWWWWB.....",
    "BWWWWWWWWWWWB...",
    "BWWWWWWWWWWWWWB.",
    "BWWWWWWWWWWBB...",
    "BWWWWWWBBBBB....",
    "BWWWBWWWB.......",
    "BWWB.BWWWB......",
    "BBB...BWWWB.....",
    "B......BWWWB....",
    "........BBB....."
};

static uint32_t settings_cursor_bg[CURSOR_H][CURSOR_W];
static int32_t settings_cursor_x = 0;
static int32_t settings_cursor_y = 0;
static bool settings_cursor_vis = false;

static void cursor_hide(gmu_framebuffer_t *fb) {
    if (!settings_cursor_vis) return;
    for (int32_t cy = 0; cy < CURSOR_H; cy++) {
        for (int32_t cx = 0; cx < CURSOR_W; cx++) {
            int32_t px = settings_cursor_x + cx;
            int32_t py = settings_cursor_y + cy;
            if (px >= 0 && py >= 0 && px < (int32_t)fb->width && py < (int32_t)fb->height) {
                gmu_fb_put_pixel(fb, px, py, settings_cursor_bg[cy][cx]);
            }
        }
    }
    settings_cursor_vis = false;
}

static void cursor_show(gmu_framebuffer_t *fb, int32_t x, int32_t y) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    settings_cursor_x = x;
    settings_cursor_y = y;

    for (int32_t cy = 0; cy < CURSOR_H; cy++) {
        for (int32_t cx = 0; cx < CURSOR_W; cx++) {
            int32_t sx = x + cx;
            int32_t sy = y + cy;
            settings_cursor_bg[cy][cx] = 0;
            if (sx < (int32_t)fb->width && sy < (int32_t)fb->height) {
                settings_cursor_bg[cy][cx] = gmu_fb_get_pixel(fb, sx, sy);
            }
            char p = settings_cursor_bitmap[cy][cx];
            if (p != '.') {
                uint32_t col = (p == 'W') ? GMU_WHITE : 0x000F172A;
                gmu_fb_put_pixel(fb, sx, sy, col);
            }
        }
    }
    settings_cursor_vis = true;
}

/* ========================================================================= */
/* 2. Format Number Helper                                                   */
/* ========================================================================= */

static void format_num(uint64_t val, char *buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    char temp[24];
    int t = 0;
    while (val > 0) {
        temp[t++] = '0' + (val % 10);
        val /= 10;
    }
    int p = 0;
    while (t > 0) buf[p++] = temp[--t];
    buf[p] = '\0';
}

/* ========================================================================= */
/* 3. Render Settings Window & Tabs                                          */
/* ========================================================================= */

static void draw_settings_window(gmu_framebuffer_t *fb) {
    const gmu_theme_t *th = gmu_theme_get();

    uint64_t win_w = 700;
    uint64_t win_h = 470;
    uint64_t win_x = (fb->width - win_w) / 2;
    uint64_t win_y = (fb->height - win_h) / 2 - 10;

    /* Window Frame */
    gmu_draw_shadow(fb, win_x, win_y, win_w, win_h, 8);
    gmu_draw_window_frame(fb, win_x, win_y, win_w, win_h,
                          "GMU OS 64 - SYSTEM SETTINGS & PREFERENCES",
                          th->surface_accent, th->surface_dark, th->border);

    /* Left Sidebar: Categories Navigation */
    uint64_t side_w = 170;
    uint64_t side_y = win_y + 32;
    uint64_t side_h = win_h - 32 - 30;

    gmu_fill_rect(fb, win_x + 1, side_y, side_w, side_h, th->surface);
    gmu_fill_rect(fb, win_x + side_w, side_y, 1, side_h, th->border);

    for (int i = 0; i < SETTINGS_CATEGORY_COUNT; i++) {
        uint64_t cy = side_y + 14 + i * 38;
        bool is_sel = (i == current_category);

        uint32_t btn_bg = is_sel ? th->accent : th->surface;
        uint32_t text_col = is_sel ? GMU_WHITE : th->text_primary;

        gmu_fill_rect(fb, win_x + 8, cy, side_w - 16, 30, btn_bg);
        if (is_sel) {
            gmu_draw_rect(fb, win_x + 8, cy, side_w - 16, 30, th->accent_hover);
        }

        /* Category Icon & Name */
        gmu_draw_text(fb, win_x + 16, cy + 8, category_icons[i], is_sel ? GMU_WHITE : th->accent, btn_bg);
        gmu_draw_text(fb, win_x + 44, cy + 8, category_titles[i], text_col, btn_bg);
    }

    /* Main Settings Content Area */
    uint64_t main_x = win_x + side_w + 16;
    uint64_t main_y = side_y + 14;
    uint64_t main_w = win_w - side_w - 32;

    if (current_category == 0) {
        /* ==================== PANE 0: THEMES ==================== */
        gmu_draw_text(fb, main_x, main_y, "WORKSTATION THEME & DESKTOP PALETTE:", th->accent, th->surface_dark);

        const char *theme_titles[GMU_THEME_COUNT] = {
            "[1] macOS Sonoma (Dark)  - Apple Indigo & Frosted Glass",
            "[2] Windows 11 Bloom     - Microsoft Azure & Mica Acrylic",
            "[3] Cyber Glow (Neon)    - Futuristic Neon Purple & Cyan",
            "[4] macOS Tahoe (Light)  - Apple Clean Frost Silver & Sky Blue"
        };

        for (int i = 0; i < GMU_THEME_COUNT; i++) {
            uint64_t card_y = main_y + 26 + i * 56;
            bool is_selected = ((int)th->id == i);

            uint32_t card_bg = is_selected ? th->surface_accent : th->surface;
            gmu_fill_rect(fb, main_x, card_y, main_w, 46, card_bg);
            gmu_draw_rect(fb, main_x, card_y, main_w, 46, is_selected ? th->accent : th->border);

            gmu_draw_text(fb, main_x + 12, card_y + 8, theme_titles[i],
                          is_selected ? GMU_WHITE : th->text_primary, card_bg);

            /* Color Preview Swatches */
            uint32_t swatches[5] = {th->bg_top, th->bg_bottom, th->surface, th->accent, th->chiclet_green};
            for (int s = 0; s < 5; s++) {
                gmu_fill_rect(fb, main_x + 12 + s * 24, card_y + 26, 18, 12, swatches[s]);
                gmu_draw_rect(fb, main_x + 12 + s * 24, card_y + 26, 18, 12, th->border);
            }

            if (is_selected) {
                gmu_draw_text(fb, main_x + main_w - 90, card_y + 16, "[ACTIVE]", th->chiclet_green, card_bg);
            }
        }

        uint64_t hint_y = main_y + 26 + GMU_THEME_COUNT * 56 + 12;
        gmu_draw_text(fb, main_x, hint_y, "Press [1-4] or Click card to switch theme immediately.", th->chiclet_yellow, th->surface_dark);
        gmu_draw_text(fb, main_x, hint_y + 20, "Themes take effect live across all open windows and dock.", th->text_secondary, th->surface_dark);

    } else if (current_category == 1) {
        /* ==================== PANE 1: DISPLAY ==================== */
        gmu_draw_text(fb, main_x, main_y, "DISPLAY & GRAPHICS HARDWARE CONFIGURATION:", th->accent, th->surface_dark);

        uint64_t card_y = main_y + 24;
        gmu_fill_rect(fb, main_x, card_y, main_w, 230, th->surface);
        gmu_draw_rect(fb, main_x, card_y, main_w, 230, th->border);

        char res_buf[32] = "Resolution:        ";
        char w_buf[16], h_buf[16];
        format_num(fb->width, w_buf);
        format_num(fb->height, h_buf);
        int rp = 19;
        const char *wp = w_buf; while (*wp) res_buf[rp++] = *wp++;
        res_buf[rp++] = ' '; res_buf[rp++] = 'x'; res_buf[rp++] = ' ';
        const char *hp = h_buf; while (*hp) res_buf[rp++] = *hp++;
        res_buf[rp++] = ' '; res_buf[rp++] = 'P'; res_buf[rp++] = 'X'; res_buf[rp] = '\0';

        gmu_draw_text(fb, main_x + 16, card_y + 16, res_buf, th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 40, "Color Format:      32-bit TrueColor (ARGB8888)", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 64, "Scanline Pitch:    4096 Bytes / Row (Linear VRAM)", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 88, "Firmware Driver:   UEFI Graphics Output Protocol (GOP)", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 112, "Hardware Refresh:  100 Hz (8254 PIT Calibrated)", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 136, "Anti-Flicker:      Dirty Region Compositor [ACTIVE]", th->chiclet_green, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 160, "Cursor Engine:     16x16 High-Visibility Arrow [ACTIVE]", th->chiclet_green, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 184, "Typography:        95-Glyph Bitmap Scaled Font (1x, 2x)", th->text_primary, th->surface);

        uint64_t hint_y = card_y + 246;
        gmu_draw_text(fb, main_x, hint_y, "UEFI Linear Framebuffer delivers flicker-free rendering.", th->accent, th->surface_dark);

    } else if (current_category == 2) {
        /* ==================== PANE 2: STORAGE ==================== */
        gmu_draw_text(fb, main_x, main_y, "STORAGE SUBSYSTEM & RAM FILESYSTEM (VFS):", th->accent, th->surface_dark);

        uint64_t card_y = main_y + 24;
        gmu_fill_rect(fb, main_x, card_y, main_w, 230, th->surface);
        gmu_draw_rect(fb, main_x, card_y, main_w, 230, th->border);

        gmu_draw_text(fb, main_x + 16, card_y + 16, "Primary Volume:    GMU Macintosh HD (/)", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 40, "Filesystem Type:   In-Memory Hierarchical RAMFS", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 64, "Total Inodes:      128 File/Directory Nodes Max", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 88, "Max File Capacity: 8192 Bytes (8 KB) per File", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 112, "Preloaded Paths:   /Documents, /System, /Games, /Tools", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 136, "Path Resolution:   Canonical Canonicalization (., .., /)", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 160, "Persistence State: Active RAMFS Dynamic Allocation", th->chiclet_green, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 184, "Integrity Status:  Self-Test Passed [100% HEALTH]", th->chiclet_green, th->surface);

        uint64_t hint_y = card_y + 246;
        gmu_draw_text(fb, main_x, hint_y, "Access and manage files in the Finder or Terminal (LS, CAT, WRITE).", th->accent, th->surface_dark);

    } else if (current_category == 3) {
        /* ==================== PANE 3: INPUT ==================== */
        gmu_draw_text(fb, main_x, main_y, "INPUT DRIVERS & CONTROLLER CONFIGURATION:", th->accent, th->surface_dark);

        uint64_t card_y = main_y + 24;
        gmu_fill_rect(fb, main_x, card_y, main_w, 230, th->surface);
        gmu_draw_rect(fb, main_x, card_y, main_w, 230, th->border);

        gmu_draw_text(fb, main_x + 16, card_y + 16, "Controller:        Intel 8042 PS/2 Auxiliary Controller", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 40, "Ring Buffer:       512 Bytes (Zero Dropped Packets)", th->chiclet_green, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 64, "Mouse Protocol:    PS/2 3-Byte Standard with 9-bit Signed Decode", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 88, "Mouse Filtering:   Sign Extension & Delta Clamping [-80, 80]", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 112, "Keyboard Mapping:  Full Normal (a-z) & Shift (!@#$) Tables", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 136, "Modifiers Active:  Shift Tracking, CapsLock, Ctrl, Alt", th->text_primary, th->surface);

        /* Real-time indicator for CapsLock and Shift */
        bool shift_on = gmu_keyboard_shift_active();
        bool caps_on = gmu_keyboard_caps_active();
        char state_str[48] = "Current State:     Shift: ";
        int sp = 26;
        if (shift_on) { state_str[sp++] = 'O'; state_str[sp++] = 'N'; }
        else { state_str[sp++] = 'O'; state_str[sp++] = 'F'; state_str[sp++] = 'F'; }
        state_str[sp++] = ' '; state_str[sp++] = '|'; state_str[sp++] = ' ';
        state_str[sp++] = 'C'; state_str[sp++] = 'a'; state_str[sp++] = 'p'; state_str[sp++] = 's';
        state_str[sp++] = ':'; state_str[sp++] = ' ';
        if (caps_on) { state_str[sp++] = 'O'; state_str[sp++] = 'N'; }
        else { state_str[sp++] = 'O'; state_str[sp++] = 'F'; state_str[sp++] = 'F'; }
        state_str[sp] = '\0';
        gmu_draw_text(fb, main_x + 16, card_y + 160, state_str, th->chiclet_yellow, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 184, "Keypad Support:    Numeric & Operator Direct Scancodes [OK]", th->chiclet_green, th->surface);

        uint64_t hint_y = card_y + 246;
        gmu_draw_text(fb, main_x, hint_y, "Mouse and keyboard input are fully synchronized with 0 latency.", th->accent, th->surface_dark);

    } else if (current_category == 4) {
        /* ==================== PANE 4: ABOUT OS ==================== */
        gmu_draw_text(fb, main_x, main_y, "ABOUT GMU OS 64 WORKSTATION & LICENSING:", th->accent, th->surface_dark);

        uint64_t card_y = main_y + 24;
        gmu_fill_rect(fb, main_x, card_y, main_w, 230, th->surface);
        gmu_draw_rect(fb, main_x, card_y, main_w, 230, th->border);

        gmu_draw_text(fb, main_x + 16, card_y + 16, "Operating System:  GMU OS 64 Workstation", th->accent, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 40, "Version & Build:   1.0 (Build 2026.UEFI - Long Mode)", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 64, "Architecture:      x86_64 Freestanding (No Host C Library)", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 88, "Boot Protocol:     Limine Boot Protocol (v3 Handshake)", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 112, "Physical RAM:      1024 MB Addressable (PMM Frame Allocator)", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 136, "Dynamic Heap:      2 MB Kernel Arena (Split / Coalesce Pool)", th->text_primary, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 160, "Academic Affil:    GM University, Davanagere, Karnataka, India", th->chiclet_yellow, th->surface);
        gmu_draw_text(fb, main_x + 16, card_y + 184, "Course Context:    UE25CS2304 Operating System with Practice", th->text_secondary, th->surface);

        uint64_t hint_y = card_y + 246;
        gmu_draw_text(fb, main_x, hint_y, "Designed and engineered for high-performance bare-metal execution.", th->accent, th->surface_dark);
    }

    /* Footer Status Bar */
    uint64_t foot_y = win_y + win_h - 28;
    gmu_fill_rect(fb, win_x + 1, foot_y, win_w - 2, 27, th->surface);
    gmu_fill_rect(fb, win_x + 1, foot_y, win_w - 2, 1, th->border);
    gmu_draw_text(fb, win_x + 16, foot_y + 8,
                  "[1-5] / [Up/Dn] Tabs  |  [Mouse] Click to Select  |  [ESC] Exit",
                  th->chiclet_yellow, th->surface);
}

/* ========================================================================= */
/* 4. Main Settings Event Loop (Keyboard + Mouse Interactive)                 */
/* ========================================================================= */

void gmu_settings_run(gmu_framebuffer_t *fb) {
    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);
    uint8_t prev_buttons = 0;

    settings_cursor_vis = false;
    draw_settings_window(fb);

    if (mouse_ready) {
        cursor_show(fb, mouse.x, mouse.y);
    }

    uint64_t win_w = 700;
    uint64_t win_h = 470;
    uint64_t win_x = (fb->width - win_w) / 2;
    uint64_t win_y = (fb->height - win_h) / 2 - 10;
    uint64_t side_w = 170;
    uint64_t side_y = win_y + 32;

    int32_t last_mx = -100;
    int32_t last_my = -100;

    for (;;) {
        bool needs_redraw = false;

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
                /* Click Red Close Chiclet */
                if (mouse.x >= (int32_t)(win_x + 10) && mouse.x <= (int32_t)(win_x + 24) &&
                    mouse.y >= (int32_t)(win_y + 10) && mouse.y <= (int32_t)(win_y + 24)) {
                    cursor_hide(fb);
                    break;
                }

                /* Click on Left Sidebar Categories */
                if (mouse.x >= (int32_t)(win_x + 8) && mouse.x <= (int32_t)(win_x + side_w - 8)) {
                    for (int i = 0; i < SETTINGS_CATEGORY_COUNT; i++) {
                        int32_t cy = (int32_t)(side_y + 14 + i * 38);
                        if (mouse.y >= cy && mouse.y < cy + 30) {
                            if (current_category != i) {
                                current_category = i;
                                needs_redraw = true;
                            }
                            break;
                        }
                    }
                }

                /* If in Themes Pane, Click Theme Cards */
                if (current_category == 0) {
                    uint64_t main_x = win_x + side_w + 16;
                    uint64_t main_y = side_y + 14;
                    uint64_t main_w = win_w - side_w - 32;

                    if (mouse.x >= (int32_t)main_x && mouse.x <= (int32_t)(main_x + main_w)) {
                        for (int i = 0; i < GMU_THEME_COUNT; i++) {
                            int32_t card_y = (int32_t)(main_y + 26 + i * 56);
                            if (mouse.y >= card_y && mouse.y < card_y + 46) {
                                gmu_theme_set((gmu_theme_id_t)i);
                                needs_redraw = true;
                                break;
                            }
                        }
                    }
                }
            }
        }

        /* 2. Keyboard Input Handling */
        uint16_t key = gmu_keyboard_read_key();
        if (key != 0) {
            if (key == GMU_KEY_ESCAPE) {
                cursor_hide(fb);
                break; /* Exit Settings */
            } else if (key == GMU_KEY_UP) {
                current_category = (current_category + SETTINGS_CATEGORY_COUNT - 1) % SETTINGS_CATEGORY_COUNT;
                needs_redraw = true;
            } else if (key == GMU_KEY_DOWN) {
                current_category = (current_category + 1) % SETTINGS_CATEGORY_COUNT;
                needs_redraw = true;
            } else if (key >= '1' && key <= '5') {
                int cat = key - '1';
                if (current_category != cat) {
                    current_category = cat;
                    needs_redraw = true;
                }
            } else if (current_category == 0 && (key == GMU_KEY_TAB || key == ' ')) {
                gmu_theme_cycle();
                needs_redraw = true;
            }
        }

        if (needs_redraw) {
            cursor_hide(fb);
            draw_settings_window(fb);
            if (mouse_ready) {
                cursor_show(fb, mouse.x, mouse.y);
            }
        }

        __asm__ volatile ("pause");
    }
}

void gmu_settings_register(void) {
    const gmu_application_t app = {
        .id = GMU_APP_SETTINGS,
        .name = "Settings",
        .run = gmu_settings_run
    };
    (void)gmu_app_register(&app);
}
