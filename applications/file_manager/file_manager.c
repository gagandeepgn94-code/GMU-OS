#include <stdint.h>
#include <stdbool.h>
#include <gmu/file_manager.h>
#include <gmu/fs.h>
#include <gmu/draw.h>
#include <gmu/font.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/editor.h>

#define FM_BG       0x000F172A
#define FM_PANEL    0x001E293B
#define FM_HEADER   0x000F172A
#define FM_SELECTED 0x002563EB
#define FM_MUTED    0x0094A3B8
#define FM_CYAN     0x0006B6D4
#define FM_GREEN    0x0022C55E
#define FM_WHITE    0x00FFFFFF
#define FM_RED      0x00EF4444
#define FM_YELLOW   0x00FBBF24

typedef enum {
    FM_SORT_NAME_ASC = 0,
    FM_SORT_NAME_DESC,
    FM_SORT_SIZE_DESC,
    FM_SORT_SIZE_ASC,
    FM_SORT_TYPE,
    FM_SORT_COUNT
} fm_sort_t;

static uint16_t current_dir = GMU_FS_ROOT_ID;
static uint16_t selected_index = 0;
static uint16_t scroll_offset = 0;
static uint16_t visible_count = 0;
static fm_sort_t current_sort = FM_SORT_NAME_ASC;

static uint16_t fm_children[GMU_FS_MAX_NODES];
static uint16_t fm_child_count = 0;

static void format_num(uint32_t val, char *buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    char temp[16];
    int t = 0;
    while (val > 0) {
        temp[t++] = '0' + (val % 10);
        val /= 10;
    }
    int p = 0;
    while (t > 0) buf[p++] = temp[--t];
    buf[p] = '\0';
}

static int fm_str_case_cmp(const char *a, const char *b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    while (*a && *b) {
        char ca = *a;
        char cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return (int)((unsigned char)ca) - (int)((unsigned char)cb);
        a++;
        b++;
    }
    if (*a) return 1;
    if (*b) return -1;
    return 0;
}

static const char *fm_get_ext(const char *name) {
    const char *dot = 0;
    while (*name) {
        if (*name == '.') dot = name + 1;
        name++;
    }
    return dot ? dot : "";
}

static int fm_node_compare(uint16_t id_a, uint16_t id_b) {
    if (id_a == id_b) return 0;
    const gmu_fs_node_t *a = gmu_fs_get(id_a);
    const gmu_fs_node_t *b = gmu_fs_get(id_b);
    if (!a && !b) return 0;
    if (!a) return 1;
    if (!b) return -1;

    /* Folders ALWAYS come before files */
    if (a->type != b->type) {
        return (a->type == GMU_FS_DIRECTORY) ? -1 : 1;
    }

    if (a->type == GMU_FS_DIRECTORY) {
        /* Folders are always sorted alphabetically */
        int c = fm_str_case_cmp(a->name, b->name);
        if (current_sort == FM_SORT_NAME_DESC) return -c;
        return c;
    }

    /* Both are files */
    if (current_sort == FM_SORT_NAME_ASC) {
        return fm_str_case_cmp(a->name, b->name);
    } else if (current_sort == FM_SORT_NAME_DESC) {
        return -fm_str_case_cmp(a->name, b->name);
    } else if (current_sort == FM_SORT_SIZE_DESC) {
        if (b->size != a->size) return (b->size > a->size) ? 1 : -1;
        return fm_str_case_cmp(a->name, b->name);
    } else if (current_sort == FM_SORT_SIZE_ASC) {
        if (a->size != b->size) return (a->size > b->size) ? 1 : -1;
        return fm_str_case_cmp(a->name, b->name);
    } else if (current_sort == FM_SORT_TYPE) {
        const char *ext_a = fm_get_ext(a->name);
        const char *ext_b = fm_get_ext(b->name);
        int ec = fm_str_case_cmp(ext_a, ext_b);
        if (ec != 0) return ec;
        return fm_str_case_cmp(a->name, b->name);
    }

    return fm_str_case_cmp(a->name, b->name);
}

