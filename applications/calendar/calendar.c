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
#include <gmu/rtc.h>

static int view_month = 11;
static int view_year = 2026;
static gmu_time_t now = {0};

static uint64_t stopwatch_start_ticks = 0;
static uint64_t stopwatch_elapsed_ms = 0;
static bool stopwatch_running = false;

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
    const char *mn = gmu_rtc_month_name(view_month);
    while (*mn) hdr_str[hp++] = *mn++;
    hdr_str[hp++] = ' ';
    char ybuf[16];
    format_num(view_year, ybuf);
    const char *yp = ybuf; while (*yp) hdr_str[hp++] = *yp++;
    hdr_str[hp] = '\0';

    gmu_fill_rect(fb, cal_x + 1, cal_y + 1, cal_w - 2, 34, th->surface_accent);
    gmu_draw_text(fb, cal_x + 16, cal_y + 12, "< [P]", th->accent, th->surface_accent);
    gmu_draw_text(fb, cal_x + 115, cal_y + 12, hdr_str, GMU_WHITE, th->surface_accent);
    gmu_draw_text(fb, cal_x + cal_w - 56, cal_y + 12, "[N] >", th->accent, th->surface_accent);

    /* Day of week headers (Monday to Sunday) */
    static const char *day_headers[7] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
    uint64_t cell_w = 46;
    uint64_t cell_h = 32;

    for (int d = 0; d < 7; d++) {
        uint64_t dx = cal_x + 16 + d * cell_w;
        uint32_t col = (d >= 5) ? th->chiclet_red : th->text_secondary;
        gmu_draw_text(fb, dx + 12, cal_y + 44, day_headers[d], col, th->surface);
    }
    gmu_fill_rect(fb, cal_x + 12, cal_y + 60, cal_w - 24, 1, th->border);

    /* Accurate day offset calculation using Sakamoto's algorithm */
    int total_days = gmu_rtc_days_in_month(view_year, view_month);
    int dow = gmu_rtc_day_of_week(view_year, view_month, 1);
    /* Convert Sunday=0 to Monday-based (Monday=0 ... Sunday=6) */
    int start_offset = (dow + 6) % 7;

    for (int day = 1; day <= total_days; day++) {
        int idx = start_offset + day - 1;
        int row = idx / 7;
        int col = idx % 7;

        uint64_t cx = cal_x + 16 + col * cell_w;
        uint64_t cy = cal_y + 68 + row * cell_h;

        bool is_today = (day == (int)now.day && view_month == (int)now.month && view_year == (int)now.year);

        if (is_today) {
            gmu_fill_rect(fb, cx + 4, cy + 2, 34, 24, th->accent);
            gmu_draw_rect(fb, cx + 4, cy + 2, 34, 24, GMU_WHITE);
        }

        char dbuf[8];
        format_num(day, dbuf);
        uint32_t txt_col = is_today ? GMU_WHITE : (col >= 5 ? th->chiclet_red : th->text_primary);
        uint32_t bg_col = is_today ? th->accent : th->surface;

        gmu_draw_text(fb, cx + (day < 10 ? 16 : 12), cy + 8, dbuf, txt_col, bg_col);
    }

    /* Bottom Info on Calendar Card */
    uint64_t info_y = cal_y + 276;
    gmu_fill_rect(fb, cal_x + 12, info_y, cal_w - 24, 1, th->border);

    char today_str[48];
    gmu_rtc_get_date_str(today_str);
    char today_line[64] = "TODAY: ";
    int tdp = 7; const char *ts = today_str; while (*ts && tdp < 60) today_line[tdp++] = *ts++; today_line[tdp] = '\0';

    gmu_fill_rect(fb, cal_x + 16, info_y + 10, cal_w - 32, 28, th->surface_dark);
    gmu_draw_rect(fb, cal_x + 16, info_y + 10, cal_w - 32, 28, th->chiclet_green);
    gmu_draw_text(fb, cal_x + 28, info_y + 18, today_line, th->chiclet_green, th->surface_dark);

    gmu_draw_text(fb, cal_x + 24, info_y + 52, "[P] Prev Month    [N] Next Month", th->text_secondary, th->surface);
    gmu_draw_text(fb, cal_x + 24, info_y + 74, "[T] Return to Current Month", th->chiclet_yellow, th->surface);

    /* Right Side: Digital Clock & Stopwatch & World Clock */
    uint64_t right_x = cal_x + cal_w + 16;
    uint64_t right_y = cal_y;
    uint64_t right_w = ww - cal_w - 48;

    /* 1. Digital Clock Card */
    uint64_t clk_h = 130;
    gmu_fill_rect(fb, right_x, right_y, right_w, clk_h, th->surface);
    gmu_draw_rect(fb, right_x, right_y, right_w, clk_h, th->border);

    gmu_draw_text(fb, right_x + 16, right_y + 12, "HARDWARE REAL-TIME CLOCK (RTC)", th->accent, th->surface);

    char big_clk[16];
    big_clk[0] = '0' + (now.hour / 10);
    big_clk[1] = '0' + (now.hour % 10);
    big_clk[2] = ':';
    big_clk[3] = '0' + (now.minute / 10);
    big_clk[4] = '0' + (now.minute % 10);
    big_clk[5] = ':';
    big_clk[6] = '0' + (now.second / 10);
    big_clk[7] = '0' + (now.second % 10);
    big_clk[8] = '\0';

    gmu_draw_text_scaled(fb, right_x + 40, right_y + 38, big_clk, th->text_primary, th->surface, 3, false);

    /* System Uptime under clock */
    uint64_t up_sec = gmu_timer_seconds();
    uint64_t up_hrs = up_sec / 3600;
    uint64_t up_mins = (up_sec % 3600) / 60;
    uint64_t up_secs = up_sec % 60;

    char up_str[48] = "System Uptime: ";
    int upp = 15;
    char n_buf[16];
    format_num(up_hrs, n_buf);
    const char *np = n_buf; while (*np) up_str[upp++] = *np++;
    up_str[upp++] = 'h'; up_str[upp++] = ' ';
    format_num(up_mins, n_buf);
    np = n_buf; while (*np) up_str[upp++] = *np++;
    up_str[upp++] = 'm'; up_str[upp++] = ' ';
    format_num(up_secs, n_buf);
    np = n_buf; while (*np) up_str[upp++] = *np++;
    up_str[upp++] = 's'; up_str[upp++] = '\0';

    gmu_draw_text(fb, right_x + 16, right_y + 104, up_str, th->text_secondary, th->surface);

    /* 2. Stopwatch Card */
    uint64_t sw_y = right_y + clk_h + 12;
    uint64_t sw_h = 100;
    gmu_fill_rect(fb, right_x, sw_y, right_w, sw_h, th->surface);
    gmu_draw_rect(fb, right_x, sw_y, right_w, sw_h, th->border);

    gmu_draw_text(fb, right_x + 16, sw_y + 10, "STOPWATCH & LAP TIMER", th->accent, th->surface);

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

    gmu_draw_text_scaled(fb, right_x + 60, sw_y + 32, sw_str, th->chiclet_yellow, th->surface, 2, false);
    gmu_draw_text(fb, right_x + 30, sw_y + 74,
                  stopwatch_running ? "[S] PAUSE   [R] RESET" : "[S] START   [R] RESET",
                  th->chiclet_green, th->surface);

    /* 3. Live World Clock Card */
    uint64_t wld_y = sw_y + sw_h + 12;
    uint64_t wld_h = cal_h - (wld_y - cal_y);
    gmu_fill_rect(fb, right_x, wld_y, right_w, wld_h, th->surface);
    gmu_draw_rect(fb, right_x, wld_y, right_w, wld_h, th->border);

    gmu_draw_text(fb, right_x + 16, wld_y + 10, "MAJOR WORLD TIME ZONES (LIVE)", th->accent, th->surface);

    /* Formatted live strings for world zones */
    gmu_time_t t_utc, t_ny, t_ist, t_tokyo;
    gmu_rtc_get_tz_time(&now, &t_utc, 0);
    gmu_rtc_get_tz_time(&now, &t_ny, -300);
    gmu_rtc_get_tz_time(&now, &t_ist, 330);
    gmu_rtc_get_tz_time(&now, &t_tokyo, 540);

    char z_line[64];

    /* UTC / London */
    z_line[0] = '0' + (t_utc.hour / 10); z_line[1] = '0' + (t_utc.hour % 10); z_line[2] = ':';
    z_line[3] = '0' + (t_utc.minute / 10); z_line[4] = '0' + (t_utc.minute % 10); z_line[5] = ':';
    z_line[6] = '0' + (t_utc.second / 10); z_line[7] = '0' + (t_utc.second % 10); z_line[8] = '\0';
    gmu_draw_text(fb, right_x + 16, wld_y + 30, "London (UTC / GMT):", th->text_primary, th->surface);
    gmu_draw_text(fb, right_x + 230, wld_y + 30, z_line, th->accent, th->surface);

    /* New York (EST) */
    z_line[0] = '0' + (t_ny.hour / 10); z_line[1] = '0' + (t_ny.hour % 10); z_line[2] = ':';
    z_line[3] = '0' + (t_ny.minute / 10); z_line[4] = '0' + (t_ny.minute % 10); z_line[5] = ':';
    z_line[6] = '0' + (t_ny.second / 10); z_line[7] = '0' + (t_ny.second % 10); z_line[8] = '\0';
    gmu_draw_text(fb, right_x + 16, wld_y + 50, "New York (EST -5h):", th->text_primary, th->surface);
    gmu_draw_text(fb, right_x + 230, wld_y + 50, z_line, th->text_primary, th->surface);

    /* India Standard Time (IST) */
    z_line[0] = '0' + (t_ist.hour / 10); z_line[1] = '0' + (t_ist.hour % 10); z_line[2] = ':';
    z_line[3] = '0' + (t_ist.minute / 10); z_line[4] = '0' + (t_ist.minute % 10); z_line[5] = ':';
    z_line[6] = '0' + (t_ist.second / 10); z_line[7] = '0' + (t_ist.second % 10); z_line[8] = '\0';
    gmu_draw_text(fb, right_x + 16, wld_y + 70, "India Standard (IST):", th->chiclet_green, th->surface);
    gmu_draw_text(fb, right_x + 230, wld_y + 70, z_line, th->chiclet_green, th->surface);

    /* Tokyo (JST) */
    z_line[0] = '0' + (t_tokyo.hour / 10); z_line[1] = '0' + (t_tokyo.hour % 10); z_line[2] = ':';
    z_line[3] = '0' + (t_tokyo.minute / 10); z_line[4] = '0' + (t_tokyo.minute % 10); z_line[5] = ':';
    z_line[6] = '0' + (t_tokyo.second / 10); z_line[7] = '0' + (t_tokyo.second % 10); z_line[8] = '\0';
    gmu_draw_text(fb, right_x + 16, wld_y + 90, "Tokyo (JST +9h):", th->text_primary, th->surface);
    gmu_draw_text(fb, right_x + 230, wld_y + 90, z_line, th->text_primary, th->surface);

    gmu_draw_text(fb, right_x + 16, wld_y + 116, "Press ESC to close Calendar", th->text_secondary, th->surface);
}

