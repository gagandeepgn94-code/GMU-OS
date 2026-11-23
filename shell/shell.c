#include <stdint.h>
#include <stdbool.h>
#include <gmu/shell.h>
#include <gmu/keyboard.h>
#include <gmu/mouse.h>
#include <gmu/font.h>
#include <gmu/draw.h>
#include <gmu/desktop.h>
#include <gmu/app.h>
#include <gmu/fs.h>
#include <gmu/pmm.h>
#include <gmu/heap.h>
#include <gmu/timer.h>
#include <gmu/editor.h>
#include <gmu/calculator.h>
#include <gmu/monitor.h>
#include <gmu/settings.h>
#include <gmu/theme.h>
#include <gmu/paint.h>
#include <gmu/calendar.h>
#include <gmu/hexview.h>
#include <gmu/snake.h>
#include <gmu/minesweeper.h>
#include <gmu/breakout.h>
#include <gmu/game_center.h>

#define TERM_BG      0x000B1220
#define TERM_PANEL   0x001B263B
#define TERM_TEXT    0x00E2E8F0
#define TERM_GREEN   0x0022C55E
#define TERM_CYAN    0x0006B6D4
#define TERM_RED     0x00EF4444
#define TERM_YELLOW  0x00FBBF24
#define TERM_MUTED   0x0094A3B8
#define TERM_PURPLE  0x00A855F7

#define MAX_INPUT       128
#define LINE_HEIGHT     11
#define HISTORY_COUNT   16

static uint64_t cursor_y;
static uint16_t current_dir = GMU_FS_ROOT_ID;

static char history[HISTORY_COUNT][MAX_INPUT];
static int history_count = 0;
static int history_index = -1;

static void print_line(gmu_framebuffer_t *fb, const char *text, uint32_t color) {
    if (cursor_y + LINE_HEIGHT >= fb->height - 12) {
        gmu_fb_clear(fb, TERM_BG);
        cursor_y = 16;
        gmu_draw_text(fb, 16, cursor_y, "=== GMU OS 64 TERMINAL ===", TERM_CYAN, TERM_BG);
        cursor_y += 20;
    }

    gmu_draw_text(fb, 16, cursor_y, text, color, TERM_BG);
    cursor_y += LINE_HEIGHT;
}

