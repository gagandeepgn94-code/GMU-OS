#include <stdint.h>
#include <stdbool.h>
#include <gmu/editor.h>
#include <gmu/app.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/draw.h>
#include <gmu/font.h>
#include <gmu/fs.h>
#include <gmu/theme.h>

#define MAX_BUFFER 4096
#define MAX_ROWS   24
#define MAX_COLS   72

static char text_buffer[MAX_BUFFER];
static uint32_t text_length = 0;
static uint32_t cursor_index = 0;
static char active_path[64] = "/Documents/ABOUT.TXT";
static bool modified = false;

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

static void load_file_content(const char *path) {
    int32_t id = gmu_fs_resolve_path(GMU_FS_ROOT_ID, path);
    if (id >= 0) {
        const gmu_fs_node_t *node = gmu_fs_get((uint16_t)id);
        if (node && node->type == GMU_FS_FILE) {
            uint32_t read_bytes = gmu_fs_read((uint16_t)id, text_buffer, 0, MAX_BUFFER - 1);
            text_length = read_bytes;
            text_buffer[text_length] = '\0';
            cursor_index = text_length;
            modified = false;
            return;
        }
    }

    /* Fallback default sample text */
    const char *sample = "Welcome to GMU OS 64 Notepad!\n"
                         "Type your notes or code here.\n"
                         "Press F2 to Save to RAMFS.\n"
                         "Press ESC to exit to Desktop.\n";
    text_length = 0;
    while (sample[text_length] && text_length < MAX_BUFFER - 1) {
        text_buffer[text_length] = sample[text_length];
        text_length++;
    }
    text_buffer[text_length] = '\0';
    cursor_index = text_length;
    modified = false;
}

static bool save_file_content(void) {
    int32_t id = gmu_fs_resolve_path(GMU_FS_ROOT_ID, active_path);
    if (id < 0) {
        /* File doesn't exist, create it in /Documents */
        id = gmu_fs_create(GMU_FS_ROOT_ID, active_path, GMU_FS_FILE);
    }
    if (id < 0) return false;

    bool ok = gmu_fs_write((uint16_t)id, text_buffer, text_length);
    if (ok) modified = false;
    return ok;
}

