#include <stdint.h>
#include <stdbool.h>
#include <gmu/desktop.h>
#include <gmu/draw.h>
#include <gmu/font.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/app.h>
#include <gmu/timer.h>
#include <gmu/pmm.h>
#include <gmu/heap.h>
#include <gmu/fs.h>
#include <gmu/theme.h>
#include <gmu/editor.h>

#define DOCK_APP_COUNT 10

static uint64_t selected_item = 0;
static bool menu_open = false;

/* ========================================================================= */
/* 1. High-Resolution macOS / Windows 11 Arrow Cursor (16x16)               */
/* ========================================================================= */

#define CURSOR_W 16
#define CURSOR_H 16

static const char *cursor_bitmap[CURSOR_H] = {
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

static uint32_t cursor_background[CURSOR_H][CURSOR_W];
static int32_t cursor_saved_x = 0;
static int32_t cursor_saved_y = 0;
static bool cursor_visible = false;
static int32_t last_mouse_x = -100;
static int32_t last_mouse_y = -100;

static void draw_mouse_cursor(gmu_framebuffer_t *fb, int32_t x, int32_t y) {
    for (int32_t cy = 0; cy < CURSOR_H; cy++) {
        for (int32_t cx = 0; cx < CURSOR_W; cx++) {
            char p = cursor_bitmap[cy][cx];
            if (p == '.') continue;

            int32_t px = x + cx;
            int32_t py = y + cy;

            if (px >= 0 && py >= 0 &&
                px < (int32_t)fb->width &&
                py < (int32_t)fb->height) {
                uint32_t col = (p == 'W') ? GMU_WHITE : 0x000F172A;
                gmu_fb_put_pixel(fb, px, py, col);
            }
        }
    }
}

static void cursor_hide(gmu_framebuffer_t *fb) {
    if (!cursor_visible) return;

    for (int32_t cy = 0; cy < CURSOR_H; cy++) {
        for (int32_t cx = 0; cx < CURSOR_W; cx++) {
            int32_t px = cursor_saved_x + cx;
            int32_t py = cursor_saved_y + cy;

            if (px >= 0 && py >= 0 &&
                px < (int32_t)fb->width &&
                py < (int32_t)fb->height) {
                gmu_fb_put_pixel(fb, px, py, cursor_background[cy][cx]);
            }
        }
    }
    cursor_visible = false;
}

static void cursor_show(gmu_framebuffer_t *fb, int32_t x, int32_t y) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    cursor_saved_x = x;
    cursor_saved_y = y;

    for (int32_t cy = 0; cy < CURSOR_H; cy++) {
        for (int32_t cx = 0; cx < CURSOR_W; cx++) {
            int32_t sx = x + cx;
            int32_t sy = y + cy;

            cursor_background[cy][cx] = 0;
            if (sx < (int32_t)fb->width && sy < (int32_t)fb->height) {
                cursor_background[cy][cx] = gmu_fb_get_pixel(fb, sx, sy);
            }
        }
    }

    draw_mouse_cursor(fb, x, y);
    cursor_visible = true;
}

/* ========================================================================= */
/* 2. String Formatting Helpers                                             */
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
/* 3. Localized Clock & Status Tray (Zero Full Redraw = ZERO FLICKER)        */
/* ========================================================================= */

static void update_clock_tray(gmu_framebuffer_t *fb) {
    const gmu_theme_t *th = gmu_theme_get();
    uint64_t w = fb->width;
    if (w < 450) return;

    uint64_t tray_w = 420;
    uint64_t tray_x = w - tray_w - 8;
    uint64_t tray_y = 2;
    uint64_t tray_h = 24;

    /* Clear tray area */
    gmu_fill_rect(fb, tray_x, tray_y, tray_w, tray_h, th->surface);

    /* 1. Wi-Fi Status */
    gmu_fill_rect(fb, tray_x + 4, tray_y + 4, 46, 16, th->surface_dark);
    gmu_draw_text(fb, tray_x + 8, tray_y + 8, "WI-FI", th->chiclet_green, th->surface_dark);

    /* 2. Audio Volume */
    gmu_draw_text(fb, tray_x + 58, tray_y + 8, "VOL 80%", th->text_secondary, th->surface);

    /* 3. RAM Badge */
    gmu_mem_stats_t mem = gmu_pmm_get_stats();
    uint64_t ram_mb = mem.used_memory / (1024 * 1024);
    char ram_str[24] = "RAM:";
    char rmb_buf[16];
    format_num(ram_mb, rmb_buf);
    int rp = 4; const char *rp_str = rmb_buf; while (*rp_str) ram_str[rp++] = *rp_str++;
    ram_str[rp++] = 'M'; ram_str[rp++] = 'B'; ram_str[rp] = '\0';
    gmu_draw_text(fb, tray_x + 124, tray_y + 8, ram_str, th->accent, th->surface);

    /* 4. CPU frequency */
    gmu_draw_text(fb, tray_x + 208, tray_y + 8, "100Hz", th->chiclet_green, th->surface);

    /* 5. Battery */
    gmu_draw_text(fb, tray_x + 260, tray_y + 8, "[AC 100%]", th->text_primary, th->surface);

    /* 6. Live Clock */
    uint64_t total_sec = gmu_timer_seconds();
    uint64_t hrs = total_sec / 3600;
    uint64_t mins = (total_sec % 3600) / 60;
    uint64_t secs = total_sec % 60;

    char clock_str[24];
    clock_str[0] = '0' + (hrs / 10);
    clock_str[1] = '0' + (hrs % 10);
    clock_str[2] = ':';
    clock_str[3] = '0' + (mins / 10);
    clock_str[4] = '0' + (mins % 10);
    clock_str[5] = ':';
    clock_str[6] = '0' + (secs / 10);
    clock_str[7] = '0' + (secs % 10);
    clock_str[8] = '\0';

    gmu_draw_text(fb, tray_x + 344, tray_y + 8, clock_str, th->chiclet_yellow, th->surface);
}

