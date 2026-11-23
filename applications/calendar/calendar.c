#include <stdint.h>
#include <stdbool.h>
#include <gmu/calendar.h>
#include <gmu/app.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/draw.h>
#include <gmu/font.h>
#include <gmu/timer.h>
#include <gmu/theme.h>

static int current_month = 11; /* November */
static int current_year = 2026;
static int current_day = 23;

static uint64_t stopwatch_start_ticks = 0;
static uint64_t stopwatch_elapsed_ms = 0;
static bool stopwatch_running = false;

static const char *month_names[12] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static const int days_in_months[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

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

static void draw_calendar_view(gmu_framebuffer_t *fb) {
    const gmu_theme_t *th = gmu_theme_get();

    uint64_t ww = fb->width > 860 ? 800 : fb->width - 40;
    uint64_t wh = fb->height > 600 ? 520 : fb->height - 50;
    uint64_t wx = (fb->width - ww) / 2;
    uint64_t wy = (fb->height - wh) / 2 - 10;

    /* Window Frame */
    gmu_draw_window_frame(fb, wx, wy, ww, wh,
                          "CALENDAR & WORLD CLOCK",
                          th->surface_accent, th->surface_dark, th->border);

    /* Left Side: Calendar Card (Width 360) */
    uint64_t cal_x = wx + 16;
    uint64_t cal_y = wy + 42;
    uint64_t cal_w = 360;
    uint64_t cal_h = wh - 58;

    gmu_fill_rect(fb, cal_x, cal_y, cal_w, cal_h, th->surface);
    gmu_draw_rect(fb, cal_x, cal_y, cal_w, cal_h, th->border);

    /* Month Navigation Header */
    char hdr_str[48];
    int hp = 0;
    const char *mn = month_names[current_month - 1];
    while (*mn) hdr_str[hp++] = *mn++;
    hdr_str[hp++] = ' ';
    char ybuf[16];
    format_num(current_year, ybuf);
    const char *yp = ybuf; while (*yp) hdr_str[hp++] = *yp++;
    hdr_str[hp] = '\0';

    gmu_fill_rect(fb, cal_x + 1, cal_y + 1, cal_w - 2, 34, th->surface_accent);
    gmu_draw_text(fb, cal_x + 16, cal_y + 12, "< [P]", th->accent, th->surface_accent);
    gmu_draw_text(fb, cal_x + 120, cal_y + 12, hdr_str, GMU_WHITE, th->surface_accent);
    gmu_draw_text(fb, cal_x + cal_w - 56, cal_y + 12, "[N] >", th->accent, th->surface_accent);

    /* Day of week headers */
    static const char *day_headers[7] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
    uint64_t cell_w = 46;
    uint64_t cell_h = 32;

    for (int d = 0; d < 7; d++) {
        uint64_t dx = cal_x + 16 + d * cell_w;
        uint32_t col = (d >= 5) ? th->chiclet_red : th->text_secondary;
        gmu_draw_text(fb, dx + 12, cal_y + 44, day_headers[d], col, th->surface);
    }
    gmu_fill_rect(fb, cal_x + 12, cal_y + 60, cal_w - 24, 1, th->border);

    /* Render days */
    int total_days = days_in_months[current_month - 1];
    /* Calculate rough starting day offset (Nov 2026 starts on Sunday = offset 6) */
    int start_offset = 6;
    if (current_month != 11) start_offset = 1;

    for (int day = 1; day <= total_days; day++) {
        int idx = start_offset + day - 1;
        int row = idx / 7;
        int col = idx % 7;

        uint64_t cx = cal_x + 16 + col * cell_w;
        uint64_t cy = cal_y + 68 + row * cell_h;

        bool is_today = (day == current_day && current_month == 11 && current_year == 2026);

        if (is_today) {
            gmu_fill_rect(fb, cx + 4, cy + 2, 34, 24, th->accent);
            gmu_draw_rect(fb, cx + 4, cy + 2, 34, 24, GMU_WHITE);
        }

        char dbuf[8];
        format_num(day, dbuf);
        uint32_t txt_col = is_today ? GMU_WHITE : (col >= 5 ? th->chiclet_red : th->text_primary);
        uint32_t bg_col = is_today ? th->accent : th->surface;

        gmu_draw_text(fb, cx + 14, cy + 8, dbuf, txt_col, bg_col);
    }

    /* Right Side: Digital Clock & Stopwatch (Width 380) */
    uint64_t right_x = cal_x + cal_w + 16;
    uint64_t right_y = cal_y;
    uint64_t right_w = ww - cal_w - 48;

    /* Digital Clock Card */
    uint64_t clk_h = 130;
    gmu_fill_rect(fb, right_x, right_y, right_w, clk_h, th->surface);
    gmu_draw_rect(fb, right_x, right_y, right_w, clk_h, th->border);

    gmu_draw_text(fb, right_x + 16, right_y + 12, "LOCAL TIME (SYSTEM UPTIME CLOCK)", th->accent, th->surface);

    uint64_t sec = gmu_timer_seconds();
    uint64_t hrs = (sec / 3600) % 24;
    uint64_t mins = (sec % 3600) / 60;
    uint64_t secs = sec % 60;

    char big_clk[16];
    big_clk[0] = '0' + (hrs / 10);
    big_clk[1] = '0' + (hrs % 10);
    big_clk[2] = ':';
    big_clk[3] = '0' + (mins / 10);
    big_clk[4] = '0' + (mins % 10);
    big_clk[5] = ':';
    big_clk[6] = '0' + (secs / 10);
    big_clk[7] = '0' + (secs % 10);
    big_clk[8] = '\0';

    gmu_draw_text_scaled(fb, right_x + 40, right_y + 42, big_clk, th->text_primary, th->surface, 3, false);

    /* Stopwatch Card */
    uint64_t sw_y = right_y + clk_h + 14;
    uint64_t sw_h = 110;
    gmu_fill_rect(fb, right_x, sw_y, right_w, sw_h, th->surface);
    gmu_draw_rect(fb, right_x, sw_y, right_w, sw_h, th->border);

    gmu_draw_text(fb, right_x + 16, sw_y + 12, "STOPWATCH & LAP TIMER", th->accent, th->surface);

    uint64_t total_sw_ms = stopwatch_elapsed_ms;
    if (stopwatch_running) {
        uint64_t cur_ticks = gmu_timer_ticks();
        total_sw_ms += (cur_ticks - stopwatch_start_ticks) * 10;
    }

    uint64_t sw_s = total_sw_ms / 1000;
    uint64_t sw_m = sw_s / 60;
    sw_s = sw_s % 60;
    uint64_t sw_ms = (total_sw_ms % 1000) / 10;

    char sw_str[16];
    sw_str[0] = '0' + (sw_m / 10);
    sw_str[1] = '0' + (sw_m % 10);
    sw_str[2] = ':';
    sw_str[3] = '0' + (sw_s / 10);
    sw_str[4] = '0' + (sw_s % 10);
    sw_str[5] = '.';
    sw_str[6] = '0' + (sw_ms / 10);
    sw_str[7] = '0' + (sw_ms % 10);
    sw_str[8] = '\0';

    gmu_draw_text_scaled(fb, right_x + 60, sw_y + 36, sw_str, th->chiclet_yellow, th->surface, 2, false);
    gmu_draw_text(fb, right_x + 30, sw_y + 80,
                  stopwatch_running ? "[S] PAUSE   [R] RESET" : "[S] START   [R] RESET",
                  th->chiclet_green, th->surface);

    /* World Clock Card */
    uint64_t wld_y = sw_y + sw_h + 14;
    uint64_t wld_h = cal_h - (wld_y - cal_y);
    gmu_fill_rect(fb, right_x, wld_y, right_w, wld_h, th->surface);
    gmu_draw_rect(fb, right_x, wld_y, right_w, wld_h, th->border);

    gmu_draw_text(fb, right_x + 16, wld_y + 10, "MAJOR WORLD TIME ZONES", th->accent, th->surface);
    gmu_draw_text(fb, right_x + 16, wld_y + 30, "UTC / London (GMT):    +0 hrs", th->text_primary, th->surface);
    gmu_draw_text(fb, right_x + 16, wld_y + 50, "New York (EST):        -5 hrs", th->text_primary, th->surface);
    gmu_draw_text(fb, right_x + 16, wld_y + 70, "India Standard (IST):  +5:30 hrs", th->chiclet_green, th->surface);
    gmu_draw_text(fb, right_x + 16, wld_y + 90, "Tokyo (JST):           +9 hrs", th->text_primary, th->surface);

    gmu_draw_text(fb, right_x + 16, wld_y + 120, "Press ESC to close Calendar", th->text_secondary, th->surface);
}

void gmu_calendar_run(gmu_framebuffer_t *fb) {
    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);
    uint8_t prev_buttons = 0;

    draw_calendar_view(fb);

    uint64_t last_s = gmu_timer_seconds();
    uint64_t last_ticks = gmu_timer_ticks();

    uint64_t cal_w = 680;
    uint64_t cal_h = 420;
    uint64_t cal_x = (fb->width - cal_w) / 2;
    uint64_t cal_y = (fb->height - cal_h) / 2 - 10;
    uint64_t right_w = 260;
    uint64_t right_x = cal_x + cal_w - right_w - 16;
    uint64_t sw_y = cal_y + 44 + 90 + 12;

    for (;;) {
        uint64_t cur_s = gmu_timer_seconds();
        uint64_t cur_ticks = gmu_timer_ticks();

        /* Throttle updates: 10 Hz for stopwatch, 1 Hz for clock. Prevents CPU thrashing! */
        if ((stopwatch_running && (cur_ticks - last_ticks >= 10)) || (cur_s != last_s)) {
            last_s = cur_s;
            last_ticks = cur_ticks;
            draw_calendar_view(fb);
        }

        /* Mouse Input */
        if (mouse_ready && gmu_mouse_poll(&mouse)) {
            bool left_click = (mouse.buttons & 1) && !(prev_buttons & 1);
            prev_buttons = mouse.buttons;

            if (left_click) {
                /* Click Red Close Chiclet */
                if (mouse.x >= (int32_t)(cal_x + 10) && mouse.x <= (int32_t)(cal_x + 24) &&
                    mouse.y >= (int32_t)(cal_y + 10) && mouse.y <= (int32_t)(cal_y + 24)) {
                    break;
                }

                /* Click Prev Month [P] */
                if (mouse.x >= (int32_t)(cal_x + 24) && mouse.x <= (int32_t)(cal_x + 60) &&
                    mouse.y >= (int32_t)(cal_y + 44) && mouse.y <= (int32_t)(cal_y + 70)) {
                    if (current_month > 1) current_month--;
                    else { current_month = 12; current_year--; }
                    draw_calendar_view(fb);
                }

                /* Click Next Month [N] */
                if (mouse.x >= (int32_t)(cal_x + 320) && mouse.x <= (int32_t)(cal_x + 360) &&
                    mouse.y >= (int32_t)(cal_y + 44) && mouse.y <= (int32_t)(cal_y + 70)) {
                    if (current_month < 12) current_month++;
                    else { current_month = 1; current_year++; }
                    draw_calendar_view(fb);
                }

                /* Click Stopwatch [S] Start/Pause */
                if (mouse.x >= (int32_t)(right_x + 20) && mouse.x <= (int32_t)(right_x + 120) &&
                    mouse.y >= (int32_t)(sw_y + 70) && mouse.y <= (int32_t)(sw_y + 100)) {
                    if (stopwatch_running) {
                        uint64_t ticks = gmu_timer_ticks();
                        stopwatch_elapsed_ms += (ticks - stopwatch_start_ticks) * 10;
                        stopwatch_running = false;
                    } else {
                        stopwatch_start_ticks = gmu_timer_ticks();
                        stopwatch_running = true;
                    }
                    draw_calendar_view(fb);
                }

                /* Click Stopwatch [R] Reset */
                if (mouse.x >= (int32_t)(right_x + 130) && mouse.x <= (int32_t)(right_x + 220) &&
                    mouse.y >= (int32_t)(sw_y + 70) && mouse.y <= (int32_t)(sw_y + 100)) {
                    stopwatch_running = false;
                    stopwatch_elapsed_ms = 0;
                    draw_calendar_view(fb);
                }
            }
        }

        /* Keyboard Input */
        uint16_t key = gmu_keyboard_read_key();
        if (key == GMU_KEY_ESCAPE) {
            break;
        } else if (key == 'n' || key == 'N' || key == GMU_KEY_RIGHT) {
            if (current_month < 12) current_month++;
            else { current_month = 1; current_year++; }
            draw_calendar_view(fb);
        } else if (key == 'p' || key == 'P' || key == GMU_KEY_LEFT) {
            if (current_month > 1) current_month--;
            else { current_month = 12; current_year--; }
            draw_calendar_view(fb);
        } else if (key == 's' || key == 'S') {
            if (stopwatch_running) {
                uint64_t ticks = gmu_timer_ticks();
                stopwatch_elapsed_ms += (ticks - stopwatch_start_ticks) * 10;
                stopwatch_running = false;
            } else {
                stopwatch_start_ticks = gmu_timer_ticks();
                stopwatch_running = true;
            }
            draw_calendar_view(fb);
        } else if (key == 'r' || key == 'R') {
            stopwatch_running = false;
            stopwatch_elapsed_ms = 0;
            draw_calendar_view(fb);
        }

        __asm__ volatile ("pause");
    }
}

void gmu_calendar_register(void) {
    const gmu_application_t app = {
        .id = GMU_APP_CALENDAR,
        .name = "Calendar & Clock",
        .run = gmu_calendar_run
    };
    (void)gmu_app_register(&app);
}