static void reload_directory_items(void) {
    fm_child_count = 0;
    for (uint16_t i = 1; i < GMU_FS_MAX_NODES; i++) {
        const gmu_fs_node_t *node = gmu_fs_get(i);
        if (node && node->parent == current_dir) {
            fm_children[fm_child_count++] = i;
        }
    }
    visible_count = fm_child_count;

    /* Sort entries according to active sort mode */
    for (uint16_t i = 1; i < fm_child_count; i++) {
        uint16_t key = fm_children[i];
        int j = (int)i - 1;
        while (j >= 0 && fm_node_compare(fm_children[j], key) > 0) {
            fm_children[j + 1] = fm_children[j];
            j--;
        }
        fm_children[j + 1] = key;
    }

    if (visible_count == 0) {
        selected_index = 0;
        scroll_offset = 0;
    } else if (selected_index >= visible_count) {
        selected_index = visible_count - 1;
    }
}

static int32_t get_fm_child(uint16_t index) {
    if (index >= fm_child_count) return -1;
    return (int32_t)fm_children[index];
}

static void draw_file_manager(gmu_framebuffer_t *fb) {
    gmu_fb_clear(fb, FM_BG);

    /* Top application bar */
    gmu_fill_rect(fb, 0, 0, fb->width, 44, FM_PANEL);
    gmu_fill_rect(fb, 0, 43, fb->width, 2, GMU_BLUE);

    gmu_draw_text(fb, 18, 14, "GMU FILE MANAGER", GMU_WHITE, FM_PANEL);

    char path_buf[64];
    gmu_fs_get_path(current_dir, path_buf, sizeof(path_buf));
    char path_display[80] = "PATH: ";
    int pp = 6;
    const char *pth = path_buf;
    while (*pth && pp < 75) path_display[pp++] = *pth++;
    path_display[pp] = '\0';
    gmu_draw_text(fb, 220, 14, path_display, FM_CYAN, FM_PANEL);

    /* Left sidebar */
    gmu_fill_rect(fb, 0, 45, 160, fb->height - 45 - 32, FM_PANEL);
    gmu_fill_rect(fb, 159, 45, 2, fb->height - 45 - 32, 0x00334155);

    gmu_draw_text(fb, 16, 60, "LOCATIONS", FM_CYAN, FM_PANEL);
    gmu_draw_text(fb, 16, 85, "1. / (ROOT)", GMU_WHITE, FM_PANEL);
    gmu_draw_text(fb, 16, 110, "2. /Config", GMU_WHITE, FM_PANEL);
    gmu_draw_text(fb, 16, 135, "3. /Documents", GMU_WHITE, FM_PANEL);
    gmu_draw_text(fb, 16, 160, "4. /Games", GMU_WHITE, FM_PANEL);

    gmu_draw_text(fb, 16, 210, "STORAGE STATS", FM_CYAN, FM_PANEL);
    char stats_line[32] = "Used: ";
    char u_buf[16];
    format_num(gmu_fs_total_used_bytes(), u_buf);
    int sp = 6; const char *su = u_buf; while (*su) stats_line[sp++] = *su++;
    stats_line[sp++] = ' '; stats_line[sp++] = 'B'; stats_line[sp] = '\0';
    gmu_draw_text(fb, 16, 235, stats_line, FM_MUTED, FM_PANEL);

    char nodes_line[32] = "Nodes: ";
    char n_buf[16];
    format_num(gmu_fs_total_nodes_used(), n_buf);
    int np = 7; const char *sn = n_buf; while (*sn) nodes_line[np++] = *sn++;
    nodes_line[np++] = '/'; nodes_line[np++] = '1'; nodes_line[np++] = '2'; nodes_line[np++] = '8'; nodes_line[np] = '\0';
    gmu_draw_text(fb, 16, 255, nodes_line, FM_MUTED, FM_PANEL);

    /* Table column headers */
    gmu_fill_rect(fb, 165, 50, fb->width - 175, 24, 0x001E293B);

    uint32_t type_col = (current_sort == FM_SORT_TYPE) ? FM_CYAN : FM_MUTED;
    uint32_t name_col = (current_sort == FM_SORT_NAME_ASC || current_sort == FM_SORT_NAME_DESC) ? FM_CYAN : FM_MUTED;
    uint32_t size_col = (current_sort == FM_SORT_SIZE_DESC || current_sort == FM_SORT_SIZE_ASC) ? FM_CYAN : FM_MUTED;

    const char *name_label = "NAME [A-Z]";
    if (current_sort == FM_SORT_NAME_DESC) name_label = "NAME [Z-A]";

    const char *size_label = "SIZE";
    if (current_sort == FM_SORT_SIZE_DESC) size_label = "SIZE [v]";
    else if (current_sort == FM_SORT_SIZE_ASC) size_label = "SIZE [^]";

    const char *type_label = (current_sort == FM_SORT_TYPE) ? "TYPE [*]" : "TYPE";

    gmu_draw_text(fb, 180, 56, type_label, type_col, 0x001E293B);
    gmu_draw_text(fb, 260, 56, name_label, name_col, 0x001E293B);
    gmu_draw_text(fb, 520, 56, size_label, size_col, 0x001E293B);
    gmu_draw_text(fb, 640, 56, "DETAILS", FM_MUTED, 0x001E293B);

    uint16_t max_visible = (fb->height > 120) ? (uint16_t)((fb->height - 112) / 26) : 1;

    if (visible_count == 0) {
        gmu_draw_text(fb, 220, 120, "This directory is currently empty.", FM_MUTED, FM_BG);
        gmu_draw_text(fb, 220, 140, "Press 'N' to create a folder, or 'F' to create a file.", FM_CYAN, FM_BG);
        selected_index = 0;
        scroll_offset = 0;
    }

    if (selected_index >= visible_count && visible_count > 0) {
        selected_index = visible_count - 1;
    }

    if (selected_index < scroll_offset) {
        scroll_offset = selected_index;
    } else if (selected_index >= scroll_offset + max_visible) {
        scroll_offset = selected_index - max_visible + 1;
    }

    if (visible_count <= max_visible) {
        scroll_offset = 0;
    } else if (scroll_offset + max_visible > visible_count) {
        scroll_offset = visible_count - max_visible;
    }

    for (uint16_t r = 0; r < max_visible; r++) {
        uint16_t i = scroll_offset + r;
        if (i >= visible_count) break;

        int32_t cid = get_fm_child(i);
        if (cid < 0) continue;
        const gmu_fs_node_t *node = gmu_fs_get((uint16_t)cid);
        if (!node) continue;

        uint64_t y = 82 + (uint64_t)r * 26;
        uint32_t bg = (i == selected_index) ? FM_SELECTED : FM_BG;

        if (i == selected_index) {
            gmu_fill_rect(fb, 170, y - 4, fb->width - 185, 24, FM_SELECTED);
        }

        if (node->type == GMU_FS_DIRECTORY) {
            gmu_draw_text(fb, 180, y, "[DIR]", FM_CYAN, bg);
            gmu_draw_text(fb, 260, y, node->name, GMU_WHITE, bg);
            gmu_draw_text(fb, 520, y, "--", FM_MUTED, bg);
            gmu_draw_text(fb, 640, y, "Folder (Press ENTER)", FM_YELLOW, bg);
        } else {
            gmu_draw_text(fb, 180, y, "[FILE]", GMU_WHITE, bg);
            gmu_draw_text(fb, 260, y, node->name, GMU_WHITE, bg);

            char s_buf[16];
            format_num(node->size, s_buf);
            char sz_str[24];
            int sp = 0; const char *p = s_buf; while (*p) sz_str[sp++] = *p++;
            sz_str[sp++] = ' '; sz_str[sp++] = 'B'; sz_str[sp] = '\0';
            gmu_draw_text(fb, 520, y, sz_str, FM_WHITE, bg);
            gmu_draw_text(fb, 640, y, "Text / Binary (V/ENTER)", FM_GREEN, bg);
        }
    }

    /* Scroll bar indicator if more items than fit */
    if (visible_count > max_visible) {
        uint64_t track_y = 80;
        uint64_t track_h = (uint64_t)max_visible * 26;
        gmu_fill_rect(fb, fb->width - 8, track_y, 4, track_h, 0x001E293B);

        uint64_t thumb_h = (track_h * max_visible) / visible_count;
        if (thumb_h < 12) thumb_h = 12;
        uint64_t thumb_y = track_y + ((track_h - thumb_h) * scroll_offset) / (visible_count - max_visible);
        gmu_fill_rect(fb, fb->width - 8, thumb_y, 4, thumb_h, FM_CYAN);
    }

    /* Bottom footer toolbar */
    gmu_fill_rect(fb, 0, fb->height - 30, fb->width, 30, FM_PANEL);
    gmu_fill_rect(fb, 0, fb->height - 30, fb->width, 1, 0x00334155);

    gmu_draw_text(fb, 16, fb->height - 20,
        "E: NOTEPAD   V/ENTER: VIEW   N: FOLDER   F: FILE   S: SORT   R: RENAME   D: DELETE   ESC: EXIT",
        GMU_WHITE, FM_PANEL);
}

