#include <stdint.h>
#include <stdbool.h>
#include <gmu/monitor.h>
#include <gmu/app.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/draw.h>
#include <gmu/font.h>
#include <gmu/pmm.h>
#include <gmu/heap.h>
#include <gmu/fs.h>
#include <gmu/timer.h>
#include <gmu/theme.h>

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

static void draw_progress_bar(gmu_framebuffer_t *fb,
                              uint64_t x, uint64_t y,
                              uint64_t width, uint64_t height,
                              uint64_t current, uint64_t total,
                              uint32_t fill_col, uint32_t bg_col, uint32_t border_col) {
    gmu_fill_rect(fb, x, y, width, height, bg_col);
    gmu_draw_rect(fb, x, y, width, height, border_col);

    if (total > 0 && current > 0) {
        uint64_t fill_w = (current * (width - 4)) / total;
        if (fill_w > width - 4) fill_w = width - 4;
        if (fill_w > 0) {
            gmu_fill_rect(fb, x + 2, y + 2, fill_w, height - 4, fill_col);
        }
    }
}

static void draw_monitor_screen(gmu_framebuffer_t *fb) {
    const gmu_theme_t *th = gmu_theme_get();

    uint64_t win_w = fb->width > 860 ? 800 : fb->width - 40;
    uint64_t win_h = fb->height > 600 ? 530 : fb->height - 50;
    uint64_t win_x = (fb->width - win_w) / 2;
    uint64_t win_y = (fb->height - win_h) / 2 - 10;

    /* Window Frame */
    gmu_draw_window_frame(fb, win_x, win_y, win_w, win_h,
                          "ACTIVITY MONITOR - WORKSTATION DIAGNOSTICS",
                          th->surface_accent, th->surface_dark, th->border);

    /* Section 1: System Uptime & Timer Hardware */
    uint64_t sec1_y = win_y + 44;
    gmu_fill_rect(fb, win_x + 16, sec1_y, win_w - 32, 60, th->surface);
    gmu_draw_rect(fb, win_x + 16, sec1_y, win_w - 32, 60, th->border);

    uint64_t uptime_sec = gmu_timer_seconds();
    uint64_t hrs = uptime_sec / 3600;
    uint64_t mins = (uptime_sec % 3600) / 60;
    uint64_t secs = uptime_sec % 60;
    char time_str[32];
    time_str[0] = '0' + (hrs / 10);
    time_str[1] = '0' + (hrs % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (mins / 10);
    time_str[4] = '0' + (mins % 10);
    time_str[5] = ':';
    time_str[6] = '0' + (secs / 10);
    time_str[7] = '0' + (secs % 10);
    time_str[8] = '\0';

    char t_buf[24];
    format_num(gmu_timer_ticks(), t_buf);

    gmu_draw_text(fb, win_x + 28, sec1_y + 12, "CPU Core:   x86_64 Long Mode (UEFI Limine)", th->text_primary, th->surface);
    gmu_draw_text(fb, win_x + 28, sec1_y + 34, "Hardware:   8254 PIT @ 100 Hz  |  PIC Remapped", th->text_secondary, th->surface);

    char up_line[48] = "Uptime: ";
    int upp = 8; const char *ts = time_str; while (*ts) up_line[upp++] = *ts++;
    up_line[upp] = '\0';
    gmu_draw_text(fb, win_x + win_w - 180, sec1_y + 12, up_line, th->accent, th->surface);

    char tick_line[48] = "Ticks:  ";
    int tp = 8; const char *tbp = t_buf; while (*tbp) tick_line[tp++] = *tbp++;
    tick_line[tp] = '\0';
    gmu_draw_text(fb, win_x + win_w - 180, sec1_y + 34, tick_line, th->text_secondary, th->surface);

    /* Section 2: Memory Bars */
    uint64_t sec2_y = sec1_y + 72;
    gmu_fill_rect(fb, win_x + 16, sec2_y, win_w - 32, 130, th->surface);
    gmu_draw_rect(fb, win_x + 16, sec2_y, win_w - 32, 130, th->border);

    gmu_draw_text(fb, win_x + 28, sec2_y + 10, "MEMORY & STORAGE CONSUMPTION", th->accent, th->surface);

    /* Bar 1: Physical PMM */
    gmu_mem_stats_t mem = gmu_pmm_get_stats();
    uint64_t pmm_used_mb = mem.used_memory / (1024 * 1024);
    uint64_t pmm_total_mb = mem.usable_memory / (1024 * 1024);
    if (pmm_total_mb == 0) pmm_total_mb = 1024;

    char pmm_str[64] = "Physical RAM: ";
    char num_u[16], num_t[16];
    format_num(pmm_used_mb, num_u);
    format_num(pmm_total_mb, num_t);
    int pp = 14; const char *nu = num_u; while (*nu) pmm_str[pp++] = *nu++;
    pmm_str[pp++] = ' '; pmm_str[pp++] = 'M'; pmm_str[pp++] = 'B'; pmm_str[pp++] = ' ';
    pmm_str[pp++] = '/'; pmm_str[pp++] = ' ';
    const char *nt = num_t; while (*nt) pmm_str[pp++] = *nt++;
    pmm_str[pp++] = ' '; pmm_str[pp++] = 'M'; pmm_str[pp++] = 'B'; pmm_str[pp] = '\0';

    gmu_draw_text(fb, win_x + 28, sec2_y + 32, pmm_str, th->text_primary, th->surface);
    draw_progress_bar(fb, win_x + 28, sec2_y + 46, win_w - 56, 12,
                      mem.used_memory, mem.usable_memory,
                      th->accent, th->surface_dark, th->border);

    /* Bar 2: Kernel Dynamic Heap */
    size_t heap_used = gmu_heap_get_used();
    size_t heap_total = gmu_heap_get_total();
    uint64_t heap_used_kb = heap_used / 1024;
    uint64_t heap_total_kb = heap_total / 1024;
    char heap_str[64] = "Kernel Heap:  ";
    format_num(heap_used_kb, num_u);
    format_num(heap_total_kb, num_t);
    pp = 14; nu = num_u; while (*nu) heap_str[pp++] = *nu++;
    heap_str[pp++] = ' '; heap_str[pp++] = 'K'; heap_str[pp++] = 'B'; heap_str[pp++] = ' ';
    heap_str[pp++] = '/'; heap_str[pp++] = ' ';
    nt = num_t; while (*nt) heap_str[pp++] = *nt++;
    heap_str[pp++] = ' '; heap_str[pp++] = 'K'; heap_str[pp++] = 'B'; heap_str[pp] = '\0';

    gmu_draw_text(fb, win_x + 28, sec2_y + 64, heap_str, th->text_primary, th->surface);
    draw_progress_bar(fb, win_x + 28, sec2_y + 78, win_w - 56, 12,
                      heap_used, heap_total,
                      th->chiclet_yellow, th->surface_dark, th->border);

    /* Bar 3: RAMFS Inodes */
    uint16_t fs_nodes = gmu_fs_count(GMU_FS_ROOT_ID);
    char fs_str[64] = "RAMFS Storage:";
    format_num(fs_nodes, num_u);
    pp = 14; nu = num_u; while (*nu) fs_str[pp++] = *nu++;
    fs_str[pp++] = ' '; fs_str[pp++] = 'N'; fs_str[pp++] = 'o'; fs_str[pp++] = 'd'; fs_str[pp++] = 'e'; fs_str[pp++] = 's';
    fs_str[pp++] = ' '; fs_str[pp++] = '/'; fs_str[pp++] = ' ';
    fs_str[pp++] = '1'; fs_str[pp++] = '2'; fs_str[pp++] = '8'; fs_str[pp] = '\0';

    gmu_draw_text(fb, win_x + 28, sec2_y + 96, fs_str, th->text_primary, th->surface);
    draw_progress_bar(fb, win_x + 28, sec2_y + 110, win_w - 56, 12,
                      fs_nodes, 128,
                      th->chiclet_green, th->surface_dark, th->border);

    /* Section 3: Kernel Drivers & Subsystem Status Matrix */
    uint64_t sec3_y = sec2_y + 142;
    uint64_t sec3_h = win_h - (sec3_y - win_y) - 38;
    gmu_fill_rect(fb, win_x + 16, sec3_y, win_w - 32, sec3_h, th->surface);
    gmu_draw_rect(fb, win_x + 16, sec3_y, win_w - 32, sec3_h, th->border);

    gmu_fill_rect(fb, win_x + 17, sec3_y + 1, win_w - 34, 22, th->surface_accent);
    gmu_draw_text(fb, win_x + 28, sec3_y + 6, "ID", th->text_secondary, th->surface_accent);
    gmu_draw_text(fb, win_x + 60, sec3_y + 6, "SUBSYSTEM", th->text_secondary, th->surface_accent);
    gmu_draw_text(fb, win_x + 220, sec3_y + 6, "STATUS", th->text_secondary, th->surface_accent);
    gmu_draw_text(fb, win_x + 320, sec3_y + 6, "VECTOR / RESOURCE", th->text_secondary, th->surface_accent);
    gmu_draw_text(fb, win_x + 500, sec3_y + 6, "ARCHITECTURE", th->text_secondary, th->surface_accent);

    static const char *subsystems[6][4] = {
        {"Interrupt Controller", "RUNNING", "IDT 0-255 / IRQ 0-15", "x86_64 Dual 8259 PIC"},
        {"PIT Timer Driver",    "RUNNING", "100 Hz / Port 0x40",    "8254 Channel 0"},
        {"Physical Allocator",   "ACTIVE",  "1024 MB Bitmap",        "Limine Memmap v3"},
        {"Dynamic Heap Manager", "ACTIVE",  "2048 KB Arena",         "First-Fit Split/Coalesce"},
        {"RAM File System",      "MOUNTED", "128 Nodes Capacity",    "Hierarchical In-Memory"},
        {"Graphics Framebuffer", "ONLINE",  "UEFI GOP 32bpp",        "Linear Double-Buffered"}
    };

    for (int i = 0; i < 6; i++) {
        uint64_t row_y = sec3_y + 28 + i * 18;
        char id_str[4];
        id_str[0] = '0';
        id_str[1] = '1' + i;
        id_str[2] = '\0';

        gmu_draw_text(fb, win_x + 28, row_y, id_str, th->text_secondary, th->surface);
        gmu_draw_text(fb, win_x + 60, row_y, subsystems[i][0], th->text_primary, th->surface);
        gmu_draw_text(fb, win_x + 220, row_y, subsystems[i][1], th->chiclet_green, th->surface);
        gmu_draw_text(fb, win_x + 320, row_y, subsystems[i][2], th->text_secondary, th->surface);
        gmu_draw_text(fb, win_x + 500, row_y, subsystems[i][3], th->accent, th->surface);
    }

    /* Footer */
    uint64_t foot_y = win_y + win_h - 28;
    gmu_fill_rect(fb, win_x + 1, foot_y, win_w - 2, 27, th->surface);
    gmu_fill_rect(fb, win_x + 1, foot_y, win_w - 2, 1, th->border);
    gmu_draw_text(fb, win_x + 20, foot_y + 8, "[ESC] CLOSE MONITOR", th->chiclet_red, th->surface);
    gmu_draw_text(fb, win_x + 180, foot_y + 8, "[R] REFRESH STATS", th->accent, th->surface);
}

void gmu_monitor_run(gmu_framebuffer_t *fb) {
    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);
    uint8_t prev_buttons = 0;

    draw_monitor_screen(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }

    uint64_t win_w = fb->width > 860 ? 800 : fb->width - 40;
    uint64_t win_h = fb->height > 600 ? 530 : fb->height - 50;
    uint64_t win_x = (fb->width - win_w) / 2;
    uint64_t win_y = (fb->height - win_h) / 2 - 10;

    uint64_t last_sec = gmu_timer_seconds();

    for (;;) {
        uint64_t cur_sec = gmu_timer_seconds();
        if (cur_sec != last_sec) {
            last_sec = cur_sec;
            if (mouse_ready) gmu_mouse_cursor_hide(fb);
            draw_monitor_screen(fb);
            if (mouse_ready) gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
        }

        /* Mouse Input */
        if (mouse_ready && gmu_mouse_poll(&mouse)) {
            gmu_mouse_cursor_update(fb, mouse.x, mouse.y);
            bool left_click = (mouse.buttons & 1) && !(prev_buttons & 1);
            prev_buttons = mouse.buttons;

            if (left_click) {
                /* Click Red Close Chiclet */
                if (mouse.x >= (int32_t)(win_x + 10) && mouse.x <= (int32_t)(win_x + 24) &&
                    mouse.y >= (int32_t)(win_y + 10) && mouse.y <= (int32_t)(win_y + 24)) {
                    gmu_mouse_cursor_hide(fb);
                    break;
                }
            }
        }

        uint16_t key = gmu_keyboard_read_key();
        if (key == GMU_KEY_ESCAPE) {
            if (mouse_ready) gmu_mouse_cursor_hide(fb);
            break; /* Exit */
        } else if (key == 'r' || key == 'R') {
            if (mouse_ready) gmu_mouse_cursor_hide(fb);
            draw_monitor_screen(fb);
            if (mouse_ready) gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
        }

        __asm__ volatile ("pause");
    }
}

void gmu_monitor_register(void) {
    const gmu_application_t app = {
        .id = GMU_APP_MONITOR,
        .name = "Activity Monitor",
        .run = gmu_monitor_run
    };
    (void)gmu_app_register(&app);
}
