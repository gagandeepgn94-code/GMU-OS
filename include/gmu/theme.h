#ifndef GMU_THEME_H
#define GMU_THEME_H

#include <stdint.h>

typedef enum {
    GMU_THEME_MACOS_DARK = 0,
    GMU_THEME_WIN11_DARK,
    GMU_THEME_CYBER_GLOW,
    GMU_THEME_MACOS_LIGHT,
    GMU_THEME_COUNT
} gmu_theme_id_t;

typedef struct {
    gmu_theme_id_t id;
    const char *name;
    const char *os_badge;
    uint32_t bg_top;
    uint32_t bg_bottom;
    uint32_t surface;
    uint32_t surface_accent;
    uint32_t surface_dark;
    uint32_t text_primary;
    uint32_t text_secondary;
    uint32_t accent;
    uint32_t accent_hover;
    uint32_t border;
    uint32_t dock_bg;
    uint32_t chiclet_red;
    uint32_t chiclet_yellow;
    uint32_t chiclet_green;
} gmu_theme_t;

const gmu_theme_t *gmu_theme_get(void);
void gmu_theme_set(gmu_theme_id_t id);
void gmu_theme_cycle(void);

#endif