static void draw_name_dialog(gmu_framebuffer_t *fb,
                             const char *title,
                             const char *name,
                             const char *message) {
    uint64_t w = 400;
    uint64_t h = 150;
    uint64_t x = (fb->width - w) / 2;
    uint64_t y = (fb->height - h) / 2;

    gmu_fill_rect(fb, x, y, w, h, FM_PANEL);
    gmu_draw_rect(fb, x, y, w, h, GMU_BLUE);
    gmu_draw_rect(fb, x + 1, y + 1, w - 2, h - 2, GMU_BLUE);

    gmu_draw_text(fb, x + 20, y + 18, title, GMU_WHITE, FM_PANEL);
    gmu_draw_text(fb, x + 20, y + 48, "NAME:", FM_CYAN, FM_PANEL);

    gmu_fill_rect(fb, x + 20, y + 66, w - 40, 24, FM_BG);
    gmu_draw_text(fb, x + 28, y + 74, name, GMU_WHITE, FM_BG);

    gmu_draw_text(fb, x + 20, y + 102, message, FM_CYAN, FM_PANEL);
    gmu_draw_text(fb, x + 20, y + 125, "ENTER: OK    ESC: CANCEL", FM_MUTED, FM_PANEL);
}

static void view_file_modal(gmu_framebuffer_t *fb, uint16_t file_id) {
    const gmu_fs_node_t *node = gmu_fs_get(file_id);
    if (!node) return;

    uint64_t w = fb->width > 800 ? 760 : fb->width - 40;
    uint64_t h = fb->height > 500 ? 440 : fb->height - 40;
    uint64_t x = (fb->width - w) / 2;
    uint64_t y = (fb->height - h) / 2;

    gmu_fill_rect(fb, x, y, w, h, FM_PANEL);
    gmu_draw_rect(fb, x, y, w, h, GMU_BLUE);

    /* Modal header */
    gmu_fill_rect(fb, x, y, w, 32, 0x000F172A);
    char title[96] = "VIEWING: ";
    int tp = 9;
    const char *n = node->name; while (*n && tp < 50) title[tp++] = *n++;
    title[tp++] = ' '; title[tp++] = '(';
    char sb[16]; format_num(node->size, sb);
    const char *s = sb; while (*s && tp < 70) title[tp++] = *s++;
    title[tp++] = ' '; title[tp++] = 'B'; title[tp++] = ')';
    title[tp] = '\0';
    gmu_draw_text(fb, x + 16, y + 10, title, FM_CYAN, 0x000F172A);

    /* Text display area */
    uint64_t content_x = x + 18;
    uint64_t content_y = y + 44;
    char line_buf[128];
    uint32_t lp = 0;

    for (uint32_t i = 0; i < node->size; i++) {
        char c = (char)node->data[i];
        if (c == '\n' || lp >= sizeof(line_buf) - 1) {
            line_buf[lp] = '\0';
            if (content_y + 12 < y + h - 30) {
                gmu_draw_text(fb, content_x, content_y, line_buf, GMU_WHITE, FM_PANEL);
                content_y += 13;
            }
            lp = 0;
        } else if (c != '\r') {
            line_buf[lp++] = c;
        }
    }
    if (lp > 0 && content_y + 12 < y + h - 30) {
        line_buf[lp] = '\0';
        gmu_draw_text(fb, content_x, content_y, line_buf, GMU_WHITE, FM_PANEL);
    }

    /* Modal footer */
    gmu_fill_rect(fb, x, y + h - 28, w, 28, 0x000F172A);
    gmu_draw_text(fb, x + 16, y + h - 18, "PRESS ESC OR ENTER TO CLOSE VIEWER", FM_MUTED, 0x000F172A);

    for (;;) {
        uint16_t key = gmu_keyboard_read_key();
        if (key == GMU_KEY_ESCAPE || key == GMU_KEY_ENTER || key == '\n') {
            break;
        }
    }
}

