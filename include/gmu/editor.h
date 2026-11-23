#ifndef GMU_EDITOR_H
#define GMU_EDITOR_H

#include <gmu/framebuffer.h>

void gmu_editor_run(gmu_framebuffer_t *fb);
void gmu_editor_open_file(gmu_framebuffer_t *fb, const char *path);
void gmu_editor_register(void);

#endif