/* ========================================================================= */
/* 4. Desktop Custom Icon Drawings (32x32)                                  */
/* ========================================================================= */

static void draw_drive_icon(gmu_framebuffer_t *fb, uint64_t x, uint64_t y) {
    /* Silver NVMe drive chassis */
    gmu_fill_rect(fb, x, y, 32, 28, 0x0094A3B8);
    gmu_draw_rect(fb, x, y, 32, 28, 0x00E2E8F0);
    /* Drive tray slot */
    gmu_fill_rect(fb, x + 4, y + 18, 24, 4, 0x00334155);
    /* Blue power/activity LED */
    gmu_fill_rect(fb, x + 24, y + 6, 4, 4, 0x0038BDF8);
    /* Center metal plate */
    gmu_fill_rect(fb, x + 6, y + 4, 14, 10, 0x00CBD5E1);
}

static void draw_folder_icon(gmu_framebuffer_t *fb, uint64_t x, uint64_t y) {
    /* Folder tab */
    gmu_fill_rect(fb, x, y, 14, 6, 0x00D97706);
    /* Folder body */
    gmu_fill_rect(fb, x, y + 4, 32, 24, 0x00F59E0B);
    gmu_draw_rect(fb, x, y + 4, 32, 24, 0x00FBBF24);
    /* White document peeking out */
    gmu_fill_rect(fb, x + 6, y + 2, 18, 10, GMU_WHITE);
    gmu_fill_rect(fb, x + 8, y + 5, 12, 1, 0x0094A3B8);
    /* Folder front flap */
    gmu_fill_rect(fb, x, y + 10, 32, 18, 0x00D97706);
}

static void draw_arcade_icon(gmu_framebuffer_t *fb, uint64_t x, uint64_t y) {
    /* Game controller body */
    gmu_fill_rect(fb, x + 2, y + 4, 28, 22, 0x007C3AED);
    gmu_draw_rect(fb, x + 2, y + 4, 28, 22, 0x00A78BFA);
    /* D-pad on left */
    gmu_fill_rect(fb, x + 6, y + 12, 8, 4, 0x001E1B4B);
    gmu_fill_rect(fb, x + 8, y + 10, 4, 8, 0x001E1B4B);
    /* Action buttons on right */
    gmu_fill_rect(fb, x + 22, y + 9, 3, 3, 0x00EF4444);  /* Red */
    gmu_fill_rect(fb, x + 19, y + 13, 3, 3, 0x0010B981); /* Green */
    gmu_fill_rect(fb, x + 24, y + 15, 3, 3, 0x003B82F6); /* Blue */
}

static void draw_notepad_icon(gmu_framebuffer_t *fb, uint64_t x, uint64_t y) {
    /* White sheet with folded corner */
    gmu_fill_rect(fb, x + 4, y, 24, 30, GMU_WHITE);
    gmu_draw_rect(fb, x + 4, y, 24, 30, 0x00CBD5E1);
    /* Folded dog-ear corner */
    gmu_fill_rect(fb, x + 20, y, 8, 8, 0x0094A3B8);
    /* Text lines */
    gmu_fill_rect(fb, x + 8, y + 8, 10, 2, 0x000284C7);
    gmu_fill_rect(fb, x + 8, y + 13, 16, 2, 0x0064748B);
    gmu_fill_rect(fb, x + 8, y + 18, 16, 2, 0x0064748B);
    gmu_fill_rect(fb, x + 8, y + 23, 12, 2, 0x0064748B);
}

static void draw_trash_icon(gmu_framebuffer_t *fb, uint64_t x, uint64_t y) {
    /* Wastebasket mesh */
    gmu_fill_rect(fb, x + 4, y + 6, 24, 22, 0x00475569);
    gmu_draw_rect(fb, x + 4, y + 6, 24, 22, 0x0094A3B8);
    /* Rim */
    gmu_fill_rect(fb, x + 2, y + 4, 28, 3, 0x00CBD5E1);
    /* Vertical slats */
    gmu_fill_rect(fb, x + 9, y + 9, 2, 16, 0x001E293B);
    gmu_fill_rect(fb, x + 15, y + 9, 2, 16, 0x001E293B);
    gmu_fill_rect(fb, x + 21, y + 9, 2, 16, 0x001E293B);
}

/* ========================================================================= */
/* 5. Floating Glass Dock (macOS Style)                                     */
/* ========================================================================= */