static void create_item(gmu_framebuffer_t *fb, gmu_fs_type_t type) {
    char name[GMU_FS_NAME_MAX] = {0};
    uint32_t length = 0;
    const char *title = (type == GMU_FS_DIRECTORY) ? "CREATE FOLDER" : "CREATE FILE";
    const char *message = "Alphanumeric, -, ., _";

    draw_name_dialog(fb, title, name, message);

    for (;;) {
        uint16_t key = gmu_keyboard_read_key();
        if (!key) continue;

        if (key == GMU_KEY_ESCAPE) return;

        if (key == GMU_KEY_ENTER || key == '\n') {
            if (length > 0) {
                int32_t id = gmu_fs_create(current_dir, name, type);
                if (id >= 0) {
                    if (type == GMU_FS_FILE) {
                        /* Provide default template */
                        static const char initial[] = "GMU OS 64 File Created\n";
                        gmu_fs_write((uint16_t)id, initial, sizeof(initial) - 1);
                    }
                    reload_directory_items();
                    /* Select the newly created item */
                    for (uint16_t i = 0; i < visible_count; i++) {
                        if (get_fm_child(i) == id) {
                            selected_index = i;
                            break;
                        }
                    }
                    return;
                }
                draw_name_dialog(fb, title, name, "NAME EXISTS OR INVALID");
            }
            continue;
        }

        if (key == '\b') {
            if (length > 0) {
                name[--length] = '\0';
                draw_name_dialog(fb, title, name, message);
            }
            continue;
        }

        if (key <= 0xFF && length < GMU_FS_NAME_MAX - 1) {
            char c = (char)key;
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_') {
                name[length++] = c;
                name[length] = '\0';
                draw_name_dialog(fb, title, name, message);
            }
        }
    }
}

