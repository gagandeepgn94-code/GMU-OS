#ifndef GMU_HEXVIEW_H
#define GMU_HEXVIEW_H

#include <gmu/framebuffer.h>

void gmu_hexview_run(gmu_framebuffer_t *fb);
void gmu_hexview_open_file(gmu_framebuffer_t *fb, const char *path);
void gmu_hexview_register(void);

#endif