static void draw_dock(gmu_framebuffer_t *fb) {
    const gmu_theme_t *th = gmu_theme_get();
    uint64_t w = fb->width;
    uint64_t h = fb->height;

    static const char *labels[DOCK_APP_COUNT] = {
        "Finder", "Terminal", "Notes", "Paint", "Calc",
        "Calendar", "Monitor", "HexView", "Arcade", "Settings"
    };

    uint64_t item_w = 68;
    uint64_t gap = 8;
    uint64_t dock_w = DOCK_APP_COUNT * item_w + (DOCK_APP_COUNT - 1) * gap + 24;
    uint64_t dock_x = (w - dock_w) / 2;
    uint64_t dock_y = h - 66;
    uint64_t dock_h = 54;

    /* Frosted dock container with deep shadow */
    gmu_draw_shadow(fb, dock_x, dock_y, dock_w, dock_h, 8);
    gmu_fill_rect(fb, dock_x, dock_y, dock_w, dock_h, th->dock_bg);
    gmu_draw_rect(fb, dock_x, dock_y, dock_w, dock_h, th->border);

    for (uint64_t i = 0; i < DOCK_APP_COUNT; i++) {
        uint64_t ix = dock_x + 12 + i * (item_w + gap);
        bool is_sel = (i == selected_item);

        uint32_t btn_bg = is_sel ? th->accent : th->surface_dark;
        uint32_t text_col = is_sel ? GMU_WHITE : th->text_primary;

        /* Icon tile container */
        gmu_fill_rect(fb, ix, dock_y + 6, item_w, 38, btn_bg);
        gmu_draw_rect(fb, ix, dock_y + 6, item_w, 38, is_sel ? th->accent_hover : th->border);

        /* Mini visual glyph for each dock icon */
        uint64_t mx = ix + item_w / 2 - 8;
        uint64_t my = dock_y + 10;
        if (i == 0) {
            /* Finder face */
            gmu_fill_rect(fb, mx, my, 16, 12, 0x000284C7);
            gmu_fill_rect(fb, mx + 3, my + 3, 3, 3, GMU_WHITE);
            gmu_fill_rect(fb, mx + 10, my + 3, 3, 3, GMU_WHITE);
            gmu_fill_rect(fb, mx + 5, my + 8, 6, 2, GMU_WHITE);
        } else if (i == 1) {
            /* Terminal */
            gmu_fill_rect(fb, mx, my, 16, 12, 0x000F172A);
            gmu_draw_text(fb, mx + 2, my + 3, ">_", 0x0022C55E, 0x000F172A);
        } else if (i == 2) {
            /* Notes */
            gmu_fill_rect(fb, mx, my, 16, 12, 0x00F59E0B);
            gmu_fill_rect(fb, mx + 3, my + 3, 10, 2, GMU_WHITE);
            gmu_fill_rect(fb, mx + 3, my + 7, 8, 2, GMU_WHITE);
        } else if (i == 3) {
            /* Paint */
            gmu_fill_rect(fb, mx, my, 16, 12, 0x00EC4899);
            gmu_fill_circle(fb, mx + 4, my + 4, 2, GMU_WHITE);
            gmu_fill_circle(fb, mx + 11, my + 4, 2, 0x00FBBF24);
            gmu_fill_circle(fb, mx + 8, my + 8, 2, 0x0038BDF8);
        } else if (i == 4) {
            /* Calculator */
            gmu_fill_rect(fb, mx, my, 16, 12, 0x00F97316);
            gmu_draw_text(fb, mx + 2, my + 3, "+=", GMU_WHITE, 0x00F97316);
        } else if (i == 5) {
            /* Calendar */
            gmu_fill_rect(fb, mx, my, 16, 12, GMU_WHITE);
            gmu_fill_rect(fb, mx, my, 16, 3, 0x00EF4444);
            gmu_draw_text(fb, mx + 3, my + 4, "23", 0x001E293B, GMU_WHITE);
        } else if (i == 6) {
            /* Monitor pulse */
            gmu_fill_rect(fb, mx, my, 16, 12, 0x00111827);
            gmu_fill_rect(fb, mx + 2, my + 6, 4, 2, 0x0022C55E);
            gmu_fill_rect(fb, mx + 6, my + 2, 2, 8, 0x0022C55E);
            gmu_fill_rect(fb, mx + 8, my + 6, 6, 2, 0x0022C55E);
        } else if (i == 7) {
            /* HexView */
            gmu_fill_rect(fb, mx, my, 16, 12, 0x000369A1);
            gmu_draw_text(fb, mx + 2, my + 3, "FF", 0x0038BDF8, 0x000369A1);
        } else if (i == 8) {
            /* Arcade */
            gmu_fill_rect(fb, mx, my, 16, 12, 0x008B5CF6);
            gmu_fill_circle(fb, mx + 5, my + 6, 2, 0x00EF4444);
            gmu_fill_circle(fb, mx + 11, my + 6, 2, 0x0010B981);
        } else {
            /* Settings gear */
            gmu_fill_rect(fb, mx, my, 16, 12, 0x0064748B);
            gmu_fill_circle(fb, mx + 8, my + 6, 3, GMU_WHITE);
        }

        /* Label centered below icon */
        uint64_t llen = 0;
        const char *lp = labels[i];
        while (*lp++) llen++;
        uint64_t tx = ix + (item_w - llen * GMU_FONT_WIDTH) / 2;
        gmu_draw_text(fb, tx, dock_y + 26, labels[i], text_col, btn_bg);

        if (is_sel) {
            /* Active glowing emerald indicator pill */
            gmu_fill_rect(fb, ix + item_w / 2 - 6, dock_y + 48, 12, 3, th->chiclet_green);
        }
    }
}

/* ========================================================================= */
/* 6. Apple Dropdown Menu ( GMU OS)                                        */
/* ========================================================================= */