static bool str_equals(const char *a, const char *b) {
    while (*a && *b) {
        char ca = *a;
        char cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return false;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static void reboot_system(void) {
    /* 8042 keyboard controller reset */
    uint8_t good = 0x02;
    while (good & 0x02) {
        __asm__ volatile ("inb $0x64, %0" : "=a"(good));
    }
    __asm__ volatile ("outb %0, $0x64" :: "a"((uint8_t)0xFE));
    for (;;) __asm__ volatile ("cli; hlt");
}

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

static void cmd_help(gmu_framebuffer_t *fb) {
    print_line(fb, "GMU OS 64 - SYSTEM COMMANDS", TERM_CYAN);
    print_line(fb, "--------------------------------------------------", TERM_MUTED);
    print_line(fb, "FILESYSTEM:", TERM_YELLOW);
    print_line(fb, "  LS [dir]            - List directory contents", TERM_TEXT);
    print_line(fb, "  CD <dir>            - Change directory (.., /, path)", TERM_TEXT);
    print_line(fb, "  PWD                 - Print working directory", TERM_TEXT);
    print_line(fb, "  CAT <file>          - View text file contents", TERM_TEXT);
    print_line(fb, "  TOUCH <file>        - Create empty file", TERM_TEXT);
    print_line(fb, "  WRITE <file> <text> - Write text to file", TERM_TEXT);
    print_line(fb, "  APPEND <file> <txt> - Append text to file", TERM_TEXT);
    print_line(fb, "  MKDIR <name>        - Create directory", TERM_TEXT);
    print_line(fb, "  RM <name>           - Delete file or empty directory", TERM_TEXT);
    print_line(fb, "  STAT <name>         - View node details & size", TERM_TEXT);
    print_line(fb, "SYSTEM & UTILITIES:", TERM_YELLOW);
    print_line(fb, "  SYSINFO             - System hardware specs & banner", TERM_TEXT);
    print_line(fb, "  MEM                 - Physical & heap memory stats", TERM_TEXT);
    print_line(fb, "  UPTIME              - System running time", TERM_TEXT);
    print_line(fb, "  EDIT / NOTEPAD [f]  - Launch GUI Notepad text editor", TERM_TEXT);
    print_line(fb, "  PAINT               - Launch Paint & Pixel Art Studio", TERM_TEXT);
    print_line(fb, "  CALENDAR / CLOCK    - Launch Calendar, World Clock & Stopwatch", TERM_TEXT);
    print_line(fb, "  HEXVIEW / HEX [f]   - Hex & Memory Inspector", TERM_TEXT);
    print_line(fb, "  CALC [expression]   - Graphical or inline calculator", TERM_TEXT);
    print_line(fb, "  MONITOR / TOP       - Launch Activity Monitor", TERM_TEXT);
    print_line(fb, "  SETTINGS / THEME    - System settings & themes", TERM_TEXT);
    print_line(fb, "  CLEAR / CLS         - Clear screen", TERM_TEXT);
    print_line(fb, "  DESKTOP / EXIT      - Return to graphical desktop", TERM_TEXT);
    print_line(fb, "GAMES & ARCADE:", TERM_YELLOW);
    print_line(fb, "  ARCADE / GAMES      - Launch Arcade Hub (Snake, Mines, Breakout, Strike)", TERM_TEXT);
    print_line(fb, "  SNAKE               - Launch GMU Snake retro arcade", TERM_TEXT);
    print_line(fb, "  MINES / MINESWEEPER - Launch Minesweeper classic", TERM_TEXT);
    print_line(fb, "  BREAKOUT            - Launch Breakout brick breaker", TERM_TEXT);
    print_line(fb, "  STRIKE              - Launch GMU Strike space arcade", TERM_TEXT);
    print_line(fb, "  REBOOT / HALT       - Restart or stop computer", TERM_TEXT);
}

static void cmd_ls(gmu_framebuffer_t *fb, const char *arg) {
    uint16_t target = current_dir;
    if (arg && arg[0]) {
        int32_t resolved = gmu_fs_resolve_path(current_dir, arg);
        if (resolved < 0) {
            print_line(fb, "Directory not found.", TERM_RED);
            return;
        }
        target = (uint16_t)resolved;
    }

    const gmu_fs_node_t *dir_node = gmu_fs_get(target);
    if (!dir_node || dir_node->type != GMU_FS_DIRECTORY) {
        print_line(fb, "Target is not a directory.", TERM_RED);
        return;
    }

    char path_buf[64];
    gmu_fs_get_path(target, path_buf, sizeof(path_buf));

    char header[96] = "Directory of ";
    uint32_t hp = 13;
    const char *p = path_buf;
    while (*p && hp < sizeof(header) - 1) header[hp++] = *p++;
    header[hp] = '\0';
    print_line(fb, header, TERM_CYAN);

    uint16_t count = gmu_fs_count(target);
    if (count == 0) {
        print_line(fb, "  (empty directory)", TERM_MUTED);
        return;
    }

    for (uint16_t i = 0; i < count; i++) {
        int32_t cid = gmu_fs_get_child(target, i);
        if (cid < 0) continue;
        const gmu_fs_node_t *node = gmu_fs_get((uint16_t)cid);
        if (!node) continue;

        char size_buf[16];
        format_num(node->size, size_buf);

        if (node->type == GMU_FS_DIRECTORY) {
            char dline[80] = "  [DIR]  ";
            int dp = 9;
            const char *n = node->name;
            while (*n && dp < 50) dline[dp++] = *n++;
            dline[dp] = '\0';
            print_line(fb, dline, TERM_CYAN);
        } else {
            char fline[80] = "  [FILE] ";
            int fp = 9;
            const char *n = node->name;
            while (*n && fp < 35) fline[fp++] = *n++;
            while (fp < 36) fline[fp++] = ' ';
            const char *s = size_buf;
            while (*s && fp < 50) fline[fp++] = *s++;
            fline[fp++] = ' ';
            fline[fp++] = 'B';
            fline[fp] = '\0';
            print_line(fb, fline, TERM_TEXT);
        }
    }
}

static void cmd_cd(gmu_framebuffer_t *fb, const char *arg) {
    if (!arg || !arg[0] || str_equals(arg, "~") || str_equals(arg, "/")) {
        current_dir = GMU_FS_ROOT_ID;
        return;
    }

    int32_t target = gmu_fs_resolve_path(current_dir, arg);
    if (target < 0) {
        print_line(fb, "Directory does not exist.", TERM_RED);
        return;
    }

    const gmu_fs_node_t *node = gmu_fs_get((uint16_t)target);
    if (!node || node->type != GMU_FS_DIRECTORY) {
        print_line(fb, "Path is not a directory.", TERM_RED);
        return;
    }

    current_dir = (uint16_t)target;
}

static void cmd_pwd(gmu_framebuffer_t *fb) {
    char path[128];
    gmu_fs_get_path(current_dir, path, sizeof(path));
    print_line(fb, path, TERM_GREEN);
}

static void cmd_cat(gmu_framebuffer_t *fb, const char *arg) {
    if (!arg || !arg[0]) {
        print_line(fb, "Usage: CAT <filename>", TERM_YELLOW);
        return;
    }

    int32_t fid = gmu_fs_resolve_path(current_dir, arg);
    if (fid < 0) {
        print_line(fb, "File not found.", TERM_RED);
        return;
    }

    const gmu_fs_node_t *node = gmu_fs_get((uint16_t)fid);
    if (!node || node->type != GMU_FS_FILE) {
        print_line(fb, "Target is not a file.", TERM_RED);
        return;
    }

    if (node->size == 0) {
        print_line(fb, "(file is empty)", TERM_MUTED);
        return;
    }

    char line[128];
    uint32_t lp = 0;

    for (uint32_t i = 0; i < node->size; i++) {
        char c = (char)node->data[i];
        if (c == '\n' || lp >= sizeof(line) - 1) {
            line[lp] = '\0';
            print_line(fb, line, TERM_TEXT);
            lp = 0;
        } else if (c != '\r') {
            line[lp++] = c;
        }
    }
    if (lp > 0) {
        line[lp] = '\0';
        print_line(fb, line, TERM_TEXT);
    }
}

static void cmd_sysinfo(gmu_framebuffer_t *fb) {
    print_line(fb, "   ______  __  __ __  __     ____   _____   ____  _  _", TERM_CYAN);
    print_line(fb, "  / ____/ / / / // / / /    / __ \\ / ___/  / __ \\| |/ /", TERM_CYAN);
    print_line(fb, " / / __  / / / // / / /    / / / / \\__ \\  / /_/ /|   / ", TERM_CYAN);
    print_line(fb, "/ /_/ / / /_/ // /_/ /    / /_/ / ___/ / / ____//   |  ", TERM_CYAN);
    print_line(fb, "\\____/  \\____/ \\____/     \\____/ /____/ /_/    /_/|_|  ", TERM_CYAN);
    print_line(fb, "-------------------------------------------------------", TERM_MUTED);
    print_line(fb, "OS:          GMU OS 64 (Modern Long Mode x86_64)", TERM_GREEN);
    print_line(fb, "Affiliation: GM University, Davanagere, Karnataka", TERM_TEXT);
    print_line(fb, "Course:      UE25CS2304 Operating System with Practice", TERM_TEXT);
    print_line(fb, "Bootloader:  Limine Bootloader Protocol v3 (UEFI)", TERM_TEXT);

    char res_buf[64] = "Display:     ";
    char w_buf[12], h_buf[12];
    format_num(fb->width, w_buf);
    format_num(fb->height, h_buf);
    int rp = 13;
    const char *rw = w_buf; while (*rw) res_buf[rp++] = *rw++;
    res_buf[rp++] = 'x';
    const char *rh = h_buf; while (*rh) res_buf[rp++] = *rh++;
    res_buf[rp++] = ' '; res_buf[rp++] = '3'; res_buf[rp++] = '2';
    res_buf[rp++] = 'b'; res_buf[rp++] = 'p'; res_buf[rp++] = 'p';
    res_buf[rp] = '\0';
    print_line(fb, res_buf, TERM_TEXT);

    gmu_mem_stats_t mem = gmu_pmm_get_stats();
    char mem_line[80] = "Memory:      ";
    char used_mb[16], total_mb[16];
    format_num(mem.used_memory / (1024 * 1024), used_mb);
    format_num(mem.usable_memory / (1024 * 1024), total_mb);
    int mp = 13;
    const char *u = used_mb; while (*u) mem_line[mp++] = *u++;
    mem_line[mp++] = ' '; mem_line[mp++] = 'M'; mem_line[mp++] = 'B';
    mem_line[mp++] = ' '; mem_line[mp++] = '/'; mem_line[mp++] = ' ';
    const char *t = total_mb; while (*t) mem_line[mp++] = *t++;
    mem_line[mp++] = ' '; mem_line[mp++] = 'M'; mem_line[mp++] = 'B';
    mem_line[mp] = '\0';
    print_line(fb, mem_line, TERM_YELLOW);

    char fs_line[80] = "RAMFS:       ";
    char fcount[12], fbytes[16];
    format_num(gmu_fs_total_nodes_used(), fcount);
    format_num(gmu_fs_total_used_bytes(), fbytes);
    int fsp = 13;
    const char *fc = fcount; while (*fc) fs_line[fsp++] = *fc++;
    fs_line[fsp++] = ' '; fs_line[fsp++] = 'n'; fs_line[fsp++] = 'o'; fs_line[fsp++] = 'd'; fs_line[fsp++] = 'e'; fs_line[fsp++] = 's';
    fs_line[fsp++] = ' '; fs_line[fsp++] = '(';
    const char *fb_str = fbytes; while (*fb_str) fs_line[fsp++] = *fb_str++;
    fs_line[fsp++] = ' '; fs_line[fsp++] = 'B'; fs_line[fsp++] = ')';
    fs_line[fsp] = '\0';
    print_line(fb, fs_line, TERM_PURPLE);
}

static void cmd_mem(gmu_framebuffer_t *fb) {
    gmu_mem_stats_t mem = gmu_pmm_get_stats();
    char n1[24], n2[24];

    print_line(fb, "PHYSICAL MEMORY MANAGER (PMM):", TERM_CYAN);
    format_num(mem.usable_memory / (1024 * 1024), n1);
    char l1[60] = "  Total Usable RAM: ";
    int p = 20; const char *s = n1; while (*s) l1[p++] = *s++;
    l1[p++] = ' '; l1[p++] = 'M'; l1[p++] = 'B'; l1[p] = '\0';
    print_line(fb, l1, TERM_TEXT);

    format_num(mem.free_memory / (1024 * 1024), n2);
    char l2[60] = "  Free Physical:    ";
    p = 20; s = n2; while (*s) l2[p++] = *s++;
    l2[p++] = ' '; l2[p++] = 'M'; l2[p++] = 'B'; l2[p] = '\0';
    print_line(fb, l2, TERM_GREEN);

    print_line(fb, "KERNEL DYNAMIC HEAP:", TERM_CYAN);
    format_num(gmu_heap_get_total() / 1024, n1);
    char l3[60] = "  Heap Capacity:    ";
    p = 20; s = n1; while (*s) l3[p++] = *s++;
    l3[p++] = ' '; l3[p++] = 'K'; l3[p++] = 'B'; l3[p] = '\0';
    print_line(fb, l3, TERM_TEXT);

    format_num(gmu_heap_get_used(), n2);
    char l4[60] = "  Heap Allocated:   ";
    p = 20; s = n2; while (*s) l4[p++] = *s++;
    l4[p++] = ' '; l4[p++] = 'b'; l4[p++] = 'y'; l4[p++] = 't'; l4[p++] = 'e'; l4[p++] = 's'; l4[p] = '\0';
    print_line(fb, l4, TERM_YELLOW);
}

static void cmd_uptime(gmu_framebuffer_t *fb) {
    uint64_t total_sec = gmu_timer_seconds();
    uint64_t hrs = total_sec / 3600;
    uint64_t mins = (total_sec % 3600) / 60;
    uint64_t secs = total_sec % 60;

    char line[64] = "System Uptime: ";
    char h[8], m[8], s[8];
    format_num(hrs, h);
    format_num(mins, m);
    format_num(secs, s);

    int p = 15;
    if (hrs < 10) line[p++] = '0';
    const char *ph = h; while (*ph) line[p++] = *ph++;
    line[p++] = ':';
    if (mins < 10) line[p++] = '0';
    const char *pm = m; while (*pm) line[p++] = *pm++;
    line[p++] = ':';
    if (secs < 10) line[p++] = '0';
    const char *ps = s; while (*ps) line[p++] = *ps++;
    line[p] = '\0';
    print_line(fb, line, TERM_GREEN);
}

static void cmd_calc(gmu_framebuffer_t *fb, const char *args) {
    if (!args || !args[0]) {
        if (!gmu_app_launch(GMU_APP_CALCULATOR, fb)) {
            print_line(fb, "Usage: CALC <a> <op> <b>  (e.g. CALC 45 + 12)", TERM_YELLOW);
        } else {
            gmu_fb_clear(fb, TERM_BG);
            cursor_y = 16;
            print_line(fb, "Returned to GMU Terminal.", TERM_GREEN);
        }
        return;
    }

    uint64_t a = 0, b = 0;
    char op = 0;
    int state = 0; /* 0: reading a, 1: reading op, 2: reading b */

    while (*args) {
        if (*args == ' ') {
            if (state == 0 && a > 0) state = 1;
            else if (state == 1 && op) state = 2;
            args++;
            continue;
        }

        if (state == 0) {
            if (*args >= '0' && *args <= '9') {
                a = a * 10 + (*args - '0');
            } else {
                op = *args;
                state = 2;
            }
        } else if (state == 1) {
            op = *args;
            state = 2;
        } else if (state == 2) {
            if (*args >= '0' && *args <= '9') {
                b = b * 10 + (*args - '0');
            }
        }
        args++;
    }

    uint64_t res = 0;
    if (op == '+') res = a + b;
    else if (op == '-') res = a >= b ? a - b : 0;
    else if (op == '*') res = a * b;
    else if (op == '/') {
        if (b == 0) { print_line(fb, "Error: Division by zero", TERM_RED); return; }
        res = a / b;
    } else if (op == '%') {
        if (b == 0) { print_line(fb, "Error: Division by zero", TERM_RED); return; }
        res = a % b;
    } else {
        print_line(fb, "Unknown operator. Use + - * / %", TERM_RED);
        return;
    }

    char res_str[32];
    format_num(res, res_str);
    char out[48] = "Result: ";
    int p = 8;
    const char *r = res_str;
    while (*r) out[p++] = *r++;
    out[p] = '\0';
    print_line(fb, out, TERM_GREEN);
}

static bool execute_command(gmu_framebuffer_t *fb, const char *command) {
    while (*command == ' ') command++;
    if (*command == '\0') return true;

    /* Add to history */
    if (history_count < HISTORY_COUNT) {
        int idx = history_count++;
        int p = 0;
        while (command[p] && p < MAX_INPUT - 1) {
            history[idx][p] = command[p];
            p++;
        }
        history[idx][p] = '\0';
    } else {
        for (int i = 0; i < HISTORY_COUNT - 1; i++) {
            for (int j = 0; j < MAX_INPUT; j++) history[i][j] = history[i+1][j];
        }
        int p = 0;
        while (command[p] && p < MAX_INPUT - 1) {
            history[HISTORY_COUNT - 1][p] = command[p];
            p++;
        }
        history[HISTORY_COUNT - 1][p] = '\0';
    }
    history_index = -1;

    char cmd[32];
    const char *args = "";
    int p = 0;

    while (*command && *command != ' ' && p < 31) {
        cmd[p++] = *command++;
    }
    cmd[p] = '\0';
    while (*command == ' ') command++;
    args = command;

    if (str_equals(cmd, "HELP") || str_equals(cmd, "?")) {
        cmd_help(fb);
    } else if (str_equals(cmd, "LS") || str_equals(cmd, "DIR")) {
        cmd_ls(fb, args);
    } else if (str_equals(cmd, "CD")) {
        cmd_cd(fb, args);
    } else if (str_equals(cmd, "PWD")) {
        cmd_pwd(fb);
    } else if (str_equals(cmd, "CAT") || str_equals(cmd, "TYPE")) {
        cmd_cat(fb, args);
    } else if (str_equals(cmd, "SYSINFO") || str_equals(cmd, "NEOFETCH") || str_equals(cmd, "INFO")) {
        cmd_sysinfo(fb);
    } else if (str_equals(cmd, "MEM")) {
        cmd_mem(fb);
    } else if (str_equals(cmd, "UPTIME")) {
        cmd_uptime(fb);
    } else if (str_equals(cmd, "CALC")) {
        cmd_calc(fb, args);
    } else if (str_equals(cmd, "TOUCH")) {
        if (!args[0]) print_line(fb, "Usage: TOUCH <filename>", TERM_YELLOW);
        else {
            int32_t id = gmu_fs_create(current_dir, args, GMU_FS_FILE);
            if (id >= 0) print_line(fb, "File created.", TERM_GREEN);
            else print_line(fb, "Could not create file.", TERM_RED);
        }
    } else if (str_equals(cmd, "MKDIR")) {
        if (!args[0]) print_line(fb, "Usage: MKDIR <dirname>", TERM_YELLOW);
        else {
            int32_t id = gmu_fs_create(current_dir, args, GMU_FS_DIRECTORY);
            if (id >= 0) print_line(fb, "Directory created.", TERM_GREEN);
            else print_line(fb, "Could not create directory.", TERM_RED);
        }
    } else if (str_equals(cmd, "RM") || str_equals(cmd, "DEL")) {
        if (!args[0]) print_line(fb, "Usage: RM <name>", TERM_YELLOW);
        else {
            int32_t id = gmu_fs_lookup(current_dir, args);
            if (id >= 0 && gmu_fs_delete((uint16_t)id)) print_line(fb, "Item removed.", TERM_GREEN);
            else print_line(fb, "Could not remove item.", TERM_RED);
        }
    } else if (str_equals(cmd, "WRITE")) {
        char fname[32];
        int fp = 0;
        while (*args && *args != ' ' && fp < 31) fname[fp++] = *args++;
        fname[fp] = '\0';
        while (*args == ' ') args++;
        int32_t id = gmu_fs_lookup(current_dir, fname);
        if (id < 0) id = gmu_fs_create(current_dir, fname, GMU_FS_FILE);
        if (id >= 0) {
            uint32_t len = 0;
            while (args[len]) len++;
            gmu_fs_write((uint16_t)id, args, len);
            print_line(fb, "File written successfully.", TERM_GREEN);
        } else {
            print_line(fb, "Could not write to file.", TERM_RED);
        }
    } else if (str_equals(cmd, "APPEND")) {
        char fname[32];
        int fp = 0;
        while (*args && *args != ' ' && fp < 31) fname[fp++] = *args++;
        fname[fp] = '\0';
        while (*args == ' ') args++;
        int32_t id = gmu_fs_lookup(current_dir, fname);
        if (id >= 0) {
            uint32_t len = 0;
            while (args[len]) len++;
            gmu_fs_append((uint16_t)id, "\n", 1);
            gmu_fs_append((uint16_t)id, args, len);
            print_line(fb, "Appended to file.", TERM_GREEN);
        } else {
            print_line(fb, "File not found.", TERM_RED);
        }
    } else if (str_equals(cmd, "STAT")) {
        if (!args[0]) print_line(fb, "Usage: STAT <name>", TERM_YELLOW);
        else {
            int32_t id = gmu_fs_resolve_path(current_dir, args);
            if (id < 0) print_line(fb, "Not found.", TERM_RED);
            else {
                const gmu_fs_node_t *node = gmu_fs_get((uint16_t)id);
                char s_buf[16];
                format_num(node->size, s_buf);
                print_line(fb, node->type == GMU_FS_DIRECTORY ? "Type: DIRECTORY" : "Type: FILE", TERM_CYAN);
                char sl[40] = "Size: ";
                int p = 6; const char *sz = s_buf; while (*sz) sl[p++] = *sz++;
                sl[p++] = ' '; sl[p++] = 'B'; sl[p] = '\0';
                print_line(fb, sl, TERM_TEXT);
            }
        }
    } else if (str_equals(cmd, "CLEAR") || str_equals(cmd, "CLS")) {
        gmu_fb_clear(fb, TERM_BG);
        cursor_y = 16;
    } else if (str_equals(cmd, "DESKTOP") || str_equals(cmd, "GUI") || str_equals(cmd, "EXIT") || str_equals(cmd, "QUIT")) {
        return false;
    } else if (str_equals(cmd, "EDIT") || str_equals(cmd, "NOTEPAD")) {
        if (args && args[0]) {
            gmu_editor_open_file(fb, args);
        } else {
            gmu_editor_run(fb);
        }
        gmu_fb_clear(fb, TERM_BG);
        cursor_y = 16;
        print_line(fb, "Returned to GMU Terminal.", TERM_GREEN);
    } else if (str_equals(cmd, "MONITOR") || str_equals(cmd, "TOP")) {
        gmu_app_launch(GMU_APP_MONITOR, fb);
        gmu_fb_clear(fb, TERM_BG);
        cursor_y = 16;
        print_line(fb, "Returned to GMU Terminal.", TERM_GREEN);
    } else if (str_equals(cmd, "SETTINGS")) {
        gmu_app_launch(GMU_APP_SETTINGS, fb);
        gmu_fb_clear(fb, TERM_BG);
        cursor_y = 16;
        print_line(fb, "Returned to GMU Terminal.", TERM_GREEN);
    } else if (str_equals(cmd, "THEME")) {
        if (args && args[0] >= '1' && args[0] <= '4') {
            gmu_theme_set((gmu_theme_id_t)(args[0] - '1'));
        } else {
            gmu_theme_cycle();
        }
        const gmu_theme_t *th = gmu_theme_get();
        char msg[64] = "Switched to theme: ";
        int mp = 19; const char *tn = th->name; while (*tn) msg[mp++] = *tn++; msg[mp] = '\0';
        print_line(fb, msg, TERM_CYAN);
    } else if (str_equals(cmd, "PAINT")) {
        gmu_paint_run(fb);
        gmu_fb_clear(fb, TERM_BG);
        cursor_y = 16;
        print_line(fb, "Returned to GMU Terminal.", TERM_GREEN);
    } else if (str_equals(cmd, "CALENDAR") || str_equals(cmd, "CAL") || str_equals(cmd, "CLOCK")) {
        gmu_calendar_run(fb);
        gmu_fb_clear(fb, TERM_BG);
        cursor_y = 16;
        print_line(fb, "Returned to GMU Terminal.", TERM_GREEN);
    } else if (str_equals(cmd, "HEXVIEW") || str_equals(cmd, "HEX") || str_equals(cmd, "HD")) {
        if (args && args[0]) {
            gmu_hexview_open_file(fb, args);
        } else {
            gmu_hexview_run(fb);
        }
        gmu_fb_clear(fb, TERM_BG);
        cursor_y = 16;
        print_line(fb, "Returned to GMU Terminal.", TERM_GREEN);
    } else if (str_equals(cmd, "ARCADE") || str_equals(cmd, "GAMES")) {
        gmu_game_center_run(fb);
        gmu_fb_clear(fb, TERM_BG);
        cursor_y = 16;
        print_line(fb, "Returned to GMU Terminal.", TERM_GREEN);
    } else if (str_equals(cmd, "SNAKE")) {
        gmu_snake_run(fb);
        gmu_fb_clear(fb, TERM_BG);
        cursor_y = 16;
        print_line(fb, "Returned to GMU Terminal.", TERM_GREEN);
    } else if (str_equals(cmd, "MINES") || str_equals(cmd, "MINESWEEPER")) {
        gmu_minesweeper_run(fb);
        gmu_fb_clear(fb, TERM_BG);
        cursor_y = 16;
        print_line(fb, "Returned to GMU Terminal.", TERM_GREEN);
    } else if (str_equals(cmd, "BREAKOUT")) {
        gmu_breakout_run(fb);
        gmu_fb_clear(fb, TERM_BG);
        cursor_y = 16;
        print_line(fb, "Returned to GMU Terminal.", TERM_GREEN);
    } else if (str_equals(cmd, "STRIKE")) {
        if (!gmu_app_launch(GMU_APP_SHOOTER, fb)) {
            print_line(fb, "Could not launch GMU Strike.", TERM_RED);
        }
        gmu_fb_clear(fb, TERM_BG);
        cursor_y = 16;
        print_line(fb, "Returned to GMU Terminal.", TERM_GREEN);
    } else if (str_equals(cmd, "REBOOT")) {
        print_line(fb, "Rebooting system...", TERM_YELLOW);
        reboot_system();
    } else if (str_equals(cmd, "HALT") || str_equals(cmd, "SHUTDOWN")) {
        print_line(fb, "System halted. Safe to turn off.", TERM_RED);
        for (;;) __asm__ volatile ("cli; hlt");
    } else if (str_equals(cmd, "VERSION")) {
        print_line(fb, "GMU OS 64 - Modern UEFI Long Mode Edition 1.0", TERM_CYAN);
        print_line(fb, "GM University (Davanagere, Karnataka)", TERM_TEXT);
    } else {
        print_line(fb, "Unknown command. Type 'HELP' for commands list.", TERM_RED);
    }
    return true;
}

void gmu_shell_run(gmu_framebuffer_t *fb) {
    char input[MAX_INPUT];
    uint64_t length = 0;

    gmu_fb_clear(fb, TERM_BG);
    cursor_y = 16;

    print_line(fb, "=======================================================", TERM_CYAN);
    print_line(fb, "            GMU OS 64 - MODERN WORKSTATION            ", TERM_GREEN);
    print_line(fb, "=======================================================", TERM_CYAN);
    print_line(fb, "Type 'HELP' for commands, or 'EXIT' / ESC for GUI Desktop.", TERM_TEXT);
    print_line(fb, "", TERM_TEXT);

    gmu_keyboard_init();
    gmu_mouse_t mouse = {0};
    bool mouse_ready = gmu_mouse_init(&mouse, fb->width, fb->height);

    for (;;) {
        char prompt[64] = "GMU [";
        char path_buf[48];
        gmu_fs_get_path(current_dir, path_buf, sizeof(path_buf));
        int pp = 5;
        const char *pth = path_buf;
        while (*pth && pp < 55) prompt[pp++] = *pth++;
        prompt[pp++] = ']';
        prompt[pp++] = '>';
        prompt[pp++] = ' ';
        prompt[pp] = '\0';

        print_line(fb, prompt, TERM_GREEN);

        uint64_t line_y = cursor_y - LINE_HEIGHT;
        uint64_t input_x = 16 + pp * GMU_FONT_WIDTH;

        length = 0;
        input[0] = '\0';

        for (;;) {
            if (mouse_ready) {
                gmu_mouse_poll(&mouse);
            }

            uint16_t key = gmu_keyboard_read_key();
            if (!key) {
                __asm__ volatile ("pause");
                continue;
            }

            if (key == '\n' || key == GMU_KEY_ENTER) {
                input[length] = '\0';
                break;
            }

            if (key == GMU_KEY_ESCAPE) {
                if (length == 0) {
                    return; /* Return to desktop */
                }
                length = 0;
                input[0] = '\0';
                gmu_fill_rect(fb, 16, line_y, fb->width - 32, LINE_HEIGHT, TERM_BG);
                gmu_draw_text(fb, 16, line_y, prompt, TERM_GREEN, TERM_BG);
                continue;
            }

            if (key == '\b') {
                if (length > 0) {
                    length--;
                    input[length] = '\0';

                    gmu_fill_rect(fb, 16, line_y, fb->width - 32, LINE_HEIGHT, TERM_BG);
                    gmu_draw_text(fb, 16, line_y, prompt, TERM_GREEN, TERM_BG);

                    for (uint64_t i = 0; i < length; i++) {
                        gmu_draw_char(fb, input_x + i * GMU_FONT_WIDTH,
                                      line_y, input[i], TERM_TEXT, TERM_BG);
                    }
                }
                continue;
            }

            /* History navigation */
            if (key == GMU_KEY_UP) {
                if (history_count > 0) {
                    if (history_index < history_count - 1) history_index++;
                    int h_idx = history_count - 1 - history_index;
                    length = 0;
                    while (history[h_idx][length] && length < MAX_INPUT - 1) {
                        input[length] = history[h_idx][length];
                        length++;
                    }
                    input[length] = '\0';

                    gmu_fill_rect(fb, 16, line_y, fb->width - 32, LINE_HEIGHT, TERM_BG);
                    gmu_draw_text(fb, 16, line_y, prompt, TERM_GREEN, TERM_BG);
                    for (uint64_t i = 0; i < length; i++) {
                        gmu_draw_char(fb, input_x + i * GMU_FONT_WIDTH, line_y, input[i], TERM_TEXT, TERM_BG);
                    }
                }
                continue;
            }

            if (key == GMU_KEY_DOWN) {
                if (history_index > 0) {
                    history_index--;
                    int h_idx = history_count - 1 - history_index;
                    length = 0;
                    while (history[h_idx][length] && length < MAX_INPUT - 1) {
                        input[length] = history[h_idx][length];
                        length++;
                    }
                    input[length] = '\0';

                    gmu_fill_rect(fb, 16, line_y, fb->width - 32, LINE_HEIGHT, TERM_BG);
                    gmu_draw_text(fb, 16, line_y, prompt, TERM_GREEN, TERM_BG);
                    for (uint64_t i = 0; i < length; i++) {
                        gmu_draw_char(fb, input_x + i * GMU_FONT_WIDTH, line_y, input[i], TERM_TEXT, TERM_BG);
                    }
                } else if (history_index == 0) {
                    history_index = -1;
                    length = 0;
                    input[0] = '\0';
                    gmu_fill_rect(fb, 16, line_y, fb->width - 32, LINE_HEIGHT, TERM_BG);
                    gmu_draw_text(fb, 16, line_y, prompt, TERM_GREEN, TERM_BG);
                }
                continue;
            }

            if (key >= 32 && key <= 126 && length < MAX_INPUT - 1 &&
                input_x + (length + 1) * GMU_FONT_WIDTH < fb->width - 20) {
                input[length++] = (char)key;
                input[length] = '\0';
                gmu_draw_char(fb, input_x + (length - 1) * GMU_FONT_WIDTH,
                              line_y, (char)key, TERM_TEXT, TERM_BG);
            }
        }

        if (!execute_command(fb, input)) {
            return; /* Return to desktop */
        }
    }
}