static void draw_editor_ui(gmu_framebuffer_t *fb, const char *status_msg, uint32_t status_col) {
    const gmu_theme_t *th = gmu_theme_get();

    uint64_t win_w = fb->width > 880 ? 820 : fb->width - 40;
    uint64_t win_h = fb->height > 600 ? 540 : fb->height - 50;
    uint64_t win_x = (fb->width - win_w) / 2;
    uint64_t win_y = (fb->height - win_h) / 2 - 10;

    /* Window Header & Frame */
    char title[96] = "GMU NOTEPAD - ";
    int tp = 14;
    const char *ap = active_path;
    while (*ap && tp < 70) title[tp++] = *ap++;
    if (modified) {
        title[tp++] = ' '; title[tp++] = '*';
    }
    title[tp] = '\0';

    gmu_draw_window_frame(fb, win_x, win_y, win_w, win_h, title,
                          th->surface_accent, th->surface_dark, th->border);

    /* Sub-header Action Bar */
    uint64_t bar_y = win_y + 32;
    gmu_fill_rect(fb, win_x + 1, bar_y, win_w - 2, 26, th->surface);
    gmu_fill_rect(fb, win_x + 1, bar_y + 25, win_w - 2, 1, th->border);

    gmu_draw_text(fb, win_x + 16, bar_y + 8, "[F2] SAVE", th->chiclet_green, th->surface);
    gmu_draw_text(fb, win_x + 100, bar_y + 8, "[F1] CLEAR", th->chiclet_yellow, th->surface);
    gmu_draw_text(fb, win_x + 190, bar_y + 8, "[ESC] CLOSE", th->chiclet_red, th->surface);

    char stat_chars[32] = "Bytes: ";
    char num_buf[16];
    format_num(text_length, num_buf);
    int sp = 7; const char *nb = num_buf; while (*nb) stat_chars[sp++] = *nb++;
    stat_chars[sp] = '\0';
    gmu_draw_text(fb, win_x + win_w - 120, bar_y + 8, stat_chars, th->text_secondary, th->surface);

    /* Text & Gutter area */
    uint64_t edit_y = bar_y + 26;
    uint64_t edit_h = win_h - 32 - 26 - 28;
    uint64_t gutter_w = 40;

    gmu_fill_rect(fb, win_x + 1, edit_y, gutter_w, edit_h, th->surface);
    gmu_fill_rect(fb, win_x + gutter_w, edit_y, 1, edit_h, th->border);
    gmu_fill_rect(fb, win_x + gutter_w + 1, edit_y, win_w - gutter_w - 2, edit_h, th->surface_dark);

    /* Render lines & line numbers */
    uint64_t line_num = 1;
    uint64_t draw_y = edit_y + 8;
    uint64_t text_start_x = win_x + gutter_w + 12;
    uint64_t cur_x = text_start_x;

    char lno_buf[8];
    format_num(line_num, lno_buf);
    gmu_draw_text(fb, win_x + 8, draw_y, lno_buf, th->text_secondary, th->surface);

    for (uint32_t i = 0; i <= text_length; i++) {
        if (i == cursor_index) {
            /* Draw cursor */
            gmu_fill_rect(fb, cur_x, draw_y, 2, 10, th->accent);
        }

        if (i == text_length) break;

        char c = text_buffer[i];
        if (c == '\n') {
            line_num++;
            draw_y += 13;
            cur_x = text_start_x;
            if (draw_y + 13 >= edit_y + edit_h) break;

            format_num(line_num, lno_buf);
            gmu_draw_text(fb, win_x + 8, draw_y, lno_buf, th->text_secondary, th->surface);
        } else {
            if (cur_x + GMU_FONT_WIDTH < win_x + win_w - 16) {
                gmu_draw_char(fb, cur_x, draw_y, c, th->text_primary, th->surface_dark);
                cur_x += GMU_FONT_WIDTH;
            }
        }
    }

    /* Bottom status message bar */
    uint64_t status_y = win_y + win_h - 26;
    gmu_fill_rect(fb, win_x + 1, status_y, win_w - 2, 25, th->surface);
    gmu_fill_rect(fb, win_x + 1, status_y, win_w - 2, 1, th->border);

    if (status_msg) {
        gmu_draw_text(fb, win_x + 16, status_y + 7, status_msg, status_col, th->surface);
    } else {
        gmu_draw_text(fb, win_x + 16, status_y + 7, "Ready. Use arrows to move, typing modifies text.", th->text_secondary, th->surface);
    }
}

void gmu_editor_open_file(gmu_framebuffer_t *fb, const char *path) {
    if (path && path[0]) {
        int p = 0;
        while (path[p] && p < 63) {
            active_path[p] = path[p];
            p++;
        }
        active_path[p] = '\0';
    }
    load_file_content(active_path);
    gmu_editor_run(fb);
}

