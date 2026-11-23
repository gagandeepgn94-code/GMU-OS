#include <gmu/app.h>
#include <gmu/file_manager.h>
#include <gmu/editor.h>
#include <gmu/calculator.h>
#include <gmu/monitor.h>
#include <gmu/settings.h>
#include <gmu/shooter.h>

static gmu_application_t applications[GMU_APP_COUNT];
static bool registered[GMU_APP_COUNT];

void gmu_app_manager_init(void) {
    for (uint32_t i = 0; i < GMU_APP_COUNT; i++) {
        registered[i] = false;
    }
}

bool gmu_app_register(const gmu_application_t *app) {
    if (!app || !app->name || !app->run)
        return false;

    if (app->id >= GMU_APP_COUNT)
        return false;

    if (registered[app->id])
        return false;

    applications[app->id] = *app;
    registered[app->id] = true;

    return true;
}

bool gmu_app_launch(gmu_app_id_t id, gmu_framebuffer_t *fb) {
    if (!fb || !fb->initialized)
        return false;

    if (id >= GMU_APP_COUNT || !registered[id])
        return false;

    applications[id].run(fb);

    return true;
}

void gmu_file_manager_register(void) {
    const gmu_application_t app = {
        .id = GMU_APP_FILE_MANAGER,
        .name = "File Manager",
        .run = gmu_file_manager_run
    };
    (void)gmu_app_register(&app);
}