static void delete_selected_item(gmu_framebuffer_t *fb) {
    int32_t cid = get_fm_child(selected_index);
    if (cid < 0) return;

    const gmu_fs_node_t *node = gmu_fs_get((uint16_t)cid);
    if (!node) return;

    draw_name_dialog(fb, "CONFIRM DELETE?", node->name, "Y: CONFIRM   N: CANCEL");

    for (;;) {
        uint16_t key = gmu_keyboard_read_key();
        if (!key) continue;

        if (key == GMU_KEY_ESCAPE || key == 'N' || key == 'n') return;

        if (key == 'Y' || key == 'y') {
            if (gmu_fs_delete((uint16_t)cid)) {
                reload_directory_items();
            } else {
                draw_name_dialog(fb, "DELETE FAILED", node->name, "DIRECTORY IS NOT EMPTY");
                for (;;) {
                    if (gmu_keyboard_read_key() != 0) break;
                }
            }
            return;
        }
    }
}

static void rename_selected_item(gmu_framebuffer_t *fb) {
    int32_t cid = get_fm_child(selected_index);
    if (cid < 0) return;

    const gmu_fs_node_t *node = gmu_fs_get((uint16_t)cid);
    if (!node) return;

    char name[GMU_FS_NAME_MAX] = {0};
    uint32_t length = 0;
    while (node->name[length] && length < GMU_FS_NAME_MAX - 1) {
        name[length] = node->name[length];
        length++;
    }
    name[length] = '\0';

    draw_name_dialog(fb, "RENAME ITEM", name, "ENTER: SAVE   ESC: CANCEL");

    for (;;) {
        uint16_t key = gmu_keyboard_read_key();
        if (!key) continue;

        if (key == GMU_KEY_ESCAPE) return;

        if (key == GMU_KEY_ENTER || key == '\n') {
            if (length > 0) {
                if (gmu_fs_rename((uint16_t)cid, name)) {
                    reload_directory_items();
                    /* Keep selection on renamed item */
                    for (uint16_t i = 0; i < visible_count; i++) {
                        if (get_fm_child(i) == cid) {
                            selected_index = i;
                            break;
                        }
                    }
                    return;
                }
                draw_name_dialog(fb, "RENAME FAILED", name, "NAME EXISTS OR INVALID");
            }
            continue;
        }

        if (key == '\b') {
            if (length > 0) {
                name[--length] = '\0';
                draw_name_dialog(fb, "RENAME ITEM", name, "ENTER: SAVE   ESC: CANCEL");
            }
            continue;
        }

        if (key <= 0xFF && length < GMU_FS_NAME_MAX - 1) {
            char c = (char)key;
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_') {
                name[length++] = c;
                name[length] = '\0';
                draw_name_dialog(fb, "RENAME ITEM", name, "ENTER: SAVE   ESC: CANCEL");
            }
        }
    }
}

