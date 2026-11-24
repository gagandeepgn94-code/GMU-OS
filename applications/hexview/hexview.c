#include <stdint.h>
#include <stdbool.h>
#include <gmu/hexview.h>
#include <gmu/app.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/draw.h>
#include <gmu/font.h>
#include <gmu/fs.h>
#include <gmu/theme.h>

#define HEX_BUFFER_MAX 4096
#define ROWS_PER_PAGE  20
#define BYTES_PER_ROW  16

static uint8_t hex_data[HEX_BUFFER_MAX];
static uint32_t hex_size = 0;
static uint32_t current_offset = 0;
static char current_file_path[64] = "/README.TXT";

static const char hex_digits[] = "0123456789ABCDEF";

static void format_hex32(uint32_t val, char *buf) {
    for (int i = 7; i >= 0; i--) {
        buf[i] = hex_digits[val & 0x0F];
        val >>= 4;
    }
    buf[8] = '\0';
}

static void load_hex_file(const char *path) {
    int32_t id = gmu_fs_resolve_path(GMU_FS_ROOT_ID, path);
    if (id >= 0) {
        const gmu_fs_node_t *node = gmu_fs_get((uint16_t)id);
        if (node && node->type == GMU_FS_FILE) {
            hex_size = gmu_fs_read((uint16_t)id, (char *)hex_data, 0, HEX_BUFFER_MAX);
            current_offset = 0;
            return;
        }
    }

    /* Fallback test data if file not found */
    hex_size = 128;
    for (uint32_t i = 0; i < hex_size; i++) {
        hex_data[i] = (uint8_t)(i ^ 0x5A);
    }
    current_offset = 0;
}

static void draw_hexview(gmu_framebuffer_t *fb) {
    const gmu_theme_t *th = gmu_theme_get();

    uint64_t ww = fb->width > 860 ? 820 : fb->width - 40;
    uint64_t wh = fb->height > 600 ? 520 : fb->height - 50;
    uint64_t wx = (fb->width - ww) / 2;
    uint64_t wy = (fb->height - wh) / 2 - 10;

    char title[96] = "HEX & MEMORY INSPECTOR - ";
    int tp = 25;
    const char *fp = current_file_path;
    while (*fp && tp < 80) title[tp++] = *fp++;
    title[tp] = '\0';

    gmu_draw_window_frame(fb, wx, wy, ww, wh, title,
                          th->surface_accent, th->surface_dark, th->border);

    /* Quick File Selector Bar */
    uint64_t bar_y = wy + 32;
    gmu_fill_rect(fb, wx + 1, bar_y, ww - 2, 26, th->surface);
    gmu_fill_rect(fb, wx + 1, bar_y + 25, ww - 2, 1, th->border);

    gmu_draw_text(fb, wx + 16, bar_y + 8, "[1] README", th->accent, th->surface);
    gmu_draw_text(fb, wx + 120, bar_y + 8, "[2] ABOUT", th->accent, th->surface);
    gmu_draw_text(fb, wx + 210, bar_y + 8, "[3] SETTINGS", th->accent, th->surface);
    gmu_draw_text(fb, wx + 320, bar_y + 8, "[4] SYSTEM.LOG", th->accent, th->surface);
    gmu_draw_text(fb, wx + ww - 140, bar_y + 8, "[UP/DOWN] SCROLL", th->text_secondary, th->surface);

    /* Column Headers */
    uint64_t hdr_y = bar_y + 28;
    gmu_fill_rect(fb, wx + 1, hdr_y, ww - 2, 20, th->surface_accent);

    gmu_draw_text(fb, wx + 16, hdr_y + 6, "OFFSET", th->text_secondary, th->surface_accent);
    gmu_draw_text(fb, wx + 110, hdr_y + 6, "00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F",
                  th->chiclet_yellow, th->surface_accent);
    gmu_draw_text(fb, wx + ww - 160, hdr_y + 6, "ASCII DUMP", th->text_secondary, th->surface_accent);

    /* Hex Lines Area */
    uint64_t data_y = hdr_y + 22;

    for (uint32_t r = 0; r < ROWS_PER_PAGE; r++) {
        uint32_t row_offset = current_offset + r * BYTES_PER_ROW;
        uint64_t ly = data_y + r * 18;

        if (row_offset >= hex_size) break;

        /* 1. Offset String */
        char off_buf[12];
        format_hex32(row_offset, off_buf);
        gmu_draw_text(fb, wx + 16, ly, off_buf, th->text_secondary, th->surface_dark);

        /* 2. Hex Bytes */
        char hex_row[60];
        int hp = 0;
        char ascii_row[20];
        int ap = 0;

        for (uint32_t c = 0; c < BYTES_PER_ROW; c++) {
            if (c == 8) {
                hex_row[hp++] = ' ';
            }

            if (row_offset + c < hex_size) {
                uint8_t byte = hex_data[row_offset + c];
                hex_row[hp++] = hex_digits[(byte >> 4) & 0x0F];
                hex_row[hp++] = hex_digits[byte & 0x0F];
                hex_row[hp++] = ' ';

                if (byte >= 32 && byte <= 126) ascii_row[ap++] = (char)byte;
                else ascii_row[ap++] = '.';
            } else {
                hex_row[hp++] = ' ';
                hex_row[hp++] = ' ';
                hex_row[hp++] = ' ';
                ascii_row[ap++] = ' ';
            }
        }
        hex_row[hp] = '\0';
        ascii_row[ap] = '\0';

        gmu_draw_text(fb, wx + 110, ly, hex_row, th->text_primary, th->surface_dark);
        gmu_draw_text(fb, wx + ww - 160, ly, ascii_row, th->chiclet_green, th->surface_dark);
    }

    /* Footer */
    uint64_t foot_y = wy + wh - 26;
    gmu_fill_rect(fb, wx + 1, foot_y, ww - 2, 25, th->surface);
    gmu_fill_rect(fb, wx + 1, foot_y, ww - 2, 1, th->border);
    gmu_draw_text(fb, wx + 16, foot_y + 8, "Press ESC to close Hex Viewer", th->chiclet_red, th->surface);
}