void gmu_editor_run(gmu_framebuffer_t *fb) {
    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);
    uint8_t prev_buttons = 0;

    if (text_length == 0 && !modified) {
        load_file_content(active_path);
    }

    const char *status_msg = "Document Loaded. Press F2 to Save.";
    uint32_t status_col = 0x0010B981;

    draw_editor_ui(fb, status_msg, status_col);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }

    uint64_t win_w = fb->width > 860 ? 800 : fb->width - 40;
    uint64_t win_h = fb->height > 600 ? 540 : fb->height - 40;
    uint64_t win_x = (fb->width - win_w) / 2;
    uint64_t win_y = (fb->height - win_h) / 2 - 10;
    uint64_t bar_y = win_y + 32;

    for (;;) {
        bool needs_redraw = false;

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

                /* Click [ESC] CLOSE button on sub-header */
                if (mouse.x >= (int32_t)(win_x + 185) && mouse.x <= (int32_t)(win_x + 260) &&
                    mouse.y >= (int32_t)(bar_y + 4) && mouse.y <= (int32_t)(bar_y + 24)) {
                    gmu_mouse_cursor_hide(fb);
                    break;
                }

                /* Click [F2] SAVE button */
                if (mouse.x >= (int32_t)(win_x + 12) && mouse.x <= (int32_t)(win_x + 80) &&
                    mouse.y >= (int32_t)(bar_y + 4) && mouse.y <= (int32_t)(bar_y + 24)) {
                    if (save_file_content()) {
                        status_msg = "SUCCESS: File saved to RAMFS!";
                        status_col = 0x0010B981;
                    } else {
                        status_msg = "ERROR: Failed to save file!";
                        status_col = 0x00EF4444;
                    }
                    needs_redraw = true;
                }

                /* Click [F1] CLEAR button */
                if (mouse.x >= (int32_t)(win_x + 95) && mouse.x <= (int32_t)(win_x + 165) &&
                    mouse.y >= (int32_t)(bar_y + 4) && mouse.y <= (int32_t)(bar_y + 24)) {
                    text_length = 0;
                    text_buffer[0] = '\0';
                    cursor_index = 0;
                    modified = true;
                    status_msg = "New blank document created.";
                    status_col = 0x00FBBF24;
                    needs_redraw = true;
                }
            }
        }

        uint16_t key = gmu_keyboard_read_key();
        if (key != 0) {
            status_msg = 0;

            if (key == GMU_KEY_ESCAPE) {
                if (mouse_ready) gmu_mouse_cursor_hide(fb);
                break; /* Exit editor */
            } else if (key == GMU_KEY_F2) {
                if (save_file_content()) {
                    status_msg = "SUCCESS: File saved to RAMFS!";
                    status_col = 0x0010B981;
                } else {
                    status_msg = "ERROR: Failed to save file!";
                    status_col = 0x00EF4444;
                }
                needs_redraw = true;
            } else if (key == GMU_KEY_F1) {
                /* Clear buffer / New document */
                text_length = 0;
                text_buffer[0] = '\0';
                cursor_index = 0;
                modified = true;
                status_msg = "New blank document created.";
                status_col = 0x00FBBF24;
                needs_redraw = true;
            } else if (key == GMU_KEY_LEFT) {
                if (cursor_index > 0) cursor_index--;
                needs_redraw = true;
            } else if (key == GMU_KEY_RIGHT) {
                if (cursor_index < text_length) cursor_index++;
                needs_redraw = true;
            } else if (key == '\b' || key == GMU_KEY_BACKSPACE) {
                if (cursor_index > 0) {
                    for (uint32_t i = cursor_index - 1; i < text_length - 1; i++) {
                        text_buffer[i] = text_buffer[i + 1];
                    }
                    text_length--;
                    text_buffer[text_length] = '\0';
                    cursor_index--;
                    modified = true;
                    needs_redraw = true;
                }
            } else if (key == '\n' || key == GMU_KEY_ENTER) {
                if (text_length < MAX_BUFFER - 1) {
                    for (uint32_t i = text_length; i > cursor_index; i--) {
                        text_buffer[i] = text_buffer[i - 1];
                    }
                    text_buffer[cursor_index] = '\n';
                    text_length++;
                    text_buffer[text_length] = '\0';
                    cursor_index++;
                    modified = true;
                    needs_redraw = true;
                }
            } else if (key >= 32 && key <= 126) {
                if (text_length < MAX_BUFFER - 1) {
                    for (uint32_t i = text_length; i > cursor_index; i--) {
                        text_buffer[i] = text_buffer[i - 1];
                    }
                    text_buffer[cursor_index] = (char)key;
                    text_length++;
                    text_buffer[text_length] = '\0';
                    cursor_index++;
                    modified = true;
                    needs_redraw = true;
                }
            }
        }

        if (needs_redraw) {
            if (mouse_ready) gmu_mouse_cursor_hide(fb);
            draw_editor_ui(fb, status_msg, status_col);
            if (mouse_ready) gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
        }

        __asm__ volatile ("pause");
    }
}

void gmu_editor_register(void) {
    const gmu_application_t app = {
        .id = GMU_APP_EDITOR,
        .name = "Notepad",
        .run = gmu_editor_run
    };
    (void)gmu_app_register(&app);
}
