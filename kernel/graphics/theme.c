#include <gmu/theme.h>

static const gmu_theme_t themes[GMU_THEME_COUNT] = {
    [GMU_THEME_MACOS_DARK] = {
        .id = GMU_THEME_MACOS_DARK,
        .name = "macOS Sonoma (Dark)",
        .os_badge = " macOS Style",
        .bg_top = 0x001B172E,          /* macOS Deep Purple */
        .bg_bottom = 0x000A0814,       /* Midnight */
        .surface = 0x00231F3D,         /* Frosted Glass */
        .surface_accent = 0x00352F5B,  /* Elevated Header */
        .surface_dark = 0x00130F24,    /* Deep Container */
        .text_primary = 0x00F8FAFC,    /* White */
        .text_secondary = 0x00A59EC2,  /* Muted Lavender */
        .accent = 0x003B82F6,          /* Apple Blue */
        .accent_hover = 0x0060A5FA,    /* Hover Blue */
        .border = 0x00473F73,          /* Sleek Glass Border */
        .dock_bg = 0x00171329,         /* Frosted Dock */
        .chiclet_red = 0x00FF5F56,     /* Apple Close Red */
        .chiclet_yellow = 0x00FFBD2E,  /* Apple Minimize Yellow */
        .chiclet_green = 0x0027C93F    /* Apple Maximize Green */
    },
    [GMU_THEME_WIN11_DARK] = {
        .id = GMU_THEME_WIN11_DARK,
        .name = "Windows 11 Bloom",
        .os_badge = "❖ Windows 11",
        .bg_top = 0x000E192B,          /* Windows 11 Dark Blue */
        .bg_bottom = 0x00050B14,       /* Deep Abyss */
        .surface = 0x00182438,         /* Mica Dark Slate */
        .surface_accent = 0x00253752,  /* Acrylic Header */
        .surface_dark = 0x000B131F,    /* Dark Mica */
        .text_primary = 0x00FFFFFF,    /* Pure White */
        .text_secondary = 0x009BB0CF,  /* Cool Gray */
        .accent = 0x000078D4,          /* Windows Azure */
        .accent_hover = 0x002B88D8,    /* Light Azure */
        .border = 0x00334866,          /* Mica Border */
        .dock_bg = 0x000D1524,         /* Taskbar Acrylic */
        .chiclet_red = 0x00E81123,     /* Windows Red */
        .chiclet_yellow = 0x00FFB900,  /* Windows Amber */
        .chiclet_green = 0x00107C41    /* Windows Green */
    },
    [GMU_THEME_CYBER_GLOW] = {
        .id = GMU_THEME_CYBER_GLOW,
        .name = "Cyber Glow (Neon)",
        .os_badge = "⚡ Cyberpunk",
        .bg_top = 0x00180B26,
        .bg_bottom = 0x0005020A,
        .surface = 0x00241038,
        .surface_accent = 0x003B1A5C,
        .surface_dark = 0x0012061C,
        .text_primary = 0x00FAF5FF,
        .text_secondary = 0x00C084FC,
        .accent = 0x00EC4899,
        .accent_hover = 0x00F472B6,
        .border = 0x006B21A8,
        .dock_bg = 0x0011051D,
        .chiclet_red = 0x00F43F5E,
        .chiclet_yellow = 0x00FBBF24,
        .chiclet_green = 0x0006B6D4
    },
    [GMU_THEME_MACOS_LIGHT] = {
        .id = GMU_THEME_MACOS_LIGHT,
        .name = "macOS Tahoe (Light)",
        .os_badge = " macOS Light",
        .bg_top = 0x00E2E8F0,          /* Frosted Silver */
        .bg_bottom = 0x00CBD5E1,       /* Cool Fog */
        .surface = 0x00F1F5F9,         /* Pure Glass */
        .surface_accent = 0x00E2E8F0,  /* Crisp Header */
        .surface_dark = 0x00FFFFFF,    /* Clean Card */
        .text_primary = 0x000F172A,    /* Dark Slate Text */
        .text_secondary = 0x0064748B,  /* Slate 500 */
        .accent = 0x000284C7,          /* Sky Blue */
        .accent_hover = 0x000369A1,    /* Darker Sky */
        .border = 0x0094A3B8,          /* Slate Border */
        .dock_bg = 0x00E2E8F0,         /* Frosted Light Dock */
        .chiclet_red = 0x00EF4444,
        .chiclet_yellow = 0x00F59E0B,
        .chiclet_green = 0x0010B981
    }
};

static gmu_theme_id_t current_theme = GMU_THEME_MACOS_DARK;

const gmu_theme_t *gmu_theme_get(void) {
    return &themes[current_theme];
}

void gmu_theme_set(gmu_theme_id_t id) {
    if (id < GMU_THEME_COUNT) {
        current_theme = id;
    }
}

void gmu_theme_cycle(void) {
    current_theme = (current_theme + 1) % GMU_THEME_COUNT;
}