void gmu_hexview_open_file(gmu_framebuffer_t *fb, const char *path) {
    if (path && path[0]) {
        int p = 0;
        while (path[p] && p < 63) {
            current_file_path[p] = path[p];
            p++;
        }
        current_file_path[p] = '\0';
    }
    load_hex_file(current_file_path);
    gmu_hexview_run(fb);
}

void gmu_hexview_run(gmu_framebuffer_t *fb) {
    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);
    uint8_t prev_buttons = 0;

    load_hex_file(current_file_path);
    draw_hexview(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }

    uint64_t ww = fb->width > 860 ? 820 : fb->width - 40;
    uint64_t wh = fb->height > 600 ? 520 : fb->height - 50;
    uint64_t wx = (fb->width - ww) / 2;
    uint64_t wy = (fb->height - wh) / 2 - 10;
    uint64_t bar_y = wy + 32;

    for (;;) {
        /* Mouse Input */
        if (mouse_ready && gmu_mouse_poll(&mouse)) {
            gmu_mouse_cursor_update(fb, mouse.x, mouse.y);
            bool left_click = (mouse.buttons & 1) && !(prev_buttons & 1);
            prev_buttons = mouse.buttons;

            if (left_click) {
                /* Click Red Close Chiclet */
                if (mouse.x >= (int32_t)(wx + 10) && mouse.x <= (int32_t)(wx + 24) &&
                    mouse.y >= (int32_t)(wy + 10) && mouse.y <= (int32_t)(wy + 24)) {
                    gmu_mouse_cursor_hide(fb);
                    break;
                }
                /* Click Quick File Bar shortcuts */
                if (mouse.y >= (int32_t)bar_y && mouse.y <= (int32_t)(bar_y + 25)) {
                    if (mouse.x >= (int32_t)(wx + 16) && mouse.x <= (int32_t)(wx + 105)) {
                        load_hex_file("/README.TXT");
                        draw_hexview(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
                    } else if (mouse.x >= (int32_t)(wx + 120) && mouse.x <= (int32_t)(wx + 195)) {
                        load_hex_file("/Documents/ABOUT.TXT");
                        draw_hexview(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
                    } else if (mouse.x >= (int32_t)(wx + 210) && mouse.x <= (int32_t)(wx + 305)) {
                        load_hex_file("/Config/SETTINGS.TXT");
                        draw_hexview(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
                    } else if (mouse.x >= (int32_t)(wx + 320) && mouse.x <= (int32_t)(wx + 430)) {
                        load_hex_file("/SYSTEM.LOG");
                        draw_hexview(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
                    }
                }
            }
        }

        uint16_t key = gmu_keyboard_read_key();
        if (key) {
            if (key == GMU_KEY_ESCAPE) {
                if (mouse_ready) gmu_mouse_cursor_hide(fb);
                break;
            } else if (key == GMU_KEY_DOWN || key == 's' || key == 'S') {
                if (current_offset + BYTES_PER_ROW < hex_size) {
                    current_offset += BYTES_PER_ROW;
                    draw_hexview(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
                }
            } else if (key == GMU_KEY_UP || key == 'w' || key == 'W') {
                if (current_offset >= BYTES_PER_ROW) {
                    current_offset -= BYTES_PER_ROW;
                    draw_hexview(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
                }
            } else if (key == '1') {
                current_file_path[0] = '/'; current_file_path[1] = 'R'; current_file_path[2] = 'E';
                current_file_path[3] = 'A'; current_file_path[4] = 'D'; current_file_path[5] = 'M';
                current_file_path[6] = 'E'; current_file_path[7] = '.'; current_file_path[8] = 'T';
                current_file_path[9] = 'X'; current_file_path[10] = 'T'; current_file_path[11] = '\0';
                load_hex_file(current_file_path);
                draw_hexview(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
            } else if (key == '2') {
                load_hex_file("/Documents/ABOUT.TXT");
                draw_hexview(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
            } else if (key == '3') {
                load_hex_file("/Config/SETTINGS.TXT");
                draw_hexview(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
            } else if (key == '4') {
                load_hex_file("/SYSTEM.LOG");
                draw_hexview(fb);
    if (mouse_ready) {
        gmu_mouse_cursor_show(fb, mouse.x, mouse.y);
    }
            }
        }

        __asm__ volatile ("pause");
    }
}

void gmu_hexview_register(void) {
    const gmu_application_t app = {
        .id = GMU_APP_HEXVIEW,
        .name = "Hex Inspector",
        .run = gmu_hexview_run
    };
    (void)gmu_app_register(&app);
}