void gmu_calendar_run(gmu_framebuffer_t *fb) {
    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);
    uint8_t prev_buttons = 0;

    /* Initialize to today's date from hardware RTC */
    gmu_rtc_read(&now);
    view_year = now.year;
    view_month = now.month;

    draw_calendar_view(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }

    uint8_t last_s = now.second;
    uint64_t last_ticks = gmu_timer_ticks();

    uint64_t ww = fb->width > 860 ? 800 : fb->width - 40;
    uint64_t wh = fb->height > 600 ? 520 : fb->height - 50;
    uint64_t wx = (fb->width - ww) / 2;
    uint64_t wy = (fb->height - wh) / 2 - 10;
    uint64_t cal_x = wx + 16;
    uint64_t cal_y = wy + 42;
    uint64_t cal_w = 360;
    uint64_t right_x = cal_x + cal_w + 16;
    uint64_t sw_y = cal_y + 130 + 12;

    for (;;) {
        uint64_t cur_ticks = gmu_timer_ticks();

        /* Poll real-time clock */
        gmu_time_t current_time;
        gmu_rtc_read(&current_time);

        bool sec_changed = (current_time.second != last_s);
        bool sw_tick = (stopwatch_running && (cur_ticks - last_ticks >= 10));

        if (sec_changed || sw_tick) {
            last_s = current_time.second;
            last_ticks = cur_ticks;
            now = current_time;

            /* Check if date rolled over to new day */
            if (now.day != current_time.day && view_month == (int)now.month && view_year == (int)now.year) {
                view_month = now.month;
                view_year = now.year;
            }

            draw_calendar_view(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
        }

        /* Mouse Input */
        if (mouse_ready && gmu_mouse_poll(&mouse)) {
            gmu_mouse_cursor_update(fb, mouse.x, mouse.y);
            bool left_click = (mouse.buttons & 1) && !(prev_buttons & 1);
            prev_buttons = mouse.buttons;

            if (left_click) {
                /* Click Red Close Chiclet */
                if (mouse.x >= (int32_t)(wx + 10) && mouse.x <= (int32_t)(wx + 26) &&
                    mouse.y >= (int32_t)(wy + 10) && mouse.y <= (int32_t)(wy + 26)) {
                    gmu_mouse_cursor_hide(fb);
                    break;
                }

                /* Click Prev Month [P] */
                if (mouse.x >= (int32_t)(cal_x + 10) && mouse.x <= (int32_t)(cal_x + 60) &&
                    mouse.y >= (int32_t)(cal_y + 4) && mouse.y <= (int32_t)(cal_y + 36)) {
                    if (view_month > 1) view_month--;
                    else { view_month = 12; view_year--; }
                    draw_calendar_view(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
                }

                /* Click Next Month [N] */
                if (mouse.x >= (int32_t)(cal_x + cal_w - 60) && mouse.x <= (int32_t)(cal_x + cal_w - 10) &&
                    mouse.y >= (int32_t)(cal_y + 4) && mouse.y <= (int32_t)(cal_y + 36)) {
                    if (view_month < 12) view_month++;
                    else { view_month = 1; view_year++; }
                    draw_calendar_view(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
                }

                /* Click Today Banner -> Jump to Today */
                if (mouse.x >= (int32_t)(cal_x + 16) && mouse.x <= (int32_t)(cal_x + cal_w - 16) &&
                    mouse.y >= (int32_t)(cal_y + 286) && mouse.y <= (int32_t)(cal_y + 314)) {
                    gmu_rtc_read(&now);
                    view_month = now.month;
                    view_year = now.year;
                    draw_calendar_view(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
                }

                /* Click Stopwatch [S] Start/Pause */
                if (mouse.x >= (int32_t)(right_x + 20) && mouse.x <= (int32_t)(right_x + 120) &&
                    mouse.y >= (int32_t)(sw_y + 65) && mouse.y <= (int32_t)(sw_y + 95)) {
                    if (stopwatch_running) {
                        uint64_t ticks = gmu_timer_ticks();
                        stopwatch_elapsed_ms += (ticks - stopwatch_start_ticks) * 10;
                        stopwatch_running = false;
                    } else {
                        stopwatch_start_ticks = gmu_timer_ticks();
                        stopwatch_running = true;
                    }
                    draw_calendar_view(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
                }

                /* Click Stopwatch [R] Reset */
                if (mouse.x >= (int32_t)(right_x + 130) && mouse.x <= (int32_t)(right_x + 220) &&
                    mouse.y >= (int32_t)(sw_y + 65) && mouse.y <= (int32_t)(sw_y + 95)) {
                    stopwatch_running = false;
                    stopwatch_elapsed_ms = 0;
                    draw_calendar_view(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
                }
            }
        }

        /* Keyboard Input */
        uint16_t key = gmu_keyboard_read_key();
        if (key == GMU_KEY_ESCAPE) {
            if (mouse_ready) gmu_mouse_cursor_hide(fb);
            break;
        } else if (key == 'n' || key == 'N' || key == GMU_KEY_RIGHT) {
            if (view_month < 12) view_month++;
            else { view_month = 1; view_year++; }
            draw_calendar_view(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
        } else if (key == 'p' || key == 'P' || key == GMU_KEY_LEFT) {
            if (view_month > 1) view_month--;
            else { view_month = 12; view_year--; }
            draw_calendar_view(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
        } else if (key == 't' || key == 'T') {
            /* Return to Today */
            gmu_rtc_read(&now);
            view_month = now.month;
            view_year = now.year;
            draw_calendar_view(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
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
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
        } else if (key == 'r' || key == 'R') {
            stopwatch_running = false;
            stopwatch_elapsed_ms = 0;
            draw_calendar_view(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
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
