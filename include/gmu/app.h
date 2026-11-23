#ifndef GMU_APP_H
#define GMU_APP_H

#include <stdint.h>
#include <stdbool.h>
#include <gmu/framebuffer.h>

typedef enum {
    GMU_APP_FILE_MANAGER = 0,
    GMU_APP_EDITOR,
    GMU_APP_PAINT,
    GMU_APP_CALCULATOR,
    GMU_APP_CALENDAR,
    GMU_APP_HEXVIEW,
    GMU_APP_MONITOR,
    GMU_APP_SETTINGS,
    GMU_APP_GAME_CENTER,
    GMU_APP_SHOOTER,
    GMU_APP_SNAKE,
    GMU_APP_MINESWEEPER,
    GMU_APP_BREAKOUT,
    GMU_APP_COUNT
} gmu_app_id_t;

typedef struct {
    gmu_app_id_t id;
    const char *name;
    void (*run)(gmu_framebuffer_t *fb);
} gmu_application_t;

void gmu_app_manager_init(void);
bool gmu_app_register(const gmu_application_t *app);
bool gmu_app_launch(gmu_app_id_t id, gmu_framebuffer_t *fb);

#endif