static void draw_apple_menu(gmu_framebuffer_t *fb) {
    const gmu_theme_t *th = gmu_theme_get();
    uint64_t menu_w = 260;
    uint64_t menu_h = 260;
    uint64_t menu_x = 8;
    uint64_t menu_y = 30;

    gmu_draw_shadow(fb, menu_x, menu_y, menu_w, menu_h, 8);
    gmu_fill_rect(fb, menu_x, menu_y, menu_w, menu_h, th->surface);
    gmu_draw_rect(fb, menu_x, menu_y, menu_w, menu_h, th->border);

    gmu_draw_text(fb, menu_x + 16, menu_y + 14,  "About GMU OS 64...      [A]", th->accent, th->surface);
    gmu_draw_text(fb, menu_x + 16, menu_y + 36,  "Finder (File Explorer)  [F]", th->text_primary, th->surface);
    gmu_draw_text(fb, menu_x + 16, menu_y + 58,  "Paint Studio            [P]", th->text_primary, th->surface);
    gmu_draw_text(fb, menu_x + 16, menu_y + 80,  "Calendar & World Clock  [K]", th->text_primary, th->surface);
    gmu_draw_text(fb, menu_x + 16, menu_y + 102, "Hex & Memory Inspector  [H]", th->text_primary, th->surface);
    gmu_draw_text(fb, menu_x + 16, menu_y + 124, "Arcade Game Center      [G]", th->text_primary, th->surface);
    gmu_draw_text(fb, menu_x + 16, menu_y + 146, "System Settings...      [S]", th->text_primary, th->surface);

    /* Divider */
    gmu_fill_rect(fb, menu_x + 10, menu_y + 172, menu_w - 20, 1, th->border);

    gmu_draw_text(fb, menu_x + 16, menu_y + 184, "Restart Workstation...  [R]", th->chiclet_yellow, th->surface);
    gmu_draw_text(fb, menu_x + 16, menu_y + 208, "Shut Down Power...      [Q]", th->chiclet_red, th->surface);
    gmu_draw_text(fb, menu_x + 16, menu_y + 234, "Press ESC to close menu", th->text_secondary, th->surface);
}

/* ========================================================================= */
/* 7. macOS "About This Mac" System Specification Window                     */
/* ========================================================================= */

static void show_about_modal(gmu_framebuffer_t *fb) {
    const gmu_theme_t *th = gmu_theme_get();
    uint64_t w = 540;
    uint64_t h = 330;
    uint64_t x = (fb->width - w) / 2;
    uint64_t y = (fb->height - h) / 2;

    gmu_draw_shadow(fb, x, y, w, h, 10);
    gmu_fill_rect(fb, x, y, w, h, th->surface);
    gmu_draw_rect(fb, x, y, w, h, th->border);

    /* Header Bar */
    gmu_fill_rect(fb, x, y, w, 32, th->surface_dark);
    gmu_fill_circle(fb, x + 16, y + 16, 5, th->chiclet_red);
    gmu_fill_circle(fb, x + 32, y + 16, 5, th->chiclet_yellow);
    gmu_fill_circle(fb, x + 48, y + 16, 5, th->chiclet_green);
    gmu_draw_text(fb, x + 70, y + 12, "About GMU OS 64 Workstation", GMU_WHITE, th->surface_dark);
    gmu_fill_rect(fb, x, y + 32, w, 1, th->border);

    /* Crest / Badge */
    gmu_fill_rect(fb, x + 30, y + 54, 80, 80, th->surface_accent);
    gmu_draw_rect(fb, x + 30, y + 54, 80, 80, th->accent);
    gmu_draw_text(fb, x + 42, y + 84, "GMU 64", GMU_WHITE, th->surface_accent);

    /* Title & Edition */
    gmu_draw_text(fb, x + 130, y + 54, "GMU OS 64 Workstation", th->accent, th->surface);
    gmu_draw_text(fb, x + 130, y + 74, "Modern 64-Bit Operating System (UEFI Edition)", th->text_primary, th->surface);
    gmu_draw_text(fb, x + 130, y + 94, "Version 1.0 (Build 2026.UEFI - Long Mode)", th->text_secondary, th->surface);
    gmu_draw_text(fb, x + 130, y + 114, "GM University, Davanagere - Department of CSE", th->chiclet_yellow, th->surface);

    /* Divider */
    gmu_fill_rect(fb, x + 30, y + 148, w - 60, 1, th->border);

    /* Hardware Specifications Table */
    gmu_draw_text(fb, x + 30, y + 162, "Processor:    x86-64 Freestanding Long Mode (64-bit)", th->text_primary, th->surface);
    gmu_draw_text(fb, x + 30, y + 184, "Memory:       1024 MB Physical RAM (PMM Bitmap Allocator)", th->text_primary, th->surface);
    gmu_draw_text(fb, x + 30, y + 206, "Kernel Heap:  2 MB Dynamic Arena (kmalloc / kfree)", th->text_primary, th->surface);
    gmu_draw_text(fb, x + 30, y + 228, "Storage:      GMU Macintosh HD (Hierarchical VFS RAMFS)", th->text_primary, th->surface);
    gmu_draw_text(fb, x + 30, y + 250, "Graphics:     UEFI GOP Linear Framebuffer (32bpp TrueColor)", th->text_primary, th->surface);

    /* Close Button */
    gmu_fill_rect(fb, x + w / 2 - 60, y + 282, 120, 30, th->accent);
    gmu_draw_text(fb, x + w / 2 - 34, y + 292, "OK (ESC)", GMU_WHITE, th->accent);

    for (;;) {
        uint16_t key = gmu_keyboard_read_key();
        if (key == GMU_KEY_ESCAPE || key == GMU_KEY_ENTER || key == '\n' || key == 'A' || key == 'a') {
            break;
        }
        __asm__ volatile ("pause");
    }
}

