#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <gmu/framebuffer.h>
#include <gmu/draw.h>
#include <gmu/desktop.h>
#include <gmu/font.h>
#include <gmu/shell.h>
#include <gmu/ps2.h>
#include <gmu/fs.h>
#include <gmu/app.h>
#include <gmu/file_manager.h>
#include <gmu/idt.h>
#include <gmu/timer.h>
#include <gmu/rtc.h>
#include <gmu/pmm.h>
#include <gmu/heap.h>
#include <gmu/shooter.h>
#include <gmu/editor.h>
#include <gmu/calculator.h>
#include <gmu/monitor.h>
#include <gmu/settings.h>
#include <gmu/paint.h>
#include <gmu/calendar.h>
#include <gmu/hexview.h>
#include <gmu/snake.h>
#include <gmu/minesweeper.h>
#include <gmu/breakout.h>
#include <gmu/game_center.h>
#include <gmu/theme.h>
#include "../../boot/limine-src/limine-protocol/include/limine.h"

__attribute__((used, section(".limine_requests_start_marker")))
static volatile uint64_t requests_start[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile uint64_t base_revision[] = LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_end_marker")))
static volatile uint64_t requests_end[] = LIMINE_REQUESTS_END_MARKER;

static void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
}

static uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void serial_init(void) {
    outb(0x3F9, 0x00);
    outb(0x3FB, 0x80);
    outb(0x3F8, 0x01);
    outb(0x3F9, 0x00);
    outb(0x3FB, 0x03);
    outb(0x3FA, 0xC7);
    outb(0x3FC, 0x0B);
}

static void serial_print(const char *s) {
    while (*s) {
        while (!(inb(0x3FD) & 0x20)) {}
        outb(0x3F8, (uint8_t)*s++);
    }
}

static void halt(void) {
    for (;;) __asm__ volatile ("cli; hlt");
}

static bool filesystem_self_test(void) {
    static const char message[] = "GMU OS 64 filesystem test";
    char buffer[sizeof(message)];

    if (!gmu_fs_init())
        return false;

    int32_t dir = gmu_fs_create(
        GMU_FS_ROOT_ID, "TEST", GMU_FS_DIRECTORY
    );

    if (dir < 0)
        return false;

    int32_t file = gmu_fs_create(
        (uint16_t)dir, "HELLO.TXT", GMU_FS_FILE
    );

    if (file < 0)
        return false;

    if (!gmu_fs_write(
        (uint16_t)file, message, sizeof(message)
    ))
        return false;

    uint32_t bytes = gmu_fs_read(
        (uint16_t)file, buffer, 0, sizeof(buffer)
    );

    if (bytes != sizeof(message))
        return false;

    for (uint32_t i = 0; i < sizeof(message); i++) {
        if (buffer[i] != message[i])
            return false;
    }

    if (!gmu_fs_rename((uint16_t)file, "RENAMED.TXT"))
        return false;

    if (!gmu_fs_delete((uint16_t)file))
        return false;

    if (!gmu_fs_delete((uint16_t)dir))
        return false;

    return gmu_fs_count(GMU_FS_ROOT_ID) == 0;
}

void _start(void) {
    serial_init();
    serial_print("\r\n========================================\r\n");
    serial_print("  GMU OS 64 - Modern Edition (x86_64)   \r\n");
    serial_print("========================================\r\n");

    /* Initialize IDT */
    gmu_idt_init();
    serial_print("[OK] IDT initialized (256 vectors)\r\n");

    /* Verify Limine Protocol Base Revision */
    if (!LIMINE_BASE_REVISION_SUPPORTED(base_revision)) {
        serial_print("[FAIL] Limine base revision unsupported\r\n");
        halt();
    }
    serial_print("[OK] Limine protocol handshake: v3\r\n");

    /* Initialize PIC & PIT timer at 100 Hz */
    gmu_timer_init();
    serial_print("[OK] 8254 PIT Timer initialized (100 Hz)\r\n");

    /* Initialize Hardware Real-Time Clock (RTC/CMOS) */
    gmu_rtc_init();
    char rtc_init_str[64];
    gmu_rtc_get_full_str(rtc_init_str);
    serial_print("[OK] Real-Time Clock active: ");
    serial_print(rtc_init_str);
    serial_print("\r\n");

    /* Initialize Physical Memory Manager */
    if (memmap_request.response) {
        gmu_pmm_init(memmap_request.response);
        serial_print("[OK] Physical Memory Manager active\r\n");
    } else {
        serial_print("[WARN] Limine memmap unavailable, using fallback memory\r\n");
    }

    /* Initialize Kernel Dynamic Heap */
    gmu_heap_init();
    serial_print("[OK] Kernel Dynamic Heap initialized (2 MB pool)\r\n");

    /* Initialize In-Memory RAM Filesystem */
    if (!filesystem_self_test()) {
        serial_print("[FAIL] RAM filesystem self-test failed\r\n");
        halt();
    }
    serial_print("[OK] RAM filesystem self-test passed\r\n");

    /* Load default rich documents and directory tree */
    gmu_fs_load_default_data();
    serial_print("[OK] Default documents & filesystem tree loaded\r\n");

    /* Initialize Application Manager */
    gmu_app_manager_init();
    gmu_file_manager_register();
    gmu_editor_register();
    gmu_paint_register();
    gmu_calculator_register();
    gmu_calendar_register();
    gmu_hexview_register();
    gmu_monitor_register();
    gmu_settings_register();
    gmu_game_center_register();
    gmu_shooter_register();
    gmu_snake_register();
    gmu_minesweeper_register();
    gmu_breakout_register();
    serial_print("[OK] Application Manager initialized (13 GUI Apps & Games active)\r\n");

    /* Validate Framebuffer */
    if (!framebuffer_request.response ||
        framebuffer_request.response->framebuffer_count == 0) {
        serial_print("[FAIL] Framebuffer unavailable from UEFI bootloader\r\n");
        halt();
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    if (!fb || !fb->address || fb->bpp != 32) {
        serial_print("[FAIL] Unsupported framebuffer format (requires 32bpp)\r\n");
        halt();
    }
    serial_print("[OK] UEFI GOP Framebuffer acquired\r\n");

    gmu_framebuffer_t fb_state = {0};
    if (!gmu_fb_init(&fb_state, fb->address, fb->width, fb->height, fb->pitch, fb->bpp)) {
        serial_print("[FAIL] Framebuffer driver initialization failed\r\n");
        halt();
    }
    serial_print("[OK] Framebuffer driver initialized\r\n");

    /* Initialize PS/2 controller */
    gmu_ps2_init();
    serial_print("[OK] PS/2 Input Subsystem initialized\r\n");

    serial_print("----------------------------------------\r\n");
    serial_print("  System Ready. Starting GMU Desktop... \r\n");
    serial_print("----------------------------------------\r\n");

    /* Enter workstation desktop & shell lifecycle */
    for (;;) {
        gmu_desktop_run(&fb_state);
        gmu_shell_run(&fb_state);
    }
}