void gmu_file_manager_run(gmu_framebuffer_t *fb) {
    if (!fb || !fb->initialized) return;

    current_dir = GMU_FS_ROOT_ID;
    selected_index = 0;
    scroll_offset = 0;
    current_sort = FM_SORT_NAME_ASC;
    reload_directory_items();

    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);
    uint8_t prev_buttons = 0;

    draw_file_manager(fb);

    for (;;) {
        bool needs_redraw = false;

        /* Mouse Input */
        if (mouse_ready && gmu_mouse_poll(&mouse)) {
            bool left_click = (mouse.buttons & 1) && !(prev_buttons & 1);
            prev_buttons = mouse.buttons;

            if (left_click) {
                /* Click Red Close Chiclet */
                if (mouse.x >= 10 && mouse.x <= 26 && mouse.y >= 8 && mouse.y <= 24) {
                    return;
                }

                /* Click Left Sidebar Locations (Alphabetical: ROOT, Config, Documents, Games) */
                if (mouse.x >= 12 && mouse.x < 155) {
                    if (mouse.y >= 80 && mouse.y < 105) {
                        current_dir = GMU_FS_ROOT_ID;
                        selected_index = 0;
                        scroll_offset = 0;
                        reload_directory_items();
                        needs_redraw = true;
                    } else if (mouse.y >= 105 && mouse.y < 130) {
                        int32_t d = gmu_fs_lookup(GMU_FS_ROOT_ID, "Config");
                        if (d >= 0) {
                            current_dir = (uint16_t)d;
                            selected_index = 0;
                            scroll_offset = 0;
                            reload_directory_items();
                            needs_redraw = true;
                        }
                    } else if (mouse.y >= 130 && mouse.y < 155) {
                        int32_t d = gmu_fs_lookup(GMU_FS_ROOT_ID, "Documents");
                        if (d >= 0) {
                            current_dir = (uint16_t)d;
                            selected_index = 0;
                            scroll_offset = 0;
                            reload_directory_items();
                            needs_redraw = true;
                        }
                    } else if (mouse.y >= 155 && mouse.y < 180) {
                        int32_t d = gmu_fs_lookup(GMU_FS_ROOT_ID, "Games");
                        if (d >= 0) {
                            current_dir = (uint16_t)d;
                            selected_index = 0;
                            scroll_offset = 0;
                            reload_directory_items();
                            needs_redraw = true;
                        }
                    }
                }

                /* Click Column Headers to Sort */
                if (mouse.y >= 48 && mouse.y <= 74 && mouse.x >= 165) {
                    if (mouse.x >= 170 && mouse.x < 240) {
                        /* Click TYPE header */
                        current_sort = (current_sort == FM_SORT_TYPE) ? FM_SORT_NAME_ASC : FM_SORT_TYPE;
                        reload_directory_items();
                        needs_redraw = true;
                    } else if (mouse.x >= 240 && mouse.x < 500) {
                        /* Click NAME header: toggle ASC / DESC */
                        current_sort = (current_sort == FM_SORT_NAME_ASC) ? FM_SORT_NAME_DESC : FM_SORT_NAME_ASC;
                        reload_directory_items();
                        needs_redraw = true;
                    } else if (mouse.x >= 500 && mouse.x < 620) {
                        /* Click SIZE header: toggle DESC / ASC */
                        current_sort = (current_sort == FM_SORT_SIZE_DESC) ? FM_SORT_SIZE_ASC : FM_SORT_SIZE_DESC;
                        reload_directory_items();
                        needs_redraw = true;
                    }
                }

                /* Click File Rows */
                if (mouse.x >= 165 && mouse.x < (int32_t)fb->width - 10 && mouse.y >= 82) {
                    uint16_t max_visible = (fb->height > 120) ? (uint16_t)((fb->height - 112) / 26) : 1;
                    int32_t row = (mouse.y - 82) / 26;
                    if (row >= 0 && row < (int32_t)max_visible) {
                        uint16_t i = scroll_offset + (uint16_t)row;
                        if (i < visible_count) {
                            if (selected_index == i) {
                                /* Double click on same item -> open */
                                int32_t cid = get_fm_child(i);
                                if (cid >= 0) {
                                    const gmu_fs_node_t *node = gmu_fs_get((uint16_t)cid);
                                    if (node) {
                                        if (node->type == GMU_FS_DIRECTORY) {
                                            current_dir = (uint16_t)cid;
                                            selected_index = 0;
                                            scroll_offset = 0;
                                            reload_directory_items();
                                        } else {
                                            view_file_modal(fb, (uint16_t)cid);
                                        }
                                    }
                                }
                            } else {
                                selected_index = i;
                            }
                            needs_redraw = true;
                        }
                    }
                }
            }
        }

        uint16_t key = gmu_keyboard_read_key();
        if (key != 0) {
            if (key == GMU_KEY_ESCAPE) return;

            if (key == 'E' || key == 'e') {
                int32_t cid = get_fm_child(selected_index);
                if (cid >= 0) {
                    const gmu_fs_node_t *node = gmu_fs_get((uint16_t)cid);
                    if (node && node->type == GMU_FS_FILE) {
                        char full_path[64];
                        gmu_fs_get_path((uint16_t)cid, full_path, sizeof(full_path));
                        gmu_editor_open_file(fb, full_path);
                        gmu_keyboard_init();
                        reload_directory_items();
                    }
                }
                needs_redraw = true;
            } else if (key == 'S' || key == 's') {
                /* Cycle sorting modes */
                current_sort = (current_sort + 1) % FM_SORT_COUNT;
                reload_directory_items();
                needs_redraw = true;
            } else if (key == 'N' || key == 'n') {
                create_item(fb, GMU_FS_DIRECTORY);
                needs_redraw = true;
            } else if (key == 'F' || key == 'f') {
                create_item(fb, GMU_FS_FILE);
                needs_redraw = true;
            } else if (key == 'D' || key == 'd') {
                delete_selected_item(fb);
                needs_redraw = true;
            } else if (key == 'R' || key == 'r') {
                rename_selected_item(fb);
                needs_redraw = true;
            } else if (key == '1') {
                current_dir = GMU_FS_ROOT_ID;
                selected_index = 0;
                scroll_offset = 0;
                reload_directory_items();
                needs_redraw = true;
            } else if (key == '2') {
                int32_t d = gmu_fs_lookup(GMU_FS_ROOT_ID, "Config");
                if (d >= 0) {
                    current_dir = (uint16_t)d;
                    selected_index = 0;
                    scroll_offset = 0;
                    reload_directory_items();
                }
                needs_redraw = true;
            } else if (key == '3') {
                int32_t d = gmu_fs_lookup(GMU_FS_ROOT_ID, "Documents");
                if (d >= 0) {
                    current_dir = (uint16_t)d;
                    selected_index = 0;
                    scroll_offset = 0;
                    reload_directory_items();
                }
                needs_redraw = true;
            } else if (key == '4') {
                int32_t d = gmu_fs_lookup(GMU_FS_ROOT_ID, "Games");
                if (d >= 0) {
                    current_dir = (uint16_t)d;
                    selected_index = 0;
                    scroll_offset = 0;
                    reload_directory_items();
                }
                needs_redraw = true;
            } else if (key == GMU_KEY_UP) {
                if (selected_index > 0) selected_index--;
                needs_redraw = true;
            } else if (key == GMU_KEY_DOWN) {
                if (selected_index + 1 < visible_count) selected_index++;
                needs_redraw = true;
            } else if (key == GMU_KEY_PAGE_UP) {
                uint16_t max_visible = (fb->height > 120) ? (uint16_t)((fb->height - 112) / 26) : 1;
                if (selected_index >= max_visible) selected_index -= max_visible;
                else selected_index = 0;
                needs_redraw = true;
            } else if (key == GMU_KEY_PAGE_DOWN) {
                uint16_t max_visible = (fb->height > 120) ? (uint16_t)((fb->height - 112) / 26) : 1;
                selected_index += max_visible;
                if (selected_index >= visible_count && visible_count > 0)
                    selected_index = visible_count - 1;
                needs_redraw = true;
            } else if (key == '\b') {
                if (current_dir != GMU_FS_ROOT_ID) {
                    const gmu_fs_node_t *node = gmu_fs_get(current_dir);
                    if (node) {
                        current_dir = node->parent;
                        selected_index = 0;
                        scroll_offset = 0;
                        reload_directory_items();
                    }
                }
                needs_redraw = true;
            } else if (key == '\n' || key == GMU_KEY_ENTER || key == 'V' || key == 'v') {
                int32_t cid = get_fm_child(selected_index);
                if (cid >= 0) {
                    const gmu_fs_node_t *node = gmu_fs_get((uint16_t)cid);
                    if (node) {
                        if (node->type == GMU_FS_DIRECTORY) {
                            current_dir = (uint16_t)cid;
                            selected_index = 0;
                            scroll_offset = 0;
                            reload_directory_items();
                        } else {
                            view_file_modal(fb, (uint16_t)cid);
                        }
                    }
                }
                needs_redraw = true;
            }
        }

        if (needs_redraw) {
            draw_file_manager(fb);
        }

        __asm__ volatile ("pause");
    }
}