/* ========================================================================= */
/* 8. Full Desktop Rendering                                                */
/* ========================================================================= */

void gmu_desktop_draw(gmu_framebuffer_t *fb) {
    const gmu_theme_t *th = gmu_theme_get();
    uint64_t w = fb->width;
    uint64_t h = fb->height;

    /* 1. High-Resolution Atmospheric Wallpaper Gradient */
    gmu_draw_gradient_v(fb, 0, 0, w, h, th->bg_top, th->bg_bottom);

    /* Twinkling ambient celestial stars across the upper sky */
    static const uint16_t star_coords[28][2] = {
        {60, 48}, {140, 92}, {220, 64}, {310, 110}, {400, 52},
        {480, 85}, {560, 44}, {630, 98}, {720, 60}, {810, 115},
        {890, 75}, {960, 50}, {100, 150}, {180, 180}, {270, 140},
        {350, 175}, {440, 160}, {530, 190}, {620, 145}, {700, 170},
        {780, 155}, {860, 185}, {940, 130}, {120, 220}, {300, 240},
        {500, 230}, {680, 235}, {850, 225}
    };
    for (int s = 0; s < 28; s++) {
        uint64_t sx = star_coords[s][0];
        uint64_t sy = star_coords[s][1];
        if (sx < w && sy < h) {
            uint32_t star_col = (s % 3 == 0) ? GMU_WHITE : ((s % 3 == 1) ? 0x0038BDF8 : 0x00FDE047);
            gmu_fb_put_pixel(fb, sx, sy, star_col);
            if (s % 4 == 0) {
                gmu_fb_put_pixel(fb, sx + 1, sy, star_col);
                gmu_fb_put_pixel(fb, sx, sy + 1, star_col);
            }
        }
    }

    /* Soft ambient horizon line */
    uint32_t horiz_col = gmu_blend_color(th->bg_bottom, th->accent, 25);
    for (uint64_t gy = h - 220; gy < h - 75; gy += 36) {
        gmu_fill_rect(fb, 20, gy, w - 40, 1, horiz_col);
    }

    /* 2. Desktop Shortcut Icons (macOS / Windows 11 Left Column) */
    static const struct {
        const char *name;
        const char *subtext;
    } desktop_items[4] = {
        {"GMU Macintosh HD", "1.0 GB NVMe"},
        {"User Documents",   "4 Files"},
        {"Arcade Games",     "4 Games"},
        {"README.TXT",       "Guide Doc"}
    };

    for (int i = 0; i < 4; i++) {
        uint64_t ix = 28;
        uint64_t iy = 50 + i * 72;

        /* Icon Container with frosted acrylic background */
        gmu_draw_shadow(fb, ix, iy, 150, 56, 4);
        gmu_fill_rect(fb, ix, iy, 150, 56, th->surface_dark);
        gmu_draw_rect(fb, ix, iy, 150, 56, th->border);

        /* Render custom 32x32 pixel icon */
        if (i == 0) draw_drive_icon(fb, ix + 10, iy + 12);
        else if (i == 1) draw_folder_icon(fb, ix + 10, iy + 12);
        else if (i == 2) draw_arcade_icon(fb, ix + 10, iy + 12);
        else draw_notepad_icon(fb, ix + 10, iy + 12);

        /* Icon text labels */
        gmu_draw_text(fb, ix + 48, iy + 14, desktop_items[i].name, th->text_primary, th->surface_dark);
        gmu_draw_text(fb, ix + 48, iy + 30, desktop_items[i].subtext, th->text_secondary, th->surface_dark);
    }

    /* Trash icon on bottom right */
    uint64_t trash_x = w - 100;
    uint64_t trash_y = h - 145;
    gmu_draw_shadow(fb, trash_x, trash_y, 76, 56, 4);
    gmu_fill_rect(fb, trash_x, trash_y, 76, 56, th->surface_dark);
    gmu_draw_rect(fb, trash_x, trash_y, 76, 56, th->border);
    draw_trash_icon(fb, trash_x + 22, trash_y + 8);
    gmu_draw_text(fb, trash_x + 20, trash_y + 40, "Trash", th->text_secondary, th->surface_dark);

    /* 3. Top macOS Sonoma Frosted Glass Menu Bar (30px) */
    gmu_fill_rect(fb, 0, 0, w, 28, th->surface);
    gmu_fill_rect(fb, 0, 27, w, 1, th->border);

    /* Apple Logo Pill ` GMU OS` */
    gmu_fill_rect(fb, 8, 3, 94, 22, th->surface_dark);
    gmu_draw_rect(fb, 8, 3, 94, 22, th->border);
    gmu_draw_text(fb, 16, 10, " GMU OS", GMU_WHITE, th->surface_dark);

    /* Classic System Menus */
    gmu_draw_text(fb, 116, 10, "File", th->text_primary, th->surface);
    gmu_draw_text(fb, 156, 10, "Edit", th->text_primary, th->surface);
    gmu_draw_text(fb, 196, 10, "View", th->text_primary, th->surface);
    gmu_draw_text(fb, 236, 10, "Go", th->text_primary, th->surface);
    gmu_draw_text(fb, 268, 10, "Window", th->text_primary, th->surface);
    gmu_draw_text(fb, 324, 10, "Help", th->text_primary, th->surface);

    /* Clock & Tray on right */
    update_clock_tray(fb);

    /* 4. Elegant Central Workstation Dashboard Widget */
    uint64_t card_w = w > 880 ? 560 : w - 80;
    uint64_t card_h = 190;
    uint64_t card_x = (w - card_w) / 2 + 50;
    uint64_t card_y = (h - card_h) / 2 - 30;

    gmu_draw_shadow(fb, card_x, card_y, card_w, card_h, 8);
    gmu_fill_rect(fb, card_x, card_y, card_w, card_h, th->surface);
    gmu_draw_rect(fb, card_x, card_y, card_w, card_h, th->border);

    /* Chiclets on window header */
    gmu_fill_circle(fb, card_x + 16, card_y + 16, 5, th->chiclet_red);
    gmu_fill_circle(fb, card_x + 32, card_y + 16, 5, th->chiclet_yellow);
    gmu_fill_circle(fb, card_x + 48, card_y + 16, 5, th->chiclet_green);

    gmu_draw_text(fb, card_x + 68, card_y + 12, "GMU OS 64 - Long Mode Workstation", GMU_WHITE, th->surface);
    gmu_draw_text(fb, card_x + card_w - 140, card_y + 12, th->os_badge, th->accent, th->surface);
    gmu_fill_rect(fb, card_x, card_y + 30, card_w, 1, th->border);

    gmu_draw_text(fb, card_x + 24, card_y + 44, "GM University, Davanagere - Department of CSE", th->accent, th->surface);
    gmu_draw_text(fb, card_x + 24, card_y + 66, "Kernel:      x86-64 Freestanding UEFI / Limine v3", th->text_primary, th->surface);
    gmu_draw_text(fb, card_x + 24, card_y + 88, "Physical:    1024 MB RAM (Bitmap PMM)  |  Heap: 2 MB Pool", th->text_primary, th->surface);
    gmu_draw_text(fb, card_x + 24, card_y + 110, "Filesystem:  RAMFS 128 Nodes Hierarchical Tree [READY]", th->text_primary, th->surface);
    gmu_draw_text(fb, card_x + 24, card_y + 132, "Apps/Games:  13 Applications & Arcade Games active", th->text_primary, th->surface);

    gmu_fill_rect(fb, card_x + 20, card_y + 154, card_w - 40, 1, th->border);
    gmu_draw_text(fb, card_x + 24, card_y + 166,
                  "[1-9,0] Dock  |  [M] Apple Menu  |  [Tab] Theme  |  Mouse Ready",
                  th->chiclet_yellow, th->surface);

    /* 5. Bottom Floating Glass Dock */
    draw_dock(fb);
}

/* ========================================================================= */
/* 9. Power Modal                                                           */
/* ========================================================================= */

static void show_power_modal(gmu_framebuffer_t *fb) {
    const gmu_theme_t *th = gmu_theme_get();
    uint64_t w = 460;
    uint64_t h = 210;
    uint64_t x = (fb->width - w) / 2;
    uint64_t y = (fb->height - h) / 2;

    gmu_draw_window_frame(fb, x, y, w, h, "POWER OPTIONS",
                          th->surface_accent, th->surface, th->border);

    gmu_draw_text(fb, x + 30, y + 54, "Select an action for GMU OS 64:", th->text_primary, th->surface);

    /* Option 1: Reboot */
    gmu_fill_rect(fb, x + 30, y + 80, w - 60, 36, th->surface_dark);
    gmu_draw_rect(fb, x + 30, y + 80, w - 60, 36, th->chiclet_yellow);
    gmu_draw_text(fb, x + 44, y + 92, "[R] RESTART WORKSTATION", th->chiclet_yellow, th->surface_dark);

    /* Option 2: Halt */
    gmu_fill_rect(fb, x + 30, y + 124, w - 60, 36, th->surface_dark);
    gmu_draw_rect(fb, x + 30, y + 124, w - 60, 36, th->chiclet_red);
    gmu_draw_text(fb, x + 44, y + 136, "[H] HALT & POWER OFF CPU", th->chiclet_red, th->surface_dark);

    /* Footer */
    gmu_draw_text(fb, x + 30, y + 176, "Press ESC to cancel and return to desktop.", th->text_secondary, th->surface);

    for (;;) {
        uint16_t key = gmu_keyboard_read_key();
        if (key == 'R' || key == 'r') {
            /* 8042 keyboard controller reboot */
            uint8_t good = 0x02;
            while (good & 0x02) {
                __asm__ volatile ("inb $0x64, %0" : "=a"(good));
            }
            __asm__ volatile ("outb %0, $0x64" :: "a"((uint8_t)0xFE));
            for (;;) __asm__ volatile ("cli; hlt");
        } else if (key == 'H' || key == 'h' || key == 'Q' || key == 'q') {
            gmu_fb_clear(fb, GMU_BLACK);
            gmu_draw_text(fb, fb->width / 2 - 120, fb->height / 2,
                          "SYSTEM HALTED. SAFE TO TURN OFF POWER.", th->chiclet_red, GMU_BLACK);
            for (;;) __asm__ volatile ("cli; hlt");
        } else if (key == GMU_KEY_ESCAPE) {
            break;
        }
        __asm__ volatile ("pause");
    }
}

/* ========================================================================= */
/* 10. Main Desktop Run Loop with Smooth Mouse & Full Keyboard Input         */
/* ========================================================================= */

void gmu_desktop_run(gmu_framebuffer_t *fb) {
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);
    uint8_t previous_buttons = 0;

    gmu_keyboard_init();
    selected_item = 0;
    menu_open = false;
    cursor_visible = false;
    last_mouse_x = -100;
    last_mouse_y = -100;

    /* Render initial desktop ONCE */
    gmu_desktop_draw(fb);
    if (mouse_ready) {
        cursor_show(fb, mouse.x, mouse.y);
        last_mouse_x = mouse.x;
        last_mouse_y = mouse.y;
    }

    uint64_t last_time = gmu_timer_seconds();

    for (;;) {
        bool activate = false;
        uint16_t key = 0;

        /* 1. Refresh live clock on top bar (ONLY update tray, ZERO flicker!) */
        uint64_t cur_time = gmu_timer_seconds();
        if (cur_time != last_time) {
            last_time = cur_time;
            update_clock_tray(fb);
        }

        /* 2. Mouse handling with position change check (ZERO cursor flicker!) */
        if (mouse_ready && gmu_mouse_poll(&mouse)) {
            bool left_pressed = (mouse.buttons & 1) && !(previous_buttons & 1);
            previous_buttons = mouse.buttons;

            if (mouse.x != last_mouse_x || mouse.y != last_mouse_y) {
                cursor_hide(fb);
                cursor_show(fb, mouse.x, mouse.y);
                last_mouse_x = mouse.x;
                last_mouse_y = mouse.y;
            }

            if (left_pressed) {
                /* Click on Apple/GMU logo pill */
                if (mouse.x >= 8 && mouse.x <= 102 && mouse.y >= 3 && mouse.y <= 25) {
                    menu_open = !menu_open;
                    cursor_hide(fb);
                    if (menu_open) draw_apple_menu(fb);
                    else gmu_desktop_draw(fb);
                    cursor_show(fb, mouse.x, mouse.y);
                    continue;
                }

                /* If menu is open, handle clicking on Apple menu items */
                if (menu_open) {
                    uint64_t menu_x = 8;
                    uint64_t menu_y = 30;
                    uint64_t menu_w = 260;
                    uint64_t menu_h = 260;

                    if (mouse.x >= (int32_t)menu_x && mouse.x < (int32_t)(menu_x + menu_w) &&
                        mouse.y >= (int32_t)menu_y && mouse.y < (int32_t)(menu_y + menu_h)) {
                        int32_t item_idx = (mouse.y - (int32_t)menu_y - 8) / 22;
                        menu_open = false;
                        cursor_hide(fb);
                        gmu_desktop_draw(fb);

                        if (item_idx == 0) {
                            show_about_modal(fb);
                            gmu_desktop_draw(fb);
                        } else if (item_idx == 1) {
                            selected_item = 0; activate = true;
                        } else if (item_idx == 2) {
                            selected_item = 3; activate = true;
                        } else if (item_idx == 3) {
                            selected_item = 5; activate = true;
                        } else if (item_idx == 4) {
                            selected_item = 7; activate = true;
                        } else if (item_idx == 5) {
                            selected_item = 8; activate = true;
                        } else if (item_idx == 6) {
                            selected_item = 9; activate = true;
                        } else if (item_idx >= 7 && item_idx <= 8) {
                            show_power_modal(fb);
                            gmu_desktop_draw(fb);
                        }
                        cursor_show(fb, mouse.x, mouse.y);
                        if (activate) goto do_activate;
                        continue;
                    } else {
                        /* Clicked outside menu: dismiss menu */
                        menu_open = false;
                        cursor_hide(fb);
                        gmu_desktop_draw(fb);
                        cursor_show(fb, mouse.x, mouse.y);
                        continue;
                    }
                }

                /* Click on Desktop Icons (Left column) */
                if (mouse.x >= 28 && mouse.x <= 178) {
                    if (mouse.y >= 50 && mouse.y < 106) {
                        /* GMU Macintosh HD -> File Manager */
                        selected_item = 0; activate = true;
                    } else if (mouse.y >= 122 && mouse.y < 178) {
                        /* User Documents -> File Manager */
                        selected_item = 0; activate = true;
                    } else if (mouse.y >= 194 && mouse.y < 250) {
                        /* Arcade Games -> Arcade Hub */
                        selected_item = 8; activate = true;
                    } else if (mouse.y >= 266 && mouse.y < 322) {
                        /* README.TXT -> Notepad with README.TXT */
                        cursor_hide(fb);
                        gmu_editor_open_file(fb, "/README.TXT");
                        gmu_keyboard_init();
                        gmu_desktop_draw(fb);
                        cursor_show(fb, mouse.x, mouse.y);
                        continue;
                    }
                }

                /* Click on Dock */
                uint64_t item_w = 68;
                uint64_t gap = 8;
                uint64_t dock_w = DOCK_APP_COUNT * item_w + (DOCK_APP_COUNT - 1) * gap + 24;
                uint64_t dock_x = (fb->width - dock_w) / 2;
                int32_t dock_top = (int32_t)fb->height - 66;
                int32_t dock_bottom = dock_top + 54;

                if (mouse.y >= dock_top && mouse.y < dock_bottom) {
                    int32_t rel_x = mouse.x - (int32_t)dock_x - 12;
                    if (rel_x >= 0) {
                        uint64_t item = (uint64_t)rel_x / (item_w + gap);
                        int32_t item_offset = rel_x % (item_w + gap);
                        if (item < DOCK_APP_COUNT && item_offset < (int32_t)item_w) {
                            cursor_hide(fb);
                            selected_item = item;
                            draw_dock(fb);
                            cursor_show(fb, mouse.x, mouse.y);
                            activate = true;
                        }
                    }
                }
            }
        }

        /* 3. Keyboard handling */
        key = gmu_keyboard_read_key();

        if (key == GMU_KEY_LEFT) {
            cursor_hide(fb);
            selected_item = (selected_item + DOCK_APP_COUNT - 1) % DOCK_APP_COUNT;
            draw_dock(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        } else if (key == GMU_KEY_RIGHT) {
            cursor_hide(fb);
            selected_item = (selected_item + 1) % DOCK_APP_COUNT;
            draw_dock(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        } else if (key == '1' || key == 'F' || key == 'f') {
            selected_item = 0; activate = true;
        } else if (key == '2' || key == 'T' || key == 't') {
            selected_item = 1; activate = true;
        } else if (key == '3' || key == 'N' || key == 'n') {
            selected_item = 2; activate = true;
        } else if (key == '4' || key == 'P' || key == 'p') {
            selected_item = 3; activate = true;
        } else if (key == '5' || key == 'C' || key == 'c') {
            selected_item = 4; activate = true;
        } else if (key == '6' || key == 'K' || key == 'k') {
            selected_item = 5; activate = true;
        } else if (key == '7' || key == 'm') {
            selected_item = 6; activate = true;
        } else if (key == '8' || key == 'H' || key == 'h') {
            selected_item = 7; activate = true;
        } else if (key == '9' || key == 'G' || key == 'g') {
            selected_item = 8; activate = true;
        } else if (key == '0' || key == 'S' || key == 's') {
            selected_item = 9; activate = true;
        } else if (key == 'M') {
            menu_open = !menu_open;
            cursor_hide(fb);
            if (menu_open) draw_apple_menu(fb);
            else gmu_desktop_draw(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        } else if (key == 'A' || key == 'a') {
            cursor_hide(fb);
            show_about_modal(fb);
            gmu_desktop_draw(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        } else if (key == GMU_KEY_TAB) {
            gmu_theme_cycle();
            cursor_hide(fb);
            gmu_desktop_draw(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        } else if (key == GMU_KEY_ESCAPE) {
            if (menu_open) {
                menu_open = false;
                cursor_hide(fb);
                gmu_desktop_draw(fb);
                if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
                continue;
            }
            cursor_hide(fb);
            show_power_modal(fb);
            gmu_desktop_draw(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        } else if (key == '\n' || key == GMU_KEY_ENTER) {
            activate = true;
        }

do_activate:
        if (!activate) {
            __asm__ volatile ("pause");
            continue;
        }

        if (menu_open) {
            menu_open = false;
            gmu_desktop_draw(fb);
        }

        if (selected_item == 0) {
            /* 1. Launch File Manager */
            cursor_hide(fb);
            gmu_app_launch(GMU_APP_FILE_MANAGER, fb);
            gmu_keyboard_init();
            gmu_desktop_draw(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        } else if (selected_item == 1) {
            /* 2. Switch to Terminal Shell */
            return;
        } else if (selected_item == 2) {
            /* 3. Launch Modern Notepad */
            cursor_hide(fb);
            gmu_app_launch(GMU_APP_EDITOR, fb);
            gmu_keyboard_init();
            gmu_desktop_draw(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        } else if (selected_item == 3) {
            /* 4. Launch Paint Studio */
            cursor_hide(fb);
            gmu_app_launch(GMU_APP_PAINT, fb);
            gmu_keyboard_init();
            gmu_desktop_draw(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        } else if (selected_item == 4) {
            /* 5. Launch Modern Calculator */
            cursor_hide(fb);
            gmu_app_launch(GMU_APP_CALCULATOR, fb);
            gmu_keyboard_init();
            gmu_desktop_draw(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        } else if (selected_item == 5) {
            /* 6. Launch Calendar & Clock */
            cursor_hide(fb);
            gmu_app_launch(GMU_APP_CALENDAR, fb);
            gmu_keyboard_init();
            gmu_desktop_draw(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        } else if (selected_item == 6) {
            /* 7. Launch Activity Monitor */
            cursor_hide(fb);
            gmu_app_launch(GMU_APP_MONITOR, fb);
            gmu_keyboard_init();
            gmu_desktop_draw(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        } else if (selected_item == 7) {
            /* 8. Launch Hex & Memory Inspector */
            cursor_hide(fb);
            gmu_app_launch(GMU_APP_HEXVIEW, fb);
            gmu_keyboard_init();
            gmu_desktop_draw(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        } else if (selected_item == 8) {
            /* 9. Launch Arcade Game Center */
            cursor_hide(fb);
            gmu_app_launch(GMU_APP_GAME_CENTER, fb);
            gmu_keyboard_init();
            gmu_desktop_draw(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        } else if (selected_item == 9) {
            /* 10. Launch System Settings */
            cursor_hide(fb);
            gmu_app_launch(GMU_APP_SETTINGS, fb);
            gmu_keyboard_init();
            gmu_desktop_draw(fb);
            if (mouse_ready) cursor_show(fb, mouse.x, mouse.y);
        }
    }
}
